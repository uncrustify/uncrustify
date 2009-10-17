CFoo::CFoo(const DWORD something, const RECT& positionRect, const UINT aNumber, bool thisIsReadOnly, const CString& windowTitle, CInfo *pStructInfo, int widthOfSomething) : CSuperFoo(something, positionRect, aNumber, 
thisIsReadOnly, windowTitle), m_pInfo(pInfo), m_width(widthOfSomething)
{
}
