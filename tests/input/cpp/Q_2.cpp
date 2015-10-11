bool AkonadiServer::quit()
{
    QTimer::singleShot(   0, this, SLOT   (   doQuit(   )   )   );
}

void AkonadiServer::incomingConnection(quintptr socketDescriptor)
{
    QPointer<ConnectionThread> thread = new ConnectionThread(socketDescriptor, this);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
}
