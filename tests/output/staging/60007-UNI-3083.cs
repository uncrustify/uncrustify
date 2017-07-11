class ClassWithCtorICall
{
    public ClassWithCtorICall()
    {
        DoICall();
    }

    //It shouldn't add an extra space before 0x1000
    [MethodImpl((MethodImplOptions)0x1000)]
    static extern void DoICall();
}
