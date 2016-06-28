void foo()
{
    QObject::connect(m_NetworkReply,
        SIGNAL(error(QNetworkReply::NetworkError)),
        this,
        SLOT(NetworkReplyError(QNetworkReply::NetworkError)));
    QObject::connect(m_NetworkReply,
        SIGNAL(uploadProgress(qint64, qint64)),
        this,
        SLOT(NetworkReplyUploadProgress(qint64, qint64)));
    connect(&m_SendReportThread, SIGNAL(ProgressChanged(size_t, size_t)),
        SLOT(OnReportProgressChanged(size_t, size_t)));
}
