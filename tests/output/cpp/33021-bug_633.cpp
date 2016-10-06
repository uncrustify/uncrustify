typedef void (*func)();
typedef void (__stdcall *func)();

class CDataObject : public IDataObject
{
public:
    // IUnknown members
    HRESULT __stdcall QueryInterface(REFIID iid, void ** ppvObject);
    ULONG __stdcall AddRef(void);
    ULONG __stdcall Release(void);

    // IDataObject members
    HRESULT __stdcall GetData(FORMATETC *pFormatEtc, STGMEDIUM *pmedium);
    HRESULT __stdcall GetDataHere(FORMATETC *pFormatEtc, STGMEDIUM *pmedium);
    HRESULT __stdcall QueryGetData(FORMATETC *pFormatEtc);
    HRESULT __stdcall GetCanonicalFormatEtc(FORMATETC *pFormatEct,  FORMATETC *pFormatEtcOut);
    HRESULT __stdcall SetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium, BOOL fRelease);
    HRESULT __stdcall EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc);
    HRESULT __stdcall DAdvise(FORMATETC *pFormatEtc, DWORD advf, IAdviseSink *, DWORD *);
    HRESULT __stdcall DUnadvise(DWORD dwConnection);
    HRESULT __stdcall EnumDAdvise(IEnumSTATDATA **ppEnumAdvise);

    // exercise others
    HRESULT __cdecl GetData(FORMATETC *pFormatEtc, STGMEDIUM *pmedium);
    HRESULT __clrcall GetData(FORMATETC *pFormatEtc, STGMEDIUM *pmedium);
    HRESULT __fastcall GetData(FORMATETC *pFormatEtc, STGMEDIUM *pmedium);
    HRESULT __thiscall GetData(FORMATETC *pFormatEtc, STGMEDIUM *pmedium);
    HRESULT __vectorcall GetData(FORMATETC *pFormatEtc, STGMEDIUM *pmedium);
}
