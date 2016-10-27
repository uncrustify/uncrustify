// all the forms of strings
contents.Append ( "#include \"{file.GetBoundPath ()}\"" );
contents.Append ( $"#include \"{file.GetBoundPath ()}\"" );
contents.Append ( $@"#include
""{file.GetBoundPath ()}""" );
contents.Append ( @"#include
""{file.GetBoundPath ()}""" );
