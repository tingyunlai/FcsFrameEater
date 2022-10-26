

#ifndef __IEZ__
#define __IEZ__

#ifdef __cplusplus
extern "C" {
#endif


	// {A8A435F3-C365-4d4f-86DB-0BBDA08209E7}
	DEFINE_GUID(IID_IFcsFrameEater,
		0xa8a435f3, 0xc365, 0x4d4f, 0x86, 0xdb, 0xb, 0xbd, 0xa0, 0x82, 0x9, 0xe7);

	DECLARE_INTERFACE_(IFcsFrameEater, IUnknown)
	{
		STDMETHOD(get_IPEffect) (THIS_
			int *effectNum,         // The current effect
			REFTIME *StartTime,     // Start time of effect
			REFTIME *Length         // length of effect
			) PURE;

		STDMETHOD(put_IPEffect) (THIS_
			int effectNum,          // Change to this effect
			REFTIME StartTime,      // Start time of effect
			REFTIME Length,          // Length of effect
			bool bChanged
			) PURE;
		STDMETHOD(put_ImageAttribute) (THIS_
			long nOldWidth,          
			long nOldHeight,  
			long nNewWidth,
			long nNewHeight,
			long nImageType,
			bool bKeepRatio,
			bool bGraylevel,
			long nNamedType,
			TCHAR* szPath
			) PURE;
		STDMETHOD(get_ImageAttribute) (THIS_
			long* nOldWidth,          
			long* nOldHeight,  
			long* nNewWidth,
			long* nNewHeight,
			long* nImageType,
			bool* bKeepRatio,
			bool* bGraylevel,
			long* nNamedType,
			TCHAR* szPath
			) PURE;
	};

#ifdef __cplusplus
}
#endif

#endif // __IEZ__

