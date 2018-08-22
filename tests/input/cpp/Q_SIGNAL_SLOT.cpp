bool AkonadiServer :: init ()
{
    connect ( watcher , SIGNAL( serviceOwnerChanged ( QString,  QString,QString ) ),
            this, SLOT(serviceOwnerChanged(QString,QString, QString)));
    return true;
}

connect(&mapper, SIGNAL(mapped(Q1 &)), this, SLOT(onSomeEvent(const Q2 &)));

connect(&mapper,
      SIGNAL(mapped(Q1 &)),
      this,
      SLOT(onSomeEvent(const Q2 &)));

connect(&mapper,
      SIGNAL(emitted(Q1 *)),
      this,
      SLOT(accept(const Q2 *)));

connect(&mapper,
      SIGNAL(emitted(X< int >)),
      this,
      SLOT(accept(X< int >)));
