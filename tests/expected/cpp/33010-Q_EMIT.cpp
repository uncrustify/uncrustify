bool Handler::failureResponse(const QByteArray &failureMessage)
{
    response.setString(failureMessage);
    Q_EMIT responseAvailable(response);
}
