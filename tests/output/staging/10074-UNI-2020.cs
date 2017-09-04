// This has to do with the @" that opens a literal string getting aligned when we don't want it to.
// Test code:

void Func()
{
    OtherFunc(
@"multi
line");
}

// ...becomes:

void Func()
{
    OtherFunc(
        @"multi
line");
}

// (This also happens with var x = \n@"" and probably a few other scenarios.)
// There's no way to keep it from indenting the first line of the string literal. We want to leave it at column 1 because it's being used for a "here doc" that doesn't want leading spaces from indentation. It's weird looking for just the first line to be indented.
// This likely requires a new Uncrustify feature to support.
// Workaround: start the @" on the previous line and permit the string to have an extra empty first line, like this:

void Func()
{
    OtherFunc(@"
multi
line");
}

// That might be ok for many cases, especially here-docs that are for script source.
