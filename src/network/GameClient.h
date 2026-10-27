#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <string>
#include <vector>
#include <functional>
#include <map>
#include "NetworkTypes.h"

namespace Network {

/**
 * @brief High-efficiency TCP Client for connecting to remote Hill Climb multiplayer games.
 * Sends local vehicle telemetry and receives positions of remote players with lerp smoothing.
 */
class GameClient : public QObject {
    Q_OBJECT
public:
    explicit GameClient(QObject* parent = nullptr);
    ~GameClient() override;

    void connectToHost(const QString& hostAddress, quint16 port,
                       const std::string& username, int vehicleType, int driverType);
    void disconnectFromHost();
    bool isConnected() const;

    int getLocalPlayerId() const { return m_localPlayerId; }
    uint32_t getAssignedColor() const { return m_assignedColor; }
    const std::string& getTrack() const { return m_currentBiomeId; }

    void sendReady(bool isReady, int vehicleType);
    void sendTelemetry(const RemotePlayerState& state);
    void sendItemCollected(int itemIndex);

    // Callbacks for UI & Game Engine
    void setOnConnected(std::function<void()> cb) { m_onConnected = cb; }
    void setOnRejected(std::function<void(const std::string& reason, const std::vector<std::string>& suggestions)> cb) {
        m_onRejected = cb;
    }
    void setOnDisconnected(std::function<void()> cb) { m_onDisconnected = cb; }
    void setOnLobbyUpdated(std::function<void(const std::string& biomeId, const std::vector<PlayerLobbyInfo>& players)> cb) {
        m_onLobbyUpdated = cb;
    }
    void setOnRaceStart(std::function<void(const std::string& biomeId)> cb) { m_onRaceStart = cb; }
    void setOnRemoteTelemetry(std::function<void(const RemotePlayerState& state)> cb) { m_onRemoteTelemetry = cb; }
    void setOnItemCollected(std::function<void(int itemIdx, int playerId)> cb) { m_onItemCollected = cb; }
    void setOnGrandPrixNext(std::function<void(const std::string& nextBiome, int round)> cb) { m_onGrandPrixNext = cb; }

    const std::map<int, RemotePlayerState>& getRemotePlayers() const { return m_remotePlayers; }

    // Smoothly interpolate remote players between ticks
    void updateInterpolation(float dt);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket* m_socket = nullptr;
    int m_localPlayerId = -1;
    uint32_t m_assignedColor = 0xFF00E5FF;
    std::string m_username;
    int m_vehicleType = 0;
    int m_driverType = 0;
    std::string m_currentBiomeId = "countryside";

    std::map<int, RemotePlayerState> m_remotePlayers;

    std::function<void()> m_onConnected;
    std::function<void(const std::string&, const std::vector<std::string>&)> m_onRejected;
    std::function<void()> m_onDisconnected;
    std::function<void(const std::string&, const std::vector<PlayerLobbyInfo>&)> m_onLobbyUpdated;
    std::function<void(const std::string&)> m_onRaceStart;
    std::function<void(const RemotePlayerState&)> m_onRemoteTelemetry;
    std::function<void(int, int)> m_onItemCollected;
    std::function<void(const std::string&, int)> m_onGrandPrixNext;

    void processPacket(const QByteArray& packetData);
    void sendPacket(const QByteArray& packetData);
};

} // namespace Network
