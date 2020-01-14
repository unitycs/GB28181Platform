#pragma once

#ifdef GBUTILS_EXPORTS
#ifndef GBUTILS_API
#define GBUTILS_API __declspec(dllexport)
#endif
#else
#ifndef GBUTILS_API
#define GBUTILS_API __declspec(dllimport)
#endif
#endif

#ifndef SAFE_DEL_PTR
#define  SAFE_DEL_PTR(ptr) if(ptr!=nullptr) delete ptr; ptr=NULL;
#endif // !SAFE_DEL_PTR

#include <iostream>
#include <bitset>

typedef std::bitset<64> Block;

typedef std::bitset<56> Key;

typedef std::bitset<48> Code;

typedef std::bitset<32> HBlock;

typedef std::bitset<28> HKey;

typedef std::bitset<24> HCode;

typedef enum  _ACTION_ :int { Encrypt, Decrypt } ACTION;


class GBUTILS_API DESCryptoPrivoder
{
public:
	DESCryptoPrivoder() = delete;
	DESCryptoPrivoder(const char rgbKey[]);

	inline std::string Encrypt(const char* in_str);

	inline std::string Decrypt(const char* in_str);

	//加解密运算
	inline	int     des(Block & block, const ACTION action);

	inline static	void StrFromBlock(char * str, const Block & block);

	inline static	void BlockFromStr(Block & block, const char * str);

private:

	// 初始置换表
	//将数据块初始置换为左右两个部分
	inline	int ip(const Block & block, HBlock & left, HBlock & right);

	//一轮加解密运算，不带交换
	inline	int des_turn(HBlock & left, HBlock & right, const Code & subkey);

	//交换左右两个部分
	inline	int exchange(HBlock & left, HBlock & right);

	//将左右两部分数据进行末置换形成一个数据块
	inline	int rip(const HBlock & left, const HBlock & right, Block & block);

	//获取bkey产生的第n轮子密钥
	inline	Code getkey(const unsigned int n, const Block & bkey);


	typedef struct _DATA_KEY_
	{
		Block block_KEY;
		Block block_IN;
	}DATA_BLOCK;

	DATA_BLOCK dataStore;

	//out string..
	char buffer[8] = { 0 };

};



class GBUTILS_API DESCBCCryptor
{

public:
	DESCBCCryptor() = delete;
	DESCBCCryptor(const char rgbKey[], const char rgbIV[]);

	inline	 std::string Encrypt(const char* in_str);

	inline	std::string Decrypt(const char* out_str);

	inline  void reset();


private:

	typedef struct _DATA_KEY_
	{
		Block block_IN;
		Block block_IV;
		Block block_LAST;
	}DATA_BLOCK_CBC;

	DATA_BLOCK_CBC block_cbc;

	inline	void XORBlock(Block & in_block, Block & in_out);
	char buffer[8] = { 0 };
	DESCryptoPrivoder desprovider;

};




/*
/// <summary>
/// DESCryptoPrivoder tool uses DES algorithm.
/// </summary>
public class DESCryptoPrivoder
{
private SymmetricAlgorithm m_SA = null;
private const string CIV = "kXwL7X2+fgM=";  // Private key
private const string CKEY = "FwGQWRRgKCI="; // Encrypt vector.

public DESCryptoPrivoder()
{
this.m_SA = new DESCryptoServiceProvider();
}

/// <summary>
/// Encrypt string.
/// </summary>
/// <param name="strInput">strInput to encrypt</param>
/// <returns>Encrypted value</returns>
public string Encrypt(string strInput)
{
MemoryStream pMStream = new MemoryStream();
ICryptoTransform pICT = this.m_SA.CreateEncryptor(Convert.FromBase64String(CKEY), Convert.FromBase64String(CIV));

using (CryptoStream pCStream = new CryptoStream(pMStream, pICT, CryptoStreamMode.Write))
{
// Try to encrypt.
byte[] pBuf = Encoding.UTF8.GetBytes(strInput);
pCStream.Write(pBuf, 0, pBuf.Length);
pCStream.FlushFinalBlock();
pCStream.Close();

// Return encrypt value.
return Convert.ToBase64String(pMStream.ToArray());
}
}

public string Decrypt(string strInput)
{
MemoryStream pMStream = new MemoryStream();
ICryptoTransform pICT = this.m_SA.CreateDecryptor(Convert.FromBase64String(CKEY), Convert.FromBase64String(CIV));
CryptoStream pCStream = new CryptoStream(pMStream, pICT, CryptoStreamMode.Write);

// Try to decrypt.
byte[] pBuf = Convert.FromBase64String(strInput);
pCStream.Write(pBuf, 0, pBuf.Length);
pCStream.FlushFinalBlock();
pCStream.Close();

// return final value.
return Encoding.UTF8.GetString(pMStream.ToArray());
}
}
*/