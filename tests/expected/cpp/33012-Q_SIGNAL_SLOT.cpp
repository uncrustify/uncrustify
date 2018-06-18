bool AkonadiServer::init()
{
    connect(watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            this, SLOT(serviceOwnerChanged(QString,QString,QString)));
    return true;
}

connect(&mapper, SIGNAL(mapped(Q1&)), this, SLOT(onSomeEvent(const Q2&)));

connect(&mapper,
        SIGNAL(mapped(Q1&)),
        this,
        SLOT(onSomeEvent(const Q2&)));

connect(&mapper,
        SIGNAL(mapped(Q1&)),
        this,
        SLOT(onSomeEvent(const Q2&)));
