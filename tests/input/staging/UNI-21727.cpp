void foo()
{
#if defined(SUPPORT_FEATURE)
    bar();
#endif // SUPPORT_FEATURE
    // Handle error
    if (error != 0)
    {
    }
}
