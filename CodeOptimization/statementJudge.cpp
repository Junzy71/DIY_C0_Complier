#include<map>
#include<set>
#include<vector>
#include<sstream>
#include <fstream>
#include <iostream>
using namespace std;

struct sym { int kind; int type; string value; int addr; };
struct midcode { string result; string left; int op; string right; set<string> out; };

bool haveReturn;
bool haveReturnInt;
bool haveReturnChar;

extern ifstream in;
extern ofstream out;
extern char c;
extern string level;
extern int symnum;
extern int labelnum;
extern int stringnum;
extern string word;
extern map<string, map<string, struct sym>> symList;
extern map<string, map<string, struct sym>> tmpList;
enum classnum {IDENFR, INTCON, CHARCON, STRCON, CONSTTK, INTTK, CHARTK, VOIDTK, MAINTK, IFTK, ELSETK, DOTK, WHILETK, FORTK, SCANFTK, PRINTFTK, 
	RETURNTK, PLUS, MINU, MULT, DIV, LSS, LEQ, GRE, GEQ, EQL, NEQ, ASSIGN, SEMICN, COMMA, LPARENT, RPARENT, LBRACK, RBRACK, LBRACE, RBRACE};
enum midop { NEG, ADD, SUB, MUL, DIVI, ASS, GETARR, PUTARR, SETLABEL, GOTO, BNZ, BZ, LESS, LE, GREAT,
	GE, EQUAL, UNEQUAL, READ, WSTRING, WEXP, WSE, RET, RETX, PARA, FUNCALL, FUN, PAR, ASSRET };

int getsym();
void outputsym();
bool isNumber(string s);

void isConst();
void isVar(int type, string idenfr);
void isStatements();
void isStatement();
void isConditional();
void isLoop();
void isCondition(string& con);
void isScanf();
void isPrintf();
void isReturn();
string isFun_call(string idenfr);
map<string, string> isValueList(string name);
void isAssign(string idenfr);
int isExpression(string& var);
void error(char errNum);
string getTmp();
string getLabel();
string getString(string s);
void addMidCode(string result, string left, int op, string right);
void outputMidcode(struct midcode m, ostream& file);

// 复合语句
void isCompound() {
	bool isval = false;
	int type;
    string idenfr;
	// 常量说明
    if (symnum == CONSTTK) {
        isConst();
    }
	// 变量定义
    while (symnum == INTTK || symnum == CHARTK) {
		isval = true;
        type = symnum;
        symnum = getsym();

        if (symnum != IDENFR) {error('0');}
        idenfr = word;
        symnum = getsym();

        isVar(type, idenfr);
    }
    if (isval) out << "<变量说明>" << endl;
	// 语句列
    isStatements();
	out << "<复合语句>" << endl;
}

// 语句列
void isStatements() {
    while (symnum == IFTK || symnum == WHILETK || symnum == DOTK ||
    symnum == FORTK || symnum == LBRACE || symnum == IDENFR ||
    symnum == SCANFTK || symnum == PRINTFTK ||
    symnum == RETURNTK || symnum == SEMICN) {
        isStatement();
    }
    out << "<语句列>" << endl;
}

// 语句
void isStatement() {
    string idenfr;
    if (symnum == IFTK) {
        // 条件语句
        isConditional();
        out << "<语句>" << endl;
        return;
    } else if (symnum == WHILETK || symnum == DOTK || symnum == FORTK) {
        // 循环语句
        isLoop();
        out << "<语句>" << endl;
        return;
    } else if (symnum == LBRACE) {
        outputsym();
        //语句列
        symnum = getsym();
        isStatements();
        if (symnum != RBRACE) {error('0');}
        outputsym();
        symnum = getsym();
        out << "<语句>" << endl;
        return;
    } else if (symnum == IDENFR) {
        idenfr = word;
        outputsym();
        symnum = getsym();
        if (symnum == LPARENT) {
            // 有/无返回值函数调用语句
			if (symList[level].find(idenfr) == symList[level].end()
				&& symList["$all"].find(idenfr) == symList["$all"].end()) {error('c');}
            isFun_call(idenfr);
        } else if (symnum == ASSIGN || symnum == LBRACK) {
			// 赋值语句
			if (symList[level].find(idenfr) == symList[level].end()) {
				if (symList["$all"].find(idenfr) == symList["$all"].end()) { error('c'); }
				else if (symList["$all"][idenfr].kind == 1) { error('j'); }
			}
			else if (symList[level][idenfr].kind == 1) { error('j'); }
            isAssign(idenfr);
        } else {error('0');}
    } else if (symnum == SCANFTK) {
        // 读语句
        isScanf();
    } else if (symnum == PRINTFTK) {
        // 写语句
        isPrintf();
    } else if (symnum == RETURNTK) {
        // 返回语句
        isReturn();
    }
    if (symnum != SEMICN) {error('k');}
	else {
		outputsym();
		symnum = getsym();
	}
	out << "<语句>" << endl;
}

// 条件语句
void isConditional() {
	string con;
	string label1 = getLabel(), label2;
	symList["$all"][level].value = "f";
    if (symnum != IFTK) {error('0');}
    outputsym();
    symnum = getsym();

    if (symnum != LPARENT) {error('0');}
    outputsym();
    symnum = getsym();

    isCondition(con);
	addMidCode(label1, con, BZ, "");

	if (symnum != RPARENT) { error('l'); }
	else {
		outputsym();
		symnum = getsym();
	}

    isStatement();

    if (symnum == ELSETK) {
		label2 = getLabel();
		addMidCode(label2, "", GOTO, "");
		addMidCode(label1, "", SETLABEL, "");
        outputsym();    
        symnum = getsym();
        isStatement();
		addMidCode(label2, "", SETLABEL, "");
	} else {
		addMidCode(label1, "", SETLABEL, "");
	}

    out << "<条件语句>" << endl;
}

// 循环语句
void isLoop() {
	int opcode, i, j;
	string idenfr, idenfr2, step, con;
	string label1 = getLabel(), label2;
	vector<int> midTmpIndex;
	vector<struct midcode> midTmp;
	extern vector <struct midcode> midCodes;

	symList["$all"][level].value = "f";
    if (symnum == WHILETK) {
		label2 = getLabel();
		midTmpIndex.push_back(midCodes.size());
		addMidCode(label1, "", SETLABEL, "");
        outputsym();
        symnum = getsym();

        if (symnum != LPARENT) {error('0');}
        outputsym();
        symnum = getsym();

		midTmpIndex.push_back(midCodes.size());
        isCondition(con);

		midTmpIndex.push_back(midCodes.size());
		addMidCode(label2, con, BZ, "");

		if (symnum != RPARENT) { error('l'); }
		else {
			outputsym();
			symnum = getsym();
		}

		midTmpIndex.push_back(midCodes.size());
        isStatement();

		midTmpIndex.push_back(midCodes.size());
		addMidCode(label1, "", GOTO, "");
		midTmpIndex.push_back(midCodes.size());
		addMidCode(label2, "", SETLABEL, "");
		midTmpIndex.push_back(midCodes.size());
		// 调整中间代码顺序
		for (i = midTmpIndex[4]; i < midTmpIndex[6]; i++)
			midTmp.push_back(midCodes[i]);
		for (i = midTmpIndex[3]; i < midTmpIndex[4]; i++)
			midTmp.push_back(midCodes[i]);
		for (i = midTmpIndex[0]; i < midTmpIndex[2]; i++)
			midTmp.push_back(midCodes[i]);
		struct midcode m = midCodes[midTmpIndex[2]];
		m.op = BNZ;
		midTmp.push_back(m);
		for (i = midTmpIndex[0], j = 0; i < midTmpIndex[6]; i++, j++)
			midCodes[i] = midTmp[j];
    } else if (symnum == DOTK) {
		addMidCode(label1, "", SETLABEL, "");
        outputsym();
        symnum = getsym();

        isStatement();

        if (symnum != WHILETK) {
			error('n');
		}
		else {
			outputsym();
			symnum = getsym();
		}       

        if (symnum != LPARENT) {error('0');}
        outputsym();
        symnum = getsym();

        isCondition(con);
		addMidCode(label1, con, BNZ, "");

		if (symnum != RPARENT) { error('l'); }
		else {
			outputsym();
			symnum = getsym();
		}
    } else if (symnum == FORTK) {
		label2 = getLabel();
        outputsym();
        symnum = getsym();

        if (symnum != LPARENT) {error('0');}
        outputsym();
        symnum = getsym();

        if (symnum != IDENFR) {error('0');}
		if (symList[level].find(word) == symList[level].end()) {
			if (symList["$all"].find(word) == symList["$all"].end()) { error('c'); }
			else if (symList["$all"][word].kind == 1) { error('j'); }
		}
		else if (symList[level][word].kind == 1) { error('j'); }
		idenfr = word;
        outputsym();
        symnum = getsym();

        if (symnum != ASSIGN) {error('0');}
        outputsym();
        symnum = getsym();

        isExpression(idenfr2);
		midTmpIndex.push_back(midCodes.size());
		addMidCode(idenfr, idenfr2, ASS, "");

        if (symnum != SEMICN) {error('k');}
		else {
			outputsym();
			symnum = getsym();
		}

		midTmpIndex.push_back(midCodes.size());
		addMidCode(label1, "", SETLABEL, "");

		midTmpIndex.push_back(midCodes.size());
        isCondition(con);

		midTmpIndex.push_back(midCodes.size());
		addMidCode(label2, con, BZ, "");

		if (symnum != SEMICN) { error('k'); }
		else {
			outputsym();
			symnum = getsym();
		}

        if (symnum != IDENFR) {error('0');}
		if (symList[level].find(word) == symList[level].end()) {
			if (symList["$all"].find(word) == symList["$all"].end()) { error('c'); }
			else if (symList["$all"][word].kind == 1) { error('j'); }
		}
		else if (symList[level][word].kind == 1) { error('j'); }
		idenfr = word;
        outputsym();
        symnum = getsym();

        if (symnum != ASSIGN) {error('0');}
        outputsym();
        symnum = getsym();

        if (symnum != IDENFR) {error('0');}
		if (symList[level].find(word) == symList[level].end()
			&& symList["$all"].find(word) == symList["$all"].end()) {error('c');}
		idenfr2 = word;
		outputsym();
        symnum = getsym();

        if (symnum != PLUS && symnum != MINU) {error('0');}
		if (symnum == PLUS) opcode = ADD;
		else if (symnum == MINU) opcode = SUB;
        outputsym();
        symnum = getsym();

        if (symnum != INTCON) {error('0');}
		step = word;
        outputsym();
        out << "<步长>" << endl; 
        symnum = getsym();

		if (symnum != RPARENT) { error('l'); }
		else {
			outputsym();
			symnum = getsym();
		}

		midTmpIndex.push_back(midCodes.size());
        isStatement();

		midTmpIndex.push_back(midCodes.size());
		addMidCode(idenfr, idenfr2, opcode, step);
		midTmpIndex.push_back(midCodes.size());
		addMidCode(label1, "", 9, "");
		midTmpIndex.push_back(midCodes.size());
		addMidCode(label2, "", 8, "");
		midTmpIndex.push_back(midCodes.size());
		// 调整中间代码顺序
		for (i = midTmpIndex[0]; i < midTmpIndex[1]; i++)
			midTmp.push_back(midCodes[i]);
		for (i = midTmpIndex[6]; i < midTmpIndex[8]; i++)
			midTmp.push_back(midCodes[i]);
		for (i = midTmpIndex[4]; i < midTmpIndex[6]; i++)
			midTmp.push_back(midCodes[i]);
		for (i = midTmpIndex[1]; i < midTmpIndex[3]; i++)
			midTmp.push_back(midCodes[i]);
		struct midcode m = midCodes[midTmpIndex[3]];
		m.op = BNZ;
		midTmp.push_back(m);
		/*
		for (i = midTmpIndex[0], j = 0; i < midTmpIndex[8]; i++, j++) {
			outputMidcode(midCodes[i], cout);
		}
		cout << "-------------------------" << endl;
		for (i = midTmpIndex[0], j = 0; i < midTmpIndex[8]; i++, j++) {
			outputMidcode(midTmp[j], cout);
		}
		cout << "########################" << endl;*/
		for (i = midTmpIndex[0], j = 0; i < midTmpIndex[8]; i++, j++)
			midCodes[i] = midTmp[j];
    } else {error('0');}
    out << "<循环语句>" << endl;
}

// 条件
void isCondition(string& con) {
	string tmp1, tmp2;
    int type = isExpression(tmp1);
	int opcode;
	if (type != 5) { error('f');}
    if (symnum >= LSS && symnum <= NEQ) {
		opcode = symnum - LSS + LESS;
        outputsym();
        symnum = getsym();
        type = isExpression(tmp2);
		con = getTmp();
		addMidCode(con, tmp1, opcode, tmp2);
		if (type != 5) { error('f'); }
	} else {
		con = tmp1;
	}
    out << "<条件>" << endl;
}

// 读语句
void isScanf() {
	// symList["$all"][level].value = "f";
    if (symnum != SCANFTK) {error('0');}
    outputsym();
    symnum = getsym();

    if (symnum != LPARENT) {error('0');}
    outputsym();
    symnum = getsym();

    if (symnum != IDENFR) {error('0');}
	if (symList[level].find(word) == symList[level].end()) {
		if (symList["$all"].find(word) == symList["$all"].end()) { error('c'); }
		else if (symList["$all"][word].kind == 1) { error('j'); }
	} else if (symList[level][word].kind == 1) { error('j'); }
	addMidCode("", word, READ, "");
    outputsym();
    symnum = getsym();
    
    while (symnum == COMMA) {
        outputsym();
        symnum = getsym();
        if (symnum != IDENFR) {error('0');}
		if (symList[level].find(word) == symList[level].end()) {
			if (symList["$all"].find(word) == symList["$all"].end()) { error('c'); }
			else if (symList["$all"][word].kind == 1) { error('j'); }
		}
		else if (symList[level][word].kind == 1) { error('j'); }
		addMidCode("", word, READ, "");
        outputsym();
        symnum = getsym();
    }

	if (symnum != RPARENT) { error('l'); }
	else {
		outputsym();
		symnum = getsym();
	}

    out << "<读语句>" << endl;
}

// 写语句
void isPrintf() {
	string str, tmp, type;
	// symList["$all"][level].value = "f";
    if (symnum != PRINTFTK) {error('0');}
    outputsym();
    symnum = getsym();

    if (symnum != LPARENT) {error('0');}
    outputsym();
    symnum = getsym();

    if (symnum == STRCON) {
		str = getString(word);
        outputsym();
        out << "<字符串>" << endl;
        symnum = getsym();

        if (symnum == COMMA) {
            outputsym();
            symnum = getsym();
			if (isExpression(tmp) == 5) type = "int";
			else type = "char";
			addMidCode(type, str, WSE, tmp);
        } else {
			addMidCode("", str, WSTRING, "");
		}
    } else {
		if (isExpression(tmp) == 5) type = "int";
		else type = "char";
		addMidCode("", tmp, WEXP, type);
    }

	if (symnum != RPARENT) { error('l'); }
	else {
		outputsym();
		symnum = getsym();
	}
    out << "<写语句>" << endl;
}

// 返回语句
void isReturn() {
	int type;
	string tmp;
    if (symnum != RETURNTK) {error('0');}
    outputsym();
    symnum = getsym();

    if (symnum == LPARENT) {
        outputsym();
        symnum = getsym();
		type = isExpression(tmp);
		if (type == 5 && !haveReturnInt) {
			haveReturnInt = true;
			if (symList["$all"][level].type == 0) { error('g'); }
			else if (symList["$all"][level].type == 6) { error('h'); }
		} else if (type == 6 && !haveReturnChar) {
			haveReturnChar = true;
			if (symList["$all"][level].type == 0) { error('g'); }
			else if (symList["$all"][level].type == 5) { error('h'); }
		}
		if (symnum != RPARENT) { error('l'); }
		else {
			outputsym();
			symnum = getsym();
		}
		addMidCode("", tmp, RETX, "");
	}
	else {
		if (!haveReturn) {
			haveReturn = true;
			if (symList["$all"][level].type != 0) { error('h'); }
		}
		if (level != "main") addMidCode("", "", RET, "");
	}
    out << "<返回语句>" << endl;
}

// 有/无返回值函数调用语句
string isFun_call(string idenfr) {
	string ret = "";
    extern map<string, int> fun_idenfr;
	extern vector <struct midcode> midCodes;
	extern map<string, vector<int>> fun_mids;
	if (symList["$all"][idenfr].value == "f") symList["$all"][level].value = "f";

    if (symnum != LPARENT) {error('0');}
    outputsym();
    symnum = getsym();
    
	map<string, string> funVar = isValueList(idenfr);
	if (symList["$all"][idenfr].value == "f") addMidCode("", idenfr, FUNCALL, "");
	// 函数内联
	else {
		for (auto i : fun_mids[idenfr]) {
			struct midcode m = midCodes[i];
			// 对应result，局部新分配tmp，加入tmp符号表，全局不变
			if (funVar.find(m.result) != funVar.end());
			else if (symList[idenfr].find(m.result) != symList[idenfr].end() ||
				tmpList[idenfr].find(m.result) != tmpList[idenfr].end())
				funVar[m.result] = getTmp();
			else
				funVar[m.result] = m.result;
			// 对应left，常量改成值，局部变量新分配tmp，其余不变
			if (funVar.find(m.left) != funVar.end());
			else if(symList[idenfr].find(m.left) != symList[idenfr].end()) {
				if (symList[idenfr][m.left].kind == 1) funVar[m.left] = symList[idenfr][m.left].value;
				else if (symList[idenfr][m.left].kind == 0) funVar[m.left] = getTmp();
			}
			else if (tmpList[idenfr].find(m.left) != tmpList[idenfr].end()) {
				funVar[m.left] = getTmp();
			}				
			else
				funVar[m.left] = m.left;
			// 对应right，常量改成值，局部变量新分配tmp，其余不变
			if (funVar.find(m.right) != funVar.end());
			else if (symList[idenfr].find(m.right) != symList[idenfr].end()) {
				if (symList[idenfr][m.right].kind == 1) funVar[m.right] = symList[idenfr][m.right].value;
				else if (symList[idenfr][m.right].kind == 0) funVar[m.right] = getTmp();
			}
			else if (tmpList[idenfr].find(m.right) != tmpList[idenfr].end()) {
				funVar[m.right] = getTmp();
			}
			else
				funVar[m.right] = m.right;
			// 加入中间代码
			if (m.op == RET || m.op == PAR);
			else if (m.op == RETX) ret = funVar[m.left];
			else addMidCode(funVar[m.result], funVar[m.left], m.op, funVar[m.right]);
		}
	}

	if (symnum != RPARENT) { error('l'); }
	else {
		outputsym();
		symnum = getsym();
	}

	if (fun_idenfr.find(idenfr) == fun_idenfr.end()) { error('0'); }
	else if (fun_idenfr[idenfr]) {
		out << "<有返回值函数调用语句>" << endl;
	} else {
		out << "<无返回值函数调用语句>" << endl;
	}
	return ret;
}

// 值参数表
map<string, string> isValueList(string name) {
	extern map<string, vector<string>> fun_para;
	extern vector <struct midcode> midCodes;
	vector<int> valueList;
	vector <struct midcode> midTmp;
	map<string, string> funPara = {};
	int i = 0, type = 0, j = 0;
	string tmp;
    if (symnum != RPARENT) {
		valueList.push_back(midCodes.size() - 1);
        type = isExpression(tmp);
		if (i >= fun_para[name].size()) { error('d'); while (getsym() != RPARENT); return funPara; }
		if (symList[name][fun_para[name][i]].type != type) error('e');
		valueList.push_back(midCodes.size());
		addMidCode(name, tmp, PARA, fun_para[name][i++]);
        while (symnum == COMMA) {
			outputsym();
            symnum = getsym();
            type = isExpression(tmp);
			if (i >= fun_para[name].size()) { 
				error('d'); 
				while (getsym() != RPARENT);
				return funPara; 
			}
			if (symList[name][fun_para[name][i]].type != type) error('e');
			valueList.push_back(midCodes.size());
			addMidCode(name, tmp, PARA, fun_para[name][i++]);
        }
		if (i < fun_para[name].size()) { error('d'); return funPara; }
		// 调整中间代码参数计算顺序
		for (i = valueList.size() - 1; i > 0; i--) {
			for (j = valueList[i - 1] + 1; j < valueList[i]; j++)
				midTmp.push_back(midCodes[j]);
		} 
		// 函数不可内联输出push，可内联建立paraList，输出tmp赋值
		for (i = valueList.size() - 1, j = fun_para[name].size() - 1; i > 0 && j >= 0; i--, j--) {
			if (symList["$all"][name].value == "f")
				midTmp.push_back(midCodes[valueList[i]]);
			else {
				tmp = getTmp();
				midTmp.push_back({ tmp, midCodes[valueList[i]].left, ASS, "" });
				funPara[fun_para[name][j]] = tmp;
			}
		}
		for (i = valueList[0] + 1, j = 0; i < midCodes.size() && j < midTmp.size(); i++, j++) {
			midCodes[i] = midTmp[j];
			// outputMidcode(midTmp[j], cout);
		}
		while (i != midCodes.size())
			midCodes.pop_back();
		// cout << endl;
	}
    out << "<值参数表>" << endl;
	return funPara;
}

// 赋值语句
void isAssign(string idenfr) {
	string tmp1, tmp2;
    if (symnum == ASSIGN) {
        outputsym();
        symnum = getsym();
        isExpression(tmp1);
		addMidCode(idenfr, tmp1, ASS, "");
    } else if (symnum == LBRACK) {
        outputsym();
        symnum = getsym();
		if (isExpression(tmp1) != 5) { error('i'); };
        if (symnum != RBRACK) {error('m');}
		else {
			outputsym();
			symnum = getsym();
		}
        if (symnum != ASSIGN) {error('0');}
        outputsym();
        symnum = getsym();
        isExpression(tmp2);
		addMidCode(idenfr, tmp1, PUTARR, tmp2);
    } else {error('0');}
    out << "<赋值语句>" << endl;
}

string getString(string s) {
	extern map<string, map<string, struct sym>> tmpList;
	stringstream ss;
	string str;
	ss << "string$" << stringnum++;
	str = ss.str();
	tmpList["$all"][str] = { 1, 4, s };
	return str;
}