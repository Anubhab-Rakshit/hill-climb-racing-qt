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

    void reset(float startX = 5.0f, float startY = 3.0f, float startAngle = 0.0f);
    void applyInput(float gas, float brake, float reverse = 0.0f, float tiltLeft = 0.0f, float tiltRight = 0.0f);

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

    // Gauges & Engine (Recalibrated for 60% physical frame speed pacing with 100% speedometer scale retention)
    static constexpr float SPEED_CALIBRATION = 2.25f;
    float getSpeedKmh() const;
    float getEngineRpm() const;
    float fuel() const { return m_fuel; }
    void addFuel(float amount);
    void setFuel(float fuel) { m_fuel = std::clamp(fuel, 0.0f, 100.0f); }
    void consumeFuel(float amount) { m_fuel = std::max(0.0f, m_fuel - amount); }
    float reverseInput() const { return m_reverseInput; }
    float brakePressure() const { return m_brakePressure; }

    // Rendering State Interpolation (120 Hz Simulation -> Variable Display Refresh Rate)
    Vec2 interpolatedChassisPos(float alpha) const;
    Vec2 interpolatedRearWheelPos(float alpha) const;
    Vec2 interpolatedFrontWheelPos(float alpha) const;
    float interpolatedChassisAngle(float alpha) const;
    float interpolatedRearWheelAngle(float alpha) const;
    float interpolatedFrontWheelAngle(float alpha) const;
    float interpolatedDriverHeadAngle(float alpha) const;
    Vec2 interpolatedDriverHeadPos(float alpha) const;

    // States & Stunts
    bool isRearOnGround() const { return m_rearContact; }
    bool isFrontOnGround() const { return m_frontContact; }
    bool isChassisContact() const { return m_chassisContact; }
    bool isAirborne() const { return !m_rearContact && !m_frontContact && !m_chassisContact; }
    bool isDriverDown() const { return m_driverDown; }
    bool isOutOfFuel() const { return m_fuel <= 0.0f; }
    bool isLowFuel() const { return m_fuel < 25.0f; }

    float getMaxTerrainPenetration(const Terrain& terrain) const;

    float rearSuspensionCompression() const { return m_rearSuspensionLength; }
    float frontSuspensionCompression() const { return m_frontSuspensionLength; }
    float suspRestLength() const { return m_suspRestLength; }
    float wheelRadius() const { return m_wheelRadius; }

    // Tyre Contact & Slip Telemetry for Deformation and Particle Effects
    float rearSlipSpeed() const { return m_rearSlipSpeed; }
    float frontSlipSpeed() const { return m_frontSlipSpeed; }
    float rearNormalForce() const { return m_rearNormalForce; }
    float frontNormalForce() const { return m_frontNormalForce; }
    Vec2 rearContactTangent() const { return m_rearContactTangent; }
    Vec2 frontContactTangent() const { return m_frontContactTangent; }
    float rearWheelAngularVel() const { return m_rearWheelAngularVel; }
    float frontWheelAngularVel() const { return m_frontWheelAngularVel; }

    // Dynamic Audio & Physics Telemetry
    float gasInput() const { return m_gasInput; }
    float brakeInput() const { return m_brakeInput; }
    float rearSuspVel() const { return m_rearSuspVel; }
    float frontSuspVel() const { return m_frontSuspVel; }
    float lastImpactVelocity() const { return m_lastImpactVelocity; }
    float maxSlipSpeed() const { return std::max(std::abs(m_rearSlipSpeed), std::abs(m_frontSlipSpeed)); }

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
    float m_reverseInput;
    float m_tiltLeftInput;
    float m_tiltRightInput;

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
    float m_lastImpactVelocity = 0.0f;

    // Previous Physics State Caching for Smooth Frame Interpolation
    Vec2 m_prevChassisPos;
    Vec2 m_prevRearWheelPos;
    Vec2 m_prevFrontWheelPos;
    float m_prevChassisAngle = 0.0f;
    float m_prevRearWheelAngle = 0.0f;
    float m_prevFrontWheelAngle = 0.0f;
    float m_prevDriverHeadAngle = 0.0f;
    float m_prevDriverHeadShiftY = 0.0f;
    float m_brakePressure = 0.0f;

    void applyConfig();
};

} // namespace Physics
