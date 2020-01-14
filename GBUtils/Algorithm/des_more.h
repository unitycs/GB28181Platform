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

#include <cstdint>
using namespace std;

#define ui64 uint64_t
#define ui32 uint32_t
#define ui8  uint8_t

class GBUTILS_API DES
{
public:
	DES(ui64 key);
	ui64 des(ui64 block, bool mode);

	ui64 encrypt(ui64 block);
	ui64 decrypt(ui64 block);

	static ui64 encrypt(ui64 block, ui64 key);
	static ui64 decrypt(ui64 block, ui64 key);

protected:
	void keygen(ui64 key);

	ui64 ip(ui64 block);
	ui64 fp(ui64 block);

	void feistel(ui32 &L, ui32 &R, ui32 F);
	ui32 f(ui32 R, ui64 k);

private:
	ui64 sub_key[16]; // 48 bits each
};

class GBUTILS_API DESCBC
{
public:
	DESCBC(ui64 key, ui64 iv);
	ui64 encrypt(ui64 block);
	ui64 decrypt(ui64 block);
	void reset();

private:
	DES des;
	ui64 iv;
	ui64 last_block;
};
