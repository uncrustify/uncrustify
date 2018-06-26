// We want simple 4-space indentation for each nesting "level".

// cannot find a way to tell uncrustify to indent the line with parenthesis
int case2() {

    if (condition) {
	// some code here
    }

    std::out <<
        "hello " << "world " <<
        (who ? "and " : "or ") <<
        "all " <<
        "others" << ";" << std::endl;

    // and

    if (condition) {
	// some code here
    }

    std::out <<
        "hello " << "world " <<
        ("and ") <<
        "all " <<
        "others" << ";" << std::endl;

    if (cond)
	std::out << "hi";

    if (cond)
	std::out
	    << "hi"
	    << "and"
	    << "more"
	    ;

    switch (var) {
    case 0:
	log() << 5
	    << 5;
	break;
    }

#if 0
    out
        << 5;
#endif

    return log
        >> var
        >> second
        ;
}


// uncrustify aligns (with the << on the first line) instead of indenting
void case3()
{

    if (condition1) {

	if (condition2) {

	    std::out << "hello "
	        << "world "
	        << (who ? "and " : "or ")
	        << "all "
	        << "others" << ";" << std::endl;

	}
    }

    // this often works better, but has problems with parentheses:

    if (condition1) {
	if (condition2) {
	    std::out << "hello " <<
	        "world " <<
	        (who ? "and " : "or ") <<
	        "all " <<
	        "others" << ";" << std::endl;
	}
    }
}

// uncrustify does not indent >> at all!
void case4()
{
    if (condition) {
	// some code here
    }

    std::in >> a
        >> b
        >> (who ? c : d) >>
        >> e;

    // and

    if (condition1) {

	if (condition2) {
	    std::in >> a >>
	        b >>
	        (who ? c : d) >>
	        e;
	}
    }
}

void foo() {

    if (head())
	os << "HEAD,";
    else
    if (tail())
	os << "TAIL,";

    if (a >= 0 &&
        b <= 0)
	cerr << "it is";
}

int list[] = {
    1,
    2,
    1 << 5,
    1 << 6
};

void check() {
    ostream &os = Comment(1) << "error: " << workerName <<
        " terminated by signal " << WTERMSIG(exitStatus);

    return theAddr.addrN().family() == AF_INET6 ?
        (theAddr.octet(idx * 2) << 8) + theAddr.octet(idx * 2 + 1) :
        theAddr.octet(idx);
}
