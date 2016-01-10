#include "streamreceiver.h"
#include "common.h"
#include <QAudioOutput>
#include <QtTest/QTest>

StreamReceiver::StreamReceiver(QObject *parent) : QObject(parent)
{
}

void StreamReceiver::init()
{
    /**** UDP SETUP ****/

    //serverAddress = QHostAddress("193.2.178.92");
    //serverAddress = QHostAddress("109.182.180.107");
    //serverAddress = QHostAddress(server_ip);
    //serverUdpPort = 1234; // TODO: TOLE POSLE SERVER NAZAJ CLIENTU PO MESSAGU PO USPESNI POVEZAVI!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    serverTcpPort = 6666;

    //clientAddress = QHostAddress(client_ip); // TODO: a se da to avtomaticn potegnt vn... oz naj user vpise ob connectanju!
    clientUdpPort = 1233;

    clientUdpSocket = new QUdpSocket(this);

    connect(clientUdpSocket, &QIODevice::readyRead, this, &StreamReceiver::dataReceived);

    qDebug() << "UDP bind successful:" << clientUdpSocket->bind(QHostAddress::Any, clientUdpPort); // TODO: bind to ANY
    //clientUdpSocket->connectToHost(serverAddress, serverUdpPort, QIODevice::ReadWrite); // TODO: maybe fools the routers. may be necessary to connect on the server side too


    /**** TCP SETUP ****/ // TODO: dej vse v connect tko da k user stisne lahk se vpise prej ip pa to...

    clientTcpSocket = new QTcpSocket(this);

    connect(clientTcpSocket, &QIODevice::readyRead, this, &StreamReceiver::readCommand);
    connect(clientTcpSocket, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error), this, &StreamReceiver::displayError);

    /**** AUDIO SETUP ****/

    //playbuff = new QBuffer();
    //playbuff->open(QBuffer::ReadWrite);

    auto *audio = new QAudioOutput(Common::getFormat(), this);
    audio->setBufferSize(1024*100);
    playbuff = audio->start(); // TODO: WAT????
}
/*
QString getIPAddress()
{
    QString ipAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
            ipAddressesList.at(i).toIPv4Address()) {
            ipAddress = ipAddressesList.at(i).toString();
            break;
        }
    }
    if (ipAddress.isEmpty())
        ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
    return ipAddress;
}
*/
void StreamReceiver::newConnect(QString server_ip, QString client_ip)
{
    serverAddress = QHostAddress(server_ip);
    //clientAddress = QHostAddress(client_ip); // UNNEEDED

    blockSize = 0;

    clientTcpSocket->abort();
    clientTcpSocket->connectToHost(serverAddress, serverTcpPort);

    emit(activityLogChanged("Establishing connection to server at " + serverAddress.toString() + " on port " + QString::number(serverTcpPort)));

    if(clientTcpSocket->waitForConnected()) // TODO: fails on windows... correct it
    {
        QByteArray block; // for temporarily storing the data to be sent
        QDataStream out(&block, QIODevice::WriteOnly);

        // Use the data stream to write data

        out.setVersion(QDataStream::Qt_5_0);

        // Set the data stream version, the client and server side use the same version

        out << (quint16)0;
        //out << tr ("I'm a client" + clientPort );
        out << clientUdpPort;
        out.device()->seek(0);
        out << (quint16)(block.size() - sizeof(quint16));

        //QTest::qSleep (2000);

        clientTcpSocket->write(block);

        // TODO: REMOVE TEST
        clientTcpSocket->flush();
        clientTcpSocket->waitForBytesWritten(3000);

        emit(connectionStatusChanged("Connected to server"));
        emit(activityLogChanged("Connected to server at " + serverAddress.toString() + " on port " + QString::number(serverTcpPort)));
    }
    else
    {
        emit(connectionStatusChanged("Unsuccessful connection on " + serverAddress.toString() + ":" + QString::number(serverTcpPort)));
    }
}

void StreamReceiver::readCommand()
{
    QDataStream in(clientTcpSocket);
    in.setVersion(QDataStream::Qt_5_0);

    if(blockSize == 0){
        if(clientTcpSocket->bytesAvailable() < (int)sizeof(quint16)) return;
        in >> blockSize;
    }

    if(clientTcpSocket->bytesAvailable() < blockSize) return;
    blockSize = 0;

    quint8 cid;
    in >> cid;

    switch (cid)
    {
        case Common::CommandID::MESSAGE:
            readMessage(clientTcpSocket->readAll());
            break;

        case Common::CommandID::STREAM:
            updateDestinations(clientTcpSocket->readAll());
            break;

        default: qDebug() << "Command ID incorrect!";
    }
}

void StreamReceiver::readMessage(const QByteArray &data)
{
    QString message = (Common::MessageCommand().deserialize(data))->message;
    qDebug() << "Message (readMessage()): " + message;
    //qDebug() << "Pending UDP data size:" << clientUdpSocket->pendingDatagramSize();
    emit(messageChanged(message));
}

void StreamReceiver::updateDestinations(const QByteArray &data)
{
    auto stream_command = Common::StreamCommand();
    stream_command.deserialize(data); // TODOOOOO: A JE KLE POINTER???
    qDebug() << "Stream command:" << stream_command.address << stream_command.port << stream_command.reset_destinations;

    if (stream_command.reset_destinations)
    {
        clients.clear();
    }
    if (stream_command.address != QString("127.0.0.1") && stream_command.port != 0) // POPRAVI!
    {
        auto *new_client = new Common::ClientInfo(QHostAddress(stream_command.address), stream_command.port);
        clients.append(new_client);
    }
    else if (stream_command.address == QString(""))
        qDebug() << "Client chain: QHostAddress() -> this shouldn't happen!";
}

void StreamReceiver::dataReceived()
{
    //qDebug() << "New UDP data ready!";

    QByteArray buffer; // TODO: save packets (according to the slowest recipient), send from x packet on, not from current one
    QHostAddress server;
    quint16 serverPort;

    while(clientUdpSocket->hasPendingDatagrams())
    {
        buffer.resize(clientUdpSocket->pendingDatagramSize());
        clientUdpSocket->readDatagram(buffer.data(), buffer.size(), &server, &serverPort); // TODO: ce mas connecttohost mors READ od IO uporabt!

        playbuff->write(buffer.data(), buffer.size());

        // Pass to clients
        foreach(auto *c, clients)
        {
            quint64 bytes_sent = clientUdpSocket->writeDatagram(buffer, c->address, c->port);
            //qDebug() << "Bytes sent:" << bytes_sent; // TODO: TEST IT!

            if(bytes_sent == -1)
                qDebug() << "CHUNK TOO BIG! (Client pass down the chain.)";
        }

        //qDebug() << "Message (readyRead()):" << buffer.size() << QTime::currentTime(); // TODO: zakaj so umes nicni paketi?
    }
}

void StreamReceiver::displayError(QAbstractSocket::SocketError socketError)
{
    //qDebug() << tcpSocket->errorString (); // output an error message
    QString connStatus = "";
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        //connStatus = "RemoteHostClosedError";
        connStatus = "Remote Host Closed"; // TODO: pogrunti zakaj se to zgodi ce se odpre server pred clientom
        break;
    case QAbstractSocket::HostNotFoundError:
        connStatus = "The host was not found. Please check the host name and port settings.";
        break;
    case QAbstractSocket::ConnectionRefusedError:
        connStatus = "The connection was refused by the peer. Make sure the server is running,"
                     "and check that the host name and port settings are correct.";
        break;
    default:
        connStatus = "The following error occurred: " + clientTcpSocket->errorString();
    }
    qDebug() << connStatus;
    emit(connectionStatusChanged(connStatus));
    emit(activityLogChanged("Connection closed " + connStatus ));
    emit(connectButtonToggle(true));

}

void StreamReceiver::addClient(Common::ClientInfo *c){
    clients << c;
}
