// Test if Uncrustify properly handles `@synchronized` keyword for ObjC

// In keywords.cpp there is no @synchronized keyword listed and from what I've seen synchronized is only regarded as a keyword in other languages
// { "synchronized", CT_QUALIFIER, LANG_D | LANG_JAVA | LANG_ECMA },

- (void)foo
{
    @synchronized(self)
    {
        if (bar)
        {
            bar = false;
        }
    }
}
