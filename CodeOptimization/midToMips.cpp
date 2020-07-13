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
extern ofstream mid;

extern string level;

extern map<string, map<string, struct sym>> symList;
extern map<string, map<string, struct sym>> tmpList;
extern vector <struct midcode> midCodes;

map<string, int> addrSize;
map<string, int> symSize;
enum midop {NEG, ADD, SUB, MUL, DIVI, ASS, GETARR, PUTARR, SETLABEL, GOTO, BNZ, BZ, LESS, LE, GREAT, 
	GE, EQUAL, UNEQUAL, READ, WSTRING, WEXP, WSE, RET, RETX, PARA, FUNCALL, FUN, PAR, ASSRET};

bool isNumber(string s);
int str2int(string s);
void outputMidcode(struct midcode m, ostream& file);

void setAddr() {
	int addr;
	mips << ".data" << endl;
	for (auto& lev : symList) {
		if (lev.first == "$all") {
			for (auto& n : lev.second) {
				if (n.second.kind == 1)
					mips << n.first << ": .word " << n.second.value << endl;
				else if (n.second.kind == 0) {
					if (n.second.type == 5 || n.second.type == 6)
						mips << n.first << ": .space 4" << endl;
					else if (n.second.type == 3 || n.second.type == 4)
						mips << n.first << ": .space " << str2int(n.second.value) * 4 << endl;
				}
			}
			continue;
		}
		addr = 0;
		for (auto& n : lev.second) {
			n.second.addr = addr;
			if (n.second.type == 5 || n.second.type == 6) {
				addr += 4;
			} else if (n.second.type == 3 || n.second.type == 4) {
				addr += str2int(n.second.value) * 4;
			}
		}
		symSize[lev.first] = addr;
	}
	for (auto& lev : tmpList) {
		if (lev.first == "$all") {
			for (auto& n : lev.second) {
				mips << n.first << ": .asciiz \"" << n.second.value << "\"" << endl;
			}
			continue;
		}
		addr = symSize[lev.first];
		for (auto& n : lev.second) {
			n.second.addr = addr;
			addr += 4;
		}
		addrSize[lev.first] = addr;
	}
}

void getMips() {
	int i, offset, type;
	struct midcode m;
	string level = "$all";
	mips << ".text" << endl;
	mips << "subi $sp, $sp," << addrSize["main"] << "	# down sp" << endl;
	mips << "j main" << endl;

	for (i = 0; i < midCodes.size(); i++) {
		m = midCodes[i];
		mips << "\t\t\t\t\t\t\t\t\t\t # ";
		outputMidcode(m, mips);
		// 基本表达式运算、逻辑运算
		if ((m.op >= NEG && m.op <= ASS) || m.op == PUTARR || (m.op >= LESS && m.op <= UNEQUAL)) {
			// 给左操作数分配寄存器t1
			if (isNumber(m.left))
				mips << "li $t1, " << m.left;
			else if (symList[level].find(m.left) != symList[level].end()) {
				offset = symList[level][m.left].addr;
				mips << "lw $t1, " << offset << "($sp)";
			}
			else if (tmpList[level].find(m.left) != tmpList[level].end()) {
				offset = tmpList[level][m.left].addr;
				mips << "lw $t1, " << offset << "($sp)";
			}
			else if (symList["$all"].find(m.left) != symList["$all"].end())
				mips << "lw $t1, " << m.left;
			mips << "  # t1 = " << m.left << endl;
			// 计算单操作数的指令，结果放在t3里
			if (m.op == NEG) mips << "sub $t3, $0, $t1  #t3 = -t1" << endl;
			else if (m.op == ASS) mips << "move $t3, $t1  #  t3 = t1" << endl;
			// 计算双操作数指令
			else {
				// 给右操作数分配寄存器t2
				if (isNumber(m.right))
					mips << "li $t2, " << m.right;
				else if (symList[level].find(m.right) != symList[level].end()) {
					offset = symList[level][m.right].addr;
					mips << "lw $t2, " << offset << "($sp)";
				}
				else if (tmpList[level].find(m.right) != tmpList[level].end()) {
					offset = tmpList[level][m.right].addr;
					mips << "lw $t2, " << offset << "($sp)";
				}
				else if (symList["$all"].find(m.right) != symList["$all"].end())
					mips << "lw $t2, " << m.right;
				mips << "  # t2 = " << m.right << endl;
				// 计算，结果放在t3
				if (m.op == ADD) mips << "add $t3, $t1, $t2  # t3 = t1 + t2" << endl;
				else if (m.op == SUB) mips << "sub $t3, $t1, $t2  # t3 = t1 - t2" << endl;
				else if (m.op == MUL) {
					mips << "mult $t1, $t2" << endl;
					mips << "mflo $t3 # t3 = t1 * t2" << endl;
				}
				else if (m.op == DIVI) {
					mips << "div $t1, $t2" << endl;
					mips << "mflo $t3 # t3 = t1 / t2" << endl;
				}
				else if (m.op == LESS) mips << "slt $t3, $t1, $t2  # t3 = if (t1 < t2)" << endl;
				else if (m.op == LE) mips << "sle $t3, $t1, $t2  # t3 = if (t1 <= t2)" << endl;
				else if (m.op == GREAT) mips << "sgt $t3, $t1, $t2  # t3 = if (t1 > t2)" << endl;
				else if (m.op == GE) mips << "sge $t3, $t1, $t2  # t3 = if (t1 >= t2)" << endl;
				else if (m.op == EQUAL) mips << "seq $t3, $t1, $t2  # t3 = if (t1 == t2)" << endl;
				else if (m.op == UNEQUAL) mips << "sne $t3, $t1, $t2  # t3 = if (t1 != t2)" << endl;
			}
			// 回存结果
			if (m.op != PUTARR) {
				if (symList[level].find(m.result) != symList[level].end()) {
					offset = symList[level][m.result].addr;
					mips << "sw $t3, " << offset << "($sp)  # " << m.result << " = t3" << endl;
				}
				else if (tmpList[level].find(m.result) != tmpList[level].end()) {
					offset = tmpList[level][m.result].addr;
					mips << "sw $t3, " << offset << "($sp)  # " << m.result << " = t3" << endl;
				}
				else if (symList["$all"].find(m.result) != symList["$all"].end())
					mips << "sw $t3, " << m.result << "  # " << m.result << " = t3" << endl;
			}
			else {
				mips << "sll $t1, $t1, 2  # t1 = t1 * 4" << endl;
				if (symList[level].find(m.result) != symList[level].end()) {
					offset = symList[level][m.result].addr;
					mips << "addu $t1, $t1, $sp  # t1 = t1 + sp" << endl;
					mips << "sw $t2, " << offset << "($t1)  # " << m.result << "[t1] = t2" << endl;
				}
				else if (symList["$all"].find(m.result) != symList["$all"].end())
					mips << "sw $t2, " << m.result << "($t1)  # " << m.result << "[t1] = t2" << endl;
			}
		}
		else if (m.op == GETARR) {
			// 给右操作数分配寄存器t1
			if (isNumber(m.right))
				mips << "li $t1, " << m.right;
			else if (symList[level].find(m.right) != symList[level].end()) {
				offset = symList[level][m.right].addr;
				mips << "lw $t1, " << offset << "($sp)";
			}
			else if (tmpList[level].find(m.right) != tmpList[level].end()) {
				offset = tmpList[level][m.right].addr;
				mips << "lw $t1, " << offset << "($sp)";
			}
			else if (symList["$all"].find(m.right) != symList["$all"].end())
				mips << "lw $t1, " << m.right;
			mips << "  # t1 = " << m.right << endl;
			mips << "sll $t1, $t1, 2  # t1 = t1 * 4" << endl;
			// 对于全局数组，直接取值到t3，局部数组计算偏移值
			if (symList[level].find(m.left) != symList[level].end()) {
				offset = symList[level][m.left].addr;
				mips << "addu $t1, $t1, $sp  # t1 = t1 + sp" << endl;
				mips << "lw $t3, " << offset << "($t1)  # t3 = " << m.left << "[t1]" << endl;
			}
			else if (symList["$all"].find(m.left) != symList["$all"].end()) {
				mips << "lw $t3, " << m.left << "($t1)  # t3 = " << m.left << "[t1]" << endl;
			}// 结果存回
			if (symList[level].find(m.result) != symList[level].end()) {
				offset = symList[level][m.result].addr;
				mips << "sw $t3, " << offset << "($sp)  # " << m.result << " = t3" << endl;
			}
			else if (tmpList[level].find(m.result) != tmpList[level].end()) {
				offset = tmpList[level][m.result].addr;
				mips << "sw $t3, " << offset << "($sp)  # " << m.result << " = t3" << endl;
			}
			else if (symList["$all"].find(m.result) != symList["$all"].end())
				mips << "sw $t3, " << m.result << "  # " << m.result << " = t3" << endl;
		}
		// 设置标签
		else if (m.op == SETLABEL) mips << m.result << ":" << endl;
		// 无条件跳转
		else if (m.op == GOTO) mips << "j " << m.result << endl;
		// 满足/不满足跳转
		else if (m.op == BNZ || m.op == BZ) {
			// 给条件结果分配寄存器t0
			if (isNumber(m.left))
				mips << "li $t0, " << m.left;
			else if (symList[level].find(m.left) != symList[level].end()) {
				offset = symList[level][m.left].addr;
				mips << "lw $t0, " << offset << "($sp)";
			}
			else if (tmpList[level].find(m.left) != tmpList[level].end()) {
				offset = tmpList[level][m.left].addr;
				mips << "lw $t0, " << offset << "($sp)";
			}
			else if (symList["$all"].find(m.left) != symList["$all"].end())
				mips << "lw $t0, " << m.left;
			mips << "  # t0 = " << m.left << endl;
			if (m.op == BNZ) mips << "bne $0, $t0, " << m.result << endl;
			if (m.op == BZ) mips << "beq $0, $t0, " << m.result << endl;
		}
		// 读写操作
		else if (m.op == READ) {
			if (symList[level].find(m.left) != symList[level].end()) {
				if (symList[level][m.left].type == 6) type = 12;
				else type = 5;
				mips << "li $v0, " << type << endl;
				mips << "syscall" << endl;
				offset = symList[level][m.left].addr;
				mips << "sw $v0, " << offset << "($sp)  # scanf " << m.left << endl;
			}
			else if (symList["$all"].find(m.left) != symList["$all"].end()) {
				if (symList["$all"][m.left].type == 6) type = 12;
				else type = 5;
				mips << "li $v0, " << type << endl;
				mips << "syscall" << endl;
				mips << "sw $v0, " << m.left << "  # scanf " << m.left << endl;
			}
		}
		else if (m.op == WSTRING) {
			mips << "li $v0, 4" << endl;
			mips << "la $a0, " << m.left << endl;
			mips << "syscall  # printf \"" << tmpList["$all"][m.left].value << "\"" << endl;
			mips << "li $v0, 11" << endl;
			mips << "li $a0, 10" << endl;
			mips << "syscall  # printf enter" << endl;
		}
		else if (m.op == WEXP) {
			if (isNumber(m.left))
				mips << "li $a0, " << m.left;
			else if (symList[level].find(m.left) != symList[level].end()) {
				offset = symList[level][m.left].addr;
				mips << "lw $a0, " << offset << "($sp)";
			}
			else if (tmpList[level].find(m.left) != tmpList[level].end()) {
				offset = tmpList[level][m.left].addr;
				mips << "lw $a0, " << offset << "($sp)";
			}
			else if (symList["$all"].find(m.left) != symList["$all"].end())
				mips << "lw $a0, " << m.left;
			mips << "  # a0 = " << m.left << endl;
			if (m.right == "char") type = 11;
			else type = 1;
			mips << "li $v0, " << type << endl;
			mips << "syscall  # printf " << m.left << endl;
			mips << "li $v0, 11" << endl;
			mips << "li $a0, 10" << endl;
			mips << "syscall  # printf enter" << endl;
		}
		else if (m.op == WSE) {
			// 输出字符串
			mips << "li $v0, 4" << endl;
			mips << "la $a0, " << m.left << endl;
			mips << "syscall  # printf \"" << tmpList["$all"][m.left].value << "\"" << endl;
			// 输出表达式
			if (isNumber(m.right))
				mips << "li $a0, " << m.right;
			else if (symList[level].find(m.right) != symList[level].end()) {
				offset = symList[level][m.right].addr;
				mips << "lw $a0, " << offset << "($sp)";
			}
			else if (tmpList[level].find(m.right) != tmpList[level].end()) {
				offset = tmpList[level][m.right].addr;
				mips << "lw $a0, " << offset << "($sp)";
			}
			else if (symList["$all"].find(m.right) != symList["$all"].end())
				mips << "lw $a0, " << m.right;
			mips << "  # a0 = " << m.right << endl;
			if (m.result == "char") type = 11;
			else type = 1;
			mips << "li $v0, " << type << endl;
			mips << "syscall  # printf " << m.right << endl;
			// 输出回车
			mips << "li $v0, 11" << endl;
			mips << "li $a0, 10" << endl;
			mips << "syscall  # printf enter" << endl;
		}
		// 函数声明(函数参数声明无mips)
		else if (m.op == FUN) {
			level = m.right;
			mips << m.right << ":" << endl;
			mips << "sw $ra, " << addrSize[level] << "($sp)	#save ra" << endl;
			for (auto& n : symList[level]) {
				if (n.second.kind == 1) {
					mips << "li $t0, " << n.second.value << endl;
					mips << "sw $t0, " << n.second.addr << "($sp)  # " << n.first << " = " << n.second.value << endl;
				}
			}
		}
		else if (m.op == PAR);
		// 函数参数操作
		else if (m.op == PARA) {
			// 参数值存在t1
			if (isNumber(m.left))
				mips << "li $t1, " << m.left;
			else if (symList[level].find(m.left) != symList[level].end()) {
				offset = symList[level][m.left].addr;
				mips << "lw $t1, " << offset << "($sp)";
			}
			else if (tmpList[level].find(m.left) != tmpList[level].end()) {
				offset = tmpList[level][m.left].addr;
				mips << "lw $t1, " << offset << "($sp)";
			}
			else if (symList["$all"].find(m.left) != symList["$all"].end())
				mips << "lw $t1, " << m.left;
			mips << "  # t1 = " << m.left << endl;
			// 回存到下一函数区域
			if (symList[m.result].find(m.right) != symList[m.result].end()) {
				offset = symList[m.result][m.right].addr - addrSize[m.result] - 36;
				mips << "sw $t1, " << offset << "($sp)  # " << m.result << " = t1" << endl;
			}
		}
		// 函数调用
		else if (m.op == FUNCALL) {
			mips << "subi $sp, $sp, " << addrSize[m.left] + 36 << "	# down sp" << endl;
			mips << "jal " << m.left << "	# funcall" << endl;
		}
		// 返回语句
		else if (m.op == RET || m.op == RETX) {
			if (m.op == RETX) {
				// 左操作数赋值v0
				if (isNumber(m.left))
					mips << "li $v0, " << m.left;
				else if (symList[level].find(m.left) != symList[level].end()) {
					offset = symList[level][m.left].addr;
					mips << "lw $v0, " << offset << "($sp)";
				}
				else if (tmpList[level].find(m.left) != tmpList[level].end()) {
					offset = tmpList[level][m.left].addr;
					mips << "lw $v0, " << offset << "($sp)";
				}
				else if (symList["$all"].find(m.left) != symList["$all"].end())
					mips << "lw $v0, " << m.left;
				mips << "  # v0 = " << m.left << endl;
			}
			mips << "lw $ra, " << addrSize[level] << "($sp)	# get ra" << endl;
			mips << "addi $sp, $sp, " << addrSize[level] + 36 << "	# up sp" << endl;
			mips << "jr $ra	# return" << endl;
		}
		// 返回值赋值语句
		else if (m.op == ASSRET) {
			if (symList[level].find(m.result) != symList[level].end()) {
				offset = symList[level][m.result].addr;
				mips << "sw $v0, " << offset << "($sp)  # " << m.result << " = ret" << endl;
			}
			else if (tmpList[level].find(m.result) != tmpList[level].end()) {
				offset = tmpList[level][m.result].addr;
				mips << "sw $v0, " << offset << "($sp)  # " << m.result << " = ret" << endl;
			}
			else if (symList["$all"].find(m.result) != symList["$all"].end())
				mips << "sw $v0, " << m.result << "  # " << m.result << " = ret" << endl;
		}
		// else mips << "wait!" << endl;
	}
}

bool isNumber(string s) {
	if (s[0] >= '0' && s[0] <= '9') return true;
	else if (s[0] == '-' && s[1] >= '0' && s[1] <= '9') return true;
	else return false;
}

int str2int(string s) {
	int a;
	stringstream ss;
	ss << s;
	ss >> a;
	return a;
}

