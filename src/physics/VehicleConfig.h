#pragma once

#include "PhysicsTypes.h"
#include <cstdint>
#include <string>
#include <vector>

namespace Physics {

enum class VehicleType {
    OFFROADER = 0,          // 1. Classic Off-Roader
    RALLY_CAR,              // 2. Rally Racing Car
    SPORTS_COUPE,           // 3. Modern Sports Coupe
    SUPERCAR,               // 4. Supercar
    PICKUP_TRUCK,           // 5. Heavy-Duty Pickup Truck
    MONSTER_TRUCK,          // 6. Monster Truck
    DESERT_RAID,            // 7. Desert Rally Raid
    FORMULA_RACER,          // 8. Formula Racing Car
    MUSCLE_CAR,             // 9. Retro Muscle Car
    ELECTRIC_OFFROADER,     // 10. Futuristic Electric Off-Roader
    COUNT,

    // Backward-compatibility alias
    SPORTS_CAR = SPORTS_COUPE
};

enum class DriverType {
    BILL = 0,
    SARAH,
    BOB,
    NEIL,
    COUNT
};

struct VehicleConfig {
    VehicleType type;
    std::string id;
    std::string name;
    std::string tagline;
    int unlockCost;

    // Physical dimensions & mass properties
    float mass;               // kg
    float inertia;            // kg * m^2
    float wheelRadius;        // meters
    float wheelMass;          // kg
    float wheelInertia;       // kg * m^2

    // Mount points in chassis local space (meters, center = (0, 0))
    Vec2 rearMountOffset;
    Vec2 frontMountOffset;
    Vec2 driverSeatOffset;

    // Telescopic suspension strut parameters
    float suspRestLength;     // meters
    float suspMinLength;      // meters
    float suspMaxLength;      // meters
    float baseSpringRate;     // N/m
    float baseDamperRate;     // N*s/m

    // Powertrain & dynamics
    float basePeakTorque;     // N*m
    float baseTopSpeedKmh;    // km/h
    float baseGrip;           // friction multiplier
    float base4wdSplit;       // 0.0 = RWD, 0.5 = 50/50 AWD
    float baseBrakingTorque;  // N*m
    float baseDownforce;      // N / (m/s)^2
    float baseAirAgility;     // rotational torque multiplier in air

    // Sprite drawing dimensions (pixels)
    int spriteWidth;
    int spriteHeight;
    Vec2 spriteAnchor;        // pivot point in sprite space
    float wheelSpriteRadius;  // sprite pixel radius
    bool hasIntegratedDriver = true; // concept sheet vehicles have bespoke drivers inside cockpits
};

struct DriverConfig {
    DriverType type;
    std::string id;
    std::string name;
    std::string title;
    std::string bio;
    uint32_t themeColor;
};

class VehicleRegistry {
public:
    static const VehicleConfig& getConfig(VehicleType type);
    static const VehicleConfig& getConfig(const std::string& id);
    static const std::vector<VehicleConfig>& getAllVehicles();

    static const DriverConfig& getDriver(DriverType type);
    static const DriverConfig& getDriver(const std::string& id);
    static const std::vector<DriverConfig>& getAllDrivers();
};

} // namespace Physics
