#include <map>
#include <set>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
using namespace std;

struct sym { int kind; int type; string value; int addr; };
struct midcode { string result; string left; int op; string right; set<string> out; };

extern ofstream mips;
extern ofstream optmid;

extern string level;

extern map<string, map<string, struct sym>> symList;
extern map<string, map<string, struct sym>> tmpList;
extern vector <struct midcode> midCodes;

extern map<string, int> addrSize;
extern map<string, int> symSize;
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
extern vector<struct block> blocks;
extern map<string, vector<int>> fun_blocks;
extern vector<string> funs;

bool isNumber(string s);
int str2int(string s);
string int2str(int i);
void outputMidcode(struct midcode m, ostream& file);
string allocRegT(string var, int protect1, int protect2, vector<string>& reg_var_T, string level, set<string> out);

void getRegS(string fun, map<string, int>& color) {
	int i, j;
	int K = 8;
	set<string> v;
	map<string, set<string>> e;
	// 确定待分配寄存器的变量集合，冲突图点集
	for (auto i : fun_blocks[fun]) {
		for (auto s : blocks[i].out) {
			if (symList[fun].find(s) != symList[fun].end() && 
				symList[fun][s].kind == 0 && (symList[fun][s].type == 5 || symList[fun][s].type == 6))
				v.insert(s);
			else if (tmpList[fun].find(s) != tmpList[fun].end())
				v.insert(s);
		}
	}
	// 冲突图构建，冲突图边集
	for (auto i : fun_blocks[fun]) {
		for (auto m : blocks[i].midcodes) {
			set<string> var;
			if ((m.op >= NEG && m.op <= ASS) || m.op == PUTARR || (m.op >= BNZ && m.op <= READ) || m.op == WEXP || m.op == RETX || m.op == PARA)
				if (!isNumber(m.left) && v.find(m.left) != v.end())
					var.insert(m.left);
			if ((m.op >= ADD && m.op <= DIVI) || (m.op >= LESS && m.op <= UNEQUAL) || m.op == GETARR || m.op == PUTARR || m.op == WSE)
				if (!isNumber(m.right) && v.find(m.right) != v.end())
					var.insert(m.right);
			if ((m.op >= NEG && m.op <= GETARR) || (m.op >= LESS && m.op <= UNEQUAL) || m.op == ASSRET)
				if (v.find(m.result) != v.end())
					var.insert(m.result);
			for (auto var_ : var) {
				for (auto o : m.out)
					if (v.find(o) != v.end() && o != var_) { 
						e[var_].insert(o); 
						e[o].insert(var_); 
					}
			}
		}
	}
	if (v.size() == 0) 
		return;
	// 图着色算法分配寄存器
	vector<string> remove;
	set<string> v_ = v;
	map<string, set<string>> e_ = e;
	// 移走点
	while (v_.size() > 1) {
		while (v_.size() > 1) {
			string remove_v = "";
			for (auto v_tmp : e_)
				if (v_tmp.second.size() < K) { 
					remove_v = v_tmp.first;  
					break; 
				}
			if (remove_v == "")
				break;
			remove.push_back(remove_v);
			v_.erase(remove_v);
			e_.erase(remove_v);
			for (auto& v_tmp : e_)
				v_tmp.second.erase(remove_v);
		}
		if (v_.size() == 1) 
			break;
		string remove_v = *(v_.begin());
		v_.erase(remove_v);
		e_.erase(remove_v);
		for (auto& v_tmp : e_)
			v_tmp.second.erase(remove_v);
	}
	// 移回点并分配寄存器，不参与分配的不进color
	color[*(v_.begin())] = 0;
	set<int> used = { 0 };
	for (i = remove.size() - 1; i >= 0; i--) {
		set<int> used_color;
		for (j = 0; j < K; j++) {
			if (used.find(j) == used.end()) {
				used.insert(j);
				color[remove[i]] = j;
				break;
			}
		}
		if (j != K) continue;
		for (auto v2 : e[remove[i]]) {
			if (color.find(v2) != color.end())
				used_color.insert(color[v2]);
		}
		for (j = 0; j < K; j++) {
			if (used_color.find(j) == used_color.end()) {
				used.insert(j);
				color[remove[i]] = j;
				break;
			}
		}
	}
}

void getMips_OPT() {
	int i, offset, type, regnum;
	mips << ".text" << endl;
	mips << "subi $sp, $sp," << addrSize["main"] << "	# down sp" << endl;
	mips << "j main" << endl;
	for (auto level : funs) {
		map<string, int> color;
		vector<string> reg_var_S = {"", "", "", "", "", "", "", ""};
		getRegS(level, color);
		/*
		cout << level << ":" << endl;
		for (auto co : color)
			cout << "\t" << co.first << " $s" << co.second << endl;
		*/
		for (auto block_num : fun_blocks[level]) {
			bool saved = false;
			vector<string> reg_var_T = { "", "", "", "", "", "", "", "", "", "" };
			// 对块内每一条中间代码生成mips
			for (auto m : blocks[block_num].midcodes) {
				string resultReg = "$t*", leftReg = "$t*", rightReg = "$t*", offsetReg = "$t*";
				string imm = "";
				bool immLeft = false;
				mips << "\t\t\t\t\t\t\t\t\t\t # " << m.result << ", " << m.left << ", " << m.op << ", " << m.right << endl;
				mips << "\t\t # ";
				outputMidcode(m, mips);
				// 基本运算，逻辑运算
				if (m.op == NEG || m.op == ASS) {
					// 查询结果和左操作数是否占用寄存器
					for (i = 0; i < reg_var_T.size(); i++) {
						if (reg_var_T[i] == m.left) leftReg = "$t" + int2str(i);
						if (reg_var_T[i] == m.result) resultReg = "$t" + int2str(i);
					}
					// 给左操作数分配寄存器
					if (leftReg != "$t*");
					else if (isNumber(m.left)) {	
						// 立即数，记录，不分配寄存器
						imm = m.left;
					}
					else if (color.find(m.left) != color.end()) {	
						// 需要使用s寄存器
						leftReg = "$s" + int2str(color[m.left]);
						if (reg_var_S[color[m.left]] != m.left) {
							reg_var_S[color[m.left]] = m.left;
							if (symList[level].find(m.left) != symList[level].end()) offset = symList[level][m.left].addr;
							else if (tmpList[level].find(m.left) != tmpList[level].end()) offset = tmpList[level][m.left].addr;
							mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
						}						
					}
					else if (symList[level].find(m.left) != symList[level].end()) {
						// 原程序局部变量分配t寄存器
						leftReg = allocRegT(m.left, resultReg[2] - '0', -1, reg_var_T, level, m.out);
						offset = symList[level][m.left].addr;
						mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
					}
					else if (tmpList[level].find(m.left) != tmpList[level].end()) {
						// 临时变量分配t寄存器
						leftReg = allocRegT(m.left, resultReg[2] - '0', -1, reg_var_T, level, m.out);
						offset = tmpList[level][m.left].addr;
						mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
					}
					else if (symList["$all"].find(m.left) != symList["$all"].end()) {
						// 全局变量分配t寄存器
						leftReg = allocRegT(m.left, resultReg[2] - '0', -1, reg_var_T, level, m.out);
						mips << "lw " << leftReg << ", " << m.left << endl;
					}
					// 查询结果是否占用寄存器
					for (i = 0; i < reg_var_T.size(); i++)
						if (reg_var_T[i] == m.result) resultReg = "$t" + int2str(i);
					// 给结果分配寄存器
					if (resultReg != "$t*");
					else if (color.find(m.result) != color.end()) {
						// 需要使用s寄存器
						resultReg = "$s" + int2str(color[m.result]);
						if (reg_var_S[color[m.result]] != m.result) {
							reg_var_S[color[m.result]] = m.result;
						}
					}
					else if (symList[level].find(m.result) != symList[level].end()) {
						// 原程序局部变量分配t寄存器
						resultReg = allocRegT(m.result, leftReg[2] - '0', -1, reg_var_T, level, m.out);
					}
					else if (tmpList[level].find(m.result) != tmpList[level].end()) {
						// 临时变量分配t寄存器
						resultReg = allocRegT(m.result, leftReg[2] - '0', -1, reg_var_T, level, m.out);
					}
					else if (symList["$all"].find(m.result) != symList["$all"].end()) {
						// 全局变量分配t寄存器
						resultReg = allocRegT(m.result, leftReg[2] - '0', -1, reg_var_T, level, m.out);
					}
					// 进行计算
					if (m.op == NEG) {
						if (imm == "") mips << "subu " << resultReg << ", $0, " << leftReg << endl;
						else {
							if (imm[0] == '-') imm = imm.substr(1);
							else imm = "-" + imm;
							mips << "li " << resultReg << ", " << imm << endl;
						}
					}
					else if (m.op == ASS) {
						if (imm == "") mips << "move " << resultReg << ", " << leftReg << endl;
						else mips << "li " << resultReg << ", " << imm << endl;
					}					
				}
				else if ((m.op >= ADD && m.op <= DIVI) || (m.op >= LESS && m.op <= UNEQUAL)) {
					// 查询结果和左右操作数是否占用寄存器
					for (i = 0; i < reg_var_T.size(); i++) {
						if (reg_var_T[i] == m.left) leftReg = "$t" + int2str(i);
						if (reg_var_T[i] == m.right) rightReg = "$t" + int2str(i);
						if (reg_var_T[i] == m.result) resultReg = "$t" + int2str(i);
					}
					// 给左操作数分配寄存器
					if (leftReg != "$t*");
					else if (isNumber(m.left)) {
						// 立即数，是加乘操作不分配寄存器，否则分配
						if (m.op == ADD || m.op == MUL) {
							imm = m.left;
							immLeft = true;
						}
						else {
							leftReg = allocRegT(m.left, resultReg[2] - '0', rightReg[2] - '0', reg_var_T, level, m.out);
							mips << "li " << leftReg << ", " << m.left << endl;
						}
					}
					else if (color.find(m.left) != color.end()) {
						// 需要使用s寄存器
						leftReg = "$s" + int2str(color[m.left]);
						if (reg_var_S[color[m.left]] != m.left) {
							reg_var_S[color[m.left]] = m.left;
							if (symList[level].find(m.left) != symList[level].end()) offset = symList[level][m.left].addr;
							else if (tmpList[level].find(m.left) != tmpList[level].end()) offset = tmpList[level][m.left].addr;
							mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
						}
					}
					else if (symList[level].find(m.left) != symList[level].end()) {
						// 原程序局部变量分配t寄存器
						leftReg = allocRegT(m.left, resultReg[2] - '0', rightReg[2] - '0', reg_var_T, level, m.out);
						offset = symList[level][m.left].addr;
						mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
					}
					else if (tmpList[level].find(m.left) != tmpList[level].end()) {
						// 临时变量分配t寄存器
						leftReg = allocRegT(m.left, resultReg[2] - '0', rightReg[2] - '0', reg_var_T, level, m.out);
						offset = tmpList[level][m.left].addr;
						mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
					}
					else if (symList["$all"].find(m.left) != symList["$all"].end()) {
						// 全局变量分配t寄存器
						leftReg = allocRegT(m.left, resultReg[2] - '0', rightReg[2] - '0', reg_var_T, level, m.out);
						mips << "lw " << leftReg << ", " << m.left << endl;
					}
					// 查询结果和右操作数是否占用寄存器
					for (i = 0; i < reg_var_T.size(); i++) {
						if (reg_var_T[i] == m.right) rightReg = "$t" + int2str(i);
						if (reg_var_T[i] == m.result) resultReg = "$t" + int2str(i);
					}
					// 给右操作数分配寄存器
					if (rightReg != "$t*");
					else if (isNumber(m.right)) {
						// 立即数，左操作数是立即数或除法操作或超范围的小于操作分配寄存器；否则不分配
						if (immLeft || m.op == DIVI || (m.op == LESS && (str2int(m.right) <= -32768 || str2int(m.right) >= 32767))) {
							rightReg = allocRegT(m.right, resultReg[2] - '0', leftReg[2] - '0', reg_var_T, level, m.out);
							mips << "li " << rightReg << ", " << m.right << endl;
						}	
						else
							imm = m.right;
					}
					else if (color.find(m.right) != color.end()) {
						// 需要使用s寄存器
						rightReg = "$s" + int2str(color[m.right]);
						if (reg_var_S[color[m.right]] != m.right) {
							reg_var_S[color[m.right]] = m.right;
							if (symList[level].find(m.right) != symList[level].end()) offset = symList[level][m.right].addr;
							else if (tmpList[level].find(m.right) != tmpList[level].end()) offset = tmpList[level][m.right].addr;
							mips << "lw " << rightReg << ", " << offset << "($sp)	# " << m.right << endl;
						}
					}
					else if (symList[level].find(m.right) != symList[level].end()) {
						// 原程序局部变量分配t寄存器
						rightReg = allocRegT(m.right, resultReg[2] - '0', leftReg[2] - '0', reg_var_T, level, m.out);
						offset = symList[level][m.right].addr;
						mips << "lw " << rightReg << ", " << offset << "($sp)	# " << m.right << endl;
					}
					else if (tmpList[level].find(m.right) != tmpList[level].end()) {
						// 临时变量分配t寄存器
						rightReg = allocRegT(m.right, resultReg[2] - '0', leftReg[2] - '0', reg_var_T, level, m.out);
						offset = tmpList[level][m.right].addr;
						mips << "lw " << rightReg << ", " << offset << "($sp)	# " << m.right << endl;
					}
					else if (symList["$all"].find(m.right) != symList["$all"].end()) {
						// 全局变量分配t寄存器
						rightReg = allocRegT(m.right, resultReg[2] - '0', leftReg[2] - '0', reg_var_T, level, m.out);
						mips << "lw " << rightReg << ", " << m.right << endl;
					}
					// 查询结果是否占用寄存器
					for (i = 0; i < reg_var_T.size(); i++)
						if (reg_var_T[i] == m.result) resultReg = "$t" + int2str(i);
					// 给结果分配寄存器
					if (resultReg != "$t*");
					else if (color.find(m.result) != color.end()) {
						// 需要使用s寄存器
						resultReg = "$s" + int2str(color[m.result]);
						if (reg_var_S[color[m.result]] != m.result) {
							reg_var_S[color[m.result]] = m.result;
						}
					}
					else if (symList[level].find(m.result) != symList[level].end()) {
						// 原程序局部变量分配t寄存器
						resultReg = allocRegT(m.result, leftReg[2] - '0', rightReg[2] - '0', reg_var_T, level, m.out);
					}
					else if (tmpList[level].find(m.result) != tmpList[level].end()) {
						// 临时变量分配t寄存器
						resultReg = allocRegT(m.result, leftReg[2] - '0', rightReg[2] - '0', reg_var_T, level, m.out);
					}
					else if (symList["$all"].find(m.result) != symList["$all"].end()) {
						// 全局变量分配t寄存器
						resultReg = allocRegT(m.result, leftReg[2] - '0', rightReg[2] - '0', reg_var_T, level, m.out);
					}
					// 进行计算
					if (m.op == ADD) {
						if (imm == "") mips << "addu " << resultReg << ", " << leftReg << ", " << rightReg << endl;
						else if (immLeft) mips << "addiu "<< resultReg << ", " << rightReg << ", " << imm << endl;
						else mips << "addiu " << resultReg << ", " << leftReg << ", " << imm << endl;
					}
					else if (m.op == SUB) {
						if (imm == "") mips << "subu " << resultReg << ", " << leftReg << ", " << rightReg << endl;
						else  mips << "subiu " << resultReg << ", " << leftReg << ", " << imm << endl;
					}
					else if (m.op == MUL) {
						if (imm == "") mips << "mul " << resultReg << ", " << leftReg << ", " << rightReg << endl;
						else if (immLeft) mips << "mul " << resultReg << ", " << rightReg << ", " << imm << endl;
						else mips << "mul " << resultReg << ", " << leftReg << ", " << imm << endl;
					}
					else if (m.op == DIVI) {
						mips << "div " << leftReg << ", " << rightReg << endl;
						mips << "mflo " << resultReg << endl;
					}
					else if (m.op == LESS) {
						if (imm == "") mips << "slt " << resultReg << ", " << leftReg << ", " << rightReg << endl;
						else  mips << "slti " << resultReg << ", " << leftReg << ", " << imm << endl;
					}
					else {
						if (m.op == LE) mips << "sle ";
						else if (m.op == GREAT) mips << "sgt ";
						else if (m.op == GE) mips << "sge ";
						else if (m.op == EQUAL) mips << "seq ";
						else if (m.op == UNEQUAL) mips << "sne ";
						if (imm == "") mips << resultReg << ", " << leftReg << ", " << rightReg << endl;
						else  mips << resultReg << ", " << leftReg << ", " << imm << endl;
					}
				}
				else if (m.op == GETARR) {
					// 查询结果和左右操作数是否占用寄存器
					for (i = 0; i < reg_var_T.size(); i++) {
						if (reg_var_T[i] == m.left) leftReg = "$t" + int2str(i);
						if (reg_var_T[i] == m.right) rightReg = "$t" + int2str(i);
						if (reg_var_T[i] == m.result) resultReg = "$t" + int2str(i);
					}
					// 给左操作数（数组）分配寄存器
					if (leftReg == "$t*" && symList["$all"].find(m.left) == symList["$all"].end()) {
						leftReg = allocRegT("", resultReg[2] - '0', rightReg[2] - '0', reg_var_T, level, m.out);
					}
					// 查询结果和右操作数是否占用寄存器
					for (i = 0; i < reg_var_T.size(); i++) {
						if (reg_var_T[i] == m.right) rightReg = "$t" + int2str(i);
						if (reg_var_T[i] == m.result) resultReg = "$t" + int2str(i);
					}
					// 给右操作数分配寄存器
					if (rightReg != "$t*");
					else if (isNumber(m.right)) {
						// 立即数，记录，分配
						rightReg = allocRegT(m.right, resultReg[2] - '0', leftReg[2] - '0', reg_var_T, level, m.out);
						imm = m.right;
					}
					else if (color.find(m.right) != color.end()) {
						// 需要使用s寄存器
						rightReg = "$s" + int2str(color[m.right]);
						if (reg_var_S[color[m.right]] != m.right) {
							reg_var_S[color[m.right]] = m.right;
							if (symList[level].find(m.right) != symList[level].end()) offset = symList[level][m.right].addr;
							else if (tmpList[level].find(m.right) != tmpList[level].end()) offset = tmpList[level][m.right].addr;
							mips << "lw " << rightReg << ", " << offset << "($sp)	# " << m.right << endl;
						}
					}
					else if (symList[level].find(m.right) != symList[level].end()) {
						// 原程序局部变量分配t寄存器
						rightReg = allocRegT(m.right, resultReg[2] - '0', leftReg[2] - '0', reg_var_T, level, m.out);
						offset = symList[level][m.right].addr;
						mips << "lw " << rightReg << ", " << offset << "($sp)	# " << m.right << endl;
					}
					else if (tmpList[level].find(m.right) != tmpList[level].end()) {
						// 临时变量分配t寄存器
						rightReg = allocRegT(m.right, resultReg[2] - '0', leftReg[2] - '0', reg_var_T, level, m.out);
						offset = tmpList[level][m.right].addr;
						mips << "lw " << rightReg << ", " << offset << "($sp)	# " << m.right << endl;
					}
					else if (symList["$all"].find(m.right) != symList["$all"].end()) {
						// 全局变量分配t寄存器
						rightReg = allocRegT(m.right, resultReg[2] - '0', leftReg[2] - '0', reg_var_T, level, m.out);
						mips << "lw " << rightReg << ", " << m.right << endl;
					}
					// 查询结果是否占用寄存器
					for (i = 0; i < reg_var_T.size(); i++)
						if (reg_var_T[i] == m.result) resultReg = "$t" + int2str(i);
					// 给结果分配寄存器
					if (resultReg != "$t*" && resultReg != rightReg);
					else if (color.find(m.result) != color.end()) {
						// 需要使用s寄存器
						resultReg = "$s" + int2str(color[m.result]);
						if (reg_var_S[color[m.result]] != m.result) {
							reg_var_S[color[m.result]] = m.result;
						}
					}
					else if (symList[level].find(m.result) != symList[level].end()) {
						// 原程序局部变量分配t寄存器
						resultReg = allocRegT(m.result, leftReg[2] - '0', rightReg[2] - '0', reg_var_T, level, m.out);
					}
					else if (tmpList[level].find(m.result) != tmpList[level].end()) {
						// 临时变量分配t寄存器
						resultReg = allocRegT(m.result, leftReg[2] - '0', rightReg[2] - '0', reg_var_T, level, m.out);
					}
					else if (symList["$all"].find(m.result) != symList["$all"].end()) {
						// 全局变量分配t寄存器
						resultReg = allocRegT(m.result, leftReg[2] - '0', rightReg[2] - '0', reg_var_T, level, m.out);
					}
					// 计算数组内部偏移
					if (imm == "") {
						if (rightReg[1] == 's') {
							offsetReg = allocRegT("", resultReg[2] - '0', leftReg[2] - '0', reg_var_T, level, m.out);
							mips << "move " << offsetReg << ", " << rightReg << endl;
						}
						else
							offsetReg = rightReg;
						mips << "sll " << offsetReg << ", " << offsetReg << ", 2" << endl;
						reg_var_T[offsetReg[2] - '0'] = "";
					}
					else imm = int2str(str2int(imm) * 4);
					// 局部数组用偏移值取值
					if (symList[level].find(m.left) != symList[level].end()) {
						offset = symList[level][m.left].addr;
						if (imm == "") {
							mips << "addu " << offsetReg << ", " << offsetReg << ", $sp" << endl;
						}
						else {
							offsetReg = rightReg;
							mips << "addiu " << offsetReg << ", $sp" << ", " << imm << endl;
							reg_var_T[offsetReg[2] - '0'] = "";
						}
						mips << "lw " << resultReg << ", " << offset << "(" << offsetReg << ")	# " << m.left << "[" << m.right << "]" << endl;
					}
					// 全局数组直接取值
					else if (symList["$all"].find(m.left) != symList["$all"].end()) {
						if (imm != "") {
							offsetReg = rightReg;
							mips << "li " << offsetReg << ", " << imm << endl;
							reg_var_T[offsetReg[2] - '0'] = "";
						}
						mips << "lw " << resultReg << ", " << m.left << "(" << offsetReg << ")	# " << m.left << "[" << m.right << "]" << endl;
					}
				}
				else if (m.op == PUTARR) { 
					// 查询结果和左右操作数是否占用寄存器
					for (i = 0; i < reg_var_T.size(); i++) {
						if (reg_var_T[i] == m.left) leftReg = "$t" + int2str(i);
						if (reg_var_T[i] == m.right) rightReg = "$t" + int2str(i);
						if (reg_var_T[i] == m.result) resultReg = "$t" + int2str(i);
					}
					// 给左操作数分配寄存器
					if (leftReg != "$t*");
					else if (isNumber(m.left)) {
						// 立即数分配t寄存器
						leftReg = allocRegT(m.left, resultReg[2] - '0', rightReg[2] - '0', reg_var_T, level, m.out);
						mips << "li " << leftReg << ", " << m.left << endl;
					}
					else if (color.find(m.left) != color.end()) {
						// 需要使用s寄存器
						leftReg = "$s" + int2str(color[m.left]);
						if (reg_var_S[color[m.left]] != m.left) {
							reg_var_S[color[m.left]] = m.left;
							if (symList[level].find(m.left) != symList[level].end()) offset = symList[level][m.left].addr;
							else if (tmpList[level].find(m.left) != tmpList[level].end()) offset = tmpList[level][m.left].addr;
							mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
						}
					}
					else if (symList[level].find(m.left) != symList[level].end()) {
						// 原程序局部变量分配t寄存器
						leftReg = allocRegT(m.left, resultReg[2] - '0', rightReg[2] - '0', reg_var_T, level, m.out);
						offset = symList[level][m.left].addr;
						mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
					}
					else if (tmpList[level].find(m.left) != tmpList[level].end()) {
						// 临时变量分配t寄存器
						leftReg = allocRegT(m.left, resultReg[2] - '0', rightReg[2] - '0', reg_var_T, level, m.out);
						offset = tmpList[level][m.left].addr;
						mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
					}
					else if (symList["$all"].find(m.left) != symList["$all"].end()) {
						// 全局变量分配t寄存器
						leftReg = allocRegT(m.left, resultReg[2] - '0', rightReg[2] - '0', reg_var_T, level, m.out);
						mips << "lw " << leftReg << ", " << m.left << endl;
					}
					// 查询结果和右操作数是否占用寄存器
					for (i = 0; i < reg_var_T.size(); i++) {
						if (reg_var_T[i] == m.right) rightReg = "$t" + int2str(i);
						if (reg_var_T[i] == m.result) resultReg = "$t" + int2str(i);
					}
					// 给右操作数分配寄存器
					if (rightReg != "$t*" && rightReg != leftReg);
					else if (isNumber(m.right)) {
						// 立即数，分配t寄存器
						rightReg = allocRegT(m.right, resultReg[2] - '0', leftReg[2] - '0', reg_var_T, level, m.out);
						mips << "li " << rightReg << ", " << m.right << endl;
					}
					else if (color.find(m.right) != color.end()) {
						// 需要使用s寄存器
						rightReg = "$s" + int2str(color[m.right]);
						if (reg_var_S[color[m.right]] != m.right) {
							reg_var_S[color[m.right]] = m.right;
							if (symList[level].find(m.right) != symList[level].end()) offset = symList[level][m.right].addr;
							else if (tmpList[level].find(m.right) != tmpList[level].end()) offset = tmpList[level][m.right].addr;
							mips << "lw " << rightReg << ", " << offset << "($sp)	# " << m.right << endl;
						}
					}
					else if (symList[level].find(m.right) != symList[level].end()) {
						// 原程序局部变量分配t寄存器
						rightReg = allocRegT(m.right, resultReg[2] - '0', leftReg[2] - '0', reg_var_T, level, m.out);
						offset = symList[level][m.right].addr;
						mips << "lw " << rightReg << ", " << offset << "($sp)	# " << m.right << endl;
					}
					else if (tmpList[level].find(m.right) != tmpList[level].end()) {
						// 临时变量分配t寄存器
						rightReg = allocRegT(m.right, resultReg[2] - '0', leftReg[2] - '0', reg_var_T, level, m.out);
						offset = tmpList[level][m.right].addr;
						mips << "lw " << rightReg << ", " << offset << "($sp)	# " << m.right << endl;
					}
					else if (symList["$all"].find(m.right) != symList["$all"].end()) {
						// 全局变量分配t寄存器
						rightReg = allocRegT(m.right, resultReg[2] - '0', leftReg[2] - '0', reg_var_T, level, m.out);
						mips << "lw " << rightReg << ", " << m.right << endl;
					}
					// 查询结果是否占用寄存器
					for (i = 0; i < reg_var_T.size(); i++)
						if (reg_var_T[i] == m.result) resultReg = "$t" + int2str(i);
					// 给结果（数组）分配寄存器
					if (resultReg == "$t*" && symList["$all"].find(m.result) == symList["$all"].end()) {
						resultReg = allocRegT("", leftReg[2] - '0', rightReg[2] - '0', reg_var_T, level, m.out);
					}
					// 计算偏移
					if (leftReg[1] == 's') {
						offsetReg = allocRegT("", resultReg[2] - '0', rightReg[2] - '0', reg_var_T, level, m.out);
						mips << "move " << offsetReg << ", " << leftReg << endl;
					}
					else
						offsetReg = leftReg;
					mips << "sll " << offsetReg << ", " << offsetReg << ", 2" << endl;
					reg_var_T[offsetReg[2] - '0'] = "";
					// 存入结果
					if (symList[level].find(m.result) != symList[level].end()) {
						offset = symList[level][m.result].addr;
						mips << "addu " << offsetReg << ", " << offsetReg << ", $sp" << endl;
						mips << "sw " << rightReg << ", " << offset << "(" << offsetReg << ")	# ";
						outputMidcode(m, mips);
					}
					else if (symList["$all"].find(m.result) != symList["$all"].end()) {
						mips << "sw " << rightReg << ", " << m.result << "(" << offsetReg << ")	#";
						outputMidcode(m, mips);
					}
				}
				// 设置标签
				else if (m.op == SETLABEL) mips << m.result << ":" << endl;
				// 无条件跳转
				else if (m.op == GOTO) {
					// 写回t寄存器值
					mips << "\t\t # save t in block" << endl;
					saved = true;
					for (i = 0; i < reg_var_T.size(); i++) {
						if (blocks[block_num].out.find(reg_var_T[i]) != blocks[block_num].out.end() ||
							(symList["$all"].find(reg_var_T[i]) != symList["$all"].end() && symList["$all"][reg_var_T[i]].kind == 0)) {
							if (symList[level].find(reg_var_T[i]) != symList[level].end() 
								&& (symList[level][reg_var_T[i]].kind == 0 || symList[level][reg_var_T[i]].kind == 3)
								&& (symList[level][reg_var_T[i]].type == 5 || symList[level][reg_var_T[i]].type == 6)) {
								offset = symList[level][reg_var_T[i]].addr;
								mips << "sw $t" << i << ", " << offset << "($sp)	# " << reg_var_T[i] << endl;
							}
							else if (tmpList[level].find(reg_var_T[i]) != tmpList[level].end()) {
								offset = tmpList[level][reg_var_T[i]].addr;
								mips << "sw $t" << i << ", " << offset << "($sp)	# " << reg_var_T[i] << endl;
							}
							else if (symList["$all"].find(reg_var_T[i]) != symList["$all"].end() && symList["$all"][reg_var_T[i]].kind == 0
								&& (symList["$all"][reg_var_T[i]].type == 5 || symList["$all"][reg_var_T[i]].type == 6)) {
								mips << "sw $t" << i << ", " << reg_var_T[i] << endl;
							}
						}
					}
					// 跳转
					mips << "j " << m.result << endl;
				}
				// 满足/不满足跳转
				else if (m.op == BNZ || m.op == BZ) {
					// 查询条件是否占用寄存器
					for (i = 0; i < reg_var_T.size(); i++) {
						if (reg_var_T[i] == m.left) leftReg = "$t" + int2str(i);
					}
					// 给条件分配寄存器
					if (leftReg != "$t*");
					else if (isNumber(m.left)) {
						// 立即数分配t寄存器
						leftReg = allocRegT(m.left, resultReg[2] - '0', -1, reg_var_T, level, m.out);
						mips << "li " << leftReg << ", " << m.left << endl;
					}
					else if (color.find(m.left) != color.end()) {
						// 需要使用s寄存器
						leftReg = "$s" + int2str(color[m.left]);
						if (reg_var_S[color[m.left]] != m.left) {
							reg_var_S[color[m.left]] = m.left;
							if (symList[level].find(m.left) != symList[level].end()) offset = symList[level][m.left].addr;
							else if (tmpList[level].find(m.left) != tmpList[level].end()) offset = tmpList[level][m.left].addr;
							mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
						}
					}
					else if (symList[level].find(m.left) != symList[level].end()) {
						// 原程序局部变量分配t寄存器
						leftReg = allocRegT(m.left, resultReg[2] - '0', -1, reg_var_T, level, m.out);
						offset = symList[level][m.left].addr;
						mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
					}
					else if (tmpList[level].find(m.left) != tmpList[level].end()) {
						// 临时变量分配t寄存器
						leftReg = allocRegT(m.left, resultReg[2] - '0', -1, reg_var_T, level, m.out);
						offset = tmpList[level][m.left].addr;
						mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
					}
					else if (symList["$all"].find(m.left) != symList["$all"].end()) {
						// 全局变量分配t寄存器
						leftReg = allocRegT(m.left, resultReg[2] - '0', -1, reg_var_T, level, m.out);
						mips << "lw " << leftReg << ", " << m.left << endl;
					}
					// 写回t寄存器值
					mips << "\t\t # save t in block" << endl;
					saved = true;
					for (i = 0; i < reg_var_T.size(); i++) {
						if (blocks[block_num].out.find(reg_var_T[i]) != blocks[block_num].out.end() ||
							(symList["$all"].find(reg_var_T[i]) != symList["$all"].end() && symList["$all"][reg_var_T[i]].kind == 0)) {
							if (symList[level].find(reg_var_T[i]) != symList[level].end() 
								&& (symList[level][reg_var_T[i]].kind == 0 || symList[level][reg_var_T[i]].kind == 3)
								&& (symList[level][reg_var_T[i]].type == 5 || symList[level][reg_var_T[i]].type == 6)) {
								offset = symList[level][reg_var_T[i]].addr;
								mips << "sw $t" << i << ", " << offset << "($sp)	# " << reg_var_T[i] << endl;
							}
							else if (tmpList[level].find(reg_var_T[i]) != tmpList[level].end()) {
								offset = tmpList[level][reg_var_T[i]].addr;
								mips << "sw $t" << i << ", " << offset << "($sp)	# " << reg_var_T[i] << endl;
							}
							else if (symList["$all"].find(reg_var_T[i]) != symList["$all"].end() && symList["$all"][reg_var_T[i]].kind == 0
								&& (symList["$all"][reg_var_T[i]].type == 5 || symList["$all"][reg_var_T[i]].type == 6)) {
								mips << "sw $t" << i << ", " << reg_var_T[i] << endl;
							}
						}
					}
					// 跳转
					if (m.op == BNZ) mips << "bne $0, " << leftReg << ", " << m.result << endl;
					if (m.op == BZ) mips << "beq $0, " << leftReg << ", " << m.result << endl;
				}
				// 读写操作 
				else if (m.op == READ) {
					// 读原程序的局部变量
					if (symList[level].find(m.left) != symList[level].end()) {
						if (symList[level][m.left].type == 6) type = 12;
						else type = 5;
						mips << "li $v0, " << type << endl;
						mips << "syscall" << endl;
					}
					else if (tmpList[level].find(m.left) != tmpList[level].end()) {
						if (tmpList[level][m.left].type == 6) type = 12;
						else type = 5;
						mips << "li $v0, " << type << endl;
						mips << "syscall" << endl;
					}
					else if (symList["$all"].find(m.left) != symList["$all"].end()) {
						if (symList["$all"][m.left].type == 6) type = 12;
						else type = 5;
						mips << "li $v0, " << type << endl;
						mips << "syscall" << endl;
					}
					// 查询结果是否占用寄存器
					for (i = 0; i < reg_var_T.size(); i++)
						if (reg_var_T[i] == m.left) leftReg = "$t" + int2str(i);
					// 给结果分配寄存器
					if (leftReg != "$t*");
					else if (color.find(m.left) != color.end()) {
						// 需要使用s寄存器
						leftReg = "$s" + int2str(color[m.left]);
						if (reg_var_S[color[m.left]] != m.left) {
							reg_var_S[color[m.left]] = m.left;
						}
					}
					else if (symList[level].find(m.left) != symList[level].end()) {
						// 原程序局部变量分配t寄存器
						leftReg = allocRegT(m.left, -1, -1, reg_var_T, level, m.out);
					}
					else if (tmpList[level].find(m.left) != tmpList[level].end()) {
						// 临时变量分配t寄存器
						leftReg = allocRegT(m.left, -1, -1, reg_var_T, level, m.out);
					}
					else if (symList["$all"].find(m.left) != symList["$all"].end()) {
						// 全局变量分配t寄存器
						leftReg = allocRegT(m.left, -1, -1, reg_var_T, level, m.out);
					}
					// 将结果存到寄存器中
					mips << "move " << leftReg << ", $v0" << endl;
				}
				else if (m.op == WSTRING) {
					mips << "li $v0, 4" << endl;
					mips << "la $a0, " << m.left << endl;
					mips << "syscall" << endl;
					mips << "li $v0, 11" << endl;
					mips << "li $a0, 10" << endl;
					mips << "syscall" << endl;
				}
				else if (m.op == WEXP) {
					// 查询左操作数是否占用寄存器
					for (i = 0; i < reg_var_T.size(); i++)
						if (reg_var_T[i] == m.left) leftReg = "$t" + int2str(i);
					// 给左操作数分配寄存器a0
					if (leftReg != "$t*");
					else if (isNumber(m.left)) {
						// 立即数分配a0寄存器
						leftReg = "$a0";
						mips << "li " << leftReg << ", " << m.left << endl;
					}
					else if (color.find(m.left) != color.end()) {
						// 需要使用s寄存器
						leftReg = "$s" + int2str(color[m.left]);
						if (reg_var_S[color[m.left]] != m.left) {
							reg_var_S[color[m.left]] = m.left;
							if (symList[level].find(m.left) != symList[level].end()) offset = symList[level][m.left].addr;
							else if (tmpList[level].find(m.left) != tmpList[level].end()) offset = tmpList[level][m.left].addr;
							mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
						}
					}
					else if (symList[level].find(m.left) != symList[level].end()) {
						// 原程序局部变量分配a0寄存器
						leftReg = "$a0";
						offset = symList[level][m.left].addr;
						mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
					}
					else if (tmpList[level].find(m.left) != tmpList[level].end()) {
						// 临时变量分配a0寄存器
						leftReg = "$a0";
						offset = tmpList[level][m.left].addr;
						mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
					}
					else if (symList["$all"].find(m.left) != symList["$all"].end()) {
						// 全局变量分配a0寄存器
						leftReg = "$a0";
						mips << "lw " << leftReg << ", " << m.left << endl;
					}
					// 确定表达式类型并输出
					if (m.right == "char") type = 11;
					else type = 1;
					if (leftReg != "$a0") mips << "move $a0, " << leftReg << endl;
					mips << "li $v0, " << type << endl;
					mips << "syscall" << endl;
					mips << "li $v0, 11" << endl;
					mips << "li $a0, 10" << endl;
					mips << "syscall" << endl;
				}
				else if (m.op == WSE) {
					// 输出字符串
					mips << "li $v0, 4" << endl;
					mips << "la $a0, " << m.left << endl;
					mips << "syscall" << endl;
					// 输出表达式
					// 查询右操作数是否占用寄存器
					for (i = 0; i < reg_var_T.size(); i++)
						if (reg_var_T[i] == m.right) rightReg = "$t" + int2str(i);
					// 给右操作数分配寄存器a0
					if (rightReg != "$t*");
					else if (isNumber(m.right)) {
						// 立即数分配a0寄存器
						rightReg = "$a0";
						mips << "li " << rightReg << ", " << m.right << endl;
					}
					else if (color.find(m.right) != color.end()) {
						// 需要使用s寄存器
						rightReg = "$s" + int2str(color[m.right]);
						if (reg_var_S[color[m.right]] != m.right) {
							reg_var_S[color[m.right]] = m.right;
							if (symList[level].find(m.right) != symList[level].end()) offset = symList[level][m.right].addr;
							else if (tmpList[level].find(m.right) != tmpList[level].end()) offset = tmpList[level][m.right].addr;
							mips << "lw " << rightReg << ", " << offset << "($sp)	# " << m.right << endl;
						}
					}
					else if (symList[level].find(m.right) != symList[level].end()) {
						// 原程序局部变量分配a0寄存器
						rightReg = "$a0";
						offset = symList[level][m.right].addr;
						mips << "lw " << rightReg << ", " << offset << "($sp)	# " << m.right << endl;
					}
					else if (tmpList[level].find(m.right) != tmpList[level].end()) {
						// 临时变量分配a0寄存器
						rightReg = "$a0";
						offset = tmpList[level][m.right].addr;
						mips << "lw " << rightReg << ", " << offset << "($sp)	# " << m.right << endl;
					}
					else if (symList["$all"].find(m.right) != symList["$all"].end()) {
						// 全局变量分配a0寄存器
						rightReg = "$a0";
						mips << "lw " << rightReg << ", " << m.right << endl;
					}
					// 确定表达式类型并输出
					if (m.result == "char") type = 11;
					else type = 1;
					if (rightReg != "$a0") mips << "move $a0, " << rightReg << endl;
					mips << "li $v0, " << type << endl;
					mips << "syscall" << endl;
					mips << "li $v0, 11" << endl;
					mips << "li $a0, 10" << endl;
					mips << "syscall" << endl;
				}
				// 函数声明(函数参数声明无mips)
				else if (m.op == FUN) {
					level = m.right;
					// 标签记录函数开始
					mips << m.right << ":" << endl;
					if (m.right != "main") {
						// 存函数返回地址进入主存
						mips << "sw $ra, " << addrSize[level] << "($sp)" << endl;
						// 存该函数需要的s寄存器的值进入主存
						set<int> usedColor;
						for (auto co : color)
							usedColor.insert(co.second);
						for (i = 0; i < reg_var_S.size(); i++) {
							if (usedColor.find(i) != usedColor.end())
								mips << "sw $s" << i << ", " << addrSize[level] + 4 + 4 * i << "($sp)" << endl;
						}
					}					
					/* 函数内部常数初始化，存入主存
					for (auto& n : symList[level]) {
						leftReg = allocRegT("", -1, -1, reg_var_T, level, m.out);
						if (n.second.kind == 1) {
							mips << "li " << leftReg << ", " << n.second.value << endl;
							mips << "sw " << leftReg << ", " << n.second.addr << "($sp)	# save const " << n.second.value << endl;
						}
					}*/
				}
				else if (m.op == PAR);
				// 函数参数操作
				else if (m.op == PARA) {
					// 查询左操作数是否占用寄存器
					for (i = 0; i < reg_var_T.size(); i++)
						if (reg_var_T[i] == m.left) leftReg = "$t" + int2str(i);
					// 给左操作数分配寄存器
					if (leftReg != "$t*");
					else if (isNumber(m.left)) {
						// 立即数分配t寄存器
						leftReg = allocRegT(m.left, -1, -1, reg_var_T, level, m.out);
						mips << "li " << leftReg << ", " << m.left << endl;
					}
					else if (color.find(m.left) != color.end()) {
						// 需要使用s寄存器
						leftReg = "$s" + int2str(color[m.left]);
						if (reg_var_S[color[m.left]] != m.left) {
							reg_var_S[color[m.left]] = m.left;
							if (symList[level].find(m.left) != symList[level].end()) offset = symList[level][m.left].addr;
							else if (tmpList[level].find(m.left) != tmpList[level].end()) offset = tmpList[level][m.left].addr;
							mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
						}
					}
					else if (symList[level].find(m.left) != symList[level].end()) {
						// 原程序局部变量分配t寄存器
						leftReg = allocRegT(m.left, -1, -1, reg_var_T, level, m.out);
						offset = symList[level][m.left].addr;
						mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
					}
					else if (tmpList[level].find(m.left) != tmpList[level].end()) {
						// 临时变量分配t寄存器
						leftReg = allocRegT(m.left, -1, -1, reg_var_T, level, m.out);
						offset = tmpList[level][m.left].addr;
						mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
					}
					else if (symList["$all"].find(m.left) != symList["$all"].end()) {
						// 全局变量分配t寄存器
						leftReg = allocRegT(m.left, resultReg[2] - '0', -1, reg_var_T, level, m.out);
						mips << "lw " << leftReg << ", " << m.left << endl;
					}
					// 回存到下一函数区域
					if (symList[m.result].find(m.right) != symList[m.result].end()) {
						offset = symList[m.result][m.right].addr - addrSize[m.result] - 36;
						mips << "sw " << leftReg << ", " << offset << "($sp)	# save para " << m.left << " in fun " << m.result << endl;
					}
				}
				// 函数调用
				else if (m.op == FUNCALL) {
					// 写回t寄存器存的值
					mips << "\t\t # save t in block" << endl;
					saved = true;
					for (i = 0; i < reg_var_T.size(); i++) {
						if (blocks[block_num].out.find(reg_var_T[i]) != blocks[block_num].out.end() ||
							(symList["$all"].find(reg_var_T[i]) != symList["$all"].end() && symList["$all"][reg_var_T[i]].kind == 0)) {
							if (symList[level].find(reg_var_T[i]) != symList[level].end() 
								&& (symList[level][reg_var_T[i]].kind == 0 || symList[level][reg_var_T[i]].kind == 3)
								&& (symList[level][reg_var_T[i]].type == 5 || symList[level][reg_var_T[i]].type == 6)) {
								offset = symList[level][reg_var_T[i]].addr;
								mips << "sw $t" << i << ", " << offset << "($sp)	# " << reg_var_T[i] << endl;
							}
							else if (tmpList[level].find(reg_var_T[i]) != tmpList[level].end()) {
								offset = tmpList[level][reg_var_T[i]].addr;
								mips << "sw $t" << i << ", " << offset << "($sp)	# " << reg_var_T[i] << endl;
							}
							else if (symList["$all"].find(reg_var_T[i]) != symList["$all"].end() && symList["$all"][reg_var_T[i]].kind == 0
								&& (symList["$all"][reg_var_T[i]].type == 5 || symList["$all"][reg_var_T[i]].type == 6)) {
								mips << "sw $t" << i << ", " << reg_var_T[i] << endl;
							}
						}
					}
					// 下移栈指针并跳转
					mips << "subi $sp, $sp, " << addrSize[m.left] + 36 << endl;
					mips << "jal " << m.left << endl;
				}
				// 返回语句
				else if (m.op == RET || m.op == RETX) {
					if (m.op == RETX) {
						// 查询左操作数是否占用寄存器
						for (i = 0; i < reg_var_T.size(); i++)
							if (reg_var_T[i] == m.left) leftReg = "$t" + int2str(i);
						// 给左操作数分配寄存器v0
						if (leftReg != "$t*");
						else if (isNumber(m.left)) {
							// 立即数分配v0寄存器
							leftReg = "$v0";
							mips << "li " << leftReg << ", " << m.left << endl;
						}
						else if (color.find(m.left) != color.end()) {
							// 需要使用s寄存器
							leftReg = "$s" + int2str(color[m.left]);
							if (reg_var_S[color[m.left]] != m.left) {
								reg_var_S[color[m.left]] = m.left;
								if (symList[level].find(m.left) != symList[level].end()) offset = symList[level][m.left].addr;
								else if (tmpList[level].find(m.left) != tmpList[level].end()) offset = tmpList[level][m.left].addr;
								mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
							}
						}
						else if (symList[level].find(m.left) != symList[level].end()) {
							// 原程序局部变量分配v0寄存器
							leftReg = "$v0";
							offset = symList[level][m.left].addr;
							mips << "lw " << leftReg << ", " << offset << "($sp)	# " << m.left << endl;
						}
						else if (tmpList[level].find(m.left) != tmpList[level].end()) {
							// 临时变量分配v0寄存器
							leftReg = "$v0";
							offset = tmpList[level][m.left].addr;
							mips << "lw " << leftReg << ", " << offset << "($sp)" << endl;
						}
						else if (symList["$all"].find(m.left) != symList["$all"].end()) {
							// 全局变量分配v0寄存器
							leftReg = "$v0";
							mips << "lw " << leftReg << ", " << m.left << endl;
						}
						if (leftReg != "$v0") mips << "move $v0, " << leftReg << endl;
					}
					// 取出返回地址ra
					mips << "lw $ra, " << addrSize[level] << "($sp)" << endl;
					// 取出s寄存器的值
					set<int> usedColor;
					for (auto co : color)
						usedColor.insert(co.second);
					for (i = 0; i < reg_var_S.size(); i++) {
						if (usedColor.find(i) != usedColor.end())
							mips << "lw $s" << i << ", " << addrSize[level] + 4 + 4 * i << "($sp)" << endl;
					}
					// 退栈
					mips << "addi $sp, $sp, " << addrSize[level] + 36 << endl;
					// 写回t寄存器存的全局变量的值
					mips << "\t\t # save t in block" << endl;
					for (i = 0; i < reg_var_T.size(); i++) {
						if (symList[level].find(reg_var_T[i]) == symList[level].end()
							&& symList["$all"].find(reg_var_T[i]) != symList["$all"].end() 
							&& symList["$all"][reg_var_T[i]].kind == 0
							&& (symList["$all"][reg_var_T[i]].type == 5 || symList["$all"][reg_var_T[i]].type == 6)) {
								mips << "sw $t" << i << ", " << reg_var_T[i] << endl;
						}
					}
					// 返回返回地址
					mips << "jr $ra" << endl;
				}
				// 返回值赋值语句
				else if (m.op == ASSRET) {
					// 查询结果是否占用寄存器
					for (i = 0; i < reg_var_T.size(); i++)
						if (reg_var_T[i] == m.result) resultReg = "$t" + int2str(i);
					// 给结果分配寄存器
					if (resultReg != "$t*");
					else if (color.find(m.result) != color.end()) {
						// 需要使用s寄存器
						resultReg = "$s" + int2str(color[m.result]);
						if (reg_var_S[color[m.result]] != m.result) {
							reg_var_S[color[m.result]] = m.result;
						}
					}
					else if (symList[level].find(m.result) != symList[level].end()) {
						// 原程序局部变量分配t寄存器
						resultReg = allocRegT(m.result, -1, -1, reg_var_T, level, m.out);
					}
					else if (tmpList[level].find(m.result) != tmpList[level].end()) {
						// 临时变量分配t寄存器
						resultReg = allocRegT(m.result, -1, -1, reg_var_T, level, m.out);
					}
					else if (symList["$all"].find(m.result) != symList["$all"].end()) {
						// 全局变量分配t寄存器
						resultReg = allocRegT(m.result, -1, -1, reg_var_T, level, m.out);
					}
					mips << "move " << resultReg << ", $v0" << endl;
				}
			}
			// 写回t寄存器存的值
			if (saved) continue;
			mips << "\t\t # save t in block" << endl;
			for (i = 0; i < reg_var_T.size(); i++) {
				if (blocks[block_num].out.find(reg_var_T[i]) != blocks[block_num].out.end() ||
					(symList["$all"].find(reg_var_T[i]) != symList["$all"].end() && symList["$all"][reg_var_T[i]].kind == 0)) {
					if (symList[level].find(reg_var_T[i]) != symList[level].end() 
						&& (symList[level][reg_var_T[i]].kind == 0 || symList[level][reg_var_T[i]].kind == 3)
						&& (symList[level][reg_var_T[i]].type == 5 || symList[level][reg_var_T[i]].type == 6)) {
						offset = symList[level][reg_var_T[i]].addr;
						mips << "sw $t" << i << ", " << offset << "($sp)	# " << reg_var_T[i] << endl;
					}
					else if (tmpList[level].find(reg_var_T[i]) != tmpList[level].end()) {
						offset = tmpList[level][reg_var_T[i]].addr;
						mips << "sw $t" << i << ", " << offset << "($sp)	# " << reg_var_T[i] << endl;
					}
					else if (symList["$all"].find(reg_var_T[i]) != symList["$all"].end() && symList["$all"][reg_var_T[i]].kind == 0
						&&(symList["$all"][reg_var_T[i]].type == 5 || symList["$all"][reg_var_T[i]].type == 6)) {
						mips << "sw $t" << i << ", " << reg_var_T[i] << endl;
					}
				}
			}
		}
	}
}

string allocRegT(string var, int protect1, int protect2, vector<string>& reg_var_T, string level, set<string> out) {
	int i, offset;
	// 找一个没被分配的寄存器
	for (i = 0; i < reg_var_T.size(); i++) {
		if (reg_var_T[i] == "") {
			reg_var_T[i] = var; 
			return "$t" + int2str(i);
		}
	}
	// 找一个不再活跃的非全局变量占用的寄存器
	for (i = 0; i < reg_var_T.size(); i++) {
		if (i != protect1 && i != protect2 && out.find(reg_var_T[i]) == out.end() && 
			symList["$all"].find(reg_var_T[i]) == symList["$all"].end()) {
			reg_var_T[i] = var;
			return "$t" + int2str(i);
		}
	}
	// 找一个存着立即数的寄存器
	for (i = 0; i < reg_var_T.size(); i++) {
		if (i != protect1 && i != protect2 && isNumber(reg_var_T[i])) {
			reg_var_T[i] = var;
			return "$t" + int2str(i);
		}
	}
	// 找一个不被保护的寄存器写回主存
	for (i = 0; i < reg_var_T.size(); i++) {
		if (i != protect1 && i != protect2) break;
	}
	if (symList[level].find(reg_var_T[i]) != symList[level].end()) {
		offset = symList[level][reg_var_T[i]].addr;
		mips << "sw $t" << i << ", " << offset << "($sp)";
	}
	else if (tmpList[level].find(reg_var_T[i]) != tmpList[level].end()) {
		offset = tmpList[level][reg_var_T[i]].addr;
		mips << "sw $t" << i << ", " << offset << "($sp)";
	}
	else if (symList["$all"].find(reg_var_T[i]) != symList["$all"].end())
		mips << "sw $t" << i << ", " << reg_var_T[i];
	mips << "\t	#save t" << i << " to alloc" << endl;
	reg_var_T[i] = var;
	return "$t" + int2str(i);
}

void outputOPTMID() {
	for (auto level : funs) {
		for (auto block_num : fun_blocks[level]) {
			for (auto m : blocks[block_num].midcodes) {
				outputMidcode(m, optmid);
			}
		}
	}
}

string int2str(int i) {
	stringstream ss;
	string s;
	ss << i;
	s = ss.str();
	return s;
}