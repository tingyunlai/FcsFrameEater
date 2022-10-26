#include <streams.h>
#include <ctime>
#include <GdiPlus.h>
#include "FilterInterface.h"
#include <tchar.h>
#include <atlimage.h>
using namespace Gdiplus;
#define HDIB HANDLE

FilterInterface::FilterInterface(){
	//set max posibility of pixel value of m_pixels
	m_nWidth = 0;
	m_nHeight = 0;
	m_pImagePixel = NULL;
	m_nBitperPixel = 24;
	m_bFirstImage = TRUE;
	m_nImageCount = 0;
	//initial value of attributes
	m_nOldWidth = 0;
	m_nOldHeight = 0;
	m_nNewWidth = 480;
	m_nNewHeight = 270;
	m_nImageType = TypeJpg;
	m_bKeepRatio = false;
	m_bGraylevel = false;
	m_nNamedType = NamedByIndex;
	_stprintf_s(m_szExportPath,  L"%s",  L"c:\\a6\\");


}   // Constructor


FilterInterface::~FilterInterface()
{
	m_nImageCount = 0;
}	// Destructor


int FilterInterface::runInterface(BYTE *pData, VIDEOINFOHEADER* pviHeader, REFERENCE_TIME tmNow)
{
	BITMAPINFO bmpInfo;
	RGBQUAD bmiColor;
	bmiColor.rgbBlue = 255;
	bmiColor.rgbGreen = 255;
	bmiColor.rgbRed = 255;
	bmiColor.rgbReserved = 0;
	bmpInfo.bmiHeader = pviHeader->bmiHeader;
	int nBitPerPixel = pviHeader->bmiHeader.biBitCount;
	int imgWidth = pviHeader->bmiHeader.biWidth;
	int imgHeight = pviHeader->bmiHeader.biHeight;

	wchar_t sz[STR_MAX_LENGTH];
	wchar_t path[MAX_PATH];
	wsprintf(path, L"%s%I64d.jpg", m_szExportPath, tmNow);
	CImage im;
	BOOL hr1 = im.Create(imgWidth, imgHeight, 32, 1);
	HDC dc = im.GetDC();
	Graphics plot(dc);
	SetDIBitsToDevice(dc, 0, 0, imgWidth, imgHeight, 0, 0, 0, imgHeight, pData, &bmpInfo, DIB_RGB_COLORS);
	im.ReleaseDC();
	if (m_nNamedType == NamedByIndex)
	{
		wsprintf(sz, L"%s%ld", m_szExportPath, m_nImageCount);
	}
	if (m_nNamedType == NamedByTime)
	{
		wsprintf(sz, L"%s%lf", m_szExportPath, (double)m_tmTimeNow);
	}
	switch (m_nImageType)
	{
	case TypeBmp:
		wsprintf(path, L"%s.bmp", sz);
		im.Save(path, Gdiplus::ImageFormatBMP);
		break;
	case TypeJpg:
		wsprintf(path, L"%s.jpg", sz);
		im.Save(path, Gdiplus::ImageFormatJPEG);
		break;
	case TypePng:
		wsprintf(path, L"%s.png", sz);
		im.Save(path, Gdiplus::ImageFormatPNG);
		break;
	}
	m_nImageCount++;

	return 0;
}

void FilterInterface::SetImageAttribute(long nOldWidth,long nOldHeight, long nNewWidth,long nNewHeight,
	long nImageType, bool bKeepRatio, bool bGraylevel, long nNamedType, TCHAR* szPath, REFTIME tmTimeNow, long nIndex)
{
	m_nOldWidth = nOldWidth;
	m_nOldHeight = nOldHeight;
	m_nNewWidth = nNewWidth;
	m_nNewHeight = nNewHeight;
	m_nImageType = nImageType;
	m_bKeepRatio = bKeepRatio;
	m_bGraylevel = bGraylevel;
	m_nNamedType = nNamedType;
	_stprintf(m_szExportPath, L"%s",  szPath);
	m_tmTimeNow = tmTimeNow;
	m_nImageCount = nIndex;
}

