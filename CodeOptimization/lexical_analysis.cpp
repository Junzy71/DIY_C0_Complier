#include <string>
#include <fstream>
#include <iostream>
using namespace std;

int enter;
extern ifstream in;
extern ofstream out;
extern char c;
extern string word;
extern int line_num;

bool isAlpha(char ch);
string isIdenfr();
string isInteger();
string isString();
int isReserved(string s);
void error(char errNum);

int getsym() {
	word = "";
	enter = 0;
	while (isspace(c)) {
		if (c == '\n') {
			line_num++; 
			enter++;
		}
		c = in.get();
	}
	// 从第一个不为空白符的字符c起，分析一个单词
	if (isAlpha(c)) {
		word = isIdenfr();
		return isReserved(word);
	}
	else if (isdigit(c)) {
		word = isInteger();
		return 1;	
	}
	else if (c == '\'') {
		c = in.get();
		if (c == '+' || c == '-' || c == '*' || c == '/' || isdigit(c) || isAlpha(c)) {
			word = word + c;
		} else {
			error('a');
		}
		c = in.get();
		if (c != '\'') {
			error('a');
		} else {
			c = in.get();
		}
		return 2;
	}
	else if (c == '\"') {
		c = in.get();
		word = isString();
		if (c != '\"') {
			error('a');
		} else {
			c = in.get();
		}
		return 3;
	} 
	else if (c == '+') {
		word = word + c;
		c = in.get();
		return 17;
	}
	else if (c == '-') {
		word = word + c;
		c = in.get();
		return 18;
	}
	else if (c == '*') {
		word = word + c;
		c = in.get();
		return 19;
	}
	else if (c == '/') {
		word = word + c;
		c = in.get();
		return 20;
	}
	else if (c == '<') {
		word = word + c;
		c = in.get();
		if (c != '=') {
			return 21;
		} else {
			word = word + c;
			c = in.get();
			return 22;
		}
	}
	else if (c == '>') {
		word = word + c;
		c = in.get();
		if (c != '=') {
			return 23;
		}
		else {
			word = word + c;
			c = in.get();
			return 24;
		}
	}
	else if (c == '=') {
		word = word + c;
		c = in.get();
		if (c != '=') {
			return 27;
		}
		else {
			word = word + c;
			c = in.get();
			return 25;
		}
	}
	else if (c == '!') {
		word = word + c;
		c = in.get();
		if (c != '=') {
			error('a');
			return 26;
		}
		else {
			word = word + c;
			c = in.get();
			return 26;
		}
	}
	else if (c == ';') {
		word = word + c;
		c = in.get();
		return 28;
	}
	else if (c == ',') {
	word = word + c;
	c = in.get();
	return 29;
	}
	else if (c == '(') {
	word = word + c;
	c = in.get();
	return 30;
	}
	else if (c == ')') {
	word = word + c;
	c = in.get();
	return 31;
	}
	else if (c == '[') {
	word = word + c;
	c = in.get();
	return 32;
	}
	else if (c == ']') {
	word = word + c;
	c = in.get();
	return 33;
	}
	else if (c == '{') {
	word = word + c;
	c = in.get();
	return 34;
	}
	else if (c == '}') {
	word = word + c;
	c = in.get();
	return 35;
	}
	else {
		error('a');
		return -1;
	}
}