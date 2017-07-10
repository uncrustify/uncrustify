// Pointer mark should be formatted (WINAPI* SetXX)
typedef DWORD (WINAPI *SetDllDirectory) (LPCSTR);
// Pointer mark should be formatted (EXCEPTION_POINTERS* pExt)
static LONG WINAPI CustomUnhandledExceptionFilter(EXCEPTION_POINTERS * pExInfo)
{
    if (EXCEPTION_BREAKPOINT == pExInfo->ExceptionRecord->ExceptionCode) // Breakpoint. Don't treat this as a normal crash.
        return EXCEPTION_CONTINUE_SEARCH;
}