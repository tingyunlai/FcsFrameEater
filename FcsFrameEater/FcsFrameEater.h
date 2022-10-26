
#pragma warning(disable: 4097 4511 4512 4514 4705 4099)
#include "FilterInterface.h"
class CFcsFrameEater : public CTransformFilter,
	public IFcsFrameEater,
	public ISpecifyPropertyPages,
	public CPersistStream
{

public:

	DECLARE_IUNKNOWN;
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

	// Reveals IEZrgb24 and ISpecifyPropertyPages
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// CPersistStream stuff
	HRESULT ScribbleToStream(IStream *pStream);
	HRESULT ReadFromStream(IStream *pStream);

	// Overrriden from CTransformFilter base class

	HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
	HRESULT CheckInputType(const CMediaType *mtIn);
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc,
		ALLOCATOR_PROPERTIES *pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

	// These implement the custom IFcsFrameEater interface

	STDMETHODIMP get_IPEffect(int *IPEffect, REFTIME *StartTime, REFTIME *Length);
	STDMETHODIMP put_IPEffect(int IPEffect, REFTIME StartTime, REFTIME Length, bool bChanged);
	STDMETHODIMP put_ImageAttribute(long nOldWidth,long nOldHeight, long nNewWidth,long nNewHeight,
		long nImageType, bool bKeepRatio, bool bGraylevel, long nNamedType, TCHAR* szPath);
	STDMETHODIMP get_ImageAttribute(long* nOldWidth,long* nOldHeight, long* nNewWidth,long* nNewHeight,
		long* nImageType, bool* bKeepRatio, bool* bGraylevel, long* nNamedType, TCHAR* szPath);

	// ISpecifyPropertyPages interface
	STDMETHODIMP GetPages(CAUUID *pPages);

	// CPersistStream override
	STDMETHODIMP GetClassID(CLSID *pClsid);

private:

	// Constructor
	CFcsFrameEater(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	~CFcsFrameEater();

	// Look after doing the special effect
	BOOL CanPerformFrameEater(const CMediaType *pMediaType) ;
	HRESULT Copy(IMediaSample *pSource, IMediaSample *pDest) ;
	HRESULT Transform(IMediaSample *pMediaSample);

	CCritSec    m_FrameEaterLock;          // Private play critical section
	int         m_effect;               // Which effect are we FrameEatering
	CRefTime    m_effectStartTime;      // When the effect will begin
	CRefTime    m_effectTime;           // And how long it will last for
	const long m_lBufferRequest;        // The number of buffers to use
	CRefTime    m_currentTime;      // time of currently executing filter.
	CRefTime    m_lastValidTime;    // last time something valid was tracked
	CRefTime    m_resetInterval;    // when this much time passes since last valid
	FilterInterface m_interface;
	int m_imgWidth;
	int m_imgHeight;
	bool m_bChanged;

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
	long m_nIndex;

}; 

