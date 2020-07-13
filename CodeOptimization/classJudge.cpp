#include <map>
#include <string>
#include <fstream>
#include <iostream>
using namespace std;

extern ifstream in;
extern ofstream out;
extern char c;
map<string, int> reserved;

// ＜字母＞   :: = ＿｜a｜．．．｜z｜A｜．．．｜Z
bool isAlpha(char ch) {
	if (isalpha(ch) || ch == '_')
		return true;
	else
		return false;
}

// ＜标识符＞::=＜字母＞｛＜字母＞｜＜数字＞｝
string isIdenfr() {
	string s = "";
	s = s + c;
	c = in.get();
	while (isAlpha(c) || isdigit(c)) {
		s = s + c;
		c = in.get();
	}
	return s;
}

// ＜数字＞   :: = ０｜＜非零数字＞
// ＜非零数字＞  :: = １｜．．．｜９
// ＜无符号整数＞::=＜非零数字＞＜数字＞｝| 0
string isInteger() {
	//int n = c - '0';
	string s = "";
	s = s + c;
	c = in.get();
	if (s == "0")
		return s;
	while (isdigit(c)) {
		//n = n * 10 + (c - '0');
		s = s + c;
		c = in.get();
	}
	return s;
}

// ＜字符串＞::="｛十进制编码为32,33,35-126的ASCII字符｝"
string isString() {
	string s = "";
	while ((int)c == 32 || (int)c == 33 || ((int)c >= 35 && (int)c <= 126)) {
		if (c == '\\') s = s + '\\';
		s = s + c;
		c = in.get();
	}
	return s;
}

// 保留字map初始化
void init_reserved() {
	reserved["const"] = 4;
	reserved["int"] = 5;
	reserved["char"] = 6;
	reserved["void"] = 7;
	reserved["main"] = 8;
	reserved["if"] = 9;
	reserved["else"] =10;
	reserved["do"] = 11;
	reserved["while"] = 12;
	reserved["for"] = 13;
	reserved["scanf"] = 14;
	reserved["printf"] = 15;
	reserved["return"] = 16;
}

// 判断是否为保留字
int isReserved(string s) {
	if (reserved.find(s) == reserved.end())
		return 0;
	else
		return reserved[s];
}