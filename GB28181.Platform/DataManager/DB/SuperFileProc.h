// SuperFileProc.h : header file
#include "../../stdafx.h"
#pragma once
#	define WM_SHELL_NOTIFY		WM_USER + 1
#	define NM_SH_SHELLMENU		1

// helper struct that holds list item data
typedef struct tagLVID {
	//LPSHELLFOLDER lpsfParent;
	LPITEMIDLIST  lpi;
	ULONG         ulAttribs;
} LVITEMDATA, *LPLVITEMDATA;

// helper struct that holds tree item data
typedef struct tagTVID {
	//LPSHELLFOLDER lpsfParent;
	LPITEMIDLIST  lpi;
	LPITEMIDLIST  lpifq;
} TVITEMDATA, *LPTVITEMDATA;

#include <vector>

//////////////////////////////////////////////////////////////////////////
// CSuperFileProc: class is used to perform shell file operations
class CSuperFileProc {
public:
	typedef std::vector<CString> _StringArray;

	CSuperFileProc(LPCTSTR lpszFileName = nullptr);
	virtual ~CSuperFileProc();

	CString GetFileName() const;

	BOOL FileReName(LPCTSTR lpszOldName, LPCTSTR lpszNewName);

	BOOL Exist(LPCTSTR lpszFileName);

	CString GetCurrentFolder() const;

	BOOL	IsDrive(const CString& sPath);

	BOOL	CreateFolder(CString sPath) const;

	// this member function is used to retrieve a key from the registry
	//
	static LONG GetRegKey(HKEY key, LPCTSTR subkey, LPTSTR retdata);

	// this member function returns the size of the file formatted in KB.
	//
	static CString GetFileSize(
		// a string containing the system file size
		LPCTSTR lpszFileSize);

	CString GetFileDrive(
		// full path of file name to parse
		LPCTSTR lpszFileName = nullptr);

	// this member function retrieves the file extension. if the filename is
	// "c:\incoming\hello.txt", this function returns "txt".
	//
	CString GetFileExt(
		// full path of file or directory name.
		LPCTSTR lpszFileName = nullptr);

	// this member function retrieves the title of the filename excluding
	// the path and extension. if the filename is "c:\incoming\hello.txt",
	// this function returns "hello".
	//
	CString GetFileTitle(
		// full path of file or directory name.
		LPCTSTR lpszFileName = nullptr);

	// this member function retrieves the path only of the current filename.
	// if the filename is "c:\incoming\hello.txt", this function returns "c:\incoming\".
	//
	CString GetRoot(
		// full path of file or directory name.
		LPCTSTR lpszFileName = nullptr);

	// this member function retrieves current filename minus the path if the
	// filename is "c:\incoming\hello.txt", this function returns "hello.txt".
	//
	CString GetFileName(
		// full path of file or directory name.
		LPCTSTR lpszFileName = nullptr);

	int			m_iImageIndex;	// index into system image list
	CString		m_strPath;		// full file path used with BrowseForFolder(...)
	CString		m_strInitDir;	// start directory used with BrowseForFolder(...)
	CString		m_strSelDir;	// selected directory used with BrowseForFolder(...)
	CString		m_strTitle;		// dialog title used with BrowseForFolder(...)

protected:

	CString		m_strFileName;			// contains the full path to a file or folder
	TCHAR		m_szDrive[_MAX_DRIVE];	// contains the drive letter
	TCHAR		m_szDir[_MAX_DIR];		// contains the directory string
	TCHAR		m_szFname[_MAX_FNAME];	// contains the file name minus path and ext
	TCHAR		m_szExt[_MAX_EXT];		// contains the file extension
};