#pragma once

#include "PhysicsTypes.h"
#include "Terrain.h"
#include "ProfileManager.h"

namespace Physics {

/**
 * @brief Multi-Body 2D Vehicle with Chassis, Wheels, Suspension Struts, and Driver Ragdoll.
 * Fully interactive with working suspension physics and clear formulas for the backend team.
 */
class Vehicle {
public:
    Vehicle();

    void reset(float startX = 5.0f, float startY = 3.0f);
    void applyInput(float gas, float brake);

    void step(float dt, const Terrain& terrain, const Core::ProfileManager& profile);

    // Chassis Telemetry
    const Vec2& chassisPos() const { return m_chassisPos; }
    const Vec2& chassisVel() const { return m_chassisVel; }
    float chassisAngle() const { return m_chassisAngle; }
    float chassisAngularVel() const { return m_chassisAngularVel; }

    // Wheels Telemetry
    const Vec2& rearWheelPos() const { return m_rearWheelPos; }
    const Vec2& frontWheelPos() const { return m_frontWheelPos; }
    float rearWheelAngle() const { return m_rearWheelAngle; }
    float frontWheelAngle() const { return m_frontWheelAngle; }
    float rearWheelRadius() const { return m_wheelRadius; }
    float frontWheelRadius() const { return m_wheelRadius; }

    // Driver Head
    Vec2 driverHeadPos() const;
    float driverHeadAngle() const { return m_driverHeadAngle; }

    // Gauges & Engine
    float getSpeedKmh() const;
    float getEngineRpm() const;
    float fuel() const { return m_fuel; }
    void addFuel(float amount);

    // States & Stunts
    bool isRearOnGround() const { return m_rearContact; }
    bool isFrontOnGround() const { return m_frontContact; }
    bool isAirborne() const { return !m_rearContact && !m_frontContact; }
    bool isDriverDown() const { return m_driverDown; }
    bool isOutOfFuel() const { return m_fuel <= 0.0f; }

    float rearSuspensionCompression() const { return m_rearSuspensionLength; }
    float frontSuspensionCompression() const { return m_frontSuspensionLength; }

private:
    // Chassis State
    Vec2 m_chassisPos;
    Vec2 m_chassisVel;
    float m_chassisAngle;
    float m_chassisAngularVel;
    float m_chassisMass;
    float m_chassisInertia;

    // Wheel States
    Vec2 m_rearWheelPos;
    Vec2 m_rearWheelVel;
    float m_rearWheelAngle;
    float m_rearWheelAngularVel;

    Vec2 m_frontWheelPos;
    Vec2 m_frontWheelVel;
    float m_frontWheelAngle;
    float m_frontWheelAngularVel;

    float m_wheelRadius;
    float m_wheelMass;
    float m_wheelInertia;

    // Suspension
    float m_suspRestLength;
    float m_rearSuspensionLength;
    float m_frontSuspensionLength;

    // Driver Head Pendulum
    float m_driverHeadAngle;
    float m_driverHeadAngularVel;

    // Fuel & Controls
    float m_fuel;
    float m_gasInput;
    float m_brakeInput;

    bool m_rearContact;
    bool m_frontContact;
    bool m_driverDown;
};

} // namespace Physics
