#include "serverstreamer.h"
#include <QTime>
#include <iostream>
#include <QtTest/QTest>

ServerStreamer::ServerStreamer(QObject *parent) : QObject(parent) {}

void ServerStreamer::init()
{
    //serverAddress = QHostAddress::LocalHost;
    //serverUdpPort = 1234; // unneeded?
    serverTcpPort = 6666;

    serverUdpSocket = new QUdpSocket(this); // TODO: mogoc most odpret se incoming, pa pol na clientu connecttohost da bo po netu delal!!!! virtualna dvosmerna povezava

    // TODO: setup from main window.. all of it.
    // also, tole je za INCOMING udp packets ane?

    //socket->bind(QHostAddress("193.2.178.92"), 1234);
    //serverUdpSocket->bind(serverAddress, serverUdpPort);
    //socket->bind(QHostAddress("109.182.180.107"), 1234);

    // TDODOOOOOOOOOOO: server: bind? connectToHost? / client: connectToHost? udp... to fool the routers

    tcpServer = new QTcpServer(this);

    if (!tcpServer->listen(serverAddress, serverTcpPort)){
        qDebug () << tcpServer->errorString();
        tcpServer->close();
        return;
    }

    connect(tcpServer, &QTcpServer::newConnection, this, &ServerStreamer::clientConnected);
}

void ServerStreamer::startStream(QString ip, bool chain_streaming)
{
    serverAddress = QHostAddress(ip);
    init();

    player = new Player;
    connect(player->source, &AudioSource::dataReady, this, &ServerStreamer::write);
}

void ServerStreamer::clientConnected() // TODO: close all connections when...?
{
    qint16 blockSize = 0;

    QTcpSocket *clientConnection = tcpServer->nextPendingConnection();
    qDebug() << "Client ID: " << clientConnection->socketDescriptor();

    // TODO: a ce tole gre cez se pol klice readyread pa gre v uno metodo takoj pred ostalimi povezavami? kr sele tm se pol connecta zares naj!
    if(clientConnection->waitForReadyRead()) // TODO: spremen zarad windowsov! see documentation. verjetn bi jih blo za v queue dat pa pol spodi processat...
    {                                        // TODO: tole queue. za test das kle 1, pa sleep v clienta in se ne connecta! potestiraj k bo connect queue!
        QDataStream in(clientConnection);    // TODO: NARED RECIEVE METODO ZA SERVER!
        in.setVersion(QDataStream::Qt_5_0);

        // TODO: pri testiranju mas kle umes un blockSize...

        /*if(blockSize == 0)
        {
            qDebug() << "block size = 0";
            if(clientConnection->bytesAvailable() < (int)sizeof(quint16)) return;
            in >> blockSize;
        }
        qDebug() << "Why am I here multiple times?";
        if(clientConnection->bytesAvailable() < blockSize) return;
        blockSize = 0;*/

        in >> blockSize; // TODO: read the above

        qint16 clientPort;
        in >> clientPort;

        if (QString::number(clientPort).isEmpty())
        {
            std::cerr << "Client data took too long to arrive!" << std::endl;
            return;
        }

        qDebug() << "Client sent its info. UDP port: " + QString::number(clientPort);
        clients << new Common::ClientInfo(clientConnection, clientConnection->localAddress(), clientPort);

        //serverUdpSocket->connectToHost(clientConnection->localAddress(), clientPort);
        // We have obtained sub-socket, connection has been established

        // TODO: tole vse samo ce se connecta!!
        connect(clientConnection, &QAbstractSocket::disconnected, clientConnection, &QAbstractSocket::deleteLater);
        connect(clientConnection, &QAbstractSocket::disconnected, this, &ServerStreamer::clientDisconnected); // TODO: a je treba to pol disconnectat?

        emit clientCountChanged(clients.length());
    }
}

void ServerStreamer::clientDisconnected()
{
    QMutableVectorIterator<Common::ClientInfo *> iterator(clients);

    while(iterator.hasNext())
    {
        auto c = iterator.next();
        qDebug() << "DC:" << c->connection->socketDescriptor() << c->connection->isOpen() << c->connection->state();

        if (c->connection->state() == QAbstractSocket::UnconnectedState)
        {
            c->connection->close();
            iterator.remove();
        }
        // TODO: kle nardis prevezavo povezav med klienti in dolocs od kje kdo posilja, k en linked list? mogoce se kr to res ponuca...
    }

    emit clientCountChanged(clients.length());
}

void ServerStreamer::write(QVector<QByteArray> data)
{
    //qDebug() << "Writing to clients!";

    foreach (auto chunk, data)
    {
        foreach(auto *c, clients){
            quint64 bytes_sent = serverUdpSocket->writeDatagram(chunk, chunk.size(), c->address, c->port);
            //qDebug() << "Bytes sent:" << bytes_sent;

            if(bytes_sent == -1)
                qDebug() << "CHUNK TOO BIG! (ServerStreamer::write)";
        }
    }
}

void ServerStreamer::sendMessage(QVector<Common::ClientInfo *> dsts) // TODO: args: paket, recipient vector, default = all
{
    QByteArray block; // for temporarily storing the data to be sent
    QDataStream out(&block, QIODevice::WriteOnly);

    // Use the data stream to write data

    out.setVersion (QDataStream::Qt_5_0);

    // Set the data stream version, the client and server side use the same version

    out << (quint16)0;
    block.append(Common::MessageCommand(QString("Ping! Time: %1").arg(QTime::currentTime().toString())).serialize());
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));

    foreach (Common::ClientInfo *c, dsts.isEmpty() ? clients : dsts)
        c->connection->write(block);

    qDebug() << "Server: Ping message sent.";
}

void ServerStreamer::addClient(Common::ClientInfo *c){
    clients << c;
}
