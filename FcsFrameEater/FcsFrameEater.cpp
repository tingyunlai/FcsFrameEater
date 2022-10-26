
#include <windows.h>
#include <streams.h>
#include <initguid.h>

#if (1100 > _MSC_VER)
#include <olectlid.h>
#else
#include <olectl.h>
#endif

#include "FcsUids.h"
#include "iEZ.h"
#include "FcsFrameEaterProperties.h"
#include "FcsFrameEater.h"
#include "resource.h"
#include "GdiPlus.h"

#include <ctime>


// Setup information
using namespace Gdiplus;
GdiplusStartupInput m_gdiplusStartupInput; 
ULONG_PTR m_pGdiToken; 

const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
	&MEDIATYPE_Video,       // Major type
	&MEDIASUBTYPE_NULL      // Minor type
};

const AMOVIESETUP_PIN sudpPins[] =
{
	{ L"Input",             // Pins string name
	FALSE,                // Is it rendered
	FALSE,                // Is it an output
	FALSE,                // Are we allowed none
	FALSE,                // And allowed many
	&CLSID_NULL,          // Connects to filter
	NULL,                 // Connects to pin
	1,                    // Number of types
	&sudPinTypes          // Pin information
	},
	{ L"Output",            // Pins string name
	FALSE,                // Is it rendered
	TRUE,                 // Is it an output
	FALSE,                // Are we allowed none
	FALSE,                // And allowed many
	&CLSID_NULL,          // Connects to filter
	NULL,                 // Connects to pin
	1,                    // Number of types
	&sudPinTypes          // Pin information
	}
};

const AMOVIESETUP_FILTER sudEZrgb24 =
{
	&CLSID_FcsFrameEater,         // Filter CLSID
	L"Fcs FrameEater",       // String name
	MERIT_DO_NOT_USE,       // Filter merit
	2,                      // Number of pins
	sudpPins                // Pin information
};


// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance

CFactoryTemplate g_Templates[] = {
	{ L"Fcs FrameEater"
	, &CLSID_FcsFrameEater
	, CFcsFrameEater::CreateInstance
	, NULL
	, &sudEZrgb24 }
	,
	{ L"Property"
	, &CLSID_FcsFrameEaterPropertyPage
	, CFcsFrameEaterProperties::CreateInstance }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


////////////////////////////////////////////////////////////////////////
//
// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).
//
////////////////////////////////////////////////////////////////////////

//
// DllRegisterServer
//
// Handles sample registry and unregistry
//
STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2( TRUE );

} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2( FALSE );

} // DllUnregisterServer

//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
					  DWORD  dwReason, 
					  LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}


//
// Constructor
//
CFcsFrameEater::CFcsFrameEater(TCHAR *tszName,
						 LPUNKNOWN punk,
						 HRESULT *phr) :
CTransformFilter(tszName, punk, CLSID_FcsFrameEater),
m_effect(IDC_NONE),
m_lBufferRequest(1),
CPersistStream(punk, phr)
{
	char sz[60];

	GetProfileStringA("Quartz", "EffectStart", "0.0", sz, 60);
	m_effectStartTime = COARefTime(atof(sz));

	GetProfileStringA("Quartz", "EffectLength", "5000000000.0", sz, 60);
	m_effectTime = COARefTime(atof(sz));
	m_currentTime = COARefTime(0.0);     
	m_lastValidTime = COARefTime(0.0);
	//initial value of attributes
	m_nOldWidth = 0;
	m_nOldHeight = 0;
	m_nNewWidth = 0;
	m_nNewHeight = 0;
	m_nImageType = TypeJpg;
	m_bKeepRatio = false;
	m_bGraylevel = false;
	m_nNamedType = NamedByIndex;
	wsprintf(m_szExportPath,  L"%s",  L"c:\\a6\\");
	m_nIndex = 0;
	GdiplusStartup(&m_pGdiToken,&m_gdiplusStartupInput,NULL); 

} // (Constructor)


//
// CreateInstance
//
// Provide the way for COM to create a EZrgb24 object
//
CFcsFrameEater::~CFcsFrameEater()
{
	m_nIndex = 0;
	GdiplusShutdown(m_pGdiToken); 

}
CUnknown *CFcsFrameEater::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	ASSERT(phr);

	CFcsFrameEater *pNewObject = new CFcsFrameEater(NAME("Fcs FrameEater"), punk, phr);

	if (pNewObject == NULL) {
		if (phr)
			*phr = E_OUTOFMEMORY;
	}
	return pNewObject;

} // CreateInstance


//
// NonDelegatingQueryInterface
//
// Reveals IFcsFrameEater and ISpecifyPropertyPages
//
STDMETHODIMP CFcsFrameEater::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	CheckPointer(ppv,E_POINTER);

	if (riid == IID_IFcsFrameEater) {
		return GetInterface((IFcsFrameEater *) this, ppv);

	} else if (riid == IID_ISpecifyPropertyPages) {
		return GetInterface((ISpecifyPropertyPages *) this, ppv);

	} else {
		return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
	}

} // NonDelegatingQueryInterface


//
// Transform
//
// Copy the input sample into the output sample - then transform the output
// sample 'in place'. If we have all keyframes, then we shouldn't do a copy
// If we have cinepak or indeo and are decompressing frame N it needs frame
// decompressed frame N-1 available to calculate it, unless we are at a
// keyframe. So with keyframed codecs, you can't get away with applying the
// transform to change the frames in place, because you'll mess up the next
// frames decompression. The runtime MPEG decoder does not have keyframes in
// the same way so it can be done in place. We know if a sample is key frame
// as we transform because the sync point property will be set on the sample
//
HRESULT CFcsFrameEater::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	CheckPointer(pIn,E_POINTER);   
	CheckPointer(pOut,E_POINTER);   

	// Copy the properties across

	HRESULT hr = Copy(pIn, pOut);
	if (FAILED(hr)) {
		return hr;
	}

	m_nIndex++;
	// Check to see if it is time to do the sample

	CRefTime tStart, tStop ;
	hr = pIn->GetTime((REFERENCE_TIME *) &tStart, (REFERENCE_TIME *) &tStop);

	if (tStart >= m_effectStartTime) 
	{
		if (tStop <= (m_effectStartTime + m_effectTime)) 
		{
			return Transform(pOut);
		}
	}

	return NOERROR;

} // Transform


//
// Copy
//
// Make destination an identical copy of source
//
HRESULT CFcsFrameEater::Copy(IMediaSample *pSource, IMediaSample *pDest) 
{
	CheckPointer(pSource,E_POINTER);   
	CheckPointer(pDest,E_POINTER);   

	// Copy the sample data

	BYTE *pSourceBuffer, *pDestBuffer;
	long lSourceSize = pSource->GetActualDataLength();

#ifdef DEBUG
	long lDestSize = pDest->GetSize();
	ASSERT(lDestSize >= lSourceSize);
#endif

	pSource->GetPointer(&pSourceBuffer);
	pDest->GetPointer(&pDestBuffer);

	//m_interface.Draw((RGBTRIPLE*)pSourceBuffer, m_imgWidth, m_imgHeight);
	CopyMemory( (PVOID) pDestBuffer,(PVOID) pSourceBuffer,lSourceSize);

	// Copy the sample times

	REFERENCE_TIME TimeStart, TimeEnd;
	if (NOERROR == pSource->GetTime(&TimeStart, &TimeEnd)) {
		pDest->SetTime(&TimeStart, &TimeEnd);
	}

	LONGLONG MediaStart, MediaEnd;
	if (pSource->GetMediaTime(&MediaStart,&MediaEnd) == NOERROR) {
		pDest->SetMediaTime(&MediaStart,&MediaEnd);
	}

	// Copy the Sync point property

	HRESULT hr = pSource->IsSyncPoint();
	if (hr == S_OK) {
		pDest->SetSyncPoint(TRUE);
	}
	else if (hr == S_FALSE) {
		pDest->SetSyncPoint(FALSE);
	}
	else {  // an unexpected error has occured...
		return E_UNEXPECTED;
	}

	// Copy the media type

	AM_MEDIA_TYPE *pMediaType;
	pSource->GetMediaType(&pMediaType);
	pDest->SetMediaType(pMediaType);
	DeleteMediaType(pMediaType);

	// Copy the preroll property

	hr = pSource->IsPreroll();
	if (hr == S_OK) {
		pDest->SetPreroll(TRUE);
	}
	else if (hr == S_FALSE) {
		pDest->SetPreroll(FALSE);
	}
	else {  // an unexpected error has occured...
		return E_UNEXPECTED;
	}

	// Copy the discontinuity property

	hr = pSource->IsDiscontinuity();
	if (hr == S_OK) {
		pDest->SetDiscontinuity(TRUE);
	}
	else if (hr == S_FALSE) {
		pDest->SetDiscontinuity(FALSE);
	}
	else {  // an unexpected error has occured...
		return E_UNEXPECTED;
	}

	// Copy the actual data length

	long lDataLength = pSource->GetActualDataLength();
	pDest->SetActualDataLength(lDataLength);

	return NOERROR;

} // Copy


//
// Transform (in place)
//
// 'In place' apply the image effect to this sample
//
HRESULT CFcsFrameEater::Transform(IMediaSample *pMediaSample)
{
	BYTE *pData;                // Pointer to the actual image buffer
	long lDataLen;              // Holds length of any given sample

	AM_MEDIA_TYPE* pType = &m_pInput->CurrentMediaType();
	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pType->pbFormat;
	BITMAPINFOHEADER  bmi = pvi->bmiHeader;
	ASSERT(pvi);

	CheckPointer(pMediaSample,E_POINTER);
	pMediaSample->GetPointer(&pData);
	lDataLen = pMediaSample->GetSize();

	// Get the image properties from the BITMAPINFOHEADER

	m_imgWidth    = pvi->bmiHeader.biWidth;
	m_imgHeight    = pvi->bmiHeader.biHeight;
	int numPixels  = m_imgWidth * m_imgHeight;
	m_nOldWidth = m_imgWidth;
	m_nOldHeight = m_imgHeight;
	REFERENCE_TIME TimeStart, TimeEnd;
	pMediaSample->GetTime(&TimeStart, &TimeEnd);
	m_lastValidTime = TimeStart;
	m_interface.m_tmTimeNow = TimeStart;
	if (m_bChanged)
	{
		m_interface.SetImageAttribute(m_nOldWidth, m_nOldHeight, m_nNewWidth, m_nNewHeight, m_nImageType,
			m_bKeepRatio, m_bGraylevel, m_nNamedType, m_szExportPath, TimeStart, m_nIndex);
		m_bChanged = FALSE;
	}
	m_interface.runInterface(pData, pvi, TimeStart);

	return NOERROR;

} // Transform (in place)


// Check the input type is OK - return an error otherwise

HRESULT CFcsFrameEater::CheckInputType(const CMediaType *mtIn)
{
	CheckPointer(mtIn,E_POINTER);

	// check this is a VIDEOINFOHEADER type

	if (*mtIn->FormatType() != FORMAT_VideoInfo) {
		return E_INVALIDARG;
	}

	// Can we transform this type

	if (CanPerformFrameEater(mtIn)) {
		return NOERROR;
	}
	return E_FAIL;
}


//
// Checktransform
//
// Check a transform can be done between these formats
//
HRESULT CFcsFrameEater::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	CheckPointer(mtIn,E_POINTER);
	CheckPointer(mtOut,E_POINTER);

	if (CanPerformFrameEater(mtIn)) 
	{
		if (*mtIn == *mtOut) 
		{
			return NOERROR;
		}
	}

	return E_FAIL;

} // CheckTransform


//
// DecideBufferSize
//
// Tell the output pin's allocator what size buffers we
// require. Can only do this when the input is connected
//
HRESULT CFcsFrameEater::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
	// Is the input pin connected

	if (m_pInput->IsConnected() == FALSE) {
		return E_UNEXPECTED;
	}

	CheckPointer(pAlloc,E_POINTER);
	CheckPointer(pProperties,E_POINTER);
	HRESULT hr = NOERROR;

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = m_pInput->CurrentMediaType().GetSampleSize();
	ASSERT(pProperties->cbBuffer);

	// Ask the allocator to reserve us some sample memory, NOTE the function
	// can succeed (that is return NOERROR) but still not have allocated the
	// memory that we requested, so we must check we got whatever we wanted

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProperties,&Actual);
	if (FAILED(hr)) {
		return hr;
	}

	ASSERT( Actual.cBuffers == 1 );

	if (pProperties->cBuffers > Actual.cBuffers ||
		pProperties->cbBuffer > Actual.cbBuffer) {
			return E_FAIL;
	}
	return NOERROR;

} // DecideBufferSize


//
// GetMediaType
//
// I support one type, namely the type of the input pin
// This type is only available if my input is connected
//
HRESULT CFcsFrameEater::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	// Is the input pin connected

	if (m_pInput->IsConnected() == FALSE) {
		return E_UNEXPECTED;
	}

	// This should never happen

	if (iPosition < 0) {
		return E_INVALIDARG;
	}

	// Do we have more items to offer

	if (iPosition > 0) {
		return VFW_S_NO_MORE_ITEMS;
	}

	CheckPointer(pMediaType,E_POINTER);
	*pMediaType = m_pInput->CurrentMediaType();

	return NOERROR;

} // GetMediaType


//
// CanPerformEZrgb24
//
// Check if this is a RGB24 true colour format
//
BOOL CFcsFrameEater::CanPerformFrameEater(const CMediaType *pMediaType) 
{
	CheckPointer(pMediaType,FALSE);

	if (IsEqualGUID(*pMediaType->Type(), MEDIATYPE_Video)) 
	{
		if (IsEqualGUID(*pMediaType->Subtype(), MEDIASUBTYPE_RGB24)) 
		{
			VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pMediaType->Format();
			return (pvi->bmiHeader.biBitCount == 24);
		}
	}

	return FALSE;

} // CanPerformEZrgb24


#define WRITEOUT(var)  hr = pStream->Write(&var, sizeof(var), NULL); \
	if (FAILED(hr)) return hr;

#define READIN(var)    hr = pStream->Read(&var, sizeof(var), NULL); \
	if (FAILED(hr)) return hr;


//
// GetClassID
//
// This is the only method of IPersist
//
STDMETHODIMP CFcsFrameEater::GetClassID(CLSID *pClsid)
{
	return CBaseFilter::GetClassID(pClsid);

} // GetClassID


//
// ScribbleToStream
//
// Overriden to write our state into a stream
//
HRESULT CFcsFrameEater::ScribbleToStream(IStream *pStream)
{
	HRESULT hr;

	WRITEOUT(m_effect);
	WRITEOUT(m_effectStartTime);
	WRITEOUT(m_effectTime);

	return NOERROR;

} // ScribbleToStream


//
// ReadFromStream
//
// Likewise overriden to restore our state from a stream
//
HRESULT CFcsFrameEater::ReadFromStream(IStream *pStream)
{
	HRESULT hr;

	READIN(m_effect);
	READIN(m_effectStartTime);
	READIN(m_effectTime);

	return NOERROR;

} // ReadFromStream


//
// GetPages
//
// Returns the clsid's of the property pages we support
//
STDMETHODIMP CFcsFrameEater::GetPages(CAUUID *pPages)
{
	CheckPointer(pPages,E_POINTER);

	pPages->cElems = 1;
	pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
	if (pPages->pElems == NULL) {
		return E_OUTOFMEMORY;
	}

	*(pPages->pElems) = CLSID_FcsFrameEaterPropertyPage;
	return NOERROR;

} // GetPages


//
// get_IPEffect
//
// Return the current effect selected
//
STDMETHODIMP CFcsFrameEater::get_IPEffect(int *IPEffect,REFTIME *pStart,REFTIME *pLength)
{
	CAutoLock cAutolock(&m_FrameEaterLock);
	CheckPointer(IPEffect,E_POINTER);
	CheckPointer(pStart,E_POINTER);
	CheckPointer(pLength,E_POINTER);

	*IPEffect = m_effect;
	*pStart = COARefTime(m_effectStartTime);
	*pLength = COARefTime(m_effectTime);

	return NOERROR;

} // get_IPEffect

STDMETHODIMP CFcsFrameEater::put_IPEffect(int IPEffect,REFTIME start,REFTIME length, bool bChanged)
{
	CAutoLock cAutolock(&m_FrameEaterLock);

	m_effect = IPEffect;
	m_effectStartTime = COARefTime(start);
	m_effectTime = COARefTime(length);
	m_bChanged = bChanged;
	SetDirty(TRUE);
	return NOERROR;

} // put_IPEffect

STDMETHODIMP CFcsFrameEater::put_ImageAttribute(long nOldWidth,long nOldHeight, long nNewWidth,long nNewHeight,
	long nImageType, bool bKeepRatio, bool bGraylevel, long nNamedType, TCHAR* szPath)
{
	CAutoLock cAutolock(&m_FrameEaterLock);
	m_nOldWidth = nOldWidth;
	m_nOldHeight = nOldHeight;
	m_nNewWidth = nNewWidth;
	m_nNewHeight = nNewHeight;
	m_nImageType = nImageType;
	m_bKeepRatio = bKeepRatio;
	m_bGraylevel = bGraylevel;
	m_nNamedType = nNamedType;
	wsprintf(m_szExportPath, L"%s",  szPath);

	return NOERROR;

}
STDMETHODIMP CFcsFrameEater::get_ImageAttribute(long* nOldWidth,long* nOldHeight, long* nNewWidth,long* nNewHeight,
	long* nImageType, bool* bKeepRatio, bool* bGraylevel, long* nNamedType, TCHAR* szPath)
{
	CAutoLock cAutolock(&m_FrameEaterLock);
	CheckPointer(szPath,E_POINTER);
	*nOldWidth = m_nOldWidth;
	*nOldHeight = m_nOldHeight;
	*nNewWidth = m_nNewWidth;
	*nNewHeight = m_nNewHeight;
	*nImageType = m_nImageType;
	*bKeepRatio = m_bKeepRatio;
	*bGraylevel = m_bGraylevel;
	*nNamedType = m_nNamedType;
	wcscpy(szPath, m_szExportPath);
	return NOERROR;

}
