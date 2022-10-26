#ifndef __FILTERINTERFACE_H__
#define __FILTERINTERFACE_H__
#include <windows.h>
#include <fstream>
enum ImageType
{
	TypeBmp = 1034,
	TypeJpg = 1035,
	TypePng = 1036
};

enum NamedType
{
	NamedByTime = 1046,
	NamedByIndex = 1047
};

class FilterInterface
{
public:

	FilterInterface();    // Constructor
	~FilterInterface();	// Destructor
	int runInterface(BYTE *pData, VIDEOINFOHEADER* pviHeader, REFERENCE_TIME tmNow);
	void SetImageAttribute(long nOldWidth,long nOldHeight, long nNewWidth,long nNewHeight,
		long nImageType, bool bKeepRatio, bool bGraylevel, long nNamedType, TCHAR* szPath, REFTIME tmTimeNow, long nIndex);
	long m_nScale; // the interval of image
	REFTIME m_tmTimeNow;

private:
	BYTE* m_pImagePixel;
	int m_nWidth; // video width and height
	int m_nHeight;
	int m_nBitperPixel;
	BOOL m_bFirstImage;

	//attributes of image
	long m_nOldWidth;
	long m_nOldHeight;
	long m_nNewWidth;
	long m_nNewHeight;
	long m_nImageType;
	bool m_bKeepRatio;
	bool m_bGraylevel;
	long m_nNamedType;
	TCHAR m_szExportPath[STR_MAX_LENGTH];
	long m_nImageCount;
	

}; // FilterInterface

#endif __FILTERINTERFACE_H__
