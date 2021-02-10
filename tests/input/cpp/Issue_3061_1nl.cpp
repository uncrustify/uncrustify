DCOPClient::DCOPClient()
{
	TQObject::connect(&d->postMessageTimer, TQT_SIGNAL(timeout()), this,
	        TQT_SLOT(processPostedMessagesInternal()));
	TQObject::connect(&d->eventLoopTimer, TQT_SIGNAL(timeout()), this, TQT_SLOT(eventLoopTimeout()));
}

#include <dcopclient.moc>
