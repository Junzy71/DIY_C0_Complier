# DIY C0 Complier
北京航空航天大学 2019年秋季学期 编译原理课程设计<br>
Project of ' Design of Complier Technology' course  of SCSE,BUAA in 2019 fall.<br>
## Overview 概述
本项目实现了一个编译器，可以将简化的C语言（C0文法,后面有介绍）翻译为MIPS汇编语言。本编译器主要使用了递归下降法实现。此外，本编译器还实现了一些编译器中常见的且效果显著的优化。<br>
In this project we implemented a compiler, which can translate C0 language (C language simplified on grammar. We will explain it later) into MIPS assembly language. This complier mainly use the recursive-descend method. In addition, this complier also implements some common but effective optimizations. 

## About C0 language 关于C0文法
鉴于课程时间与精力有限，本编译器编译的是C0语言。这种语言是基于C语言的简化版本，简化后的版本完全符合LL(1)文法。具体的文法如下。<br>
Considering of limited time and vigour we can pour into this course, this compiler translates C0 language. C0 language is a kind of simplified C language, which is conform the LL(1) grammar. The C0 grammar is shown below.

```
＜加法运算符＞ ::= +｜-
＜乘法运算符＞  ::= *｜/
＜关系运算符＞  ::=  <｜<=｜>｜>=｜!=｜==
＜字母＞   ::= ＿｜a｜．．．｜z｜A｜．．．｜Z
＜数字＞   ::= 0｜＜非零数字＞
＜非零数字＞  ::= 1｜．．．｜9
＜字符＞    ::=  '＜加法运算符＞'｜'＜乘法运算符＞'｜'＜字母＞'｜'＜数字＞'
＜字符串＞   ::=  "{十进制编码为32,33,35-126的ASCII字符}"
＜程序＞    ::= ［＜常量说明＞］［＜变量说明＞］{＜有返回值函数定义＞|＜无返回值函数定义＞}＜主函数＞
＜常量说明＞ ::=  const＜常量定义＞;{ const＜常量定义＞;}
＜常量定义＞   ::=   int＜标识符＞＝＜整数＞{,＜标识符＞＝＜整数＞}
                  | char＜标识符＞＝＜字符＞{,＜标识符＞＝＜字符＞}
＜无符号整数＞  ::= ＜非零数字＞｛＜数字＞｝| 0
＜整数＞        ::= ［＋｜－］＜无符号整数＞
＜标识符＞    ::=  ＜字母＞｛＜字母＞｜＜数字＞｝
＜声明头部＞   ::=  int＜标识符＞ |char＜标识符＞
＜变量说明＞  ::= ＜变量定义＞;{＜变量定义＞;}
＜变量定义＞  ::= ＜类型标识符＞(＜标识符＞|＜标识符＞'['＜无符号整数＞']'){,(＜标识符＞|＜标识符＞'['＜无符号整数＞']' )} 
                 //＜无符号整数＞表示数组元素的个数，其值需大于0
＜类型标识符＞      ::=  int | char
＜有返回值函数定义＞  ::=  ＜声明头部＞'('＜参数表＞')' '{'＜复合语句＞'}'
＜无返回值函数定义＞  ::= void＜标识符＞'('＜参数表＞')''{'＜复合语句＞'}'
＜复合语句＞   ::=  ［＜常量说明＞］［＜变量说明＞］＜语句列＞
＜参数表＞    ::=  ＜类型标识符＞＜标识符＞{,＜类型标识符＞＜标识符＞}| ＜空＞
＜主函数＞    ::= void main'('')''{'＜复合语句＞'}'
＜表达式＞    ::= ［＋｜－］＜项＞{＜加法运算符＞＜项＞}   //[+|-]只作用于第一个<项>
＜项＞     ::= ＜因子＞{＜乘法运算符＞＜因子＞}
＜因子＞    ::= ＜标识符＞｜＜标识符＞'['＜表达式＞']'|'('＜表达式＞')'｜＜整数＞|＜字符＞｜＜有返回值函数调用语句＞         
＜语句＞    ::= ＜条件语句＞｜＜循环语句＞| '{'＜语句列＞'}'| ＜有返回值函数调用语句＞; 
                           |＜无返回值函数调用语句＞;｜＜赋值语句＞;｜＜读语句＞;｜＜写语句＞;｜＜空＞;|＜返回语句＞;
＜赋值语句＞   ::=  ＜标识符＞＝＜表达式＞|＜标识符＞'['＜表达式＞']'=＜表达式＞
＜条件语句＞  ::= if '('＜条件＞')'＜语句＞［else＜语句＞］
＜条件＞    ::=  ＜表达式＞＜关系运算符＞＜表达式＞｜＜表达式＞ //表达式为0条件为假，否则为真
＜循环语句＞   ::=  while '('＜条件＞')'＜语句＞| do＜语句＞while '('＜条件＞')' |for'('＜标识符＞＝＜表达式＞;＜条件＞;＜标识符＞＝＜标识符＞(+|-)＜步长＞')'＜语句＞
＜步长＞::= ＜无符号整数＞  
＜有返回值函数调用语句＞ ::= ＜标识符＞'('＜值参数表＞')'
＜无返回值函数调用语句＞ ::= ＜标识符＞'('＜值参数表＞')'
＜值参数表＞   ::= ＜表达式＞{,＜表达式＞}｜＜空＞
＜语句列＞   ::= ｛＜语句＞｝
＜读语句＞    ::=  scanf '('＜标识符＞{,＜标识符＞}')'
＜写语句＞    ::= printf '(' ＜字符串＞,＜表达式＞ ')'| printf '('＜字符串＞ ')'| printf '('＜表达式＞')'
＜返回语句＞   ::=  return['('＜表达式＞')']
```
## About the project structure 关于编译器结构
本编译器分为前端和后端两部分. 前端主要进行词法分析，语法分析，将源程序翻译为四元式中间代码；后端则依托中间代码对其进行优化并生成最终的MIPS代码。<br>
This complier consists two parts: front end and back end. The Front end implements the lexical analysis, grammar analysis(based on recursive-descend methods) and translate the source code into 4-variables midcode. The backend implements optimizations based on midcode and finally translate them into target assembly code.
## About the Optimization 关于优化
本编译器实现了包括但不限于下列优化<br>
This complier implements at least the following optimizations
- 活跃变量分析 Active variables anslysis
- 函数内联展开 automatic inline function expansion
- DAG图优化删除公共子表达式 DAG optimization (elimination of shared expression)
- 死代码消除 Elimination of dead code
- 赋值替换 substitution for assign instruction
- 常量传播与替换 constant proporgation and substitution
- 窥孔优化 peephole optimization
- 编译时简单表达式计算 Simple calculation in compilation phase
- 图着色算法分配全局寄存器 graph coloring algorithm for distribution of global register
- 基于活跃变量分析结果的临时寄存器分配 distribution of temporary register based on active variables analysis
##  About Development Environment 关于开发环境
IDE environment: VS2019 community<br>
Comlier: clang++ 8.0/ msvc<br>
Debugger: MSVC debugger.



