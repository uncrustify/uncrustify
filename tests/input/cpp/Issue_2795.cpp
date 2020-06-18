void SnRequestTracefork::onCurlTestError(QProcess::ProcessError _error) {
    myerror(QString("Curl process failed with error %1").arg(_error));
}
