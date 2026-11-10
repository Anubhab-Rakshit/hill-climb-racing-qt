#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <QtGlobal>

namespace Network {

constexpr quint16 DEFAULT_PORT = 7777;
constexpr int PROTOCOL_VERSION = 2;

enum class PacketType : uint8_t {
    CONNECT_REQ = 1,
    CONNECT_RESP,
    LOBBY_UPDATE,
    PLAYER_READY,
    RACE_START,
    PLAYER_TELEMETRY,
    ITEM_COLLECTED,
    PLAYER_CRASHED,
    PLAYER_FINISHED,
    RACE_RESULT,
    REMATCH,
    RETURN_TO_LOBBY,
    DISCONNECT
};

struct PlayerLobbyInfo {
    int id = 0;
    std::string name;
    int vehicleType = 0;
    int driverType = 0;
    bool isReady = false;
    bool isHost = false;
    uint32_t color = 0xFF00E5FF;
};

struct RemotePlayerState {
    int id = 0;
    std::string name;
    uint32_t color = 0xFF00E5FF;
    int vehicleType = 0;
    int driverType = 0;

    // Kinematics & Physics
    float posX = 5.0f;
    float posY = 2.0f;
    float velX = 0.0f;
    float velY = 0.0f;
    float chassisAngle = 0.0f;
    float chassisAngularVel = 0.0f;

    // Wheels & Suspension
    float rearWheelAngle = 0.0f;
    float frontWheelAngle = 0.0f;
    float rearSuspLen = 0.2f;
    float frontSuspLen = 0.2f;

    // Driver head angle
    float driverHeadAngle = 0.0f;

    // Telemetry status
    float speedKmh = 0.0f;
    float fuel = 100.0f;
    float distance = 0.0f;
    int coins = 0;
    int flips = 0;
    bool isAirborne = false;
    bool isCrashed = false;
    bool isFinished = false;
    float finishTime = 0.0f;

    // Dead-reckoning lerp targets
    float targetX = 5.0f;
    float targetY = 2.0f;
    float targetAngle = 0.0f;
};

} // namespace Network
