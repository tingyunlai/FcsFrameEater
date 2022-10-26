//------------------------------------------------------------------------------
// File: EZProp.cpp
//
// Desc: DirectShow sample code - implementation of CEZRgb24Properties class.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include <windows.h>
#include <windowsx.h>
#include <GdiPlus.h>
#include <streams.h>
#include <commctrl.h>
#include <olectl.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include "resource.h"
#include "FcsUids.h"
#include "iEZ.h"
#include "FcsFrameEater.h"
#include "FcsFrameEaterProperties.h"


//
// CreateInstance
//
// Used by the DirectShow base classes to create instances
//
TCHAR* oldFont = _T("");

CUnknown *CFcsFrameEaterProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);

	CUnknown *punk = new CFcsFrameEaterProperties(lpunk, phr);

	if (punk == NULL) {
		if (phr)
			*phr = E_OUTOFMEMORY;
	}

	return punk;

} // CreateInstance


//
// Constructor
//
CFcsFrameEaterProperties::CFcsFrameEaterProperties(LPUNKNOWN pUnk, HRESULT *phr) :
CBasePropertyPage(NAME("Special Effects Property Page"), pUnk,
				  IDD_EZrgb24PROP, IDS_TITLE),
				  m_pIPEffect(NULL),
				  m_bIsInitialized(FALSE)
{
	ASSERT(phr);
	//initial value of attributes
	m_nOldWidth = 0;
	m_nOldHeight = 0;
	m_nNewWidth = 0;
	m_nNewHeight = 0;
	m_nImageType = TypeJpg;
	m_bKeepRatio = false;
	m_bGraylevel = false;
	m_nNamedType = NamedByTime;
	_stprintf_s(m_szExportPath,  _T("%s"),  _T("d:\\a1"));
} // (Constructor)


//
// OnReceiveMessage
//
// Handles the messages for our property window
//
INT_PTR CFcsFrameEaterProperties::OnReceiveMessage(HWND hwnd,
													   UINT uMsg,
													   WPARAM wParam,
													   LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		{
			if (m_bIsInitialized)
			{
				m_bDirty = TRUE;
				if (m_pPageSite)
				{
					m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
				}
			}
			return (LRESULT) 1;
		}

	}

	return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);

} // OnReceiveMessage


//
// OnConnect
//
// Called when we connect to a transform filter
//
HRESULT CFcsFrameEaterProperties::OnConnect(IUnknown *pUnknown)
{
	CheckPointer(pUnknown,E_POINTER);
	ASSERT(m_pIPEffect == NULL);

	HRESULT hr = pUnknown->QueryInterface(IID_IFcsFrameEater, (void **) &m_pIPEffect);
	if (FAILED(hr)) {
		return E_NOINTERFACE;
	}

	// Get the initial image FX property
	CheckPointer(m_pIPEffect,E_FAIL);
	m_pIPEffect->get_ImageAttribute(&m_nOldWidth, &m_nOldHeight, &m_nNewWidth, &m_nNewHeight, &m_nImageType,
		&m_bKeepRatio, &m_bGraylevel, &m_nNamedType, m_szExportPath);
	m_pIPEffect->get_IPEffect(&m_effect, &m_start, &m_length);
	m_bIsInitialized = FALSE ;

	return NOERROR;

} // OnConnect


//
// OnDisconnect
//
// Likewise called when we disconnect from a filter
//
HRESULT CFcsFrameEaterProperties::OnDisconnect()
{
	// Release of Interface after setting the appropriate old effect value
	if(m_pIPEffect)
	{
		m_pIPEffect->Release();
		m_pIPEffect = NULL;
	}
	return NOERROR;

} // OnDisconnect


//
// OnActivate
//
// We are being activated
//
HRESULT CFcsFrameEaterProperties::OnActivate()
{
	TCHAR sz[60];

	(void)StringCchPrintf(sz, NUMELMS(sz), TEXT("%f\0"), m_length);
	Edit_SetText(GetDlgItem(m_Dlg, IDC_LENGTH), sz);

	(void)StringCchPrintf(sz, NUMELMS(sz), TEXT("%f\0"), m_start);
	Edit_SetText(GetDlgItem(m_Dlg, IDC_START), sz);

	m_bIsInitialized = TRUE;

	// set attributes
	(void)StringCchPrintf(sz, NUMELMS(sz), TEXT("%ld\0"), m_nOldWidth);
	Edit_SetText(GetDlgItem(m_Dlg, IDC_OLD_WIDTH), sz);
	(void)StringCchPrintf(sz, NUMELMS(sz), TEXT("%ld\0"), m_nOldHeight);
	Edit_SetText(GetDlgItem(m_Dlg, IDC_OLD_HEIGHT), sz);
	(void)StringCchPrintf(sz, NUMELMS(sz), TEXT("%ld\0"), m_nNewWidth);
	Edit_SetText(GetDlgItem(m_Dlg, IDC_NEW_WIDTH), sz);
	(void)StringCchPrintf(sz, NUMELMS(sz), TEXT("%ld\0"), m_nNewHeight);
	Edit_SetText(GetDlgItem(m_Dlg, IDC_NEW_HEIGHT), sz);
	CheckRadioButton(m_Dlg, IDC_RADIO_BMP, IDC_RADIO_PNG, m_nImageType);
	Button_SetCheck(GetDlgItem(m_Dlg, IDC_CHECK_RATIO), m_bKeepRatio);
	Button_SetCheck(GetDlgItem(m_Dlg, IDC_CHECK_GRAYLEVEL), m_bGraylevel);
	CheckRadioButton(m_Dlg, IDC_RADIO_TIME, IDC_RADIO_INDEX, m_nNamedType);
	Edit_SetText(GetDlgItem(m_Dlg, IDC_EXPORTED_PATH), m_szExportPath);


	return NOERROR;

} // OnActivate


//
// OnDeactivate
//
// We are being deactivated
//
HRESULT CFcsFrameEaterProperties::OnDeactivate(void)
{
	ASSERT(m_pIPEffect);

	m_bIsInitialized = FALSE;
	GetControlValues();

	return NOERROR;

} // OnDeactivate


//
// OnApplyChanges
//
// Apply any changes so far made
//
HRESULT CFcsFrameEaterProperties::OnApplyChanges()
{
	GetControlValues();

	CheckPointer(m_pIPEffect,E_POINTER);

	m_pIPEffect->put_IPEffect(m_effect, m_start, m_length, TRUE);
	m_pIPEffect->put_ImageAttribute(m_nOldWidth, m_nOldHeight, m_nNewWidth, m_nNewHeight, m_nImageType,
		m_bKeepRatio, m_bGraylevel, m_nNamedType, m_szExportPath);
	return NOERROR;

} // OnApplyChanges


void CFcsFrameEaterProperties::GetControlValues()
{
	TCHAR sz[STR_MAX_LENGTH];
	REFTIME tmp1, tmp2 ;
	long i;
	// Get the start and effect times
	Edit_GetText(GetDlgItem(m_Dlg, IDC_LENGTH), sz, STR_MAX_LENGTH);

#ifdef UNICODE
	// Convert Multibyte string to ANSI
	char szANSI[STR_MAX_LENGTH];

	int rc = WideCharToMultiByte(CP_ACP, 0, sz, -1, szANSI, STR_MAX_LENGTH, NULL, NULL);
	tmp2 = COARefTime(atof(szANSI));
#else
	tmp2 = COARefTime(atof(sz));
#endif

	Edit_GetText(GetDlgItem(m_Dlg, IDC_START), sz, STR_MAX_LENGTH);

#ifdef UNICODE
	// Convert Multibyte string to ANSI
	rc = WideCharToMultiByte(CP_ACP, 0, sz, -1, szANSI, STR_MAX_LENGTH, NULL, NULL);
	tmp1 = COARefTime(atof(szANSI));
#else
	tmp1 = COARefTime(atof(sz));
#endif

	// Quick validation of the fields

	if (tmp1 >= 0 && tmp2 >= 0) {
		m_start  = tmp1;
		m_length = tmp2;
	}

	//get attributes
	Edit_GetText(GetDlgItem(m_Dlg, IDC_OLD_WIDTH), sz, STR_MAX_LENGTH);
#ifdef UNICODE
	// Convert Multibyte string to ANSI
	rc = WideCharToMultiByte(CP_ACP, 0, sz, -1, szANSI, STR_MAX_LENGTH, NULL, NULL);
	m_nOldWidth = atol(szANSI);
#else
	m_nOldWidth = atol(szANSI);
#endif

	Edit_GetText(GetDlgItem(m_Dlg, IDC_OLD_HEIGHT), sz, STR_MAX_LENGTH);
#ifdef UNICODE
	// Convert Multibyte string to ANSI
	rc = WideCharToMultiByte(CP_ACP, 0, sz, -1, szANSI, STR_MAX_LENGTH, NULL, NULL);
	m_nOldHeight = atol(szANSI);
#else
	m_nOldHeight = atol(szANSI);
#endif

	Edit_GetText(GetDlgItem(m_Dlg, IDC_NEW_WIDTH), sz, STR_MAX_LENGTH);
#ifdef UNICODE
	// Convert Multibyte string to ANSI
	rc = WideCharToMultiByte(CP_ACP, 0, sz, -1, szANSI, STR_MAX_LENGTH, NULL, NULL);
	m_nNewWidth = atol(szANSI);
#else
	m_nNewWidth = atol(szANSI);
#endif

	Edit_GetText(GetDlgItem(m_Dlg, IDC_NEW_HEIGHT), sz, STR_MAX_LENGTH);
#ifdef UNICODE
	// Convert Multibyte string to ANSI
	rc = WideCharToMultiByte(CP_ACP, 0, sz, -1, szANSI, STR_MAX_LENGTH, NULL, NULL);
	m_nNewHeight = atol(szANSI);
#else
	m_nNewHeight = atol(szANSI);
#endif

	for (i = IDC_RADIO_BMP; i <= IDC_RADIO_PNG; i++) 
	{
		if (IsDlgButtonChecked(m_Dlg, i)) 
		{
			m_nImageType = i;
			break;
		}
	}

	m_bKeepRatio = (bool)Button_GetCheck(GetDlgItem(m_Dlg, IDC_CHECK_RATIO));
	m_bGraylevel = (bool)Button_GetCheck(GetDlgItem(m_Dlg, IDC_CHECK_GRAYLEVEL));

	for (i = IDC_RADIO_TIME; i <= IDC_RADIO_INDEX; i++) 
	{
		if (IsDlgButtonChecked(m_Dlg, i)) 
		{
			m_nNamedType = i;
			break;
		}
	}

	Edit_GetText(GetDlgItem(m_Dlg, IDC_EXPORTED_PATH), m_szExportPath, STR_MAX_LENGTH);


}

