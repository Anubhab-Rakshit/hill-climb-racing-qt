#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QMap>
#include <QByteArray>
#include <functional>
#include "NetworkTypes.h"

namespace Network {

struct ConnectedClient {
    int id = 0;
    QTcpSocket* socket = nullptr;
    std::string name;
    int vehicleType = 0;
    int driverType = 0;
    bool isReady = false;
    uint32_t assignedColor = 0xFF00E5FF;
    RemotePlayerState lastState;
};

/**
 * @brief High-performance non-blocking TCP Server for Remote Multi-Laptop Multiplayer.
 * Handles room hosting, unique name validation, real-time telemetry broadcast, and race sync.
 */
class GameServer : public QObject {
    Q_OBJECT
public:
    explicit GameServer(QObject* parent = nullptr);
    ~GameServer() override;

    bool startServer(quint16 port = DEFAULT_PORT);
    void stopServer();
    bool isRunning() const;

    QString getLocalIpAddress() const;
    quint16 getPort() const { return m_port; }

    void setHostPlayerInfo(const std::string& name, int vehicleType, int driverType);
    void setTrack(const std::string& biomeId);
    const std::string& getTrack() const { return m_biomeId; }

    void broadcastStartRace();
    void broadcastNextRound(const std::string& nextBiome, int roundNumber);
    void sendItemCollected(int itemIndex, int collectedByPlayerId);

    // Callbacks for local game engine integration
    void setOnPlayerConnected(std::function<void(int id, const std::string& name)> cb) { m_onPlayerConnected = cb; }
    void setOnPlayerDisconnected(std::function<void(int id)> cb) { m_onPlayerDisconnected = cb; }
    void setOnTelemetryReceived(std::function<void(const RemotePlayerState& state)> cb) { m_onTelemetryReceived = cb; }
    void setOnItemCollected(std::function<void(int itemIdx, int playerId)> cb) { m_onItemCollected = cb; }

    const QMap<int, ConnectedClient>& getClients() const { return m_clients; }
    int getPlayerCount() const { return m_clients.size() + 1; /* +1 for host */ }

    void updateHostTelemetry(const RemotePlayerState& hostState);

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onClientReadyRead();

private:
    QTcpServer* m_tcpServer = nullptr;
    quint16 m_port = DEFAULT_PORT;
    int m_nextClientId = 1;
    std::string m_biomeId = "countryside";

    ConnectedClient m_hostClient;
    QMap<int, ConnectedClient> m_clients;
    QMap<QTcpSocket*, int> m_socketToId;

    std::function<void(int, const std::string&)> m_onPlayerConnected;
    std::function<void(int)> m_onPlayerDisconnected;
    std::function<void(const RemotePlayerState&)> m_onTelemetryReceived;
    std::function<void(int, int)> m_onItemCollected;

    void processPacket(QTcpSocket* senderSocket, const QByteArray& packetData);
    void broadcastPacket(const QByteArray& packetData, QTcpSocket* excludeSocket = nullptr);
    void sendPacket(QTcpSocket* destSocket, const QByteArray& packetData);
    void broadcastLobbyState();

    uint32_t assignColorForId(int id);
};

} // namespace Network
