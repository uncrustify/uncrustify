// * detecting the * as a ARITH because MIDL_INTERFACE includes 'class' in its definition.
// adding a 'class' before IFileDialogEvents 'resolves' the issue.

EXTERN_C const IID IID_IFileDialogEvents;

MIDL_INTERFACE("973510db-7d7f-452b-8975-74a85828d354")
IFileDialogEvents : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE OnStuff(
		/* [in] */ __RPC__in_opt IFileDialog *pfd,
		/* [in] */ __RPC__in_opt IShellItem *psi,
		/* [out] */ __RPC__out FDE_SHAREVIOLATION_RESPONSE *pGoodResponse,
		/* [out] */ __RPC__out FDE_OVERWRITE_RESPONSE *pBadResponse) = 0;
};
