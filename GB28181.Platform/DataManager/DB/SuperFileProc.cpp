// SuperFileProc.cpp : implementation file
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "SuperFileProc.h"

CSuperFileProc::CSuperFileProc(LPCTSTR lpszFileName) : m_iImageIndex(0)
{
	m_strFileName = lpszFileName;
	_tsplitpath_s(static_cast<LPCTSTR>(m_strFileName), m_szDrive, m_szDir, m_szFname, m_szExt);
}

CSuperFileProc::~CSuperFileProc()
{
}

CString CSuperFileProc::GetFileName() const {
	return m_strFileName;
}

CString CSuperFileProc::GetFileName(LPCTSTR lpszFileName/*=nullptr*/)
{
	// Parse the file path.
	if (lpszFileName != nullptr) {
		_tsplitpath_s(lpszFileName, m_szDrive, m_szDir, m_szFname, m_szExt);
	}

	// Just return the file name + extension.
	CString str; str.Format(_T("%s%s"), m_szFname, m_szExt);
	return str;
}

CString CSuperFileProc::GetFileDrive(LPCTSTR lpszFileName/*=nullptr*/)
{
	// Parse the file path.
	if (lpszFileName != nullptr) {
		_tsplitpath_s(lpszFileName, m_szDrive, m_szDir, m_szFname, m_szExt);
	}

	return m_szDrive;
}

CString CSuperFileProc::GetRoot(LPCTSTR lpszFileName/*=nullptr*/)
{
	// Parse the file path.
	if (lpszFileName != nullptr) {
		_tsplitpath_s(lpszFileName, m_szDrive, m_szDir, m_szFname, m_szExt);
	}

	// Just return the file name + extension.
	CString str; str.Format(_T("%s%s"), m_szDrive, m_szDir);
	return str;
}

CString CSuperFileProc::GetFileTitle(LPCTSTR lpszFileName/*=nullptr*/)
{
	// Parse the file path.
	if (lpszFileName != nullptr) {
		_tsplitpath_s(lpszFileName, m_szDrive, m_szDir, m_szFname, m_szExt);
	}

	return m_szFname;
}

CString CSuperFileProc::GetFileExt(LPCTSTR lpszFileName/*=nullptr*/)
{
	// Parse the file path.
	if (lpszFileName != nullptr) {
		_tsplitpath_s(lpszFileName, m_szDrive, m_szDir, m_szFname, m_szExt);
	}

	return m_szExt;
}

CString CSuperFileProc::GetFileSize(LPCTSTR lpszFileSize)
{
	CString strFileSize(lpszFileSize);

	UINT nSize = _ttoi(strFileSize);
	if (nSize == 0) return _T("0KB");

	nSize /= 1024;
	if (nSize == 0) return _T("1KB");

	strFileSize.Format(_T("%dKB"), nSize);
	return strFileSize;
}

//
// FUNCTIONS THAT DEAL WITH PIDLs
//

LONG CSuperFileProc::GetRegKey(HKEY key, LPCTSTR subkey, LPTSTR retdata) {
	HKEY hkey;
	LONG retval = RegOpenKeyEx(key, subkey, 0, KEY_QUERY_VALUE, &hkey);

	if (retval == ERROR_SUCCESS) {
		long datasize = MAX_PATH;
		TCHAR data[MAX_PATH];
		RegQueryValue(hkey, nullptr, data, &datasize);
		std::string instr(retdata);
		_tcscpy_s(const_cast<TCHAR*>(instr.c_str()), instr.size(), data);
		RegCloseKey(hkey);
	}

	return retval;
}

BOOL CSuperFileProc::CreateFolder(CString sPath) const
{
	int nPos = 0;

	sPath.TrimRight(_T(' '));
	sPath.TrimRight(_T('\\'));

	// Look for existing object:
	if (0xFFFFFFFF == GetFileAttributes(sPath)) {
		// doesn't exist yet - create it!
		if ((nPos = sPath.ReverseFind(_T('\\'))) > 0) {
			// Create parent directs:
			if (!CreateFolder(sPath.Mid(0, nPos))) {
				return FALSE;
			}
		}

		// Create node:
		CreateDirectory(sPath, nullptr);
	}

	return TRUE;
}

BOOL CSuperFileProc::IsDrive(const CString &sPath) {
	return (sPath.GetLength() == 3 && sPath.GetAt(1) == _T(':') && sPath.GetAt(2) == _T('\\'));
}

CString CSuperFileProc::GetCurrentFolder() const
{
	return m_strPath;
}

BOOL CSuperFileProc::FileReName(LPCTSTR lpszOldName, LPCTSTR lpszNewName) {
	if (!Exist(lpszOldName)) return FALSE;
	CFile f;
	f.Rename(lpszOldName, lpszNewName);
	return TRUE;
}

BOOL CSuperFileProc::Exist(LPCTSTR lpszFileName)
{
	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile((lpszFileName == nullptr) ? m_strFileName : lpszFileName, &fd);

	if (hFind != INVALID_HANDLE_VALUE)
		FindClose(hFind);

	return (hFind != INVALID_HANDLE_VALUE);
}