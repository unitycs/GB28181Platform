//////////////////////////////////////////////////////////////////////////
#ifndef SIMOBJ_H
#define SIMOBJ_H

#include "stdafx.h"
using namespace ATL;
// CComSimObject: Simple object with/without coclass declare.
template<class T, const CLSID* pclsid = &CLSID_NULL>
class ATL_NO_VTABLE CComSimCoClass :
	public IUnknown,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<T, (const CLSID*) pclsid>
{
public:
	typedef CComSimCoClass<T, pclsid> _BaseObject;

public:
	CComSimCoClass() {
	}

	virtual ~CComSimCoClass() {
	}
};

//////////////////////////////////////////////////////////////////////////
// CComSimDispatchObject: Simple object a specified default dispatch interface.
template<class T, class TIDispatch, const CLSID* pclsid = &CLSID_NULL>
class ATL_NO_VTABLE CComSimDispatchCoClass : 
	public CComSimCoClass<T, pclsid>,
	public IDispatchImpl<TIDispatch, &__uuidof(TIDispatch), &LIBID_HUS_DataManager_Search_Contract, /* wMajor = */ 1, /* wMinor = */ 0>
{
public:
	CComSimDispatchCoClass() {
	}

public:
	BEGIN_COM_MAP(T)
		COM_INTERFACE_ENTRY2(IDispatch, TIDispatch)
		COM_INTERFACE_ENTRY(TIDispatch)
	END_COM_MAP()
};

// Create a com object and get the corresponding interface.
template<class TObject, class TInterface>
HRESULT ComObjectCreate(CComObject<TObject>*& pObjCom, TInterface*& pICom) {
	ATLASSERT(pObjCom == NULL);
	ATLASSERT(pICom == NULL);
	pObjCom = NULL, pICom = NULL;

	HRESULT rc = CComObject<TObject>::CreateInstance(&pObjCom);
	if (FAILED(rc)) {
		return rc;
	}
    
    if(NULL != pObjCom){
	    if (FAILED(rc = pObjCom->QueryInterface(&pICom))) {
		    delete pObjCom;
		    pObjCom = NULL;
		    pICom = NULL;
		    return rc;
	    }
    }

	return S_OK;
}

#endif