#include "serverstreamer.h"


ServerStreamer::ServerStreamer(QObject *parent) : QObject(parent)
{
    socket = new QUdpSocket(this);
    socket->bind(QHostAddress::LocalHost, 1234);

    tcpServer = new QTcpServer(this);

    if (! tcpServer-> listen (QHostAddress :: LocalHost, 6666)){
        // Monitor port 6666 of the local host, if the error output an error message and close the
        qDebug () << tcpServer-> errorString ();
        return;
    }

    connect (tcpServer, SIGNAL (newConnection ()), this, SLOT (sendMessage ()));

    /*QNetworkConfigurationManager manager;
    if (manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired) {
       // Get saved network configuration
       QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
       settings.beginGroup(QLatin1String("QtNetwork"));
       const QString id = settings.value(QLatin1String("DefaultNetworkConfiguration")).toString();
       settings.endGroup();

       // If the saved network configuration is not currently discovered use the system default
       QNetworkConfiguration config = manager.configurationFromIdentifier(id);
       if ((config.state() & QNetworkConfiguration::Discovered) !=
           QNetworkConfiguration::Discovered) {
           config = manager.defaultConfiguration();
       }

       networkSession = new QNetworkSession(config, this);
       connect(networkSession, SIGNAL(opened()), this, SLOT(sessionOpened()));

       qDebug() << "Opening network session.";
       networkSession->open();
       } else {
           sessionOpened();
       }

       fortunes << tr("You've been leading a dog's life. Stay off the furniture.")
                << tr("You've got to think about tomorrow.")
                << tr("You will be surprised by a loud noise.")
                << tr("You will feel hungry again in another hour.")
                << tr("You might have mail.")
                << tr("You cannot kill time without injuring eternity.")
                << tr("Computers are not intelligent. They only think they are.");

       connect(tcpServer, SIGNAL(newConnection()), this, SLOT(sendFortune()));*/

}


void ServerStreamer::write(QByteArray data)
{
    socket->writeDatagram(data, QHostAddress::LocalHost, 1233);
}


void ServerStreamer::sendMessage()
{
    QByteArray block; // for temporarily storing the data to be sent

    QDataStream out (& block, QIODevice :: WriteOnly);

    // Use the data stream to write data

    out.setVersion (QDataStream :: Qt_5_0);

    // Set the data stream version, the client and server side use the same version

    out << 2;

    out << tr ("hello TCP!!!");

    out.device () -> seek (0);

    out << (quint16) (block.size () - sizeof (quint16));

    QTcpSocket * clientConnection = tcpServer-> nextPendingConnection ();

    // We have obtained sub-socket connection has been established

    connect(clientConnection, SIGNAL(disconnected()), clientConnection, SLOT(deleteLater()));

    clientConnection-> write (block);

    clientConnection-> disconnectFromHost ();

    qDebug() << "send message successful!!!";

    // Send data successfully, the display prompts
}

/*void ServerStreamer::sessionOpened()
{
    // Save the used configuration
    if (networkSession) {
        QNetworkConfiguration config = networkSession->configuration();
        QString id;
        if (config.type() == QNetworkConfiguration::UserChoice)
            id = networkSession->sessionProperty(QLatin1String("UserChoiceConfiguration")).toString();
        else
            id = config.identifier();

        QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
        settings.beginGroup(QLatin1String("QtNetwork"));
        settings.setValue(QLatin1String("DefaultNetworkConfiguration"), id);
        settings.endGroup();
    }

    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen()) {
        qDebug() << "Unable to start the server: " + tcpServer->errorString();
        tcpServer->close();
        return;
    }
    QString ipAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
            ipAddressesList.at(i).toIPv4Address()) {
            ipAddress = ipAddressesList.at(i).toString();
            break;
        }
    }
    // if we did not find one, use IPv4 localhost
    if (ipAddress.isEmpty())
        ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
    qDebug() << "Server is runing on " + ipAddress + " " + tcpServer->serverPort();

}*/

/*void ServerStreamer::sendFortune()
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << (quint16)0;
    out << fortunes.at(qrand() % fortunes.size());
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));

    QTcpSocket *clientConnection = tcpServer->nextPendingConnection();
    connect(clientConnection, SIGNAL(disconnected()),
            clientConnection, SLOT(deleteLater()));

    clientConnection->write(block);
    clientConnection->disconnectFromHost();
}*/

