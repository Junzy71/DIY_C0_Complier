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
extern vector<struct block> blocks;
extern map<string, vector<int>> fun_blocks;
extern vector<string> funs;
enum midop {
	NEG, ADD, SUB, MUL, DIVI, ASS, GETARR, PUTARR, SETLABEL, GOTO, BNZ, BZ, LESS, LE, GREAT,
	GE, EQUAL, UNEQUAL, READ, WSTRING, WEXP, WSE, RET, RETX, PARA, FUNCALL, FUN, PAR, ASSRET
};

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


void outputMidcode(struct midcode m, ostream& file);
void activeAnalysis();
void test_block();

void reTmpList() {	
	for (auto level : funs) {
		map<string, struct sym> tmp;
		for (auto blockNum : fun_blocks[level]) {
			for (auto m:blocks[blockNum].midcodes) {
				if (tmpList[level].find(m.result) != tmpList[level].end() && tmp.find(m.result) == tmp.end())
					tmp[m.result] = tmpList[level][m.result];
				if (tmpList[level].find(m.right) != tmpList[level].end() && tmp.find(m.right) == tmp.end())
					tmp[m.right] = tmpList[level][m.right];
				if (tmpList[level].find(m.left) != tmpList[level].end() && tmp.find(m.left) == tmp.end())
					tmp[m.left] = tmpList[level][m.left];
			}
		}
		tmpList[level] = tmp;
	}
}

void assignPro() {
	int i;
	for (auto level : funs) {
		for (auto blockNum : fun_blocks[level]) {
			map<string, string> replace;
			for (i = 0; i < blocks[blockNum].midcodes.size(); i++) {
				struct midcode m = blocks[blockNum].midcodes[i];
				// 对赋值语句更新替换表
				if (m.op == ASS) {
					if (replace.find(m.left) != replace.end()) m.left = replace[m.left];
					replace[m.result] = m.left;
					vector<string> correct;
					for (auto s : replace) {
						if (s.second == m.result) correct.push_back(s.first);
					}
					for (auto s : correct)
						replace[s] = m.left;
				}
				// 对运算语句，更新后删除替换表
				else if ((m.op >= ADD && m.op <= GETARR) || (m.op >= LESS && m.op <= READ) || m.op == ASSRET) {
					if (m.op != GETARR && m.op != ASSRET && replace.find(m.left) != replace.end()) m.left = replace[m.left];
					if (m.op != ASSRET && replace.find(m.right) != replace.end()) m.right = replace[m.right];
					vector<string> correct;
					for (auto s : replace) {
						if (s.second == m.result) correct.push_back(s.first);
					}
					for (auto s : correct)
						replace.erase(s);
					if (replace.find(m.result) != replace.end())
						replace.erase(m.result);
				}
				// 其余语句正常替换
				else {
					if (replace.find(m.left) != replace.end()) m.left = replace[m.left];
					if (replace.find(m.right) != replace.end()) m.right = replace[m.right];
				}
			}
		}
	}
	activeAnalysis();
	reTmpList();
	test_block();
}
