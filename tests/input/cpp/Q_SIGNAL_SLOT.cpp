bool AkonadiServer :: init ()
{
    connect ( watcher , SIGNAL( serviceOwnerChanged ( QString,  QString,QString ) ),
            this, SLOT(serviceOwnerChanged(QString,QString, QString)));
    return true;
}
