#ifndef AKONADISERVER_H
#define AKONADISERVER_H

#include <QtCore/QPointer>
#include <QtCore/QVector>

#include <QtNetwork/QLocalServer>

class QProcess;

namespace Akonadi {
namespace Server {

class AkonadiServer : public QLocalServer
{
    Q_OBJECT

public:
    ~AkonadiServer();
    static AkonadiServer *instance();
};

} // namespace Server
} // namespace Akonadi
#endif
