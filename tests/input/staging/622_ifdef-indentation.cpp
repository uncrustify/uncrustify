f()
{
    {
        {
            {
#               if 1
                return 0;
#               endif

                #if 1
                return 0;
                #endif
            }
        }
    }
}
