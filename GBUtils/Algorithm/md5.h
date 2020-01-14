/* MD5.H - header file for MD5C.CPP
 */

 /* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
 rights reserved.

 License to copy and use this software is granted provided that it
 is identified as the "RSA Data Security, Inc. MD5 Message-Digest
 Algorithm" in all material mentioning or referencing this software
 or this function.

 License is also granted to make and use derivative works provided
 that such works are identified as "derived from the RSA Data
 Security, Inc. MD5 Message-Digest Algorithm" in all material
 mentioning or referencing the derived work.

 RSA Data Security, Inc. makes no representations concerning either
 the merchantability of this software or the suitability of this
 software for any particular purpose. It is provided "as is"
 without express or implied warranty of any kind.

 These notices must be retained in any copies of any part of this
 documentation and/or software.
  */

#ifdef GBUTILS_EXPORTS
# ifndef GBUTILS_API
# define GBUTILS_API __declspec(dllexport)
#endif
#else
# ifndef GBUTILS_API
# define GBUTILS_API __declspec(dllimport)
#endif
#endif

  /* BYTE_PTR defines a generic pointer type */
typedef unsigned char *BYTE_PTR;

/* WORD16 defines a two byte word */
typedef unsigned short int WORD16;

/*  DWORD32 defines a four byte word */
typedef unsigned int DWORD32;

/* PROTO_LIST is defined depending on how PROTOTYPES is defined above.
If using PROTOTYPES, then PROTO_LIST returns the list, otherwise it
  returns an empty list.
 */

#define PROTO_LIST(list) list

class GBUTILS_API MD5CryptoPrivoder
{
public:
	/**
	 * Structure for holding MD5 context.
	 * @var MD5_CTX
	 */
	typedef struct {
		DWORD32 state[4];             /* state (ABCD) */
		DWORD32 count[2];             /* number of bits, modulo 2^64 (lsb first) */
		unsigned char buffer[64];   /* input buffer */
	} MD5_CTX;

	void MD5Init(MD5_CTX *);
	void MD5Update(MD5_CTX *, unsigned char *, unsigned int);
	void MD5Final(unsigned char[16], MD5_CTX *);
	void Encode(unsigned char *, DWORD32 *, unsigned int);
	void Decode(DWORD32 *, unsigned char *, unsigned int);

private:

	void MD5Transform(DWORD32[4], unsigned char[64]);
	void MD5_memcpy(BYTE_PTR, BYTE_PTR, unsigned int);
	void MD5_memset(BYTE_PTR, int, unsigned int);
};