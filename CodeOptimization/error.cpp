#include <fstream>
#include <iostream>
using namespace std;

extern ofstream err;
extern int line_num;
extern int enter;
extern int symnum;
extern string word;

void error(char errNum) {
	if (errNum == 'k') err << line_num - enter;
	else err << line_num;
	err << " " << errNum << endl;
	return;
}