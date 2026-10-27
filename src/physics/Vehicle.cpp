#define _USE_MATH_DEFINES
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

void Vehicle::reset(float startX, float startY, float startAngle) {
    applyConfig();

    m_chassisPos = {startX, startY};
    m_chassisVel = {0.0f, 0.0f};
    m_prevChassisVel = {0.0f, 0.0f};
    m_chassisAngle = startAngle;
    m_chassisAngularVel = 0.0f;

    m_rearSuspensionLength = m_suspRestLength;
    m_frontSuspensionLength = m_suspRestLength;
    m_rearSuspVel = 0.0f;
    m_frontSuspVel = 0.0f;
    m_lastImpactVelocity = 0.0f;

    // Wheel initial positions oriented according to chassis angle
    float cosA = std::cos(startAngle);
    float sinA = std::sin(startAngle);
    auto localToWorld = [&](float lx, float ly) -> Vec2 {
        return {startX + (lx * cosA - ly * sinA),
                startY + (lx * sinA + ly * cosA)};
    };

    m_rearWheelPos = localToWorld(m_rearMountOffset.x, m_rearMountOffset.y - m_suspRestLength);
    m_rearWheelVel = {0.0f, 0.0f};
    m_rearWheelAngle = 0.0f;
    m_rearWheelAngularVel = 0.0f;

    m_frontWheelPos = localToWorld(m_frontMountOffset.x, m_frontMountOffset.y - m_suspRestLength);
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
    m_reverseInput = 0.0f;
    m_tiltLeftInput = 0.0f;
    m_tiltRightInput = 0.0f;

    m_rearContact = true;
    m_frontContact = true;
    m_chassisContact = false;
    m_driverDown = false;
    m_spawnGraceTimer = 3.0f;

    m_rearSlipSpeed = 0.0f;
    m_frontSlipSpeed = 0.0f;
    m_rearContactTangent = {1.0f, 0.0f};
    m_frontContactTangent = {1.0f, 0.0f};

    // State caching initialization for smooth rendering interpolation
    m_prevChassisPos = m_chassisPos;
    m_prevChassisAngle = m_chassisAngle;
    m_prevRearWheelPos = m_rearWheelPos;
    m_prevRearWheelAngle = m_rearWheelAngle;
    m_prevFrontWheelPos = m_frontWheelPos;
    m_prevFrontWheelAngle = m_frontWheelAngle;
    m_prevDriverHeadAngle = m_driverHeadAngle;
    m_prevDriverHeadShiftY = m_driverHeadShiftY;
    m_brakePressure = 0.0f;
}

void Vehicle::applyInput(float gas, float brake, float reverse, float tiltLeft, float tiltRight) {
    m_tiltLeftInput = std::clamp(tiltLeft, 0.0f, 1.0f);
    m_tiltRightInput = std::clamp(tiltRight, 0.0f, 1.0f);
    m_gasInput = std::clamp(gas, 0.0f, 1.0f);
    m_brakeInput = std::clamp(brake, 0.0f, 1.0f);
    m_reverseInput = std::clamp(reverse, 0.0f, 1.0f);
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
    // True calibrated speed: magnitude of velocity vector calibrated to the 0-140 km/h speedometer scale
    return m_chassisVel.length() * 3.6f * SPEED_CALIBRATION;
}

float Vehicle::getEngineRpm() const {
    float wheelRpm = (std::abs(m_rearWheelAngularVel) / (2.0f * static_cast<float>(M_PI))) * 60.0f;
    float idleRpm = 1000.0f;
    float gearRatio = 7.0f;
    float throttleBoost = std::max(m_gasInput, m_reverseInput) * 2500.0f;
    return std::clamp(idleRpm + wheelRpm * gearRatio + throttleBoost, 1000.0f, 8500.0f);
}

void Vehicle::step(float dt, const Terrain& terrain, const Core::ProfileManager& profile) {
    m_lastImpactVelocity = 0.0f;

    // Cache previous physics tick state for rendering interpolation
    m_prevChassisPos = m_chassisPos;
    m_prevChassisAngle = m_chassisAngle;
    m_prevRearWheelPos = m_rearWheelPos;
    m_prevRearWheelAngle = m_rearWheelAngle;
    m_prevFrontWheelPos = m_frontWheelPos;
    m_prevFrontWheelAngle = m_frontWheelAngle;
    m_prevDriverHeadAngle = m_driverHeadAngle;
    m_prevDriverHeadShiftY = m_driverHeadShiftY;

    // When driver is down, engine input is cut to simulate realistic passive wreck tumbling and settling
    float effectiveGas = (!m_driverDown && m_fuel > 0.0f) ? m_gasInput : 0.0f;
    float effectiveReverse = (!m_driverDown && m_fuel > 0.0f) ? m_reverseInput : 0.0f;
    float effectiveBrake = (!m_driverDown) ? m_brakeInput : 0.0f;

    // Smooth progressive hydraulic brake pressure filter (4.5s^-1 attack, 12.0s^-1 release)
    if (effectiveBrake > m_brakePressure) {
        m_brakePressure = std::min(effectiveBrake, m_brakePressure + 4.5f * dt);
    } else {
        m_brakePressure = std::max(effectiveBrake, m_brakePressure - 12.0f * dt);
    }

    // Upgrades Multipliers
    float torqueMult = profile.getEngineTorqueMultiplier(m_vehicleType);
    float suspStiffnessMult = profile.getSuspensionStiffnessMultiplier(m_vehicleType);
    float tireGripMult = profile.getTireGripMultiplier(m_vehicleType);
    float fourWdAdd = profile.get4wdTorqueSplit(m_vehicleType);
    float brakeMult = profile.getBrakingMultiplier(m_vehicleType);
    float transMult = profile.getTransmissionMultiplier(m_vehicleType);
    float chassisMassMult = profile.getChassisMassMultiplier(m_vehicleType);
    float downforceMult = profile.getDownforceMultiplier(m_vehicleType);

    // 1. Drain Fuel
    // Strict rule: Fuel must decrease ONLY while the player actively propels the vehicle (forward throttle OR reverse drive)!
    // Stationary with throttle released: no fuel consumption.
    // Coasting downhill with throttle released: no fuel consumption.
    // Airborne with throttle released: no fuel consumption.
    // Braking with throttle released: no fuel consumption.
    // Throttle / Reverse pressed: consume fuel at a calibrated rate matching distance traveled per tank.
    float activeThrottle = std::max(effectiveGas, effectiveReverse);
    if (m_infiniteFuel) {
        m_fuel = 100.0f;
    } else if (!m_driverDown && m_fuel > 0.0f && activeThrottle > 0.0f) {
        float speedRatio = std::clamp(getSpeedKmh() / (m_config.baseTopSpeedKmh * transMult), 0.0f, 1.2f);
        float slope = terrain.getSlope(m_chassisPos.x);
        float load = std::clamp(0.6f + std::abs(slope) * 1.5f + (1.0f - speedRatio * 0.35f), 0.5f, 1.5f);
        float drainRate = activeThrottle * (3.3f + 0.9f * load) * 0.60f * m_fuelBurnMultiplier;
        m_fuel = std::max(0.0f, m_fuel - drainRate * dt);
    }

    // Apply mass tuning
    m_chassisMass = m_config.mass * chassisMassMult;
    m_chassisInertia = m_config.inertia * chassisMassMult;

    float gravity = (m_customGravity < -0.1f) ? -m_customGravity : terrain.getGravity();
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

    // Aerodynamic Downforce (Pushes chassis onto terrain at high speed, absent in vacuum)
    float speedSq = m_chassisVel.lengthSquared();
    float downforceMag = m_config.baseDownforce * downforceMult * speedSq * terrain.getAirDensity();
    m_chassisVel += downDir * (downforceMag / m_chassisMass) * dt;

    // 2. Calibrated Ground-Hugging Suspension & Traction Solver
    float k_susp = m_config.baseSpringRate * suspStiffnessMult * m_suspStiffnessMultiplier;
    float c_susp = m_config.baseDamperRate * std::sqrt(suspStiffnessMult) * m_suspDampingMultiplier;

    auto solveWheel = [&](const Vec2& mount, float& suspLen, float& suspVel,
                          Vec2& wheelPos, Vec2& wheelVel,
                          float& wheelAng, float& wheelAngVel,
                          float driveRatio, bool& contact,
                          float& outSlipSpeed, float& outNormalForce, Vec2& outTangent) {
        // Evaluate ground interaction along the telescopic suspension strut axis
        wheelPos = mount + downDir * suspLen;

        // Ground sample below current wheel position
        float groundY = terrain.getHeight(wheelPos.x);
        Vec2 normal = terrain.getNormal(wheelPos.x);
        Vec2 tangent(normal.y, -normal.x); // Forward along ground surface

        // Check if strut points downwards towards the ground surface
        float downDotNormal = Vec2::dot(-downDir, normal);
        bool pointingDown = (downDotNormal > 0.15f && downDir.y < -0.10f);

        // Geometric distance from mount point to terrain contact surface (minus tire radius)
        // mount.y + downDir.y * L = groundY + wheelRadius * normal.y
        float L_ground = m_suspMaxLength + 1.0f;
        if (pointingDown) {
            float targetWheelY = groundY + m_wheelRadius * std::max(0.4f, normal.y);
            float deltaY = mount.y - targetWheelY;
            L_ground = deltaY / (-downDir.y);
        }

        // Velocity of mount point on chassis
        Vec2 mountVel = m_chassisVel + Vec2(-m_chassisAngularVel * (mount.y - m_chassisPos.y),
                                             m_chassisAngularVel * (mount.x - m_chassisPos.x));

        // Rate of suspension compression (positive when chassis moves down toward ground)
        float v_comp = Vec2::dot(mountVel, downDir);

        if (pointingDown && L_ground <= m_suspMaxLength) {
            contact = true;

            // Target suspension stroke clamped to physical limits
            float L_target = std::clamp(L_ground, m_suspMinLength, m_suspMaxLength);

            // Dynamic suspension compliance (tracks terrain smoothly without high-frequency oscillations)
            float complianceRate = 48.0f;
            suspLen += (L_target - suspLen) * std::min(1.0f, complianceRate * dt);
            suspVel = -v_comp;

            // Physical spring compression from rest length
            float deltaX = m_suspRestLength - suspLen;
            float springForce = (deltaX > 0.0f) ? (k_susp * deltaX) : 0.0f;

            // Asymmetric damping: compliant in bump (compression), heavy on rebound (2.8x) to prevent catapulting
            float damperForce = 0.0f;
            if (v_comp >= 0.0f) {
                // Compression stroke (bump)
                damperForce = c_susp * (v_comp + 0.10f * v_comp * std::abs(v_comp));
            } else {
                // Rebound stroke: 2.8x damping dissipates stored spring energy and stops repeated bouncing
                damperForce = (2.8f * c_susp) * v_comp;
            }

            float suspForceMag = springForce + damperForce;
            suspForceMag = std::max(0.0f, suspForceMag);

            // Progressive elastomeric bump-stop when fully compressed
            if (L_ground < m_suspMinLength) {
                float bumpPen = m_suspMinLength - L_ground;
                float bumpSpring = k_susp * (2.2f * bumpPen + 25.0f * bumpPen * bumpPen);
                float bumpDamper = 3.5f * c_susp * std::max(0.0f, v_comp);
                suspForceMag += (bumpSpring + bumpDamper);
            }
            suspForceMag = std::min(suspForceMag, 38000.0f);

            // Apply normal ground force to chassis
            Vec2 normalForceVec = normal * (suspForceMag * std::max(0.35f, downDotNormal));
            m_chassisVel += (normalForceVec / m_chassisMass) * dt;
            Vec2 r = mount - m_chassisPos;
            float torqueNormal = r.x * normalForceVec.y - r.y * normalForceVec.x;
            m_chassisAngularVel += (torqueNormal / m_chassisInertia) * dt;

            // Re-evaluate wheel position and velocity
            wheelPos = mount + downDir * suspLen;
            wheelVel = mountVel + downDir * (-v_comp);

            // Drive Torque, Braking, and Calibrated Pacing (Scaled for 60% physical velocity)
            float peakTorque = m_config.basePeakTorque * torqueMult * 0.72f * m_customTorqueMultiplier;
            float topSpeedMs = (m_config.baseTopSpeedKmh * transMult) / (3.6f * SPEED_CALIBRATION);
            float maxDriveOmega = (m_wheelRadius > 0.001f) ? (topSpeedMs / m_wheelRadius) : 100.0f;
            float currentOmega = std::max(0.0f, wheelAngVel);
            float speedRatio = std::clamp(currentOmega / maxDriveOmega, 0.0f, 1.0f);
            float currentMaxTorque = peakTorque * (1.0f - speedRatio * speedRatio);

            // Forward Engine Drive
            if (effectiveGas > 0.0f) {
                float driveTorque = effectiveGas * currentMaxTorque * driveRatio;
                wheelAngVel += (driveTorque / m_wheelInertia) * dt;
                if (wheelAngVel > maxDriveOmega) {
                    wheelAngVel = maxDriveOmega;
                }
            }

            // Dedicated Reverse Engine Drive
            if (effectiveReverse > 0.0f) {
                float maxRevOmega = maxDriveOmega * 0.45f;
                float currentRevOmega = std::max(0.0f, -wheelAngVel);
                float revSpeedRatio = std::clamp(currentRevOmega / maxRevOmega, 0.0f, 1.0f);
                float currentMaxRevTorque = (peakTorque * 0.65f) * (1.0f - revSpeedRatio * revSpeedRatio);
                float revDriveTorque = effectiveReverse * currentMaxRevTorque * driveRatio;
                wheelAngVel -= (revDriveTorque / m_wheelInertia) * dt;
                if (wheelAngVel < -maxRevOmega) {
                    wheelAngVel = -maxRevOmega;
                }
            }

            // Surface velocity relative to ground tangent and slope pull
            float surfSpeed = Vec2::dot(wheelVel, tangent);
            float downslopePull = 0.5f * m_chassisMass * gravity * (-tangent.y);
            const bool braking = (m_brakePressure > 0.001f && effectiveGas < 0.01f && effectiveReverse < 0.01f);

            // Traction & Friction calculation (Pacejka Curve)
            float wheelLinSpeed = wheelAngVel * m_wheelRadius;
            float slipSpeed = surfSpeed - wheelLinSpeed;
            float targetOmega = (m_wheelRadius > 0.001f) ? (surfSpeed / m_wheelRadius) : 0.0f;

            float normalForce = suspForceMag + m_wheelMass * gravity * std::max(0.3f, normal.y);
            float maxFriction = std::max(300.0f, normalForce * terrain.getFriction() * m_config.baseGrip * tireGripMult * 1.35f * m_customFrictionMultiplier);

            // Unified Physics-Based Braking & Traction Model
            // Deceleration is generated entirely through tyre-ground contact forces (tractiveForce)
            // with zero artificial velocity resets.
            float tractiveForce = 0.0f;

            if (braking) {
                // Caliper braking torque applied to this wheel
                float wheelBrakeTorque = 0.65f * m_config.baseBrakingTorque * brakeMult * m_brakePressure;
                float brakeForceDemand = (m_wheelRadius > 0.001f) ? (wheelBrakeTorque / m_wheelRadius) : 0.0f;

                // Caliper torque retards wheel rotation towards zero
                float maxOmegaDot = wheelBrakeTorque / std::max(0.02f, m_wheelInertia);
                float dOmega = maxOmegaDot * dt;
                if (wheelAngVel > 0.0f) {
                    wheelAngVel = std::max(0.0f, wheelAngVel - dOmega);
                } else if (wheelAngVel < 0.0f) {
                    wheelAngVel = std::min(0.0f, wheelAngVel + dOmega);
                }

                // Wheel lockup detection: does braking demand exceed available tyre-ground friction?
                bool wheelLocked = (brakeForceDemand > maxFriction);
                if (!wheelLocked && m_wheelRadius > 0.001f) {
                    // Contact patch reaction torque keeps the tyre rolling with realistic slip
                    float targetOmega = surfSpeed / m_wheelRadius;
                    wheelAngVel += (targetOmega - wheelAngVel) * std::min(1.0f, 25.0f * dt);
                }

                wheelLinSpeed = wheelAngVel * m_wheelRadius;
                slipSpeed = surfSpeed - wheelLinSpeed;

                // Slope holding capacity evaluation (baseline 35 deg limit, expandable with brake upgrades)
                float sinHoldMax = std::min(1.0f, 0.585f * brakeMult);
                float maxHoldForce = 0.5f * m_chassisMass * gravity * sinHoldMax * m_brakePressure;
                maxHoldForce = std::min(maxHoldForce, maxFriction);

                bool isNearRest = (std::abs(surfSpeed) < 0.25f);

                if (isNearRest) {
                    // Near-rest regime: static friction and hill-hold
                    if (std::abs(downslopePull) <= maxHoldForce) {
                        // Slope is within holding capacity: static friction exactly cancels gravity
                        float balanceForce = -downslopePull;
                        // Smoothly damp residual velocity to zero without overshoot or snapping
                        float dampForce = -0.5f * m_chassisMass * (20.0f * surfSpeed);
                        tractiveForce = balanceForce + dampForce;
                        wheelAngVel *= std::max(0.0f, 1.0f - 24.0f * dt);
                        if (std::abs(surfSpeed) < 0.005f) {
                            wheelAngVel = 0.0f;
                        }
                    } else {
                        // Slope exceeds holding capacity: brakes resist up to maximum hold capacity,
                        // producing a smooth, controlled downhill roll under excess gravity.
                        float slideDir = (downslopePull > 0.0f) ? 1.0f : -1.0f;
                        tractiveForce = -slideDir * maxHoldForce;
                        if (m_wheelRadius > 0.001f) {
                            wheelAngVel = (surfSpeed / m_wheelRadius) * 0.40f;
                        }
                    }
                } else {
                    // Dynamic braking regime: contact-patch force opposes longitudinal motion
                    float availBrakeForce = std::min(brakeForceDemand, maxFriction);
                    if (wheelLocked) {
                        // Kinetic sliding friction during wheel lockup
                        availBrakeForce = 0.82f * maxFriction;
                    }
                    // Prevent force from reversing velocity in a single physics timestep
                    float maxStopForce = (0.5f * m_chassisMass * std::abs(surfSpeed)) / dt;
                    float effectiveForce = std::min(availBrakeForce, maxStopForce);
                    tractiveForce = -std::copysign(effectiveForce, surfSpeed);
                }
            } else {
                // Non-braking regime: standard Pacejka traction curve
                float normSlip = slipSpeed / 2.0f;
                float tractiveRatio = (2.0f * normSlip) / (1.0f + normSlip * normSlip);
                if (std::abs(normSlip) > 1.0f) {
                    float sign = (normSlip > 0.0f) ? 1.0f : -1.0f;
                    tractiveRatio = sign * std::max(std::abs(tractiveRatio), 0.88f);
                }
                tractiveForce = -std::clamp(tractiveRatio * maxFriction, -maxFriction, maxFriction);
            }

            // Apply tractive force to chassis at mount point
            Vec2 tractiveVec = tangent * tractiveForce;
            m_chassisVel += (tractiveVec / m_chassisMass) * dt;
            float trqTractive = (r.x * tractiveVec.y - r.y * tractiveVec.x) * 0.16f;
            m_chassisAngularVel += (trqTractive / m_chassisInertia) * dt;

            // Rolling resistance on terrain
            float rollResist = 0.014f * normalForce;
            if (std::abs(surfSpeed) > 0.05f && effectiveGas <= 0.01f && effectiveReverse <= 0.01f && m_brakePressure <= 0.01f) {
                float rollDir = (surfSpeed > 0.0f) ? -1.0f : 1.0f;
                m_chassisVel += tangent * (rollDir * rollResist / m_chassisMass * dt);
            }

            // Wheel rotation reaction from traction (only active when not braking)
            if (!braking) {
                float frictionDeltaOmega = -(tractiveForce * m_wheelRadius / m_wheelInertia) * dt;
                float diffBefore = targetOmega - wheelAngVel;
                wheelAngVel += frictionDeltaOmega;
                float diffAfter = targetOmega - wheelAngVel;

                // Lock to rolling speed when throttle released, braking released, and near synchronous speed
                if (diffBefore * diffAfter < 0.0f && effectiveGas <= 0.05f && effectiveReverse <= 0.05f && m_brakePressure <= 0.05f) {
                    wheelAngVel = targetOmega;
                }
            }

            // Prevent continuous wheel rotation when stationary without genuine wheel spin
            if (std::abs(surfSpeed) < 0.04f && effectiveGas <= 0.0f && effectiveReverse <= 0.0f && m_brakePressure <= 0.0f && std::abs(wheelAngVel) < 0.20f) {
                wheelAngVel = 0.0f;
            }

            // Telemetry
            outSlipSpeed = surfSpeed - wheelLinSpeed;
            outNormalForce = normalForce;
            outTangent = tangent;
        } else {
            contact = false;
            outSlipSpeed = 0.0f;
            outNormalForce = 0.0f;
            outTangent = {1.0f, 0.0f};

            // Smoothly relax towards rest length in air with critical damping
            float deltaX = m_suspRestLength - suspLen;
            suspLen += deltaX * std::min(1.0f, 15.0f * dt);
            suspVel = deltaX * 15.0f;
            wheelPos = mount + downDir * suspLen;
            wheelVel = mountVel + downDir * suspVel;

            // Radial 360-degree tyre terrain collision (prevents tyre penetration when overturned or sideways)
            if (!pointingDown) {
                float wheelGroundY = terrain.getHeight(wheelPos.x);
                Vec2 wheelNormal = terrain.getNormal(wheelPos.x);
                float targetWheelY = wheelGroundY + m_wheelRadius * std::max(0.20f, wheelNormal.y);
                float pen = targetWheelY - wheelPos.y;
                if (pen > 0.0f) {
                    contact = true;
                    Vec2 r = mount - m_chassisPos;
                    Vec2 mountV = m_chassisVel + Vec2(-m_chassisAngularVel * r.y, m_chassisAngularVel * r.x);
                    float normV = Vec2::dot(mountV, wheelNormal);
                    if (normV < 0.0f) {
                        float impactSpeed = -normV;
                        if (impactSpeed > m_lastImpactVelocity) m_lastImpactVelocity = impactSpeed;
                        float rCrossN = r.x * wheelNormal.y - r.y * wheelNormal.x;
                        float Kn = (1.0f / m_chassisMass) + (rCrossN * rCrossN) / m_chassisInertia;
                        float Jn = -(1.05f * normV) / Kn;
                        Jn = std::clamp(Jn, 0.0f, m_chassisMass * 35.0f);
                        m_chassisVel += wheelNormal * (Jn / m_chassisMass);
                        m_chassisAngularVel += (rCrossN * Jn / m_chassisInertia);

                        Vec2 wheelTan(wheelNormal.y, -wheelNormal.x);
                        float tanV = Vec2::dot(mountV, wheelTan);
                        float rCrossT = r.x * wheelTan.y - r.y * wheelTan.x;
                        float Kt = (1.0f / m_chassisMass) + (rCrossT * rCrossT) / m_chassisInertia;
                        float Jt = -std::clamp(tanV / Kt, -Jn * 0.75f, Jn * 0.75f);
                        m_chassisVel += wheelTan * (Jt / m_chassisMass);
                        m_chassisAngularVel += (rCrossT * Jt / m_chassisInertia);
                    }
                    m_chassisPos += wheelNormal * (pen * 0.50f);
                    wheelPos.y = targetWheelY;
                    wheelVel = mountV;
                    outNormalForce = m_wheelMass * gravity * 1.5f;
                    outTangent = {wheelNormal.y, -wheelNormal.x};
                }
            }

            // Airborne wheel behavior: throttle spins free wheels; reverse spins backward; brake stops them; idle spins down smoothly
            if (effectiveGas > 0.0f) {
                float topSpeedMs = (m_config.baseTopSpeedKmh * transMult) / 3.6f;
                float freeWheelTorque = m_config.basePeakTorque * torqueMult * 0.35f;
                wheelAngVel += (effectiveGas * freeWheelTorque * driveRatio / m_wheelInertia) * dt;
                float maxFreeOmega = (topSpeedMs * 1.25f) / m_wheelRadius;
                wheelAngVel = std::min(wheelAngVel, maxFreeOmega);
            } else if (effectiveReverse > 0.0f) {
                float topSpeedMs = (m_config.baseTopSpeedKmh * transMult) / 3.6f;
                float freeWheelTorque = m_config.basePeakTorque * torqueMult * 0.35f;
                wheelAngVel -= (effectiveReverse * freeWheelTorque * driveRatio / m_wheelInertia) * dt;
                float maxFreeRevOmega = (topSpeedMs * 0.60f) / m_wheelRadius;
                wheelAngVel = std::max(wheelAngVel, -maxFreeRevOmega);
            } else if (effectiveBrake > 0.0f) {
                float airBrakeTorque = m_config.baseBrakingTorque * brakeMult * m_brakePressure;
                float dOmega = (airBrakeTorque / std::max(0.05f, m_wheelInertia)) * dt;
                if (wheelAngVel > 0.0f) {
                    wheelAngVel = std::max(0.0f, wheelAngVel - dOmega);
                } else if (wheelAngVel < 0.0f) {
                    wheelAngVel = std::min(0.0f, wheelAngVel + dOmega);
                }
            } else {
                wheelAngVel *= std::clamp(1.0f - 3.5f * dt, 0.0f, 1.0f);
                if (std::abs(wheelAngVel) < 0.05f) wheelAngVel = 0.0f;
            }
        }

        // Integrate wheel angle
        wheelAng += wheelAngVel * dt;
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
        float airPitchTorque = 780.0f * m_config.baseAirAgility * downforceMult * m_customAirGyroMultiplier;
        float pitchBack = std::max(effectiveGas, m_tiltLeftInput);
        float pitchForward = std::max(effectiveBrake, m_tiltRightInput);
        if (pitchBack > 0.0f) {
            m_chassisAngularVel += (airPitchTorque * pitchBack / m_chassisInertia) * dt;
        }
        if (pitchForward > 0.0f) {
            m_chassisAngularVel -= (airPitchTorque * pitchForward / m_chassisInertia) * dt;
        }
    } else {
        // Dynamic Weight Transfer (Poki/Fingersoft authentic squat & dive)
        if (effectiveGas > 0.05f) {
            float squatTorque = 360.0f * effectiveGas * m_customTorqueMultiplier;
            m_chassisAngularVel += (squatTorque / m_chassisInertia) * dt;
        } else if (effectiveBrake > 0.05f && getSpeedKmh() > 2.0f) {
            float diveTorque = -420.0f * effectiveBrake;
            m_chassisAngularVel += (diveTorque / m_chassisInertia) * dt;
        }

        if (m_rearContact && !m_frontContact) {
            if (m_chassisAngle > 0.40f && m_chassisAngularVel > 0.0f) {
                m_chassisAngularVel *= std::max(0.0f, 1.0f - 10.0f * dt);
            }
            if (m_chassisAngle > 0.55f) {
                m_chassisAngularVel -= (850.0f * (m_chassisAngle - 0.55f) / m_chassisInertia) * dt;
            }
            if (effectiveBrake > 0.0f) {
                m_chassisAngularVel -= (1000.0f * effectiveBrake / m_chassisInertia) * dt;
            }
        } else if (!m_rearContact && m_frontContact) {
            if (effectiveGas > 0.0f) {
                m_chassisAngularVel += (650.0f * effectiveGas / m_chassisInertia) * dt;
            }
        }
    }

    // 4. Chassis Terrain Contact & 360-Degree Silhouette Collision
    m_chassisContact = false;
    float lx_min = -m_config.spriteAnchor.x / 80.25f;
    float lx_max = (m_config.spriteWidth - m_config.spriteAnchor.x) / 80.25f;
    float ly_max = m_config.spriteAnchor.y / 80.25f;
    float ly_min = -(m_config.spriteHeight - m_config.spriteAnchor.y) / 80.25f;
    float skidY = std::min(-0.06f, ly_min - 0.04f);
    float midRoofY = ly_max;
    float hoodY = skidY + (ly_max - skidY) * 0.45f;
    float trunkY = skidY + (ly_max - skidY) * 0.50f;

    struct HullPoint {
        Vec2 localPos;
        float radius;
        bool isRoof;
    };

    const HullPoint hull[14] = {
        // Front Bumper / Nose
        {{lx_max - 0.02f, skidY + 0.08f}, 0.05f, false},                     // 0. Front lower bumper
        {{lx_max, skidY + 0.20f}, 0.05f, false},                            // 1. Front nose tip
        {{lx_max - 0.10f, hoodY}, 0.05f, false},                            // 2. Front hood leading edge

        // Cabin / Roof / Pillars
        {{m_frontMountOffset.x * 0.70f, hoodY + (midRoofY - hoodY) * 0.5f}, 0.05f, true}, // 3. Windshield base
        {{m_frontMountOffset.x * 0.45f, midRoofY - 0.02f}, 0.05f, true},    // 4. Roof front / A-pillar top
        {{m_driverSeatOffset.x, midRoofY}, 0.05f, true},                    // 5. Roof peak / center
        {{m_rearMountOffset.x * 0.55f, midRoofY - 0.02f}, 0.05f, true},     // 6. Roof rear / C-pillar top
        {{m_driverSeatOffset.x + m_driverHeadShiftX, m_driverSeatOffset.y + 0.18f + m_driverHeadShiftY}, 0.06f, true}, // 7. Driver helmet

        // Rear Bumper / Tail
        {{m_rearMountOffset.x * 0.85f, trunkY}, 0.05f, false},              // 8. Rear trunk / spoiler
        {{lx_min, skidY + 0.20f}, 0.05f, false},                            // 9. Rear tail tip
        {{lx_min + 0.02f, skidY + 0.08f}, 0.05f, false},                    // 10. Rear lower bumper

        // Underbody Skidplate
        {{lx_min + 0.20f, skidY}, 0.04f, false},                            // 11. Rear overhang underbody
        {{0.0f, skidY - 0.02f}, 0.04f, false},                              // 12. Mid chassis belly plate
        {{lx_max - 0.20f, skidY}, 0.04f, false}                             // 13. Front overhang underbody
    };

    // Evaluate dynamic hull collision impulses
    for (int i = 0; i < 14; ++i) {
        Vec2 pt = getMount(hull[i].localPos.x, hull[i].localPos.y);
        float gy = terrain.getHeight(pt.x);
        float pen = gy - (pt.y - hull[i].radius);
        if (pen > 0.0f) {
            m_chassisContact = true;
            Vec2 normal = terrain.getNormal(pt.x);
            Vec2 tangent(normal.y, -normal.x);
            Vec2 r = pt - m_chassisPos;

            // Velocity of contact point
            Vec2 ptVel = m_chassisVel + Vec2(-m_chassisAngularVel * r.y, m_chassisAngularVel * r.x);
            float normVel = Vec2::dot(ptVel, normal);

            // Inward velocity impact absorption
            if (normVel < 0.0f) {
                float impactSpeed = -normVel;
                if (impactSpeed > m_lastImpactVelocity) m_lastImpactVelocity = impactSpeed;
                float rCrossN = r.x * normal.y - r.y * normal.x;
                float Kn = (1.0f / m_chassisMass) + (rCrossN * rCrossN) / m_chassisInertia;
                float e = (std::abs(normVel) > 1.2f) ? 0.08f : 0.0f; // Plastic stop for low speed, slight restitution for hard impacts
                float Jn = -(1.0f + e) * normVel / Kn;
                Jn = std::clamp(Jn, 0.0f, m_chassisMass * 35.0f);

                m_chassisVel += normal * (Jn / m_chassisMass);
                m_chassisAngularVel += (rCrossN * Jn / m_chassisInertia);

                // Tangential friction along surface
                float surfVel = Vec2::dot(ptVel, tangent);
                float rCrossT = r.x * tangent.y - r.y * tangent.x;
                float Kt = (1.0f / m_chassisMass) + (rCrossT * rCrossT) / m_chassisInertia;
                float mu = terrain.getFriction() * 0.80f;
                float Jt = -std::clamp(surfVel / Kt, -mu * Jn, mu * Jn);

                m_chassisVel += tangent * (Jt / m_chassisMass);
                m_chassisAngularVel += (rCrossT * Jt / m_chassisInertia);
            }

            // Direct smooth Baumgarte position resolution
            m_chassisPos += normal * (pen * 0.50f);
        }
    }

    // 5. Directional Aerodynamic Drag & Rigid Body Integration (Symplectic Euler)
    float speed = m_chassisVel.length();
    float airDensity = terrain.getAirDensity();

    if (airDensity > 0.0001f && speed > 0.05f) {
        // Vehicle longitudinal (front) and transverse (roof) unit vectors
        Vec2 u_long(cosA, sinA);
        Vec2 u_perp(-sinA, cosA);

        // Project velocity onto vehicle body frame
        float v_long = Vec2::dot(m_chassisVel, u_long);
        float v_perp = Vec2::dot(m_chassisVel, u_perp);

        // Vehicle aerodynamic profile factor
        float vehDragScale = 1.0f;
        switch (m_vehicleType) {
            case VehicleType::FORMULA_RACER:      vehDragScale = 0.70f; break; // Streamlined aero monocoque
            case VehicleType::SUPERCAR:           vehDragScale = 0.78f; break; // Ultra-low Cd
            case VehicleType::SPORTS_COUPE:       vehDragScale = 0.85f; break; // Aerodynamic sports silhouette
            case VehicleType::RALLY_CAR:          vehDragScale = 0.90f; break; // Rally hatchback profile
            case VehicleType::ELECTRIC_OFFROADER: vehDragScale = 1.00f; break; // Balanced offroader
            case VehicleType::OFFROADER:          vehDragScale = 1.10f; break; // Classic boxy trail rig
            case VehicleType::MUSCLE_CAR:         vehDragScale = 1.15f; break; // Wide front fascia
            case VehicleType::DESERT_RAID:        vehDragScale = 1.20f; break; // Rugged trophy truck
            case VehicleType::PICKUP_TRUCK:       vehDragScale = 1.30f; break; // High frontal area
            case VehicleType::MONSTER_TRUCK:      vehDragScale = 1.45f; break; // Huge wheels & lifted chassis
            default: vehDragScale = 1.0f; break;
        }

        // Physically grounded aerodynamic drag acceleration:
        // Streamlined frontal drag vs broadside belly/roof drag (2.8x higher)
        float k_long = 0.009f * vehDragScale * airDensity;
        float k_perp = 0.025f * vehDragScale * airDensity;

        Vec2 dragAcc = u_long * (-k_long * std::abs(v_long) * v_long)
                     + u_perp * (-k_perp * std::abs(v_perp) * v_perp);

        // Unconditionally preserve numerical stability
        float maxDragAcc = speed / dt;
        if (dragAcc.length() > maxDragAcc) {
            dragAcc = dragAcc.normalized() * maxDragAcc;
        }
        m_chassisVel += dragAcc * dt;
    }

    m_chassisVel += (gravForce / m_chassisMass) * dt;
    m_chassisVel *= 0.9995f;

    // In-air rotational damping and aerodynamic attitude stability
    if (!m_rearContact && !m_frontContact) {
        if (airDensity > 0.0001f) {
            // Aerodynamic pitch damping (opposes rotation as bumpers sweep through atmosphere)
            float aeroPitchDamp = (0.75f + 0.06f * speed) * airDensity;
            m_chassisAngularVel *= std::max(0.0f, 1.0f - aeroPitchDamp * dt);

            // Aerodynamic weather-vane torque aligns vehicle nose along velocity vector
            if (speed > 2.0f) {
                Vec2 u_long(cosA, sinA);
                Vec2 u_perp(-sinA, cosA);
                float v_long = Vec2::dot(m_chassisVel, u_long);
                float v_perp = Vec2::dot(m_chassisVel, u_perp);
                float angleOfAttack = std::atan2(v_perp, std::max(1.0f, v_long));
                float weatherVaneTorque = -angleOfAttack * (0.35f + 0.04f * speed) * airDensity;
                m_chassisAngularVel += weatherVaneTorque * dt;
            }
        } else {
            // Strict lunar vacuum: zero air resistance, zero aerodynamic attitude torque!
            // Full conservation of angular momentum in vacuum for multi-flips
            m_chassisAngularVel *= (1.0f - 0.04f * dt);
        }
    } else {
        m_chassisAngularVel *= 0.994f;
    }

    m_chassisPos += m_chassisVel * dt;
    m_chassisAngle += m_chassisAngularVel * dt;

    // Post-Integration Rigid-Body Position Projection (Constraint Solver)
    // Eliminates all residual penetration so the chassis and wheels strictly rest ON the terrain surface
    for (int pass = 0; pass < 2; ++pass) {
        float curCos = std::cos(m_chassisAngle);
        float curSin = std::sin(m_chassisAngle);
        auto getCurMount = [&](float lx, float ly) -> Vec2 {
            return {m_chassisPos.x + (lx * curCos - ly * curSin),
                    m_chassisPos.y + (lx * curSin + ly * curCos)};
        };

        // Project chassis hull points
        for (int i = 0; i < 14; ++i) {
            Vec2 pt = getCurMount(hull[i].localPos.x, hull[i].localPos.y);
            float gy = terrain.getHeight(pt.x);
            float pen = gy - (pt.y - hull[i].radius);
            if (pen > 0.0005f) {
                Vec2 norm = terrain.getNormal(pt.x);
                Vec2 r = pt - m_chassisPos;
                float rCrossN = r.x * norm.y - r.y * norm.x;
                float denom = (1.0f / m_chassisMass) + (rCrossN * rCrossN) / m_chassisInertia;
                if (denom > 0.0001f) {
                    float impulse = (pen * 0.65f) / denom;
                    m_chassisPos += norm * (impulse / m_chassisMass);
                    float deltaTheta = (rCrossN * impulse / m_chassisInertia);
                    m_chassisAngle += std::clamp(deltaTheta, -0.05f, 0.05f);
                }
            }
        }

        // Project wheel contact envelopes (active when overturned or when suspension is bottomed out)
        Vec2 downD(curSin, -curCos);
        auto checkWheelProjection = [&](const Vec2& mountOff, float suspL, float rWheel) {
            bool overturned = (curCos < 0.20f);
            bool bottomedOut = (suspL <= m_suspMinLength + 0.008f);
            if (!overturned && !bottomedOut) return;

            Vec2 m = getCurMount(mountOff.x, mountOff.y);
            Vec2 w = m + downD * suspL;
            float gy = terrain.getHeight(w.x);
            Vec2 norm = terrain.getNormal(w.x);
            float targetY = gy + rWheel * std::max(0.20f, norm.y);
            float pen = targetY - w.y;
            if (pen > 0.0005f) {
                Vec2 r = m - m_chassisPos;
                float rCrossN = r.x * norm.y - r.y * norm.x;
                float denom = (1.0f / m_chassisMass) + (rCrossN * rCrossN) / m_chassisInertia;
                if (denom > 0.0001f) {
                    float impulse = (pen * 0.65f) / denom;
                    m_chassisPos += norm * (impulse / m_chassisMass);
                    float deltaTheta = (rCrossN * impulse / m_chassisInertia);
                    m_chassisAngle += std::clamp(deltaTheta, -0.04f, 0.04f);
                }
            }
        };
        checkWheelProjection(m_rearMountOffset, m_rearSuspensionLength, m_wheelRadius);
        checkWheelProjection(m_frontMountOffset, m_frontSuspensionLength, m_wheelRadius);
    }

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

    // Ensure tyre circles never penetrate terrain surface under any rotation or compression
    Vec2 rMount = getFinalMount(m_rearMountOffset);
    float rearGroundY = terrain.getHeight(m_rearWheelPos.x);
    float rearTargetY = rearGroundY + m_wheelRadius * std::max(0.20f, terrain.getNormal(m_rearWheelPos.x).y);
    if (m_rearWheelPos.y < rearTargetY) {
        if (finalDownDir.y < -0.20f) {
            float neededL = (rMount.y - rearTargetY) / (-finalDownDir.y);
            m_rearSuspensionLength = std::clamp(neededL, m_suspMinLength, m_suspMaxLength);
            m_rearWheelPos = rMount + finalDownDir * m_rearSuspensionLength;
        }
        if (m_rearWheelPos.y < rearTargetY) {
            m_rearWheelPos.y = rearTargetY;
        }
    }

    Vec2 fMount = getFinalMount(m_frontMountOffset);
    float frontGroundY = terrain.getHeight(m_frontWheelPos.x);
    float frontTargetY = frontGroundY + m_wheelRadius * std::max(0.20f, terrain.getNormal(m_frontWheelPos.x).y);
    if (m_frontWheelPos.y < frontTargetY) {
        if (finalDownDir.y < -0.20f) {
            float neededL = (fMount.y - frontTargetY) / (-finalDownDir.y);
            m_frontSuspensionLength = std::clamp(neededL, m_suspMinLength, m_suspMaxLength);
            m_frontWheelPos = fMount + finalDownDir * m_frontSuspensionLength;
        }
        if (m_frontWheelPos.y < frontTargetY) {
            m_frontWheelPos.y = frontTargetY;
        }
    }

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

    // 8. Fatal Driver Head & Rollover Crash Check
    if (m_spawnGraceTimer > 0.0f) {
        m_spawnGraceTimer -= dt;
    } else if (!m_driverDown) {
        bool isOverturned = (finalCosA < -0.15f); // Tilted past ~98 degrees
        Vec2 head = driverHeadPos();
        float headGround = terrain.getHeight(head.x);

        // Check if any roof, rollcage, or helmet point has made contact with terrain
        bool roofGroundContact = false;
        for (int i = 0; i < 14; ++i) {
            if (hull[i].isRoof) {
                Vec2 p = getFinalMount(hull[i].localPos);
                float gy = terrain.getHeight(p.x);
                if (p.y - hull[i].radius <= gy + 0.04f) {
                    roofGroundContact = true;
                    break;
                }
            }
        }

        if (isOverturned && (roofGroundContact || head.y <= headGround + 0.04f)) {
            m_driverDown = true;
        }

        // Violent high-speed roof impact while severely tilted
        if (finalCosA < 0.20f && roofGroundContact && m_chassisVel.length() > 6.0f) {
            m_driverDown = true;
        }
    }

    // Resting stabilization: eliminate micro-jitter when vehicle is at rest on terrain
    bool atRestChassis = m_chassisContact && m_chassisVel.length() < 0.08f && std::abs(m_chassisAngularVel) < 0.08f;
    float sinSlope = std::abs(terrain.getNormal(m_chassisPos.x).x);
    float sinHoldMax = std::min(1.0f, 0.585f * brakeMult);
    bool slopeCanHold = (sinSlope <= sinHoldMax + 0.02f);
    bool atRestBraking = (m_rearContact || m_frontContact) && m_brakePressure > 0.3f
                         && m_chassisVel.length() < 0.01f && std::abs(m_chassisAngularVel) < 0.02f
                         && slopeCanHold;
    if ((atRestChassis || atRestBraking) && effectiveGas <= 0.01f && effectiveReverse <= 0.01f) {
        m_chassisVel = {0.0f, 0.0f};
        m_chassisAngularVel = 0.0f;
    }
}

float Vehicle::getMaxTerrainPenetration(const Terrain& terrain) const {
    float maxPen = 0.0f;
    float cosA = std::cos(m_chassisAngle);
    float sinA = std::sin(m_chassisAngle);
    auto getM = [&](float lx, float ly) -> Vec2 {
        return {m_chassisPos.x + (lx * cosA - ly * sinA),
                m_chassisPos.y + (lx * sinA + ly * cosA)};
    };

    float lx_min = -m_config.spriteAnchor.x / 80.25f;
    float lx_max = (m_config.spriteWidth - m_config.spriteAnchor.x) / 80.25f;
    float ly_max = m_config.spriteAnchor.y / 80.25f;
    float ly_min = -(m_config.spriteHeight - m_config.spriteAnchor.y) / 80.25f;
    float skidY = std::min(-0.06f, ly_min - 0.04f);
    float midRoofY = ly_max;
    float hoodY = skidY + (ly_max - skidY) * 0.45f;
    float trunkY = skidY + (ly_max - skidY) * 0.50f;

    Vec2 pts[14] = {
        {lx_max - 0.02f, skidY + 0.08f},
        {lx_max, skidY + 0.20f},
        {lx_max - 0.10f, hoodY},
        {m_frontMountOffset.x * 0.70f, hoodY + (midRoofY - hoodY) * 0.5f},
        {m_frontMountOffset.x * 0.45f, midRoofY - 0.02f},
        {m_driverSeatOffset.x, midRoofY},
        {m_rearMountOffset.x * 0.55f, midRoofY - 0.02f},
        {m_driverSeatOffset.x + m_driverHeadShiftX, m_driverSeatOffset.y + 0.18f + m_driverHeadShiftY},
        {m_rearMountOffset.x * 0.85f, trunkY},
        {lx_min, skidY + 0.20f},
        {lx_min + 0.02f, skidY + 0.08f},
        {lx_min + 0.20f, skidY},
        {0.0f, skidY - 0.02f},
        {lx_max - 0.20f, skidY}
    };

    for (int i = 0; i < 14; ++i) {
        Vec2 p = getM(pts[i].x, pts[i].y);
        float gy = terrain.getHeight(p.x);
        float pen = gy - p.y;
        if (pen > maxPen) maxPen = pen;
    }

    float rearGy = terrain.getHeight(m_rearWheelPos.x);
    float rearPen = (rearGy + m_wheelRadius) - m_rearWheelPos.y;
    if (rearPen > maxPen) maxPen = rearPen;

    float frontGy = terrain.getHeight(m_frontWheelPos.x);
    float frontPen = (frontGy + m_wheelRadius) - m_frontWheelPos.y;
    if (frontPen > maxPen) maxPen = frontPen;

    return maxPen;
}

Vec2 Vehicle::interpolatedChassisPos(float alpha) const {
    alpha = std::clamp(alpha, 0.0f, 1.0f);
    return {m_prevChassisPos.x + (m_chassisPos.x - m_prevChassisPos.x) * alpha,
            m_prevChassisPos.y + (m_chassisPos.y - m_prevChassisPos.y) * alpha};
}

Vec2 Vehicle::interpolatedRearWheelPos(float alpha) const {
    alpha = std::clamp(alpha, 0.0f, 1.0f);
    return {m_prevRearWheelPos.x + (m_rearWheelPos.x - m_prevRearWheelPos.x) * alpha,
            m_prevRearWheelPos.y + (m_rearWheelPos.y - m_prevRearWheelPos.y) * alpha};
}

Vec2 Vehicle::interpolatedFrontWheelPos(float alpha) const {
    alpha = std::clamp(alpha, 0.0f, 1.0f);
    return {m_prevFrontWheelPos.x + (m_frontWheelPos.x - m_prevFrontWheelPos.x) * alpha,
            m_prevFrontWheelPos.y + (m_frontWheelPos.y - m_prevFrontWheelPos.y) * alpha};
}

float Vehicle::interpolatedChassisAngle(float alpha) const {
    alpha = std::clamp(alpha, 0.0f, 1.0f);
    float diff = std::remainder(m_chassisAngle - m_prevChassisAngle, 2.0f * M_PI);
    return m_prevChassisAngle + diff * alpha;
}

float Vehicle::interpolatedRearWheelAngle(float alpha) const {
    alpha = std::clamp(alpha, 0.0f, 1.0f);
    float diff = std::remainder(m_rearWheelAngle - m_prevRearWheelAngle, 2.0f * M_PI);
    return m_prevRearWheelAngle + diff * alpha;
}

float Vehicle::interpolatedFrontWheelAngle(float alpha) const {
    alpha = std::clamp(alpha, 0.0f, 1.0f);
    float diff = std::remainder(m_frontWheelAngle - m_prevFrontWheelAngle, 2.0f * M_PI);
    return m_prevFrontWheelAngle + diff * alpha;
}

float Vehicle::interpolatedDriverHeadAngle(float alpha) const {
    alpha = std::clamp(alpha, 0.0f, 1.0f);
    float diff = std::remainder(m_driverHeadAngle - m_prevDriverHeadAngle, 2.0f * M_PI);
    return m_prevDriverHeadAngle + diff * alpha;
}

Vec2 Vehicle::interpolatedDriverHeadPos(float alpha) const {
    float a = std::clamp(alpha, 0.0f, 1.0f);
    float ang = interpolatedChassisAngle(a);
    Vec2 cPos = interpolatedChassisPos(a);
    float cosA = std::cos(ang);
    float sinA = std::sin(ang);
    float shiftY = m_prevDriverHeadShiftY + (m_driverHeadShiftY - m_prevDriverHeadShiftY) * a;
    Vec2 localHead = m_driverSeatOffset + Vec2(m_driverHeadShiftX, shiftY);
    return {cPos.x + (localHead.x * cosA - localHead.y * sinA),
            cPos.y + (localHead.x * sinA + localHead.y * cosA)};
}

} // namespace Physics
