// Contents of preprocessor control flow are indented, and Uncrustify (sometimes?) un-indents them.
// Except where the indented thing is a #if, in which case it leaves it. Result is weird.

// We have a lot of issues with #if related indentation. The right fix is probably to have the most recent
// #if/else/def/ndef's indentation level be used as an adjustment for its contents.
// Just keep things relative and no more. And use the location of the 'if' and not the '#' in case it's on col 1 or whatever.

// Important: if the # or if is on col 1, then do no adjustment to the contents. Maybe think about just altering if it's a negative adjustment?

// Or perhaps we should just have Uncrustify totally ignore #if's wrt alignment of contents..

// Platform
#if defined(__APPLE__)
#   if !(defined(__arm__) || defined(__arm64__))
#       define OSX 1
#   endif
#elif (defined(_WIN32) || defined(__WIN32__))
#   define WIN 1
#elif (defined(linux) || defined(__linux__)) && !defined(ANDROID) \
    && !defined(STV) && !defined(TIZEN)
#   define LINUX 1
#endif

#if WIN
    struct Foo;
#endif
