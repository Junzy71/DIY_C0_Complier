#include <map>
#include <set>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
using namespace std;

struct sym { int kind; int type; string value; int addr; };
struct midcode { string result; string left; int op; string right; set<string> out; };

ifstream in;
ofstream out;
ofstream err;
ofstream mips;
ofstream mid;
ofstream optmid;

char c;
int line_num;
string level;
int tmpnum;
int labelnum;
int stringnum;

int symnum;
string word;
string classCode[36];

map<string, map<string, struct sym>> symList;
map<string, map<string, struct sym>> tmpList;
vector <struct midcode> midCodes;
map<string, vector<int>> fun_mids;

int getsym();
void outputsym();
void isProgram();
void init_code(string classCode[36]);
void init_reserved();
void error(char errNum);
void test_symList();
void outputMidcodes();
void setAddr();
void getMips();
void getBlock();
void activeAnalysis();
void assignPro();
void outputOPTMID();
void getMips_OPT();
void outputMidcode(struct midcode m, ostream& file);

int main() {
	map<string, struct sym> all;
	map<string, struct sym> all_tmp;
	line_num = 1;
	tmpnum = 0;
	labelnum = 1;
	level = "$all";
	symList[level] = all;
	tmpList[level] = all_tmp;
	init_code(classCode);
	init_reserved();

	in.open("testfile.txt", ios_base::in);
	out.open("output.txt", ios_base::out | ios_base::trunc);
	err.open("error.txt", ios_base::out | ios_base::trunc);
	mips.open("mips.txt", ios_base::out | ios_base::trunc);
	mid.open("midcode.txt", ios_base::out | ios_base::trunc);
	optmid.open("optcode.txt", ios_base::out | ios_base::trunc);
	
	// 词法分析与语法分析
	c = in.get();
	word = "";
	symnum = getsym();
	isProgram();
	
	// 输出原始中间代码
	outputMidcodes();

	if (in.get() != EOF) { error('0'); }
	in.close();
	out.close();
	mid.close();
	err.close();
	
	// 代码优化
	getBlock();
	activeAnalysis();
	assignPro();
	outputOPTMID();

	
	// 代码生成
	setAddr();
	// 测试符号表
	test_symList();
	// getMips();
	getMips_OPT();

	mips.close();
	optmid.close();

	return 0;
}

void init_code(string classCode[36]) {
	classCode[0] = "IDENFR";
	classCode[1] = "INTCON";
	classCode[2] = "CHARCON";
	classCode[3] = "STRCON";
	classCode[4] = "CONSTTK";
	classCode[5] = "INTTK";
	classCode[6] = "CHARTK";
	classCode[7] = "VOIDTK";
	classCode[8] = "MAINTK";
	classCode[9] = "IFTK";
	classCode[10] = "ELSETK";
	classCode[11] = "DOTK";
	classCode[12] = "WHILETK";
	classCode[13] = "FORTK";
	classCode[14] = "SCANFTK";
	classCode[15] = "PRINTFTK";
	classCode[16] = "RETURNTK";
	classCode[17] = "PLUS";
	classCode[18] = "MINU";
	classCode[19] = "MULT";
	classCode[20] = "DIV";
	classCode[21] = "LSS";
	classCode[22] = "LEQ";
	classCode[23] = "GRE";
	classCode[24] = "GEQ";
	classCode[25] = "EQL";
	classCode[26] = "NEQ";
	classCode[27] = "ASSIGN";
	classCode[28] = "SEMICN";
	classCode[29] = "COMMA";
	classCode[30] = "LPARENT";
	classCode[31] = "RPARENT";
	classCode[32] = "LBRACK";
	classCode[33] = "RBRACK";
	classCode[34] = "LBRACE";
	classCode[35] = "RBRACE";
}

void outputsym() {
	if (symnum >= 0 && symnum <36)
		out << classCode[symnum] << " " << word << endl;
	if (symnum == 1)
        out << "<无符号整数>" << endl;
}

string getTmp() {
	stringstream ss;
	string s;
	ss << "t$" << tmpnum++;
	s = ss.str();
	tmpList[level][s] = { 0, 5 };
	return s;
}

string getLabel() {
	stringstream ss;
	string s;
	ss << "label$" << labelnum++;
	s = ss.str();
	return s;
}

void addMidCode(string result, string left, int op, string right) {
	struct midcode midcode = { result, left, op, right };
	if (fun_mids.find(level) == fun_mids.end())
		fun_mids[level] = {};
	fun_mids[level].push_back(midCodes.size());
	midCodes.push_back(midcode);
}

void outputMidcodes() {
	for (auto m: midCodes)
		outputMidcode(m, mid);
}

void outputMidcode(struct midcode m, ostream& file) {
	string result = m.result;
	string left = m.left;
	int op = m.op;
	string right = m.right;
	enum midop {NEG, ADD, SUB, MUL, DIVI, ASS, GETARR, PUTARR, SETLABEL, GOTO, BNZ, BZ, LESS, LE, GREAT,
		GE, EQUAL, UNEQUAL, READ, WSTRING, WEXP, WSE, RET, RETX, PARA, FUNCALL, FUN, PAR, ASSRET};
	if (op == NEG) file << result << " = -" << left << endl;
	else if (op == ADD) file << result << " = " << left << " + " << right << endl;
	else if (op == SUB) file << result << " = " << left << " - " << right << endl;
	else if (op == MUL) file << result << " = " << left << " * " << right << endl;
	else if (op == DIVI) file << result << " = " << left << " / " << right << endl;
	else if (op == ASS) file << result << " = " << left << endl;
	else if (op == GETARR) file << result << " = " << left << "[" << right << "]" << endl;
	else if (op == PUTARR) file << result << "[" << left << "] = " << right << endl;
	else if (op == SETLABEL) file << result << ":" << endl;
	else if (op == GOTO) file << "GOTO " << result << endl;
	else if (op == BNZ) file << "BNZ " << result << endl;
	else if (op == BZ) file << "BZ " << result << endl;
	else if (op == LESS) file << left << " < " << right << endl;
	else if (op == LE) file << left << " <= " << right << endl;
	else if (op == GREAT) file << left << " > " << right << endl;
	else if (op == GE) file << left << " >= " << right << endl;
	else if (op == EQUAL) file << left << " == " << right << endl;
	else if (op == UNEQUAL) file << left << " != " << right << endl;
	else if (op == READ) file << "scanf " << left << endl;
	else if (op == WSTRING) file << "printf \"" << tmpList["$all"][left].value << "\"" << endl;
	else if (op == WEXP) file << "printf " << left << endl;
	else if (op == WSE) file << "printf \"" << tmpList["$all"][left].value << "\"" << right << endl;
	else if (op == RET) file << "ret" << endl;
	else if (op == RETX) file << "ret " << left << endl;
	else if (op == PARA) file << "push " << left << endl;
	else if (op == FUNCALL) file << "call " << left << endl;
	else if (op == FUN) file << left << " " << right << "()" << endl;
	else if (op == PAR) file << "para " << left << " " << right << endl;
	else if (op == ASSRET) file << result << " = RET" << endl;
	else  mid << "error!" << endl;
}

void test_symList() {
	// 符号表建立检查
	cout << "symList:" << endl;
	for (auto& lev : symList) {
		cout << "level:" << lev.first << endl;
		for (auto& n : lev.second) {
			cout << "\tname:" << n.first << "\t kind:";
			if (n.second.kind == 0) cout << "var\ttype:";
			else if (n.second.kind == 1) cout << "const\ttype:";
			else if (n.second.kind == 2) cout << "fun\ttype:";
			else if (n.second.kind == 3) cout << "para\ttype:";
			else cout << "error\ttype:";
			if (n.second.type == 5) cout << "int";
			else if (n.second.type == 6) cout << "char";
			else if (n.second.type == 3) cout << "int[]";
			else if (n.second.type == 4) cout << "char[]";
			else if (n.second.type == 0) cout << "void";
			else cout << "error";
			cout << "\taddr:" << n.second.addr;
			if (n.second.kind == 1 || n.second.kind == 2) cout << "\tvalue:" << n.second.value;
			else if (n.second.type == 3 || n.second.type == 4) cout << "\tsize:" << n.second.value;
			cout << endl;
		}
	}
	cout << "tmpList:" << endl;
	for (auto& lev : tmpList) {
		cout << "level:" << lev.first << endl;
		if (lev.first == "$all") {
			for (auto& n : lev.second) {
				cout << "\tname:" << n.first << "\t value:";
				cout << n.second.value << endl;
			}
		} else {
			for (auto& n : lev.second) {
				cout << "\tname:" << n.first << "\t kind:";
				if (n.second.kind == 0) cout << "var\ttype:";
				else if (n.second.kind == 1) cout << "const\ttype:";
				else if (n.second.kind == 2) cout << "fun\ttype:";
				else if (n.second.kind == 3) cout << "para\ttype:";
				else cout << "error\ttype:";
				if (n.second.type == 5) cout << "int";
				else if (n.second.type == 6) cout << "char";
				else if (n.second.type == 3) cout << "int[]";
				else if (n.second.type == 4) cout << "char[]";
				else if (n.second.type == 0) cout << "void";
				else cout << "error";
				cout << "\taddr:" << n.second.addr << endl;
			}
		}		
	}
}