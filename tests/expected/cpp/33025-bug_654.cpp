A( b, c, d);
connect(&mapper, SIGNAL(mapped(Q1&)), this, SLOT(onSomeEvent(const Q2&)));
connect(&mapper,
        SIGNAL(mapped(Q1&)),
        this,
        SLOT(onSomeEvent(const Q2&)));
A( b, c, d);
