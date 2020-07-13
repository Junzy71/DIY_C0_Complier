#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
using namespace std;

struct sym { int kind; int type; string value; int addr; };

extern ifstream in;
extern ofstream out;
extern char c;
extern string level;
extern int symnum;
extern string word;
extern map<string, map<string, struct sym>> symList;
enum classnum {IDENFR, INTCON, CHARCON, STRCON, CONSTTK, INTTK, CHARTK, VOIDTK, MAINTK, IFTK, ELSETK, DOTK, WHILETK, FORTK, SCANFTK, PRINTFTK, 
	RETURNTK, PLUS, MINU, MULT, DIV, LSS, LEQ, GRE, GEQ, EQL, NEQ, ASSIGN, SEMICN, COMMA, LPARENT, RPARENT, LBRACK, RBRACK, LBRACE, RBRACE};
enum midop { NEG, ADD, SUB, MUL, DIVI, ASS, GETARR, PUTARR, SETLABEL, GOTO, BNZ, BZ, LESS, LE, GREAT,
	GE, EQUAL, UNEQUAL, READ, WSTRING, WEXP, WSE, RET, RETX, PARA, FUNCALL, FUN, PAR, ASSRET };

int getsym();
void outputsym();
void isConst();
int isTerm(string& var);
int isFactor(string& var);
string isFun_call(string idenfr);
void error(char errNum);
string getTmp();
void addMidCode(string result, string left, int op, string right);

// 表达式
int isExpression(string& var) {
	int type = -1;
	int opcode = -1;
	string tmp1, tmp2, tmp3;
    if (symnum == PLUS || symnum == MINU) {
		if (symnum == MINU) opcode = NEG;
        outputsym();
        symnum = getsym();
    }
    type = isTerm(tmp1);
	if (opcode == 0) { 
		tmp3 = getTmp();
		addMidCode(tmp3, tmp1, opcode, ""); 
		tmp1 = tmp3;
	}
    while (symnum == PLUS || symnum == MINU) {
		if (symnum == PLUS) opcode = ADD;
		else if (symnum == MINU) opcode = SUB;
        outputsym();
        symnum = getsym();
        isTerm(tmp2);
		tmp3 = getTmp();
		addMidCode(tmp3, tmp1, opcode, tmp2);
		tmp1 = tmp3;
		type = 5;
    }
	var = tmp1;
    out << "<表达式>" << endl;
	return type;
}

// 项
int isTerm(string& var) {
	string tmp1, tmp2, tmp3;
    int type = isFactor(tmp1);
	int opcode = -1;
    while (symnum == MULT || symnum == DIV) {
		if (symnum == MULT) opcode = MUL;
		else if (symnum == DIV) opcode = DIVI;
        outputsym();
        symnum = getsym();
        isFactor(tmp2);
		tmp3 = getTmp();
		addMidCode(tmp3, tmp1, opcode, tmp2);
		tmp1 = tmp3;
		type = 5;
    }
	var = tmp1;
    out << "<项>" << endl;
	return type;
}

// 因子
int isFactor(string& var) {
    extern map<string, int> fun_idenfr;
    string idenfr;
	int type = -1;
	int opcode = -1;
	string tmp;
    if (symnum == LPARENT) {
		// （表达式）
        outputsym();
        symnum = getsym();
        type = isExpression(var);
        if (symnum != RPARENT) {error('0');}
        outputsym();
        symnum = getsym();
    } else if (symnum == PLUS || symnum == MINU) {
		// 带+-号的整数
		type = 5;
		if (symnum == MINU) opcode = NEG;
        outputsym();
        symnum = getsym();
        if (symnum != INTCON) {error('0');}
        outputsym();
		if (opcode == NEG) var = "-" + word;
		else var = word;
        out << "<整数>" << endl;
        symnum = getsym();
    } else if (symnum == INTCON) {
		// 无符号整数
		type = 5;
        outputsym();
		var = word;
        out << "<整数>" << endl;
        symnum = getsym();
    } else if (symnum == CHARCON) {
		// 字符
		type = 6;
        outputsym();
		var = to_string((int)(word[0]));
        symnum = getsym();
    } else if (symnum == IDENFR) {
		if (symList[level].find(word) == symList[level].end()) {
			if (symList["$all"].find(word) == symList["$all"].end()) {
				error('c'); type = 5;
			} else { type = symList["$all"][word].type; }
		} else { type = symList[level][word].type; }
        idenfr = word;
        outputsym();
        symnum = getsym();
		if (symnum == LPARENT) {
			// 有返回值函数调用语句
			if (fun_idenfr.find(idenfr) == fun_idenfr.end() || fun_idenfr[idenfr] == 0) { error('0'); }
			var = isFun_call(idenfr);
			if (symList["$all"][idenfr].value == "f") {
				var = getTmp();
				addMidCode(var, "", ASSRET, "");
			}
		} else if (symnum == LBRACK) {
			// 数组 标识符[表达式]
			symList["$all"][level].value = "f";
			outputsym();
			symnum = getsym();
			if (isExpression(tmp) != 5) {error('i');}
			if (symnum != RBRACK) { error('0'); }
			var = getTmp();
			addMidCode(var, idenfr, GETARR, tmp);
			type = type + 2;
			outputsym();
			symnum = getsym();
		} else {
			// 单纯只是标识符
			if (symList[level].find(idenfr) != symList[level].end()) {
				if (symList[level][idenfr].kind == 1)
					var = symList[level][idenfr].value;
				else
					var = idenfr;
			}
			else if (symList["$all"].find(idenfr) != symList["$all"].end() && symList["$all"][idenfr].kind == 1)
				var = symList["$all"][idenfr].value;
			else {
				if (symList["$all"].find(idenfr) != symList["$all"].end() && symList["$all"][idenfr].kind == 0)
					symList["$all"][level].value = "f";
				var = idenfr;
			}
		}		
    } else {error('0');}
    out << "<因子>" << endl;
	return type;
}
