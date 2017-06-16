//! Create the classiest class
MyClass::MyClass()
    : mMember1
    #if( USE_MEMBER )
        , mConditionalMember
    #endif
{
    if( isSomething )
    {
        DoSomething();
    }

    #if( USE_FIVE )
        {
        DoSomethingAlso();
        }
    #endif

    #if( USE_SIX )
        {
        Six mySix;
        DoSomethingWithSix( mySix );
        }
    #endif
}