// The examples are below exceptions for conditionally compiling
// an entire function definition
#if ( USE_AWESOME_FUNCTIONS )
//! Awesome function that does things
void MyClass::SomeAwesomeFunction()
{
    DoSomethingInAFunction();
}
#endif