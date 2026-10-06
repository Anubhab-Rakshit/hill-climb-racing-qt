#include "Vehicle.h"
#include <cmath>
#include <algorithm>

namespace Physics {

Vehicle::Vehicle() {
    reset(5.0f, 1.20f);
}

void Vehicle::reset(float startX, float startY) {
    m_chassisMass = 440.0f;
    m_chassisInertia = 280.0f;
    m_chassisPos = {startX, startY};
    m_chassisVel = {0.0f, 0.0f};
    m_chassisAngle = 0.0f;
    m_chassisAngularVel = 0.0f;

    m_wheelRadius = 0.32f;
    m_wheelMass = 20.0f;
    m_wheelInertia = 0.5f * m_wheelMass * m_wheelRadius * m_wheelRadius;

    m_suspRestLength = 0.28f;
    m_suspMinLength = 0.14f;
    m_suspMaxLength = 0.44f;
    m_rearSuspensionLength = m_suspRestLength;
    m_frontSuspensionLength = m_suspRestLength;
    m_rearSuspVel = 0.0f;
    m_frontSuspVel = 0.0f;

    // Wheel initial positions directly attached to mounts along down axis
    m_rearWheelPos = {startX - 0.78f, startY - 0.06f - m_suspRestLength};
    m_rearWheelVel = {0.0f, 0.0f};
    m_rearWheelAngle = 0.0f;
    m_rearWheelAngularVel = 0.0f;

    m_frontWheelPos = {startX + 0.78f, startY - 0.06f - m_suspRestLength};
    m_frontWheelVel = {0.0f, 0.0f};
    m_frontWheelAngle = 0.0f;
    m_frontWheelAngularVel = 0.0f;

    m_driverHeadAngle = 0.0f;
    m_driverHeadAngularVel = 0.0f;

    m_fuel = 100.0f;
    m_gasInput = 0.0f;
    m_brakeInput = 0.0f;

    m_rearContact = true;
    m_frontContact = true;
    m_chassisContact = false;
    m_driverDown = false;
    m_spawnGraceTimer = 3.0f;
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
    // Head mount relative to chassis center: (-0.27, +0.38)
    float hx = m_chassisPos.x + (-0.27f * cosA - 0.38f * sinA);
    float hy = m_chassisPos.y + (-0.27f * sinA + 0.38f * cosA);
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
        float drainRate = 1.2f + m_gasInput * 3.5f;
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

    // Chassis local orientation
    float cosA = std::cos(m_chassisAngle);
    float sinA = std::sin(m_chassisAngle);

    // Unit vector pointing locally "down" from chassis
    Vec2 downDir(sinA, -cosA);

    // Mount points in world space
    auto getMount = [&](float lx, float ly) -> Vec2 {
        return {m_chassisPos.x + (lx * cosA - ly * sinA),
                m_chassisPos.y + (lx * sinA + ly * cosA)};
    };

    Vec2 rearMount = getMount(-0.78f, -0.06f);
    Vec2 frontMount = getMount(+0.78f, -0.06f);

    // 2. Harmonic Suspension & Pacejka Traction Solver
    float k_susp = 38000.0f * suspStiffnessMult;
    float c_susp = 4200.0f * suspStiffnessMult;
    float k_tire = 75000.0f;
    float c_tire = 3500.0f;

    auto solveWheel = [&](const Vec2& mount, float& suspLen, float& suspVel,
                          Vec2& wheelPos, Vec2& wheelVel,
                          float& wheelAng, float& wheelAngVel,
                          float driveRatio, bool& contact) {
        // Proposed wheel position along telescopic strut axis
        wheelPos = mount + downDir * suspLen;

        float groundY = terrain.getHeight(wheelPos.x);
        Vec2 normal = terrain.getNormal(wheelPos.x);
        Vec2 tangent(normal.y, -normal.x); // Forward along ground surface

        // Wheel only contacts ground if pointing downwards towards terrain
        bool facingGround = (downDir.y < 0.25f);
        float penetration = facingGround ? ((groundY + m_wheelRadius) - wheelPos.y) : -1.0f;

        // Linear velocity of mount point
        Vec2 mountVel = m_chassisVel + Vec2(-m_chassisAngularVel * (mount.y - m_chassisPos.y),
                                             m_chassisAngularVel * (mount.x - m_chassisPos.x));
        wheelVel = mountVel + downDir * suspVel;

        // Spring force between chassis and wheel along strut
        float deltaX = m_suspRestLength - suspLen;
        float suspForceMag = k_susp * deltaX - c_susp * suspVel;
        if (suspForceMag < 0.0f) suspForceMag = 0.0f; // Doesn't pull inward
        suspForceMag = std::min(suspForceMag, 28000.0f);

        if (penetration > 0.0f) {
            contact = true;

            // Velocity of wheel normal into ground
            float vNorm = Vec2::dot(wheelVel, normal);
            float tireForceMag = std::max(0.0f, k_tire * penetration - c_tire * vNorm);
            tireForceMag = std::min(tireForceMag, 35000.0f);

            // Alignment between strut and ground normal
            float align = std::max(0.2f, Vec2::dot(-downDir, normal));
            float compForce = tireForceMag * align - suspForceMag;

            // Smooth ODE acceleration of suspension strut length
            float suspAcc = -compForce / m_wheelMass;
            suspVel += suspAcc * dt;
            suspVel = std::clamp(suspVel, -15.0f, 15.0f);
            suspVel *= 0.94f; // Numerical damping

            suspLen += suspVel * dt;

            // Bump stop handling
            if (suspLen < m_suspMinLength) {
                float bumpPen = m_suspMinLength - suspLen;
                suspLen = m_suspMinLength;
                if (suspVel < 0.0f) suspVel = 0.0f;
                // Direct upward impulse on chassis from bottoming out
                m_chassisVel += (normal * (bumpPen * 45000.0f / m_chassisMass)) * dt;
            } else if (suspLen > m_suspMaxLength) {
                suspLen = m_suspMaxLength;
                if (suspVel > 0.0f) suspVel = 0.0f;
            }
            wheelPos = mount + downDir * suspLen;

            // Apply suspension force to chassis
            Vec2 suspForceOnChassis = downDir * (-suspForceMag);
            m_chassisVel += (suspForceOnChassis / m_chassisMass) * dt;
            float torque = (mount.x - m_chassisPos.x) * suspForceOnChassis.y - (mount.y - m_chassisPos.y) * suspForceOnChassis.x;
            m_chassisAngularVel += (torque / m_chassisInertia) * dt;

            // Drive Torque & Braking
            float peakTorque = 3400.0f * torqueMult;
            // Roll-off at high speed (~85 km/h = 24 m/s)
            float speedRatio = std::clamp(std::abs(m_chassisVel.x) / 24.0f, 0.0f, 1.0f);
            float currentMaxTorque = peakTorque * (1.0f - 0.55f * speedRatio);

            float driveTorque = effectiveGas * currentMaxTorque * driveRatio;
            if (m_brakeInput > 0.0f) {
                float brakeDir = (wheelAngVel > 0.05f) ? 1.0f : ((wheelAngVel < -0.05f) ? -1.0f : 0.0f);
                driveTorque -= m_brakeInput * 1200.0f * brakeDir;
            }
            wheelAngVel += (driveTorque / m_wheelInertia) * dt;

            // Traction & Friction calculation (Pacejka Curve)
            float surfSpeed = Vec2::dot(wheelVel, tangent);
            float wheelLinSpeed = wheelAngVel * m_wheelRadius;
            float slipSpeed = surfSpeed - wheelLinSpeed;

            float normalForce = suspForceMag + tireForceMag * 0.5f;
            float maxFriction = std::max(350.0f, normalForce * terrain.getFriction() * tireGripMult * 1.45f);

            // Progressive slip curve with sustained arcade climbing traction
            float normSlip = slipSpeed / 2.5f;
            float tractiveRatio = (2.0f * normSlip) / (1.0f + normSlip * normSlip);
            if (std::abs(normSlip) > 1.0f) {
                float sign = (normSlip > 0.0f) ? 1.0f : -1.0f;
                tractiveRatio = sign * std::max(std::abs(tractiveRatio), 0.82f);
            }
            float tractiveForce = -std::clamp(tractiveRatio * maxFriction, -maxFriction, maxFriction);

            // Apply tractive force to chassis at mount point
            Vec2 tractiveVec = tangent * tractiveForce;
            m_chassisVel += (tractiveVec / m_chassisMass) * dt;
            float trqTractive = ((mount.x - m_chassisPos.x) * tractiveVec.y - (mount.y - m_chassisPos.y) * tractiveVec.x) * 0.15f;
            m_chassisAngularVel += (trqTractive / m_chassisInertia) * dt;

            // Wheel rotation reaction from traction
            wheelAngVel -= (tractiveForce * m_wheelRadius / m_wheelInertia) * dt;
        } else {
            contact = false;
            // Smoothly relax towards rest length in air with unconditionally stable damping
            float deltaX = m_suspRestLength - suspLen;
            suspVel += deltaX * 18.0f * dt;
            suspVel *= 0.84f; // Strictly bounded contraction map (< 1.0)
            suspLen += suspVel * dt;
            suspLen = std::clamp(suspLen, m_suspMinLength, m_suspMaxLength);
            wheelPos = mount + downDir * suspLen;
        }

        // Integrate wheel angle
        wheelAng += wheelAngVel * dt;
        wheelAngVel *= 0.997f;
    };

    float rearDriveRatio = 0.65f - 0.15f * fourWdSplit;
    float frontDriveRatio = 0.35f + 0.15f * fourWdSplit;

    solveWheel(rearMount, m_rearSuspensionLength, m_rearSuspVel,
               m_rearWheelPos, m_rearWheelVel,
               m_rearWheelAngle, m_rearWheelAngularVel,
               rearDriveRatio, m_rearContact);

    solveWheel(frontMount, m_frontSuspensionLength, m_frontSuspVel,
               m_frontWheelPos, m_frontWheelVel,
               m_frontWheelAngle, m_frontWheelAngularVel,
               frontDriveRatio, m_frontContact);

    // 3. Stunt & In-Air Pitch Controls
    if (!m_rearContact && !m_frontContact) {
        // In-Air Pitch Control (Signature Hill Climb Racing flip stunt mechanic)
        float airPitchTorque = 680.0f;
        if (effectiveGas > 0.0f) {
            // Leans back (counter-clockwise rotation for backflips)
            m_chassisAngularVel += (airPitchTorque * effectiveGas / m_chassisInertia) * dt;
        }
        if (m_brakeInput > 0.0f) {
            // Leans forward (clockwise rotation for frontflips)
            m_chassisAngularVel -= (airPitchTorque * m_brakeInput / m_chassisInertia) * dt;
        }
    } else if (m_rearContact && !m_frontContact) {
        // Anti-wheelie stabilization when front wheel is airborne
        if (m_chassisAngle > 0.40f && m_chassisAngularVel > 0.0f) {
            m_chassisAngularVel *= std::max(0.0f, 1.0f - 10.0f * dt);
        }
        if (m_chassisAngle > 0.55f) {
            m_chassisAngularVel -= (850.0f * (m_chassisAngle - 0.55f) / m_chassisInertia) * dt;
        }
        // Braking slams the front wheels back down onto the ground
        if (m_brakeInput > 0.0f) {
            m_chassisAngularVel -= (1000.0f * m_brakeInput / m_chassisInertia) * dt;
        }
    } else if (!m_rearContact && m_frontContact) {
        // Stoppie recovery on throttle
        if (effectiveGas > 0.0f) {
            m_chassisAngularVel += (650.0f * effectiveGas / m_chassisInertia) * dt;
        }
    }

    // 4. Chassis Terrain Contact (Prevents undercarriage sinking into the ground)
    m_chassisContact = false;
    Vec2 underbodyPoints[3] = {getMount(-0.85f, -0.14f), getMount(0.0f, -0.14f), getMount(+0.85f, -0.14f)};
    for (int i = 0; i < 3; ++i) {
        float gy = terrain.getHeight(underbodyPoints[i].x);
        if (underbodyPoints[i].y < gy) {
            m_chassisContact = true;
            float pen = gy - underbodyPoints[i].y;
            Vec2 normal = terrain.getNormal(underbodyPoints[i].x);
            m_chassisVel += (normal * (pen * 32000.0f / m_chassisMass)) * dt;

            // Damp downward penetration velocity
            float normVel = Vec2::dot(m_chassisVel, normal);
            if (normVel < 0.0f) {
                m_chassisVel -= normal * (normVel * 0.75f);
            }
        }
    }

    // 5. Integrate Chassis Rigid Body (Symplectic Euler)
    m_chassisVel += (gravForce / m_chassisMass) * dt;
    m_chassisVel *= 0.999f;
    m_chassisAngularVel *= 0.992f;

    m_chassisPos += m_chassisVel * dt;
    m_chassisAngle += m_chassisAngularVel * dt;

    // 6. Driver Head Ragdoll Pendulum
    float headAccX = m_chassisVel.x;
    float k_head = 45.0f;
    float c_head = 6.0f;
    float targetHeadAngle = -headAccX * 0.015f + m_chassisAngularVel * 0.04f;
    float headTorque = -k_head * (m_driverHeadAngle - targetHeadAngle) - c_head * m_driverHeadAngularVel;
    m_driverHeadAngularVel += headTorque * dt;
    m_driverHeadAngle += m_driverHeadAngularVel * dt;

    // 7. Fatal Driver Head Collision Check (Neck Snap!)
    // Only triggered if vehicle has genuinely overturned (roof pointing down) and head hits terrain
    if (m_spawnGraceTimer > 0.0f) {
        m_spawnGraceTimer -= dt;
    } else {
        bool isOverturned = (cosA < -0.35f); // Inverted past ~110 degrees from upright (roof down)
        Vec2 head = driverHeadPos();
        float headGround = terrain.getHeight(head.x);
        if (isOverturned && head.y <= headGround - 0.02f) {
            m_driverDown = true;
        }
    }
}

} // namespace Physics
