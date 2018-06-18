// First comment
// Second comment

// First comment
// Second comment

// Issue #1134
class MyClass : public BaseClass
{
    //@{ BaseClass interface
#if VERY_LONG_AND_COMPLICATED_DEFINE
    void foo();
#endif // VERY_LONG_AND_COMPLICATED_DEFINE
    //@}
};

// Issue #1287
void foo()
{
#if defined(SUPPORT_FEATURE)
    bar();
#endif // SUPPORT_FEATURE
    // Handle error
    if (error != 0)
    {
    }

#if defined(SUPPORT_FEATURE)
    bar();
#endif // SUPPORT_FEATURE
    // Handle error
    // Handle error
    if (error != 0)
    {
    }

#   if defined(SUPPORT_FEATURE)
    bar();
#   endif // SUPPORT_FEATURE
          // SUPPORT_FEATURE
    // Handle error
    // Handle error
    if (error != 0)
    {
    }

#   if defined(SUPPORT_FEATURE)
    bar();
#   endif // SUPPORT_FEATURE
          // SUPPORT_FEATURE
    // Handle error
    // Handle error
    if (error != 0)
    {
    }

    #if defined(SUPPORT_FEATURE)
    bar();
    #endif /* SUPPORT_FEATURE
              SUPPORT_FEATURE */
    // Handle error
    // Handle error
    if (error != 0)
    {
    }
}

// ----- Some namespace scope --------------------------------------------------
// ----- FooNamespace scope ----------------------------------------------------
namespace FooNamespace
{
// ----- Some classes scope ----------------------------------------------------
// ----- FooClass scope --------------------------------------------------------
class FooClass
{
    using FooUsing = FooTemplate<
        param1,
        param2
        >; // FooTemplate
    // Foo description
    void foo()
    {
        if (a == b)
        {
            // Col1 comment
            // Col1 comment
            // Col1 comment
            // Baz description
            baz(); // Baz trailing comment begin
                   // Baz trailing comment ...
                   // Baz trailing comment end
        } // if (a == b)
        // Bar description begin
        // Bar description ...
        // Bar description end
        bar(
            a,
            b
            ); // bar trailing comment begin
               // bar trailing comment ...
               // Baz trailing comment end
        /*! Baz description begin
         * Baz description ...
         * Baz description end */
        baz(a,
            b); /* Baz trailing comment begin
                   Baz trailing comment ...
                   Baz trailing comment end */
        // Bar description
        bar(); // bar trailing comment begin
               // bar trailing comment ...
               // Baz trailing comment end

        // Baz description
        baz();
    }
    void bar();
    // Many methods
    void baz();
}; // FooClass
// ----- FooClass scope --------------------------------------------------------

// Many classes
// Many classes
// Many classes

class BazClass
{
    void foo();

    // Many methods
    // Many methods
    // Many methods

    // Overrides
    // Overrides
    //Overrides
protected:
    // Bar description
    void baz();
    //Overrides
}; // BazClass trailing comment begin
   // BazClass trailing comment ...
   // BazClass trailing comment end
// ----- Some classes scope ----------------------------------------------------
} // FooNamespace trailing comment begin
  // FooNamespace trailing comment end
// ----- FooNamespace scope ----------------------------------------------------
// BarNamespace description
namespace BarNamespace
{
} // namespace BarNamespace
// ----- Some namespace scope --------------------------------------------------
