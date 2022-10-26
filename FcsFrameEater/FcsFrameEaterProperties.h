
#include <strsafe.h>

class CFcsFrameEaterProperties : public CBasePropertyPage
{

public:

	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

private:

	INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	HRESULT OnConnect(IUnknown *pUnknown);
	HRESULT OnDisconnect();
	HRESULT OnActivate();
	HRESULT OnDeactivate();
	HRESULT OnApplyChanges();

	void    GetControlValues();
	CFcsFrameEaterProperties(LPUNKNOWN lpunk, HRESULT *phr);

	BOOL m_bIsInitialized;      // Used to ignore startup messages
	int m_effect;               // Which effect are we processing
	REFTIME m_start;            // When the effect will begin
	REFTIME m_length;           // And how long it will last for
	IFcsFrameEater *m_pIPEffect;     // The custom interface on the filter
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

}; 

