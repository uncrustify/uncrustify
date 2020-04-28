uint16_t testing(test_t tests)
{

    uint16_t rslt = 0;


    #if (TEST_1 == TEST_2)
        rslt = 1;
    #elif (TEST_1 == TEST_3)
        rslt = 2;
        #if (TEST_1 == TEST_2)
            rslt = 1;
        #elif (TEST_1 == TEST_3)
            rslt = 2;
            #if (TEST_1 == TEST_2)
                rslt = 1;
            #elif (TEST_1 == TEST_3)
                rslt = 2;
            #endif
        #endif
    #endif


    switch (tests)
    {
        #if (TEST_1 == TEST_2)
        case test1:
        {
            rslt = 3;
            break;
        }
        #elif (TEST_1 == TEST_3)
        case test2:
        {
            rslt = 4;
            break;
        }
        #endif

        #if (TEST_2 == TEST_4)
        case test3:
            rslt = 4;
            break;
        #elif (TEST_2 == TEST_5)
        case test3:
            rslt = 4;
            break;
        #endif
    }

    return rslt;
}