#include "Vehicle.h"
#include <cmath>
#include <algorithm>

namespace Physics {

Vehicle::Vehicle() {
    reset(5.0f, 3.0f);
}

void Vehicle::reset(float startX, float startY) {
    m_chassisMass = 600.0f;
    m_chassisInertia = 450.0f;
    m_chassisPos = {startX, startY};
    m_chassisVel = {0.0f, 0.0f};
    m_chassisAngle = 0.0f;
    m_chassisAngularVel = 0.0f;

    m_wheelRadius = 0.45f;
    m_wheelMass = 25.0f;
    m_wheelInertia = 0.5f * m_wheelMass * m_wheelRadius * m_wheelRadius;

    m_rearWheelPos = {startX - 0.9f, startY - 0.7f};
    m_rearWheelVel = {0.0f, 0.0f};
    m_rearWheelAngle = 0.0f;
    m_rearWheelAngularVel = 0.0f;

    m_frontWheelPos = {startX + 0.9f, startY - 0.7f};
    m_frontWheelVel = {0.0f, 0.0f};
    m_frontWheelAngle = 0.0f;
    m_frontWheelAngularVel = 0.0f;

    m_suspRestLength = 0.75f;
    m_rearSuspensionLength = m_suspRestLength;
    m_frontSuspensionLength = m_suspRestLength;

    m_driverHeadAngle = 0.0f;
    m_driverHeadAngularVel = 0.0f;

    m_fuel = 100.0f;
    m_gasInput = 0.0f;
    m_brakeInput = 0.0f;

    m_rearContact = false;
    m_frontContact = false;
    m_driverDown = false;
}

void Vehicle::applyInput(float gas, float brake) {
    m_gasInput = std::clamp(gas, 0.0f, 1.0f);
    m_brakeInput = std::clamp(brake, 0.0f, 1.0f);
}

void Vehicle::addFuel(float amount) {
    m_fuel = std::clamp(m_fuel + amount, 0.0f, 100.0f);
}

Vec2 Vehicle::driverHeadPos() const {
    float cosA = std::cos(m_chassisAngle);
    float sinA = std::sin(m_chassisAngle);
    // Head mount relative to chassis center: (-0.1, +0.6)
    float hx = m_chassisPos.x + (-0.1f * cosA - 0.6f * sinA);
    float hy = m_chassisPos.y + (-0.1f * sinA + 0.6f * cosA);
    return {hx, hy};
}

float Vehicle::getSpeedKmh() const {
    return std::abs(m_chassisVel.x) * 3.6f;
}

float Vehicle::getEngineRpm() const {
    float wheelRpm = (std::abs(m_rearWheelAngularVel) / (2.0f * static_cast<float>(M_PI))) * 60.0f;
    float idleRpm = 1000.0f;
    float gearRatio = 4.2f;
    float throttleBoost = m_gasInput * 2500.0f;
    return std::clamp(idleRpm + wheelRpm * gearRatio + throttleBoost, 1000.0f, 7800.0f);
}

void Vehicle::step(float dt, const Terrain& terrain, const Core::ProfileManager& profile) {
    if (m_driverDown) return;

    // 1. Drain Fuel
    if (m_fuel > 0.0f) {
        float drainRate = 1.0f + m_gasInput * 3.2f;
        m_fuel = std::max(0.0f, m_fuel - drainRate * dt);
    }
    float effectiveGas = (m_fuel > 0.0f) ? m_gasInput : 0.0f;

    // Upgrades Multipliers
    float torqueMult = profile.getEngineTorqueMultiplier();
    float suspStiffnessMult = profile.getSuspensionStiffnessMultiplier();
    float tireGripMult = profile.getTireGripMultiplier();
    float fourWdSplit = profile.get4wdTorqueSplit();

    float gravity = terrain.getGravity();
    Vec2 gravForce(0.0f, -gravity * m_chassisMass);

    // Chassis local mount points
    float cosA = std::cos(m_chassisAngle);
    float sinA = std::sin(m_chassisAngle);

    Vec2 rearMount(m_chassisPos.x + (-0.9f * cosA - (-0.2f) * sinA),
                   m_chassisPos.y + (-0.9f * sinA + (-0.2f) * cosA));
    Vec2 frontMount(m_chassisPos.x + (+0.9f * cosA - (-0.2f) * sinA),
                    m_chassisPos.y + (+0.9f * sinA + (-0.2f) * cosA));

    // 2. Suspension Calculations (Spring-Damper)
    float k_susp = 18000.0f * suspStiffnessMult;
    float c_susp = 1200.0f * suspStiffnessMult;

    auto solveSuspension = [&](const Vec2& mount, Vec2& wheelPos, Vec2& wheelVel, float& currentLen) {
        Vec2 strut = wheelPos - mount;
        float len = strut.length();
        if (len < 0.01f) len = 0.01f;
        currentLen = len;

        Vec2 strutDir = strut / len;
        float deltaX = m_suspRestLength - len;
        float relVel = Vec2::dot(wheelVel - m_chassisVel, strutDir);

        float suspForceMag = k_susp * deltaX - c_susp * relVel;
        if (suspForceMag < 0.0f) suspForceMag = 0.0f; // Strut doesn't pull inward

        Vec2 forceOnWheel = strutDir * suspForceMag;
        Vec2 forceOnChassis = strutDir * (-suspForceMag);

        // Apply to chassis
        m_chassisVel += (forceOnChassis / m_chassisMass) * dt;
        float torque = (mount.x - m_chassisPos.x) * forceOnChassis.y - (mount.y - m_chassisPos.y) * forceOnChassis.x;
        m_chassisAngularVel += (torque / m_chassisInertia) * dt;

        // Apply to wheel
        wheelVel += (forceOnWheel / m_wheelMass) * dt;
    };

    solveSuspension(rearMount, m_rearWheelPos, m_rearWheelVel, m_rearSuspensionLength);
    solveSuspension(frontMount, m_frontWheelPos, m_frontWheelVel, m_frontSuspensionLength);

    // 3. Wheel Ground Collisions & Traction
    auto solveWheelGround = [&](Vec2& pos, Vec2& vel, float& angle, float& angVel, bool isDriven, bool& contact) {
        float groundY = terrain.getHeight(pos.x);
        Vec2 normal = terrain.getNormal(pos.x);
        Vec2 tangent(normal.y, -normal.x); // Surface forward tangent

        float penetration = (groundY + m_wheelRadius) - pos.y;
        if (penetration > 0.0f) {
            contact = true;
            // Penalty spring for ground contact
            float k_ground = 35000.0f;
            float c_ground = 1800.0f;
            float normVel = Vec2::dot(vel, normal);
            float normalForceMag = std::max(0.0f, k_ground * penetration - c_ground * normVel);

            vel += (normal * normalForceMag / m_wheelMass) * dt;

            // Traction & Friction
            float frictionCoeff = terrain.getFriction() * tireGripMult;
            float maxFriction = normalForceMag * frictionCoeff;

            float driveTorque = 0.0f;
            if (isDriven) {
                float peakTorque = 380.0f * torqueMult;
                driveTorque = effectiveGas * peakTorque;
            }
            if (m_brakeInput > 0.0f) {
                driveTorque -= m_brakeInput * 450.0f * ((angVel > 0.0f) ? 1.0f : -1.0f);
            }

            angVel += (driveTorque / m_wheelInertia) * dt;

            // Slip speed
            float surfSpeed = Vec2::dot(vel, tangent);
            float wheelLinSpeed = angVel * m_wheelRadius;
            float slipSpeed = surfSpeed - wheelLinSpeed;

            float tractiveForce = -std::clamp(slipSpeed * 800.0f, -maxFriction, maxFriction);
            vel += (tangent * tractiveForce / m_wheelMass) * dt;
            angVel -= (tractiveForce * m_wheelRadius / m_wheelInertia) * dt;

            // Clamp wheel above ground
            pos.y = groundY + m_wheelRadius;
        } else {
            contact = false;
        }

        // Integrate wheel
        vel += Vec2(0.0f, -gravity) * dt;
        vel *= 0.999f;
        pos += vel * dt;
        angle += angVel * dt;
        angVel *= 0.995f;
    };

    bool rearDriven = true;
    bool frontDriven = (fourWdSplit > 0.01f);

    solveWheelGround(m_rearWheelPos, m_rearWheelVel, m_rearWheelAngle, m_rearWheelAngularVel, rearDriven, m_rearContact);
    solveWheelGround(m_frontWheelPos, m_frontWheelVel, m_frontWheelAngle, m_frontWheelAngularVel, frontDriven, m_frontContact);

    // 4. In-Air Pitch Control (Signature Hill Climb Racing Mechanic!)
    if (!m_rearContact && !m_frontContact) {
        float airPitchTorque = 400.0f;
        if (effectiveGas > 0.0f) {
            // Leans back
            m_chassisAngularVel += (airPitchTorque * effectiveGas / m_chassisInertia) * dt;
        }
        if (m_brakeInput > 0.0f) {
            // Leans forward
            m_chassisAngularVel -= (airPitchTorque * m_brakeInput / m_chassisInertia) * dt;
        }
    }

    // 5. Integrate Chassis Rigid Body (Symplectic Euler)
    m_chassisVel += (gravForce / m_chassisMass) * dt;
    m_chassisVel *= 0.999f;
    m_chassisAngularVel *= 0.995f;

    m_chassisPos += m_chassisVel * dt;
    m_chassisAngle += m_chassisAngularVel * dt;

    // 6. Driver Head Ragdoll Pendulum
    float headAccX = m_chassisVel.x;
    float k_head = 45.0f;
    float c_head = 6.0f;
    float targetHeadAngle = -headAccX * 0.02f;
    float headTorque = -k_head * (m_driverHeadAngle - targetHeadAngle) - c_head * m_driverHeadAngularVel;
    m_driverHeadAngularVel += headTorque * dt;
    m_driverHeadAngle += m_driverHeadAngularVel * dt;

    // 7. Fatal Driver Head Collision Check (Neck Snap!)
    Vec2 head = driverHeadPos();
    float headGround = terrain.getHeight(head.x);
    if (head.y <= headGround + 0.25f) {
        m_driverDown = true;
    }
}

} // namespace Physics
