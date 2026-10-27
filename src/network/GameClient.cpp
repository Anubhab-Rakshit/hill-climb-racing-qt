#include "GameClient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace Network {

GameClient::GameClient(QObject* parent)
    : QObject(parent)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &GameClient::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &GameClient::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &GameClient::onSocketReadyRead);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_socket, &QTcpSocket::errorOccurred, this, &GameClient::onSocketError);
#endif
}

GameClient::~GameClient() {
    disconnectFromHost();
}

void GameClient::connectToHost(const QString& hostAddress, quint16 port,
                               const std::string& username, int vehicleType, int driverType) {
    if (m_socket) {
        m_socket->abort();
    }
    disconnectFromHost();
    m_username = username;
    m_vehicleType = vehicleType;
    m_driverType = driverType;

    std::cout << "[GameClient] Connecting to " << hostAddress.toStdString() << ":" << port << " as " << username << "..." << std::endl;
    m_socket->connectToHost(hostAddress, port);
}

void GameClient::disconnectFromHost() {
    if (m_socket && m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
    m_remotePlayers.clear();
    m_localPlayerId = -1;
}

bool GameClient::isConnected() const {
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState && m_localPlayerId >= 0;
}

void GameClient::onSocketConnected() {
    std::cout << "[GameClient] Socket connected! Sending CONNECT_REQ..." << std::endl;

    QJsonObject obj;
    obj["type"] = static_cast<int>(PacketType::CONNECT_REQ);
    obj["name"] = QString::fromStdString(m_username);
    obj["vehicleType"] = m_vehicleType;
    obj["driverType"] = m_driverType;

    sendPacket(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void GameClient::onSocketDisconnected() {
    std::cout << "[GameClient] Socket disconnected." << std::endl;
    m_localPlayerId = -1;
    m_remotePlayers.clear();
    if (m_onDisconnected) {
        m_onDisconnected();
    }
}

void GameClient::onSocketError(QAbstractSocket::SocketError socketError) {
    std::cerr << "[GameClient] Socket error: " << socketError << " - "
              << m_socket->errorString().toStdString() << std::endl;
    if (m_onRejected && m_localPlayerId < 0) {
        m_onRejected(m_socket->errorString().toStdString(), {});
    }
}

void GameClient::onSocketReadyRead() {
    while (m_socket && m_socket->canReadLine()) {
        QByteArray line = m_socket->readLine().trimmed();
        if (!line.isEmpty()) {
            processPacket(line);
        }
    }
}

void GameClient::processPacket(const QByteArray& packetData) {
    QJsonDocument doc = QJsonDocument::fromJson(packetData);
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    PacketType type = static_cast<PacketType>(obj["type"].toInt());

    if (type == PacketType::CONNECT_RESP) {
        bool accepted = obj["accepted"].toBool();
        if (!accepted) {
            std::string reason = obj["error"].toString().toStdString();
            std::vector<std::string> suggestions;
            QJsonArray sugArr = obj["suggestions"].toArray();
            for (const auto& s : sugArr) {
                suggestions.push_back(s.toString().toStdString());
            }

            if (m_onRejected) {
                m_onRejected(reason, suggestions);
            }
            disconnectFromHost();
            return;
        }

        m_localPlayerId = obj["playerId"].toInt();
        if (obj.contains("name")) {
            m_username = obj["name"].toString().toStdString();
        }
        m_assignedColor = static_cast<uint32_t>(obj["assignedColor"].toDouble(0xFF00E5FF));
        m_currentBiomeId = obj["biomeId"].toString("countryside").toStdString();

        std::cout << "[GameClient] Connection accepted! Assigned ID: " << m_localPlayerId
                  << " Color: 0x" << std::hex << m_assignedColor << std::dec << std::endl;

        if (m_onConnected) {
            m_onConnected();
        }
    }
    else if (type == PacketType::LOBBY_UPDATE) {
        m_currentBiomeId = obj["biomeId"].toString("countryside").toStdString();
        std::vector<PlayerLobbyInfo> players;

        QJsonArray arr = obj["players"].toArray();
        for (const auto& val : arr) {
            QJsonObject pObj = val.toObject();
            PlayerLobbyInfo p;
            p.id = pObj["id"].toInt();
            p.name = pObj["name"].toString().toStdString();
            p.vehicleType = pObj["vehicleType"].toInt(0);
            p.driverType = pObj["driverType"].toInt(0);
            p.isHost = pObj["isHost"].toBool();
            p.isReady = pObj["isReady"].toBool();
            p.color = static_cast<uint32_t>(pObj["color"].toDouble(0xFF00E5FF));
            players.push_back(p);
        }

        if (m_onLobbyUpdated) {
            m_onLobbyUpdated(m_currentBiomeId, players);
        }
    }
    else if (type == PacketType::RACE_START) {
        std::string biome = obj["biomeId"].toString().toStdString();
        m_currentBiomeId = biome;
        if (m_onRaceStart) {
            m_onRaceStart(biome);
        }
    }
    else if (type == PacketType::PLAYER_TELEMETRY) {
        int id = obj["playerId"].toInt();
        if (id == m_localPlayerId) return; // Ignore own echoes

        RemotePlayerState st;
        st.id = id;
        st.name = obj["name"].toString().toStdString();
        st.color = static_cast<uint32_t>(obj["color"].toDouble(0xFF00E5FF));
        st.vehicleType = obj["vehicleType"].toInt(0);
        st.driverType = obj["driverType"].toInt(0);

        st.targetX = static_cast<float>(obj["x"].toDouble());
        st.targetY = static_cast<float>(obj["y"].toDouble());
        st.velX = static_cast<float>(obj["vx"].toDouble());
        st.velY = static_cast<float>(obj["vy"].toDouble());
        st.targetAngle = static_cast<float>(obj["angle"].toDouble());
        st.chassisAngularVel = static_cast<float>(obj["angVel"].toDouble());

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

        // If newly seen player, initialize current position immediately
        if (m_remotePlayers.find(id) == m_remotePlayers.end()) {
            st.posX = st.targetX;
            st.posY = st.targetY;
            st.chassisAngle = st.targetAngle;
        } else {
            st.posX = m_remotePlayers[id].posX;
            st.posY = m_remotePlayers[id].posY;
            st.chassisAngle = m_remotePlayers[id].chassisAngle;
        }

        m_remotePlayers[id] = st;

        if (m_onRemoteTelemetry) {
            m_onRemoteTelemetry(st);
        }
    }
    else if (type == PacketType::ITEM_COLLECTED) {
        int itemIdx = obj["itemIdx"].toInt();
        int pId = obj["playerId"].toInt();
        if (m_onItemCollected) {
            m_onItemCollected(itemIdx, pId);
        }
    }
    else if (type == PacketType::GRAND_PRIX_NEXT_ROUND) {
        std::string nextBiome = obj["biomeId"].toString().toStdString();
        int round = obj["round"].toInt(1);
        m_currentBiomeId = nextBiome;
        if (m_onGrandPrixNext) {
            m_onGrandPrixNext(nextBiome, round);
        }
    }
    else if (type == PacketType::DISCONNECT) {
        int pId = obj["playerId"].toInt();
        m_remotePlayers.erase(pId);
    }
}

void GameClient::sendReady(bool isReady, int vehicleType) {
    if (!isConnected()) return;

    QJsonObject obj;
    obj["type"] = static_cast<int>(PacketType::PLAYER_READY);
    obj["playerId"] = m_localPlayerId;
    obj["isReady"] = isReady;
    obj["vehicleType"] = vehicleType;

    sendPacket(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void GameClient::sendTelemetry(const RemotePlayerState& state) {
    if (!isConnected()) return;

    QJsonObject obj;
    obj["type"] = static_cast<int>(PacketType::PLAYER_TELEMETRY);
    obj["playerId"] = m_localPlayerId;
    obj["name"] = QString::fromStdString(m_username);
    obj["color"] = static_cast<double>(m_assignedColor);
    obj["vehicleType"] = state.vehicleType;
    obj["driverType"] = state.driverType;

    obj["x"] = state.posX;
    obj["y"] = state.posY;
    obj["vx"] = state.velX;
    obj["vy"] = state.velY;
    obj["angle"] = state.chassisAngle;
    obj["angVel"] = state.chassisAngularVel;
    obj["rearWheelAngle"] = state.rearWheelAngle;
    obj["frontWheelAngle"] = state.frontWheelAngle;
    obj["rearSusp"] = state.rearSuspLen;
    obj["frontSusp"] = state.frontSuspLen;
    obj["headAngle"] = state.driverHeadAngle;
    obj["speed"] = state.speedKmh;
    obj["fuel"] = state.fuel;
    obj["dist"] = state.distance;
    obj["air"] = state.isAirborne;
    obj["crash"] = state.isCrashed;
    obj["fin"] = state.isFinished;

    sendPacket(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void GameClient::sendItemCollected(int itemIndex) {
    if (!isConnected()) return;

    QJsonObject obj;
    obj["type"] = static_cast<int>(PacketType::ITEM_COLLECTED);
    obj["itemIdx"] = itemIndex;
    obj["playerId"] = m_localPlayerId;

    sendPacket(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void GameClient::updateInterpolation(float dt) {
    // Smooth lerp dead reckoning so remote cars glide smoothly at 60-120 FPS
    float lerpFactor = std::clamp(dt * 18.0f, 0.0f, 1.0f);
    for (auto& pair : m_remotePlayers) {
        RemotePlayerState& p = pair.second;
        p.posX += (p.targetX - p.posX) * lerpFactor;
        p.posY += (p.targetY - p.posY) * lerpFactor;

        // Angle lerp with wrap-around
        float dAngle = p.targetAngle - p.chassisAngle;
        while (dAngle > 3.14159f) dAngle -= 6.28318f;
        while (dAngle < -3.14159f) dAngle += 6.28318f;
        p.chassisAngle += dAngle * lerpFactor;
    }
}

void GameClient::sendPacket(const QByteArray& packetData) {
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(packetData + "\n");
    }
}

} // namespace Network
