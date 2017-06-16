#ifndef LOCAL_CLASS_HPP
#define LOCAL_CLASS_HPP

#include "MyClass.hpp"
#if ( USE_INCLUDE )
    #include "conditional.hpp"
#endif

class LocalClass
{
public:
    LocalClass();

    void DoAlways();

    #if ( IS_SOMETIMES )
        void DoSometimes();
        void DoUsually();
        void DoEverySoOften();
    #endif

private:
    int mAlways;        //!< This is always included
    #if ( IS_SOMETIMES )
        int mSometimes; //!< Sometimes this is included
    #endif
};


//! Create the classiest class
MyClass::MyClass()
    : mMember1
    #if ( USE_MEMBER )
        , mConditionalMember
    #endif
{
    if( isSomething )
    {
        DoSomething();
    }

    #if ( USE_FIVE )
    {
        DoSomethingAlso();
    }
    #endif

    #if ( USE_SIX )
    {
        Six mySix;
        DoSomethingWithSix( mySix );
    }
    #endif
}

// The examples are below exceptions for conditionally compiling
// an entire function definition
#if ( USE_AWESOME_FUNCTIONS )
//! Awesome function that does things
void MyClass::SomeAwesomeFunction()
{
    DoSomethingInAFunction();
}
#endif

#if ( USE_AWESOME_FUNCTIONS )
//! Even more awesome function that does things
void MyClass::SomeEvenMoreAwesomeFunction()
{
    DoSomethingInAFunction();
}
#endif

//example for case in a switch statement
switch( ... )
{
case 1:
case 2:
    {
        int v;
        ...
    }
    break;

#if ( USE_FIVE )
case 3:
    doFive();
    break;
#endif

default:
    break;
}

// Example for extern "C" block
#ifdef __cplusplus
extern "C" {

void some_c_function
(
    void
);

}
#endif

#endif // end of LOCAL_CLASS_HPP