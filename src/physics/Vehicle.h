#pragma once

#include "PhysicsTypes.h"
#include "Terrain.h"
#include "ProfileManager.h"
#include "VehicleConfig.h"

namespace Physics {

/**
 * @brief Multi-Body 2D Vehicle with Chassis, Wheels, Suspension Struts, and Driver Inertia.
 * Supports configurable vehicle archetypes and distinct driver models with exact tyre alignment.
 */
class Vehicle {
public:
    Vehicle(VehicleType type = VehicleType::OFFROADER, DriverType driver = DriverType::BILL);

    void reset(float startX = 5.0f, float startY = 3.0f);
    void applyInput(float gas, float brake);

    void step(float dt, const Terrain& terrain, const Core::ProfileManager& profile);

    // Archetype & Driver Configuration
    void setVehicleType(VehicleType type);
    VehicleType vehicleType() const { return m_vehicleType; }

    void setDriverType(DriverType driver) { m_driverType = driver; }
    DriverType driverType() const { return m_driverType; }

    const VehicleConfig& config() const { return m_config; }

    // Chassis Telemetry
    const Vec2& chassisPos() const { return m_chassisPos; }
    const Vec2& chassisVel() const { return m_chassisVel; }
    float chassisAngle() const { return m_chassisAngle; }
    float chassisAngularVel() const { return m_chassisAngularVel; }

    // Mount Points
    Vec2 rearMountOffset() const { return m_rearMountOffset; }
    Vec2 frontMountOffset() const { return m_frontMountOffset; }
    Vec2 driverSeatOffset() const { return m_driverSeatOffset; }

    // Wheels Telemetry
    const Vec2& rearWheelPos() const { return m_rearWheelPos; }
    const Vec2& frontWheelPos() const { return m_frontWheelPos; }
    float rearWheelAngle() const { return m_rearWheelAngle; }
    float frontWheelAngle() const { return m_frontWheelAngle; }
    float rearWheelRadius() const { return m_wheelRadius; }
    float frontWheelRadius() const { return m_wheelRadius; }

    // Driver Head & Dynamic Inertia
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
    bool isChassisContact() const { return m_chassisContact; }
    bool isAirborne() const { return !m_rearContact && !m_frontContact && !m_chassisContact; }
    bool isDriverDown() const { return m_driverDown; }
    bool isOutOfFuel() const { return m_fuel <= 0.0f; }

    float rearSuspensionCompression() const { return m_rearSuspensionLength; }
    float frontSuspensionCompression() const { return m_frontSuspensionLength; }

    // Tyre Contact & Slip Telemetry for Deformation and Particle Effects
    float rearSlipSpeed() const { return m_rearSlipSpeed; }
    float frontSlipSpeed() const { return m_frontSlipSpeed; }
    float rearNormalForce() const { return m_rearNormalForce; }
    float frontNormalForce() const { return m_frontNormalForce; }
    Vec2 rearContactTangent() const { return m_rearContactTangent; }
    Vec2 frontContactTangent() const { return m_frontContactTangent; }
    float rearWheelAngularVel() const { return m_rearWheelAngularVel; }
    float frontWheelAngularVel() const { return m_frontWheelAngularVel; }

private:
    VehicleType m_vehicleType;
    DriverType m_driverType;
    VehicleConfig m_config;

    // Chassis State
    Vec2 m_chassisPos;
    Vec2 m_chassisVel;
    Vec2 m_prevChassisVel;
    float m_chassisAngle;
    float m_chassisAngularVel;
    float m_chassisMass;
    float m_chassisInertia;

    // Mount Offsets
    Vec2 m_rearMountOffset;
    Vec2 m_frontMountOffset;
    Vec2 m_driverSeatOffset;

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
    float m_suspMinLength;
    float m_suspMaxLength;
    float m_rearSuspensionLength;
    float m_frontSuspensionLength;
    float m_rearSuspVel;
    float m_frontSuspVel;

    // Driver Inertia Ragdoll / Spring System
    float m_driverHeadAngle;
    float m_driverHeadAngularVel;
    float m_driverHeadShiftX;
    float m_driverHeadShiftY;
    float m_driverHeadShiftYVel;
    float m_spawnGraceTimer;

    // Fuel & Controls
    float m_fuel;
    float m_gasInput;
    float m_brakeInput;

    bool m_rearContact;
    bool m_frontContact;
    bool m_chassisContact;
    bool m_driverDown;

    float m_rearSlipSpeed = 0.0f;
    float m_frontSlipSpeed = 0.0f;
    float m_rearNormalForce = 0.0f;
    float m_frontNormalForce = 0.0f;
    Vec2 m_rearContactTangent = {1.0f, 0.0f};
    Vec2 m_frontContactTangent = {1.0f, 0.0f};

    void applyConfig();
};

} // namespace Physics
