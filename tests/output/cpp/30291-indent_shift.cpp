// We want simple 4-space indentation for each nesting "level".

// cannot find a way to tell uncrustify to indent the line with parenthesis
int case2() {

    std::out <<
        "hello " << "world " <<
        (who ? "and " : "or ") <<
        "all " <<
        "others" << ";" << std::endl;

    // and

    std::out <<
        "hello " << "world " <<
        ("and ") <<
        "all " <<
        "others" << ";" << std::endl;
}


// uncrustify aligns (with the << on the first line) instead of indenting
void case3()
{

    std::out << "hello "
        << "world "
        << (who ? "and " : "or ")
        << "all "
        << "others" << ";" << std::endl;

    // this often works better, but has problems with parentheses:

    std::out << "hello " <<
        "world " <<
        (who ? "and " : "or ") <<
        "all " <<
        "others" << ";" << std::endl;
}

// uncrustify does not indent >> at all!
void case4()
{
    std::in >> a
        >> b
        >> (who ? c : d) >>
        >> e;

    // and

    std::in >> a >>
        b >>
        (who ? c : d) >>
        e;
}
