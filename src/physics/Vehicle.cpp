#include "Vehicle.h"
#include <cmath>
#include <algorithm>

namespace Physics {

Vehicle::Vehicle(VehicleType type, DriverType driver)
    : m_vehicleType(type)
    , m_driverType(driver)
{
    applyConfig();
    reset(5.0f, 1.20f);
}

void Vehicle::setVehicleType(VehicleType type) {
    if (type >= VehicleType::OFFROADER && type < VehicleType::COUNT) {
        m_vehicleType = type;
        applyConfig();
    }
}

void Vehicle::applyConfig() {
    m_config = VehicleRegistry::getConfig(m_vehicleType);

    m_chassisMass = m_config.mass;
    m_chassisInertia = m_config.inertia;

    m_wheelRadius = m_config.wheelRadius;
    m_wheelMass = m_config.wheelMass;
    m_wheelInertia = m_config.wheelInertia;

    m_rearMountOffset = m_config.rearMountOffset;
    m_frontMountOffset = m_config.frontMountOffset;
    m_driverSeatOffset = m_config.driverSeatOffset;

    m_suspRestLength = m_config.suspRestLength;
    m_suspMinLength = m_config.suspMinLength;
    m_suspMaxLength = m_config.suspMaxLength;
}

void Vehicle::reset(float startX, float startY) {
    applyConfig();

    m_chassisPos = {startX, startY};
    m_chassisVel = {0.0f, 0.0f};
    m_prevChassisVel = {0.0f, 0.0f};
    m_chassisAngle = 0.0f;
    m_chassisAngularVel = 0.0f;

    m_rearSuspensionLength = m_suspRestLength;
    m_frontSuspensionLength = m_suspRestLength;
    m_rearSuspVel = 0.0f;
    m_frontSuspVel = 0.0f;

    // Wheel initial positions directly attached to mounts along down axis
    m_rearWheelPos = {startX + m_rearMountOffset.x, startY + m_rearMountOffset.y - m_suspRestLength};
    m_rearWheelVel = {0.0f, 0.0f};
    m_rearWheelAngle = 0.0f;
    m_rearWheelAngularVel = 0.0f;

    m_frontWheelPos = {startX + m_frontMountOffset.x, startY + m_frontMountOffset.y - m_suspRestLength};
    m_frontWheelVel = {0.0f, 0.0f};
    m_frontWheelAngle = 0.0f;
    m_frontWheelAngularVel = 0.0f;

    m_driverHeadAngle = 0.0f;
    m_driverHeadAngularVel = 0.0f;
    m_driverHeadShiftX = 0.0f;
    m_driverHeadShiftY = 0.0f;
    m_driverHeadShiftYVel = 0.0f;

    m_fuel = 100.0f;
    m_gasInput = 0.0f;
    m_brakeInput = 0.0f;

    m_rearContact = true;
    m_frontContact = true;
    m_chassisContact = false;
    m_driverDown = false;
    m_spawnGraceTimer = 3.0f;

    m_rearSlipSpeed = 0.0f;
    m_frontSlipSpeed = 0.0f;
    m_rearNormalForce = 0.0f;
    m_frontNormalForce = 0.0f;
    m_rearContactTangent = {1.0f, 0.0f};
    m_frontContactTangent = {1.0f, 0.0f};
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
    Vec2 localHead = m_driverSeatOffset + Vec2(m_driverHeadShiftX, m_driverHeadShiftY);
    float hx = m_chassisPos.x + (localHead.x * cosA - localHead.y * sinA);
    float hy = m_chassisPos.y + (localHead.x * sinA + localHead.y * cosA);
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
    return std::clamp(idleRpm + wheelRpm * gearRatio + throttleBoost, 1000.0f, 8500.0f);
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
    float torqueMult = profile.getEngineTorqueMultiplier(m_vehicleType);
    float suspStiffnessMult = profile.getSuspensionStiffnessMultiplier(m_vehicleType);
    float tireGripMult = profile.getTireGripMultiplier(m_vehicleType);
    float fourWdAdd = profile.get4wdTorqueSplit(m_vehicleType);
    float brakeMult = profile.getBrakingMultiplier(m_vehicleType);
    float transMult = profile.getTransmissionMultiplier(m_vehicleType);
    float chassisMassMult = profile.getChassisMassMultiplier(m_vehicleType);
    float downforceMult = profile.getDownforceMultiplier(m_vehicleType);

    // Apply mass tuning
    m_chassisMass = m_config.mass * chassisMassMult;
    m_chassisInertia = m_config.inertia * chassisMassMult;

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

    Vec2 rearMount = getMount(m_rearMountOffset.x, m_rearMountOffset.y);
    Vec2 frontMount = getMount(m_frontMountOffset.x, m_frontMountOffset.y);

    // Aerodynamic Downforce (Pushes chassis onto terrain at high speed)
    float speedSq = m_chassisVel.lengthSquared();
    float downforceMag = m_config.baseDownforce * downforceMult * speedSq;
    m_chassisVel += downDir * (downforceMag / m_chassisMass) * dt;

    // 2. Harmonic Suspension & Traction Solver
    float k_susp = m_config.baseSpringRate * suspStiffnessMult;
    float c_susp = m_config.baseDamperRate * std::sqrt(suspStiffnessMult);
    float k_tire = 75000.0f;
    float c_tire = 3500.0f;

    auto solveWheel = [&](const Vec2& mount, float& suspLen, float& suspVel,
                          Vec2& wheelPos, Vec2& wheelVel,
                          float& wheelAng, float& wheelAngVel,
                          float driveRatio, bool& contact,
                          float& outSlipSpeed, float& outNormalForce, Vec2& outTangent) {
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
        if (suspForceMag < 0.0f) suspForceMag = 0.0f;
        suspForceMag = std::min(suspForceMag, 35000.0f);

        if (penetration > 0.0f) {
            contact = true;

            // Velocity of wheel normal into ground
            float vNorm = Vec2::dot(wheelVel, normal);
            float tireForceMag = std::max(0.0f, k_tire * penetration - c_tire * vNorm);
            tireForceMag = std::min(tireForceMag, 45000.0f);

            // Alignment between strut and ground normal
            float align = std::max(0.2f, Vec2::dot(-downDir, normal));
            float compForce = tireForceMag * align - suspForceMag;

            // Smooth ODE acceleration of suspension strut length
            float suspAcc = -compForce / m_wheelMass;
            suspVel += suspAcc * dt;
            suspVel = std::clamp(suspVel, -18.0f, 18.0f);
            suspVel *= 0.94f; // Numerical damping

            suspLen += suspVel * dt;

            // Bump stop handling
            if (suspLen < m_suspMinLength) {
                float bumpPen = m_suspMinLength - suspLen;
                suspLen = m_suspMinLength;
                if (suspVel < 0.0f) suspVel = 0.0f;
                m_chassisVel += (normal * (bumpPen * 50000.0f / m_chassisMass)) * dt;
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
            float peakTorque = m_config.basePeakTorque * torqueMult;
            float topSpeedMs = (m_config.baseTopSpeedKmh * transMult) / 3.6f;
            float speedRatio = std::clamp(std::abs(m_chassisVel.x) / topSpeedMs, 0.0f, 1.0f);
            float currentMaxTorque = peakTorque * (1.0f - 0.45f * speedRatio);

            float driveTorque = effectiveGas * currentMaxTorque * driveRatio;
            if (m_brakeInput > 0.0f) {
                float brakeDir = (wheelAngVel > 0.05f) ? 1.0f : ((wheelAngVel < -0.05f) ? -1.0f : 0.0f);
                driveTorque -= m_brakeInput * (m_config.baseBrakingTorque * brakeMult) * brakeDir;
            }
            wheelAngVel += (driveTorque / m_wheelInertia) * dt;

            // Traction & Friction calculation (Pacejka Curve)
            float surfSpeed = Vec2::dot(wheelVel, tangent);
            float wheelLinSpeed = wheelAngVel * m_wheelRadius;
            float slipSpeed = surfSpeed - wheelLinSpeed;

            float normalForce = suspForceMag + tireForceMag * 0.5f;
            float maxFriction = std::max(350.0f, normalForce * terrain.getFriction() * m_config.baseGrip * tireGripMult * 1.45f);

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

            // Telemetry
            outSlipSpeed = slipSpeed;
            outNormalForce = normalForce;
            outTangent = tangent;
        } else {
            contact = false;
            outSlipSpeed = 0.0f;
            outNormalForce = 0.0f;
            outTangent = {1.0f, 0.0f};

            // Smoothly relax towards rest length in air
            float deltaX = m_suspRestLength - suspLen;
            suspVel += deltaX * 18.0f * dt;
            suspVel *= 0.84f;
            suspLen += suspVel * dt;
            suspLen = std::clamp(suspLen, m_suspMinLength, m_suspMaxLength);
            wheelPos = mount + downDir * suspLen;
        }

        // Integrate wheel angle
        wheelAng += wheelAngVel * dt;
        wheelAngVel *= 0.997f;
    };

    float fourWdSplit = std::clamp(m_config.base4wdSplit + fourWdAdd, 0.0f, 0.50f);
    float rearDriveRatio = 1.0f - fourWdSplit;
    float frontDriveRatio = fourWdSplit;

    solveWheel(rearMount, m_rearSuspensionLength, m_rearSuspVel,
               m_rearWheelPos, m_rearWheelVel,
               m_rearWheelAngle, m_rearWheelAngularVel,
               rearDriveRatio, m_rearContact,
               m_rearSlipSpeed, m_rearNormalForce, m_rearContactTangent);

    solveWheel(frontMount, m_frontSuspensionLength, m_frontSuspVel,
               m_frontWheelPos, m_frontWheelVel,
               m_frontWheelAngle, m_frontWheelAngularVel,
               frontDriveRatio, m_frontContact,
               m_frontSlipSpeed, m_frontNormalForce, m_frontContactTangent);

    // 3. Stunt & In-Air Pitch Controls
    if (!m_rearContact && !m_frontContact) {
        float airPitchTorque = 680.0f * m_config.baseAirAgility * downforceMult;
        if (effectiveGas > 0.0f) {
            m_chassisAngularVel += (airPitchTorque * effectiveGas / m_chassisInertia) * dt;
        }
        if (m_brakeInput > 0.0f) {
            m_chassisAngularVel -= (airPitchTorque * m_brakeInput / m_chassisInertia) * dt;
        }
    } else if (m_rearContact && !m_frontContact) {
        if (m_chassisAngle > 0.40f && m_chassisAngularVel > 0.0f) {
            m_chassisAngularVel *= std::max(0.0f, 1.0f - 10.0f * dt);
        }
        if (m_chassisAngle > 0.55f) {
            m_chassisAngularVel -= (850.0f * (m_chassisAngle - 0.55f) / m_chassisInertia) * dt;
        }
        if (m_brakeInput > 0.0f) {
            m_chassisAngularVel -= (1000.0f * m_brakeInput / m_chassisInertia) * dt;
        }
    } else if (!m_rearContact && m_frontContact) {
        if (effectiveGas > 0.0f) {
            m_chassisAngularVel += (650.0f * effectiveGas / m_chassisInertia) * dt;
        }
    }

    // 4. Chassis Terrain Contact
    m_chassisContact = false;
    float halfWheelbase = std::abs(m_frontMountOffset.x);
    Vec2 underbodyPoints[3] = {
        getMount(-halfWheelbase, m_rearMountOffset.y - 0.08f),
        getMount(0.0f, (m_rearMountOffset.y + m_frontMountOffset.y) * 0.5f - 0.08f),
        getMount(+halfWheelbase, m_frontMountOffset.y - 0.08f)
    };
    for (int i = 0; i < 3; ++i) {
        float gy = terrain.getHeight(underbodyPoints[i].x);
        if (underbodyPoints[i].y < gy) {
            m_chassisContact = true;
            float pen = gy - underbodyPoints[i].y;
            Vec2 normal = terrain.getNormal(underbodyPoints[i].x);
            m_chassisVel += (normal * (pen * 34000.0f / m_chassisMass)) * dt;

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

    // 6. TYRE ALIGNMENT RE-ANCHORING (Guarantees exact alignment under all driving speeds)
    float finalCosA = std::cos(m_chassisAngle);
    float finalSinA = std::sin(m_chassisAngle);
    Vec2 finalDownDir(finalSinA, -finalCosA);
    auto getFinalMount = [&](const Vec2& offset) -> Vec2 {
        return {m_chassisPos.x + (offset.x * finalCosA - offset.y * finalSinA),
                m_chassisPos.y + (offset.x * finalSinA + offset.y * finalCosA)};
    };
    m_rearWheelPos = getFinalMount(m_rearMountOffset) + finalDownDir * m_rearSuspensionLength;
    m_frontWheelPos = getFinalMount(m_frontMountOffset) + finalDownDir * m_frontSuspensionLength;

    // 7. DRIVER INERTIA & DYNAMIC RAGDOLL SYSTEM
    Vec2 linAcc = (m_chassisVel - m_prevChassisVel) / std::max(dt, 0.0001f);
    m_prevChassisVel = m_chassisVel;

    // Project acceleration into chassis local coordinate frame
    float a_long = linAcc.x * finalCosA + linAcc.y * finalSinA;
    float a_vert = -linAcc.x * finalSinA + linAcc.y * finalCosA;

    // Longitudinal lean: forward acceleration leans head back (-angle in screen space), braking leans forward (+angle)
    float targetHeadAngle = -0.045f * a_long + 0.12f * m_chassisAngularVel;
    targetHeadAngle = std::clamp(targetHeadAngle, -0.65f, 0.65f);

    float k_rot = 90.0f;
    float c_rot = 12.0f;
    float torqueRot = -k_rot * (m_driverHeadAngle - targetHeadAngle) - c_rot * m_driverHeadAngularVel;
    m_driverHeadAngularVel += torqueRot * dt;
    m_driverHeadAngle += m_driverHeadAngularVel * dt;

    // Vertical seat compression / bounce: hard landing pushes driver down into seat; airborne floats slightly
    float targetShiftY = -0.0035f * a_vert;
    targetShiftY = std::clamp(targetShiftY, -0.08f, 0.04f);
    float k_vert = 140.0f;
    float c_vert = 15.0f;
    float forceVert = -k_vert * (m_driverHeadShiftY - targetShiftY) - c_vert * m_driverHeadShiftYVel;
    m_driverHeadShiftYVel += forceVert * dt;
    m_driverHeadShiftY += m_driverHeadShiftYVel * dt;
    m_driverHeadShiftY = std::clamp(m_driverHeadShiftY, -0.08f, 0.04f);

    // Longitudinal seat harness slide
    float targetShiftX = -0.0018f * a_long;
    targetShiftX = std::clamp(targetShiftX, -0.04f, 0.04f);
    m_driverHeadShiftX += (targetShiftX - m_driverHeadShiftX) * std::min(1.0f, 15.0f * dt);

    // 8. Fatal Driver Head Collision Check
    if (m_spawnGraceTimer > 0.0f) {
        m_spawnGraceTimer -= dt;
    } else {
        bool isOverturned = (finalCosA < -0.32f);
        Vec2 head = driverHeadPos();
        float headGround = terrain.getHeight(head.x);
        if (isOverturned && head.y <= headGround - 0.02f) {
            m_driverDown = true;
        }
    }
}

} // namespace Physics
