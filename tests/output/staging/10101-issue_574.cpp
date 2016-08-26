class A
{
// crash (two parameter, 2nd string parameter has space)
    void check(const QObject *object, const QStringList &strList = QStringList(QString(QLatin1String(
                                                                                           "one two"))));
// no crash (two parameter, 2nd string parameter has no space)
    void check(const QObject *object, const QStringList &strList = QStringList(QString(QLatin1String(
                                                                                           "one"))));
// no crash (removed QLatin1String)
    void check(const QObject *object,
               const QStringList &strList = QStringList(QString(("one two"))));
// no crash (removed QString(QLatin1String))
    void check(const QObject *object, const QStringList &strList = QStringList());
// no crash (removed 1st parameter only)
    void check(const QStringList &strList = QStringList(QString(QLatin1String("one two"))));
};
