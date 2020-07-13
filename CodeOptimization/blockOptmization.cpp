#include <map>
#include <set>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
using namespace std;

struct sym { int kind; int type; string value; int addr; };
struct midcode { string result; string left; int op; string right; set<string> out; };

extern map<string, map<string, struct sym>> symList;
extern map<string, map<string, struct sym>> tmpList;
extern vector<struct midcode> midCodes;
enum midop {NEG, ADD, SUB, MUL, DIVI, ASS, GETARR, PUTARR, SETLABEL, GOTO, BNZ, BZ, LESS, LE, GREAT,
	GE, EQUAL, UNEQUAL, READ, WSTRING, WEXP, WSE, RET, RETX, PARA, FUNCALL, FUN, PAR, ASSRET };

struct block { 
	string level; 
	vector<int> prev;
	vector<int> next;
	set<string> def;
	set<string> use;
	set<string> in;
	set<string> out;
	vector<struct midcode> midcodes;
};
vector<struct block> blocks;
map<string, int> label_block;
map<string, vector<int>> fun_blocks;
vector<string> funs;

void outputMidcode(struct midcode m, ostream& file);
void test_block();
bool isNumber(string s);

void getBlock() {
	int i;
	string level = "$all";
	struct midcode m;
	vector<struct midcode> block_midcodes;
	// 划分基本块，建立中间代码与基本块的映射
	for (i = 0; i < midCodes.size(); i++) {
		m = midCodes[i];
		if (m.op == FUN) level = m.right;
		if (m.op == SETLABEL){
			if (block_midcodes.size() > 0) {
				struct block b;
				b.level = level;
				b.midcodes = block_midcodes;
				blocks.push_back(b);
				block_midcodes.clear();
				if (fun_blocks.find(level) == fun_blocks.end()) {
					vector<int> a;
					fun_blocks[level] = a;
					funs.push_back(level);
				}
				fun_blocks[level].push_back(blocks.size() - 1);
			}
			label_block[m.result] = blocks.size();
		}
		block_midcodes.push_back(m);
		if ((m.op == GOTO || m.op == BNZ || m.op == BZ || m.op == READ ||
			m.op == FUNCALL || m.op == RET || m.op == RETX) && block_midcodes.size() > 0) {
			struct block b;
			b.level = level;
			b.midcodes = block_midcodes;
			blocks.push_back(b);
			block_midcodes.clear();
			if (fun_blocks.find(level) == fun_blocks.end()) {
				vector<int> a;
				fun_blocks[level] = a;
				funs.push_back(level);
			}
			fun_blocks[level].push_back(blocks.size() - 1);
		}
	}
	if (block_midcodes.size() > 0) {
		struct block b;
		b.level = level;
		b.midcodes = block_midcodes;
		blocks.push_back(b);
		block_midcodes.clear();
		if (fun_blocks.find(level) == fun_blocks.end()) {
			vector<int> a;
			fun_blocks[level] = a;
			funs.push_back(level);
		}
		fun_blocks[level].push_back(blocks.size() - 1);
	}
	// test_block();
	// 建立流图
	for (i = 0; i < blocks.size(); i++) {
		m = blocks[i].midcodes.back();
		if (m.op != GOTO && m.op != RET && m.op != RETX && i != blocks.size() - 1) {
			blocks[i].next.push_back(i + 1);
			blocks[i + 1].prev.push_back(i);
		}
		if (m.op == GOTO || m.op == BNZ || m.op == BZ) {
			blocks[i].next.push_back(label_block[m.result]);
			blocks[label_block[m.result]].prev.push_back(i);
		}
	}
	// test_block();
}

void activeAnalysis() {
	int i, j, inSize, outSize;
	bool stop = false;
	struct midcode m;
	// 计算每个基本块的use和def
	for (i = 0; i < blocks.size(); i++) {
		for (j = 0; j < blocks[i].midcodes.size(); j++) {
			m = blocks[i].midcodes[j];
			if (m.op == NEG || m.op == ASS) {
				if (!isNumber(m.left) && blocks[i].def.find(m.left) == blocks[i].def.end())
					blocks[i].use.insert(m.left);
				if (blocks[i].use.find(m.result) == blocks[i].use.end())
					blocks[i].def.insert(m.result);
			} 
			else if ((m.op >= ADD && m.op <= DIVI) || (m.op >= LESS && m.op <= UNEQUAL) || m.op == GETARR || m.op == PUTARR) {
				if (!isNumber(m.left) && blocks[i].def.find(m.left) == blocks[i].def.end())
					blocks[i].use.insert(m.left);
				if (!isNumber(m.right) && blocks[i].def.find(m.right) == blocks[i].def.end())
					blocks[i].use.insert(m.right);
				if (blocks[i].use.find(m.result) == blocks[i].use.end())
					blocks[i].def.insert(m.result);
			} 
			else if (m.op == READ || m.op == WEXP || m.op == RETX || m.op == PARA || m.op == BNZ || m.op == BZ) {
				if (!isNumber(m.left) && blocks[i].def.find(m.left) == blocks[i].def.end())
					blocks[i].use.insert(m.left);
			} 
			else if (m.op == WSE) {
				if (!isNumber(m.right) && blocks[i].def.find(m.right) == blocks[i].def.end())
					blocks[i].use.insert(m.right);
			} 
			else if (m.op == ASSRET) {
				if (blocks[i].use.find(m.result) == blocks[i].use.end())
					blocks[i].def.insert(m.result);
			}
		}
	}
	// 计算每个基本块的in和out
	while (!stop) {
		stop = true;
		for (i = blocks.size() - 1; i >= 0; i--) {
			inSize = blocks[i].in.size();
			outSize = blocks[i].out.size();
			//  计算out
			for (j = 0; j < blocks[i].next.size(); j++) {
				for (auto s: blocks[blocks[i].next[j]].in) {
					blocks[i].out.insert(s);
				}
			}
			// 计算in
			for (auto s : blocks[i].out) {
				if (blocks[i].def.find(s) == blocks[i].def.end())
					blocks[i].in.insert(s);
			}
			for (auto s : blocks[i].use) {
				blocks[i].in.insert(s);
			}
			if (inSize != blocks[i].in.size() || outSize != blocks[i].out.size())
				stop = false;
		}
	}
	// test_block();
	// 计算块内每一句中间代码的结尾处活跃变量，并删除死代码
	for (i = 0; i < blocks.size(); i++) {
		set<string> out = blocks[i].out;
		vector<int> deadcode;
		for (j = blocks[i].midcodes.size() - 1; j >= 0; j--) {
			blocks[i].midcodes[j].out = out;
			m = blocks[i].midcodes[j];
			if (((m.op >= NEG && m.op <= GETARR) || (m.op >= LESS && m.op <= UNEQUAL) || m.op == ASSRET)
				&& (symList[blocks[i].level].find(m.result) != symList[blocks[i].level].end() ||
					tmpList[blocks[i].level].find(m.result) != tmpList[blocks[i].level].end())
				&& out.find(m.result) == out.end()) {
				deadcode.push_back(j);
				continue;
			}
			set<string> def;
			set<string> use;
			if (m.op == NEG || m.op == ASS) {
				if (!isNumber(m.left) && def.find(m.left) == def.end())
					use.insert(m.left);
				if (use.find(m.result) == use.end())
					def.insert(m.result);
			}
			else if ((m.op >= ADD && m.op <= DIVI) || (m.op >= LESS && m.op <= UNEQUAL) || m.op == GETARR || m.op == PUTARR) {
				if (!isNumber(m.left) && def.find(m.left) == def.end())
					use.insert(m.left);
				if (!isNumber(m.right) && def.find(m.right) == def.end())
					use.insert(m.right);
				if (use.find(m.result) ==use.end())
					def.insert(m.result);
			}
			else if (m.op == READ || m.op == WEXP || m.op == RETX || m.op == PARA || m.op == BNZ || m.op == BZ) {
				if (!isNumber(m.left) && def.find(m.left) == def.end())
					use.insert(m.left);
			}
			else if (m.op == WSE) {
				if (!isNumber(m.right) && def.find(m.right) == def.end())
					use.insert(m.right);
			}
			else if (m.op == ASSRET) {
				if (use.find(m.result) == use.end())
					def.insert(m.result);
			}
			for (auto s : def)
				out.erase(s);
			for (auto s : use)
				out.insert(s);
		}
		for (j = 0; j < deadcode.size(); j++)
			blocks[i].midcodes.erase(blocks[i].midcodes.begin() + deadcode[j]);
	}
	// test_block();
}

void test_block() {
	int i, j;
	ofstream test;
	test.open("test.txt", ios_base::out | ios_base::trunc);
	for (i = 0; i < blocks.size(); i++) {
		test << "block " << i << ":" << endl;
		test << "\tlevel: " << blocks[i].level << endl;
		test << "\tprev: ";
		for (j = 0; j < blocks[i].prev.size(); j++)
			test << blocks[i].prev[j] << " ";
		test << "\n\tnext: ";
		for (j = 0; j < blocks[i].next.size(); j++)
			test << blocks[i].next[j] << " ";
		test << "\n\tin: ";
		for (auto s: blocks[i].in)
			test << s << " ";
		test << "\n\tout: ";
		for (auto s : blocks[i].out)
			test << s << " ";
		test << "\n\tmidcodes:\n";
		for (j = 0; j < blocks[i].midcodes.size(); j++) {
			test << "\t\t"; 
			outputMidcode(blocks[i].midcodes[j], test);
			if (blocks[i].midcodes[j].out.size() > 0) {
				test << "\t\t\tout : ";
				for (auto s : blocks[i].midcodes[j].out)
					test << s << " ";
				test << endl;
			}			
		}
	}
}

