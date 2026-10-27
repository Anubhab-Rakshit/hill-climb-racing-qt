#include "GameServer.h"
#include "AccountManager.h"
#include <QNetworkInterface>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <iostream>

namespace Network {

static const uint32_t s_playerColors[] = {
    0xFFFF3D00, // Host: Neon Red / Orange
    0xFF00E5FF, // Player 2: Electric Cyan
    0xFF76FF03, // Player 3: Neon Lime
    0xFFFFD600, // Player 4: Golden Yellow
    0xFFFF00E5, // Player 5: Electric Magenta
    0xFFFF9100, // Player 6: Sunset Orange
    0xFF00E676, // Player 7: Bright Emerald
    0xFF00B0FF  // Player 8: Deep Sky Blue
};

GameServer::GameServer(QObject* parent)
    : QObject(parent)
{
    m_hostClient.id = 0;
    m_hostClient.name = "HostPlayer";
    m_hostClient.assignedColor = s_playerColors[0];
    m_hostClient.isReady = true;
}

GameServer::~GameServer() {
    stopServer();
}

bool GameServer::startServer(quint16 port) {
    stopServer();
    m_port = port;
    m_tcpServer = new QTcpServer(this);

    if (!m_tcpServer->listen(QHostAddress::AnyIPv4, m_port)) {
        std::cerr << "[GameServer] Failed to bind to port " << m_port << ": "
                  << m_tcpServer->errorString().toStdString() << std::endl;
        delete m_tcpServer;
        m_tcpServer = nullptr;
        return false;
    }

    connect(m_tcpServer, &QTcpServer::newConnection, this, &GameServer::onNewConnection);
    std::cout << "[GameServer] Listening on port " << m_port << " (LAN IP: "
              << getLocalIpAddress().toStdString() << ")" << std::endl;
    return true;
}

void GameServer::stopServer() {
    if (m_tcpServer) {
        for (auto& client : m_clients) {
            if (client.socket) {
                client.socket->disconnectFromHost();
            }
        }
        m_clients.clear();
        m_socketToId.clear();
        m_tcpServer->close();
        delete m_tcpServer;
        m_tcpServer = nullptr;
    }
}

bool GameServer::isRunning() const {
    return m_tcpServer && m_tcpServer->isListening();
}

QString GameServer::getLocalIpAddress() const {
    // 1. Try active non-virtual interfaces (Wi-Fi / Ethernet)
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const auto& iface : interfaces) {
        auto flags = iface.flags();
        if ((flags & QNetworkInterface::IsUp) &&
            (flags & QNetworkInterface::IsRunning) &&
            !(flags & QNetworkInterface::IsLoopBack)) {
            QString name = iface.humanReadableName().toLower();
            bool isVirtual = name.contains("virtual") || name.contains("vbox") ||
                             name.contains("vmware") || name.contains("vethernet") ||
                             name.contains("wintun") || name.contains("loopback");
            if (!isVirtual) {
                for (const auto& entry : iface.addressEntries()) {
                    if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                        return entry.ip().toString();
                    }
                }
            }
        }
    }
    // 2. Fallback to any non-loopback IPv4
    const QList<QHostAddress>& addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& addr : addresses) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol && !addr.isLoopback()) {
            return addr.toString();
        }
    }
    return "127.0.0.1";
}

void GameServer::setHostPlayerInfo(const std::string& name, int vehicleType, int driverType) {
    m_hostClient.name = name;
    m_hostClient.vehicleType = vehicleType;
    m_hostClient.driverType = driverType;
    broadcastLobbyState();
}

void GameServer::setTrack(const std::string& biomeId) {
    m_biomeId = biomeId;
    broadcastLobbyState();
}

uint32_t GameServer::assignColorForId(int id) {
    int idx = std::abs(id) % 8;
    return s_playerColors[idx];
}

void GameServer::onNewConnection() {
    while (m_tcpServer && m_tcpServer->hasPendingConnections()) {
        QTcpSocket* socket = m_tcpServer->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, &GameServer::onClientReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &GameServer::onClientDisconnected);
    }
}

void GameServer::onClientDisconnected() {
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    if (m_socketToId.contains(socket)) {
        int id = m_socketToId[socket];
        m_clients.remove(id);
        m_socketToId.remove(socket);

        if (m_onPlayerDisconnected) {
            m_onPlayerDisconnected(id);
        }

        // Notify remaining clients
        QJsonObject discObj;
        discObj["type"] = static_cast<int>(PacketType::DISCONNECT);
        discObj["playerId"] = id;
        broadcastPacket(QJsonDocument(discObj).toJson(QJsonDocument::Compact));

        broadcastLobbyState();
    }
    socket->deleteLater();
}

void GameServer::onClientReadyRead() {
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    while (socket->canReadLine()) {
        QByteArray line = socket->readLine().trimmed();
        if (!line.isEmpty()) {
            processPacket(socket, line);
        }
    }
}

void GameServer::processPacket(QTcpSocket* socket, const QByteArray& packetData) {
    QJsonDocument doc = QJsonDocument::fromJson(packetData);
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    PacketType type = static_cast<PacketType>(obj["type"].toInt());

    if (type == PacketType::CONNECT_REQ) {
        std::string requestedName = obj["name"].toString().toStdString();
        int vehType = obj["vehicleType"].toInt(0);
        int drvType = obj["driverType"].toInt(0);

        if (requestedName.empty()) requestedName = "Racer";
        std::string finalName = requestedName;

        auto isNameTaken = [&](const std::string& name) {
            if (QString::compare(QString::fromStdString(m_hostClient.name),
                                 QString::fromStdString(name),
                                 Qt::CaseInsensitive) == 0) {
                return true;
            }
            for (const auto& c : m_clients) {
                if (QString::compare(QString::fromStdString(c.name),
                                     QString::fromStdString(name),
                                     Qt::CaseInsensitive) == 0) {
                    return true;
                }
            }
            return false;
        };

        if (isNameTaken(finalName)) {
            int counter = 2;
            while (true) {
                std::string cand = requestedName + "_" + std::to_string(counter);
                if (!isNameTaken(cand)) {
                    finalName = cand;
                    break;
                }
                counter++;
            }
        }

        // Name accepted / auto-disambiguated! Register client
        int newId = m_nextClientId++;
        ConnectedClient client;
        client.id = newId;
        client.socket = socket;
        client.name = finalName;
        client.vehicleType = vehType;
        client.driverType = drvType;
        client.assignedColor = assignColorForId(newId);
        client.isReady = false;

        m_clients[newId] = client;
        m_socketToId[socket] = newId;

        QJsonObject resp;
        resp["type"] = static_cast<int>(PacketType::CONNECT_RESP);
        resp["accepted"] = true;
        resp["playerId"] = newId;
        resp["name"] = QString::fromStdString(finalName);
        resp["assignedColor"] = static_cast<double>(client.assignedColor);
        resp["biomeId"] = QString::fromStdString(m_biomeId);
        sendPacket(socket, QJsonDocument(resp).toJson(QJsonDocument::Compact));

        if (m_onPlayerConnected) {
            m_onPlayerConnected(newId, client.name);
        }

        broadcastLobbyState();
    }
    else if (type == PacketType::PLAYER_READY) {
        int id = obj["playerId"].toInt();
        if (m_clients.contains(id)) {
            m_clients[id].isReady = obj["isReady"].toBool();
            m_clients[id].vehicleType = obj["vehicleType"].toInt(m_clients[id].vehicleType);
            broadcastLobbyState();
        }
    }
    else if (type == PacketType::PLAYER_TELEMETRY) {
        int id = obj["playerId"].toInt();
        RemotePlayerState st;
        st.id = id;
        st.name = obj["name"].toString().toStdString();
        st.color = static_cast<uint32_t>(obj["color"].toDouble(0xFF00E5FF));
        st.vehicleType = obj["vehicleType"].toInt(0);
        st.driverType = obj["driverType"].toInt(0);

        st.posX = static_cast<float>(obj["x"].toDouble());
        st.posY = static_cast<float>(obj["y"].toDouble());
        st.velX = static_cast<float>(obj["vx"].toDouble());
        st.velY = static_cast<float>(obj["vy"].toDouble());
        st.chassisAngle = static_cast<float>(obj["angle"].toDouble());
        st.chassisAngularVel = static_cast<float>(obj["angVel"].toDouble());
        st.targetX = st.posX;
        st.targetY = st.posY;
        st.targetAngle = st.chassisAngle;

        st.rearWheelAngle = static_cast<float>(obj["rearWheelAngle"].toDouble());
        st.frontWheelAngle = static_cast<float>(obj["frontWheelAngle"].toDouble());
        st.rearSuspLen = static_cast<float>(obj["rearSusp"].toDouble(0.2));
        st.frontSuspLen = static_cast<float>(obj["frontSusp"].toDouble(0.2));
        st.driverHeadAngle = static_cast<float>(obj["headAngle"].toDouble());

        st.speedKmh = static_cast<float>(obj["speed"].toDouble());
        st.fuel = static_cast<float>(obj["fuel"].toDouble());
        st.distance = static_cast<float>(obj["dist"].toDouble());
        st.isAirborne = obj["air"].toBool();
        st.isCrashed = obj["crash"].toBool();
        st.isFinished = obj["fin"].toBool();

        if (m_clients.contains(id)) {
            m_clients[id].lastState = st;
        }

        if (m_onTelemetryReceived) {
            m_onTelemetryReceived(st);
        }

        // Rebroadcast to all other clients
        broadcastPacket(packetData, socket);
    }
    else if (type == PacketType::ITEM_COLLECTED) {
        int itemIdx = obj["itemIdx"].toInt();
        int pId = obj["playerId"].toInt();

        if (m_onItemCollected) {
            m_onItemCollected(itemIdx, pId);
        }

        // Rebroadcast so all players see the item collected and removed
        broadcastPacket(packetData, socket);
    }
}

void GameServer::updateHostTelemetry(const RemotePlayerState& hostState) {
    m_hostClient.lastState = hostState;

    QJsonObject obj;
    obj["type"] = static_cast<int>(PacketType::PLAYER_TELEMETRY);
    obj["playerId"] = 0;
    obj["name"] = QString::fromStdString(m_hostClient.name);
    obj["color"] = static_cast<double>(m_hostClient.assignedColor);
    obj["vehicleType"] = hostState.vehicleType;
    obj["driverType"] = hostState.driverType;
    obj["x"] = hostState.posX;
    obj["y"] = hostState.posY;
    obj["vx"] = hostState.velX;
    obj["vy"] = hostState.velY;
    obj["angle"] = hostState.chassisAngle;
    obj["angVel"] = hostState.chassisAngularVel;
    obj["rearWheelAngle"] = hostState.rearWheelAngle;
    obj["frontWheelAngle"] = hostState.frontWheelAngle;
    obj["rearSusp"] = hostState.rearSuspLen;
    obj["frontSusp"] = hostState.frontSuspLen;
    obj["headAngle"] = hostState.driverHeadAngle;
    obj["speed"] = hostState.speedKmh;
    obj["fuel"] = hostState.fuel;
    obj["dist"] = hostState.distance;
    obj["air"] = hostState.isAirborne;
    obj["crash"] = hostState.isCrashed;
    obj["fin"] = hostState.isFinished;

    broadcastPacket(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void GameServer::sendItemCollected(int itemIndex, int collectedByPlayerId) {
    QJsonObject obj;
    obj["type"] = static_cast<int>(PacketType::ITEM_COLLECTED);
    obj["itemIdx"] = itemIndex;
    obj["playerId"] = collectedByPlayerId;
    broadcastPacket(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void GameServer::broadcastStartRace() {
    QJsonObject obj;
    obj["type"] = static_cast<int>(PacketType::RACE_START);
    obj["biomeId"] = QString::fromStdString(m_biomeId);
    broadcastPacket(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void GameServer::broadcastNextRound(const std::string& nextBiome, int roundNumber) {
    m_biomeId = nextBiome;
    QJsonObject obj;
    obj["type"] = static_cast<int>(PacketType::GRAND_PRIX_NEXT_ROUND);
    obj["biomeId"] = QString::fromStdString(nextBiome);
    obj["round"] = roundNumber;
    broadcastPacket(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void GameServer::broadcastLobbyState() {
    QJsonObject root;
    root["type"] = static_cast<int>(PacketType::LOBBY_UPDATE);
    root["biomeId"] = QString::fromStdString(m_biomeId);

    QJsonArray playersArr;

    // 1. Host
    QJsonObject hostObj;
    hostObj["id"] = 0;
    hostObj["name"] = QString::fromStdString(m_hostClient.name);
    hostObj["vehicleType"] = m_hostClient.vehicleType;
    hostObj["driverType"] = m_hostClient.driverType;
    hostObj["isHost"] = true;
    hostObj["isReady"] = true;
    hostObj["color"] = static_cast<double>(m_hostClient.assignedColor);
    playersArr.append(hostObj);

    // 2. Clients
    for (const auto& c : m_clients) {
        QJsonObject cObj;
        cObj["id"] = c.id;
        cObj["name"] = QString::fromStdString(c.name);
        cObj["vehicleType"] = c.vehicleType;
        cObj["driverType"] = c.driverType;
        cObj["isHost"] = false;
        cObj["isReady"] = c.isReady;
        cObj["color"] = static_cast<double>(c.assignedColor);
        playersArr.append(cObj);
    }

    root["players"] = playersArr;
    broadcastPacket(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

void GameServer::broadcastPacket(const QByteArray& packetData, QTcpSocket* excludeSocket) {
    QByteArray payload = packetData + "\n";
    for (const auto& c : m_clients) {
        if (c.socket && c.socket != excludeSocket && c.socket->state() == QAbstractSocket::ConnectedState) {
            c.socket->write(payload);
        }
    }
}

void GameServer::sendPacket(QTcpSocket* destSocket, const QByteArray& packetData) {
    if (destSocket && destSocket->state() == QAbstractSocket::ConnectedState) {
        destSocket->write(packetData + "\n");
    }
}

} // namespace Network
