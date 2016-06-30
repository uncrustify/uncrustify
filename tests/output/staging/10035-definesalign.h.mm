    #define SOUNDMANAGERWATCHDOG() \
        SoundManagerWatchDog watchdog

        #define CompileTimeAssert(expression, message) \
            enum{ CT_ASSERT_HACK_JOIN(ct_assert_, __LINE__) = sizeof(CompileTimeAssertImpl<(expression)>) }
