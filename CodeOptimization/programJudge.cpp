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
extern map<string, map<string, struct sym>> tmpList;

map<string, int> fun_idenfr;
map<string, vector<string>> fun_para;
enum classnum {IDENFR, INTCON, CHARCON, STRCON, CONSTTK, INTTK, CHARTK, VOIDTK, MAINTK, IFTK, ELSETK, DOTK, WHILETK, FORTK, SCANFTK, PRINTFTK, 
	RETURNTK, PLUS, MINU, MULT, DIV, LSS, LEQ, GRE, GEQ, EQL, NEQ, ASSIGN, SEMICN, COMMA, LPARENT, RPARENT, LBRACK, RBRACK, LBRACE, RBRACE};
enum midop { NEG, ADD, SUB, MUL, DIVI, ASS, GETARR, PUTARR, SETLABEL, GOTO, BNZ, BZ, LESS, LE, GREAT,
	GE, EQUAL, UNEQUAL, READ, WSTRING, WEXP, WSE, RET, RETX, PARA, FUNCALL, FUN, PAR, ASSRET };

int getsym();
void outputsym();
void isConst();
void isVar(int type, string idenfr);
void isFun_return(int type, string idenfr);
void isParaList(string name);
void isCompound();
void isFun_void();
void isMain();
void symInsert(string name, struct sym sym);
void error(char errNum);
void addMidCode(string result, string left, int op, string right);

void isProgram() {
    int type;
    string idenfr;
	bool isval = false;
    // 处理常量说明
    if (symnum == CONSTTK) {
        isConst();
    }
    // 处理变量说明和第一个有返回值函数定义
    if (symnum == INTTK || symnum == CHARTK) {
        type = symnum;
        symnum = getsym();
        if (symnum == IDENFR) {
            idenfr = word;
            symnum = getsym();
            // 进入变量说明分支
            while (symnum == COMMA || symnum == SEMICN || symnum == LBRACK) {
				isval = true;
                isVar(type, idenfr);
                if (symnum == INTTK || symnum == CHARTK) {
                    type = symnum;
                    symnum = getsym();
                    if (symnum == IDENFR) {
                        idenfr = word;
                        symnum = getsym();
                    } else { error('0'); }
                } else {
                    // 碰到void或错误，跳出，错误在最后处理
                    if (isval) out << "<变量说明>" << endl;
                    break;
                }
            }
            // 对于正常结束的while循环，查看是否为有返回值函数定义
            if (symnum == LPARENT) {
                if (isval) out << "<变量说明>" << endl;
                isFun_return(type, idenfr);
            }
        }  else { error('0'); }
    }
    // 处理有、无返回值的函数定义和主函数
    while (symnum == VOIDTK || symnum == INTTK || symnum == CHARTK) {
        if (symnum == VOIDTK) {
            outputsym();
            symnum = getsym();
            // 主函数处理完直接返回
            if (symnum == MAINTK) {
                isMain();
				out << "<程序>" << endl;
                return;
            // 无返回值函数定义处理完继续循环
            } else if (symnum == IDENFR) {
                isFun_void();
            } else { error('0'); }
        // 有返回值函数定义的处理
        } else {
            type = symnum;
            symnum = getsym();
            if (symnum == IDENFR) {
                idenfr = word;
                symnum = getsym();
                isFun_return(type, idenfr);
            } else { error('0'); }
        }
    }
    // 没有主函数的错误
    error('0');
    return;
}

// 常量说明
void isConst() {
    int type;
	string name;
	struct sym sym = {1, 5, ""};
    if (symnum != CONSTTK) {error('0');}
    outputsym();

    symnum = getsym();
    if (symnum != INTTK && symnum != CHARTK) {error('0');}
    type = symnum;
	sym.type = symnum;
    outputsym();

    symnum = getsym();
    if (symnum != IDENFR) {error('0');}
	while (symnum == IDENFR) {
		sym.value = "";
		name = word;
        outputsym();

        symnum = getsym();
        if (symnum != ASSIGN) {error('0');}
        outputsym();

        symnum = getsym();
		if (symnum == PLUS || symnum == MINU) {
			if (symnum == MINU) sym.value = word;
			outputsym();
			symnum = getsym();
		}
        if (!(symnum == INTCON && type == INTTK) &&
            !(symnum == CHARCON && type == CHARTK)) {error('o');}
		if (symnum == INTCON) sym.value = sym.value + word;
		else sym.value = to_string((int)(word[0]));
		outputsym();
		if (symnum == INTCON)
			out << "<整数>" << endl;

		symInsert(name, sym);
        symnum = getsym();
        if (symnum == COMMA) {
			// 逗号在常量定义内部循环
            outputsym(); 
            symnum = getsym();
		}
		else if (symnum == SEMICN) {
			break;
		} else { error('k');  break; }
    }
	// 分号结束本次常量定义循环
	out << "<常量定义>" << endl;
	outputsym();
	symnum = getsym();
	if (symnum == CONSTTK) {
		isConst();
	} else {
		// 分号后不再是常量定义循环，常量说明结束.
		out << "<常量说明>" << endl;
		return;
	}
}

// 变量定义
void isVar(int type, string idenfr) {
	string name = idenfr;
	struct sym sym = { 0, type };
    if (type == INTTK)
        out << "INTTK int" << endl;
    else if (type == CHARTK)
        out << "CHARTK char" << endl;
    out << "IDENFR" << " " << idenfr << endl;

    while (symnum == SEMICN || symnum == COMMA || symnum == LBRACK) {
        if (symnum == SEMICN) {
            // 一遍变量定义结束
			symInsert(name, sym);
            out << "<变量定义>" << endl;
            outputsym();
            symnum = getsym();
            return;
        } else if (symnum == COMMA) {
			symInsert(name, sym);
			sym.type = type;
            outputsym();
            symnum = getsym();
            if (symnum == IDENFR) {
				name = word;
                outputsym();
                symnum = getsym();
                // 变量定义大括号循环
            } else { error('0'); }
        } else if (symnum == LBRACK){
			if (level != "$all") symList["$all"][level].value = "f";
			sym.type = type - 2;
			outputsym();
            symnum = getsym();
            if (symnum != INTCON || word == "0") { error('i'); }
			sym.value = word;
            outputsym();
            symnum = getsym();
            if (symnum == RBRACK) {
                outputsym();
                symnum = getsym();
                // 变量定义大括号循环
            } else { error('m'); }
        }
    }
	error('k');
}

// 有返回值函数定义
void isFun_return(int type, string idenfr) {
	extern bool haveReturn;
	extern bool haveReturnInt;
	extern bool haveReturnChar;
	map<string, struct sym> map1;
	map<string, struct sym> map_tmp;
	if (type == INTTK) {
		out << "INTTK int" << endl;
		addMidCode("", "int", FUN, idenfr);
	} else if (type == CHARTK) {
		out << "CHARTK char" << endl;
		addMidCode("", "char", FUN, idenfr);
	}
    out << "IDENFR" << " " << idenfr << endl;
	out << "<声明头部>" << endl;
	fun_idenfr[idenfr] = 1;
	symInsert(idenfr, { 2, type, "t" });
	
	level = idenfr;
	symList[level] = map1;
	tmpList[level] = map_tmp;

    if (symnum != LPARENT) {error('0');}
    outputsym();
    symnum = getsym();

    isParaList(idenfr);

    if (symnum != RPARENT) {error('l');}
	else {
		outputsym();
		symnum = getsym();
	}

    if (symnum != LBRACE) {error('0');}
    outputsym();
    symnum = getsym();

	haveReturn = false;
	haveReturnInt = false;
	haveReturnChar = false;
    isCompound();

    if (symnum != RBRACE) {error('0');}
    outputsym();
    out << "<有返回值函数定义>" << endl;
    symnum = getsym();
	level = "$all";
}

// 参数表
void isParaList(string name) {
	vector<string> para;
	struct sym sym = { 3, 0 };
	string type;
    while (symnum == INTTK || symnum == CHARTK) {
		type = word;
		sym.type = symnum;
        outputsym();

        symnum = getsym();
        if (symnum != IDENFR) {error('0');}
		symInsert(word, sym);
		para.push_back(word);
		addMidCode("", type, PAR, word);
        outputsym();

        symnum = getsym();
        if (symnum != COMMA) {
			fun_para[name] = para;
            out << "<参数表>" << endl;
            return;
        }
        outputsym();

        symnum = getsym();
    }
	if (symnum == RPARENT) {
		fun_para[name] = para;
		out << "<参数表>" << endl;
	} else
		error('0');
}

// 无返回值函数定义
void isFun_void() {
	extern bool haveReturn;
	extern bool haveReturnInt;
	extern bool haveReturnChar;
	string name = word;
	map<string, struct sym> map1;
	map<string, struct sym> map_tmp;
    if (symnum != IDENFR) {error('0');}
    outputsym();
    fun_idenfr[word] = 0;
	symInsert(word, { 2, 0, "t" });
	addMidCode("", "void", FUN, word);
	level = word;
	symList[level] = map1;
	tmpList[level] = map_tmp;
    symnum = getsym();

    if (symnum != LPARENT) {error('0');}
    outputsym();
    symnum = getsym();

    isParaList(name);

	if (symnum != RPARENT) { error('l'); }
	else {
		outputsym();
		symnum = getsym();
	}

    if (symnum != LBRACE) {error('0');}
    outputsym();
    symnum = getsym();

	haveReturn = false;
	haveReturnInt = false;
	haveReturnChar = false;
    isCompound();

	addMidCode("", "", RET, "");
    if (symnum != RBRACE) {error('0');}
    outputsym();
    out << "<无返回值函数定义>" << endl;
    symnum = getsym();
	level = "$all";
}

// 主函数
void isMain() {
	extern bool haveReturn;
	extern bool haveReturnInt;
	extern bool haveReturnChar;
	map<string, struct sym> map1;
	map<string, struct sym> map_tmp;
	
	symInsert("main", { 2, 0, "t" });
	addMidCode("", "void", FUN, "main");
	level = "main";
	symList[level] = map1;
	tmpList[level] = map_tmp;
    if (symnum != MAINTK) {error('0');}
    outputsym();
    symnum = getsym();

    if (symnum != LPARENT) {error('0');}
    outputsym();
	symnum = getsym();

	if (symnum != RPARENT) { error('l'); }
	else {
		outputsym();
		symnum = getsym();
	}

    if (symnum != LBRACE) {error('0');}
    outputsym();
    symnum = getsym();

	haveReturn = false;
	haveReturnInt = false;
	haveReturnChar = false;
    isCompound();

    if (symnum != RBRACE) {error('0');}
    outputsym();
    out << "<主函数>" << endl;
	level = "$all";
}

// 填符号表
void symInsert(string name, struct sym sym) {
	if (symList[level].find(name) == symList[level].end()) {
		symList[level][name] = sym;
	}
	else { error('b'); }
}