#pragma  once
////////////////////////////////////////////////////////////
// base64.h
// 参考文章：http://www.cstc.net.cn/docs/docs.php?id=202
//************************************************************************/
//    base64编码表
//
//		0 A 17 R 34 i 51 z
//		1 B 18 S 35 j 52 0
//		2 C 19 T 36 k 53 1
//		3 D 20 U 37 l 54 2
//		4 E 21 V 38 m 55 3
//		5 F 22 W 39 n 56 4
//		6 G 23 X 40 o 57 5
//		7 H 24 Y 41 p 58 6
//		8 I 25 Z 42 q 59 7
//		9 J 26 a 43 r 60 8
//		10 K 27 b 44 s 61 9
//		11 L 28 c 45 t 62 +
//		12 M 29 d 46 u 63 /
//		13 N 30 e 47 v (pad) =
//		14 O 31 f 48 w
//		15 P 32 g 49 x
//		16 Q 33 h 50 y
//
// base64编码步骤：
//
//		原文：
//
//		你好
//		C4 E3 BA C3
//		11000100 11100011 10111010 11000011
//		00110001 00001110 00001110 00111010
//		49       14							（十进制）
//		x        O        O        6		字符
//		01111000 01001111 01001111 00110110
//		78									（十六进制）
//		xOO6
//
//		解码：
//		xOO6
//		78 4f 4f 36
//		01111000 01001111 01001111 00110110
//		49       14
//		00110001 00001110 00001110 00111010   31 0e 0e 3a
//
//		11000100 11100011 10111010
//		C4       E3       BA
//
#ifdef GBUTILS_EXPORTS
#ifndef GBUTILS_API
#define GBUTILS_API __declspec(dllexport)
#endif
#else
#ifndef GBUTILS_API
#define GBUTILS_API __declspec(dllimport)
#endif
#endif

#include <string>

class GBUTILS_API BASE64Cryptor
{
public:
	inline std::string Decode(const char * src_bytes, int src_len = 0);
	inline std::string Encode(const char * src_bytes, int src_len = 0);

private:
	inline	int GetB64Index(char ch);
	inline	char GetB64Char(int index);
	inline	int Base64Encode(char * base64code, const char * src, int src_len = 0);
	inline	int Base64Decode(char * buf, const char * base64code, int src_len = 0);
};
