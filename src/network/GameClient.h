#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include "NetworkTypes.h"

namespace Network {

/**
 * @brief High-efficiency TCP Client for connecting to remote Hill Climb multiplayer games.
 * Sends local vehicle telemetry and receives positions of remote players with dead-reckoning smoothing.
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
    const std::string& getLocalPlayerName() const { return m_username; }
    uint32_t getAssignedColor() const { return m_assignedColor; }
    const std::string& getTrack() const { return m_currentBiomeId; }

    void sendReady(bool isReady, int vehicleType, int driverType);
    void sendTelemetry(const RemotePlayerState& state);
    void sendItemCollected(int itemIndex);
    void sendPlayerCrashed(float distance);
    void sendPlayerFinished(float distance, float time);
    void sendReturnToLobby();

    // Callbacks for UI & Game Engine
    void setOnConnected(std::function<void()> cb) { m_onConnected = cb; }
    void setOnConnectionError(std::function<void(const std::string& error)> cb) { m_onConnectionError = cb; }
    void setOnDisconnected(std::function<void()> cb) { m_onDisconnected = cb; }
    void setOnLobbyUpdated(std::function<void(const std::string& biomeId, const std::vector<PlayerLobbyInfo>& players)> cb) {
        m_onLobbyUpdated = cb;
    }
    void setOnRaceStart(std::function<void(const std::string& biomeId)> cb) { m_onRaceStart = cb; }
    void setOnRematch(std::function<void(const std::string& biomeId)> cb) { m_onRematch = cb; }
    void setOnReturnToLobby(std::function<void()> cb) { m_onReturnToLobby = cb; }
    void setOnRemoteTelemetry(std::function<void(const RemotePlayerState& state)> cb) { m_onRemoteTelemetry = cb; }
    void setOnItemCollected(std::function<void(int itemIdx, int playerId)> cb) { m_onItemCollected = cb; }
    void setOnRemotePlayerCrashed(std::function<void(int playerId, float dist)> cb) { m_onRemotePlayerCrashed = cb; }
    void setOnRemotePlayerFinished(std::function<void(int playerId, float dist, float time)> cb) { m_onRemotePlayerFinished = cb; }

    const std::map<int, RemotePlayerState>& getRemotePlayers() const { return m_remotePlayers; }
    std::map<int, RemotePlayerState>& getRemotePlayers() { return m_remotePlayers; }
    void clearRemotePlayers() { m_remotePlayers.clear(); }

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
    std::string m_username = "Racer";
    int m_vehicleType = 0;
    int m_driverType = 0;
    std::string m_currentBiomeId = "countryside";

    std::map<int, RemotePlayerState> m_remotePlayers;

    std::function<void()> m_onConnected;
    std::function<void(const std::string&)> m_onConnectionError;
    std::function<void()> m_onDisconnected;
    std::function<void(const std::string&, const std::vector<PlayerLobbyInfo>&)> m_onLobbyUpdated;
    std::function<void(const std::string&)> m_onRaceStart;
    std::function<void(const std::string&)> m_onRematch;
    std::function<void()> m_onReturnToLobby;
    std::function<void(const RemotePlayerState&)> m_onRemoteTelemetry;
    std::function<void(int, int)> m_onItemCollected;
    std::function<void(int, float)> m_onRemotePlayerCrashed;
    std::function<void(int, float, float)> m_onRemotePlayerFinished;

    void processPacket(const QByteArray& packetData);
    void sendPacket(const QByteArray& packetData);
};

} // namespace Network
