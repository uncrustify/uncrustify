void foo()
{
    int error = 0;
#if defined(SUPPORT_FEATURE)
    error = feature_bar();
#else // feature not supported
    // we call bar otherwise
    error = bar();
#endif // SUPPORT_FEATURE
    // continue with function logic
    if (error != 0)
    {
#if 0 // TODO: this is disabled
        // call final bar
        error_bar(error);
#endif
    }
}
