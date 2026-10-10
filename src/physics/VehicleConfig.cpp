#include "VehicleConfig.h"
#include <stdexcept>

namespace Physics {

static const std::vector<VehicleConfig> s_vehicles = {
    // 1. Classic Off-Roader
    {
        VehicleType::OFFROADER,
        "offroader",
        "CLASSIC OFF-ROADER",
        "Iconic balanced all-terrain utility trail rig",
        0, // Free starter vehicle
        420.0f,               // mass (kg)
        250.0f,               // inertia (kg*m^2)
        0.268f,               // wheelRadius (meters)
        16.0f,                // wheelMass
        0.55f,                // wheelInertia (0.5 * m * r^2)
        {-0.592f, 0.0f},      // rearMountOffset
        {+0.592f, 0.0f},      // frontMountOffset
        {-0.01f, +0.28f},     // driverSeatOffset
        0.18f,                // suspRestLength
        0.08f,                // suspMinLength
        0.32f,                // suspMaxLength
        38000.0f,             // baseSpringRate
        3600.0f,              // baseDamperRate
        3800.0f,              // basePeakTorque
        85.0f,                // baseTopSpeedKmh
        1.05f,                // baseGrip
        0.50f,                // base4wdSplit
        1400.0f,              // baseBrakingTorque
        0.12f,                // baseDownforce
        1.00f,                // baseAirAgility
        190, 70,              // spriteWidth, spriteHeight
        {103.50f, 72.00f},    // spriteAnchor
        21.5f                 // wheelSpriteRadius
    },

    // 2. Rally Racing Car
    {
        VehicleType::RALLY_CAR,
        "rallycar",
        "RALLY RACING CAR",
        "AWD turbocharged hatch with razor agility & high air control",
        5000,
        380.0f,               // mass
        220.0f,               // inertia
        0.243f,               // wheelRadius
        14.0f,                // wheelMass
        0.39f,                // wheelInertia
        {-0.617f, 0.0f},      // rearMountOffset
        {+0.617f, 0.0f},      // frontMountOffset
        {+0.05f, +0.22f},     // driverSeatOffset
        0.15f,                // suspRestLength
        0.06f,                // suspMinLength
        0.28f,                // suspMaxLength
        46000.0f,             // baseSpringRate
        4400.0f,              // baseDamperRate
        4400.0f,              // basePeakTorque
        118.0f,               // baseTopSpeedKmh
        1.25f,                // baseGrip
        0.50f,                // base4wdSplit
        2000.0f,              // baseBrakingTorque
        0.42f,                // baseDownforce
        1.45f,                // baseAirAgility
        185, 58,              // spriteWidth, spriteHeight
        {94.50f, 62.00f},     // spriteAnchor
        19.5f                 // wheelSpriteRadius
    },

    // 3. Modern Sports Coupe
    {
        VehicleType::SPORTS_COUPE,
        "sportscoupe",
        "MODERN SPORTS COUPE",
        "Sleek front-engine twin-turbo rear-wheel drive grand tourer",
        7500,
        430.0f,               // mass
        240.0f,               // inertia
        0.231f,               // wheelRadius
        13.0f,                // wheelMass
        0.33f,                // wheelInertia
        {-0.654f, 0.0f},      // rearMountOffset
        {+0.654f, 0.0f},      // frontMountOffset
        {-0.02f, +0.18f},     // driverSeatOffset
        0.14f,                // suspRestLength
        0.05f,                // suspMinLength
        0.24f,                // suspMaxLength
        48000.0f,             // baseSpringRate
        4600.0f,              // baseDamperRate
        4500.0f,              // basePeakTorque
        138.0f,               // baseTopSpeedKmh
        1.15f,                // baseGrip
        0.00f,                // base4wdSplit (pure RWD)
        2100.0f,              // baseBrakingTorque
        0.35f,                // baseDownforce
        1.15f,                // baseAirAgility
        185, 49,              // spriteWidth, spriteHeight
        {91.50f, 49.00f},     // spriteAnchor
        18.5f                 // wheelSpriteRadius
    },

    // 4. Supercar
    {
        VehicleType::SUPERCAR,
        "supercar",
        "VENOM HYPER-V12",
        "Ultra-low mid-engine carbon hypercar with immense downforce",
        18000,
        340.0f,               // mass
        200.0f,               // inertia
        0.231f,               // wheelRadius
        12.0f,                // wheelMass
        0.30f,                // wheelInertia
        {-0.692f, 0.0f},      // rearMountOffset
        {+0.692f, 0.0f},      // frontMountOffset
        {+0.04f, +0.18f},     // driverSeatOffset
        0.13f,                // suspRestLength
        0.05f,                // suspMinLength
        0.22f,                // suspMaxLength
        52000.0f,             // baseSpringRate
        5000.0f,              // baseDamperRate
        5200.0f,              // basePeakTorque
        165.0f,               // baseTopSpeedKmh
        1.30f,                // baseGrip
        0.30f,                // base4wdSplit
        2600.0f,              // baseBrakingTorque
        0.65f,                // baseDownforce
        1.20f,                // baseAirAgility
        185, 48,              // spriteWidth, spriteHeight
        {89.50f, 49.00f},     // spriteAnchor
        18.5f                 // wheelSpriteRadius
    },

    // 5. Heavy-Duty Pickup Truck
    {
        VehicleType::PICKUP_TRUCK,
        "pickuptruck",
        "HEAVY-DUTY PICKUP",
        "Rugged long-wheelbase hauler with immense climbing torque",
        3500,
        580.0f,               // mass
        360.0f,               // inertia
        0.268f,               // wheelRadius
        18.0f,                // wheelMass
        0.62f,                // wheelInertia
        {-0.667f, 0.0f},      // rearMountOffset
        {+0.667f, 0.0f},      // frontMountOffset
        {-0.03f, +0.32f},     // driverSeatOffset
        0.18f,                // suspRestLength
        0.07f,                // suspMinLength
        0.34f,                // suspMaxLength
        44000.0f,             // baseSpringRate
        4200.0f,              // baseDamperRate
        5000.0f,              // basePeakTorque
        88.0f,                // baseTopSpeedKmh
        1.15f,                // baseGrip
        0.40f,                // base4wdSplit
        1800.0f,              // baseBrakingTorque
        0.10f,                // baseDownforce
        0.85f,                // baseAirAgility
        188, 75,              // spriteWidth, spriteHeight
        {95.50f, 75.00f},     // spriteAnchor
        21.5f                 // wheelSpriteRadius
    },

    // 6. Monster Truck
    {
        VehicleType::MONSTER_TRUCK,
        "monstertruck",
        "MONSTER TRUCK",
        "Massive 66-inch terra tires with giant pneumatic long-travel shocks",
        12000,
        740.0f,               // mass
        520.0f,               // inertia
        0.405f,               // wheelRadius
        32.0f,                // wheelMass
        2.55f,                // wheelInertia
        {-0.611f, 0.0f},      // rearMountOffset
        {+0.611f, 0.0f},      // frontMountOffset
        {+0.02f, +0.48f},     // driverSeatOffset
        0.28f,                // suspRestLength
        0.12f,                // suspMinLength
        0.48f,                // suspMaxLength
        34000.0f,             // baseSpringRate
        3400.0f,              // baseDamperRate
        7400.0f,              // basePeakTorque
        82.0f,                // baseTopSpeedKmh
        1.40f,                // baseGrip
        0.50f,                // base4wdSplit
        2200.0f,              // baseBrakingTorque
        0.08f,                // baseDownforce
        0.90f,                // baseAirAgility
        192, 80,              // spriteWidth, spriteHeight
        {94.00f, 83.00f},     // spriteAnchor
        32.5f                 // wheelSpriteRadius
    },

    // 7. Desert Rally Raid
    {
        VehicleType::DESERT_RAID,
        "desertraid",
        "DESERT RALLY RAID",
        "Extreme long-travel Baja trophy truck built for massive dune jumps",
        9500,
        490.0f,               // mass
        310.0f,               // inertia
        0.293f,               // wheelRadius
        20.0f,                // wheelMass
        0.82f,                // wheelInertia
        {-0.667f, 0.0f},      // rearMountOffset
        {+0.667f, 0.0f},      // frontMountOffset
        {+0.06f, +0.30f},     // driverSeatOffset
        0.22f,                // suspRestLength
        0.09f,                // suspMinLength
        0.40f,                // suspMaxLength
        38000.0f,             // baseSpringRate
        4000.0f,              // baseDamperRate
        4800.0f,              // basePeakTorque
        112.0f,               // baseTopSpeedKmh
        1.28f,                // baseGrip
        0.45f,                // base4wdSplit
        1900.0f,              // baseBrakingTorque
        0.22f,                // baseDownforce
        1.30f,                // baseAirAgility
        187, 69,              // spriteWidth, spriteHeight
        {93.50f, 69.00f},     // spriteAnchor
        23.5f                 // wheelSpriteRadius
    },

    // 8. Formula Racing Car
    {
        VehicleType::FORMULA_RACER,
        "formularacer",
        "FORMULA RACING CAR",
        "Open-wheel single-seater with extreme downforce and lightning reflexes",
        25000,
        295.0f,               // mass
        160.0f,               // inertia
        0.231f,               // wheelRadius
        11.0f,                // wheelMass
        0.28f,                // wheelInertia
        {-0.667f, 0.0f},      // rearMountOffset
        {+0.667f, 0.0f},      // frontMountOffset
        {+0.08f, +0.12f},     // driverSeatOffset
        0.10f,                // suspRestLength
        0.04f,                // suspMinLength
        0.18f,                // suspMaxLength
        58000.0f,             // baseSpringRate
        5500.0f,              // baseDamperRate
        4600.0f,              // basePeakTorque
        185.0f,               // baseTopSpeedKmh
        1.45f,                // baseGrip
        0.00f,                // base4wdSplit
        3000.0f,              // baseBrakingTorque
        0.88f,                // baseDownforce
        1.50f,                // baseAirAgility
        185, 41,              // spriteWidth, spriteHeight
        {83.50f, 40.00f},     // spriteAnchor
        18.5f                 // wheelSpriteRadius
    },

    // 9. Retro Muscle Car
    {
        VehicleType::MUSCLE_CAR,
        "musclecar",
        "RETRO MUSCLE CAR",
        "Classic high-power American V8 muscle coupe with aggressive raw torque",
        6500,
        520.0f,               // mass
        320.0f,               // inertia
        0.231f,               // wheelRadius
        15.0f,                // wheelMass
        0.38f,                // wheelInertia
        {-0.636f, 0.0f},      // rearMountOffset
        {+0.636f, 0.0f},      // frontMountOffset
        {-0.04f, +0.18f},     // driverSeatOffset
        0.15f,                // suspRestLength
        0.06f,                // suspMinLength
        0.26f,                // suspMaxLength
        44000.0f,             // baseSpringRate
        4200.0f,              // baseDamperRate
        5500.0f,              // basePeakTorque
        135.0f,               // baseTopSpeedKmh
        1.10f,                // baseGrip
        0.00f,                // base4wdSplit
        1700.0f,              // baseBrakingTorque
        0.18f,                // baseDownforce
        1.05f,                // baseAirAgility
        185, 46,              // spriteWidth, spriteHeight
        {92.00f, 46.00f},     // spriteAnchor
        18.5f                 // wheelSpriteRadius
    },

    // 10. Futuristic Electric Off-Roader
    {
        VehicleType::ELECTRIC_OFFROADER,
        "electricoffroader",
        "ELECTRIC OFF-ROADER",
        "High-voltage quad-motor sci-fi buggy with instant torque and active aero",
        15000,
        460.0f,               // mass
        280.0f,               // inertia
        0.305f,               // wheelRadius
        17.0f,                // wheelMass
        0.76f,                // wheelInertia
        {-0.723f, 0.0f},      // rearMountOffset
        {+0.723f, 0.0f},      // frontMountOffset
        {+0.01f, +0.28f},     // driverSeatOffset
        0.20f,                // suspRestLength
        0.08f,                // suspMinLength
        0.36f,                // suspMaxLength
        42000.0f,             // baseSpringRate
        4200.0f,              // baseDamperRate
        6200.0f,              // basePeakTorque
        140.0f,               // baseTopSpeedKmh
        1.35f,                // baseGrip
        0.50f,                // base4wdSplit
        2400.0f,              // baseBrakingTorque
        0.38f,                // baseDownforce
        1.25f,                // baseAirAgility
        188, 79,              // spriteWidth, spriteHeight
        {94.00f, 80.00f},     // spriteAnchor
        24.5f                 // wheelSpriteRadius
    }
};

static const std::vector<DriverConfig> s_drivers = {
    {
        DriverType::BILL,
        "bill",
        "BILL NEWTON",
        "The Fearless Rookie",
        "Classic backwards trucker cap and an unwavering appetite for steep hills.",
        0xFFFF3D00
    },
    {
        DriverType::SARAH,
        "sarah",
        "SARAH SWIFT",
        "Apex Track Champion",
        "Pro racing driver equipped with high-tech tinted visor and precision reflexes.",
        0xFF00E5FF
    },
    {
        DriverType::BOB,
        "bob",
        "RUSTY BOB",
        "Veteran Dirt Mechanic",
        "Old-school grease monkey sporting brass aviator goggles and a flat driving cap.",
        0xFFFFB300
    },
    {
        DriverType::NEIL,
        "neil",
        "COSMO NEIL",
        "Deep Space Explorer",
        "NASA astronaut helmet with golden solar visor, perfectly tuned for lunar jumps.",
        0xFFECEFF1
    }
};

const VehicleConfig& VehicleRegistry::getConfig(VehicleType type) {
    size_t idx = static_cast<size_t>(type);
    if (idx < s_vehicles.size()) {
        return s_vehicles[idx];
    }
    return s_vehicles[0];
}

const VehicleConfig& VehicleRegistry::getConfig(const std::string& id) {
    for (const auto& v : s_vehicles) {
        if (v.id == id) return v;
    }
    return s_vehicles[0];
}

const std::vector<VehicleConfig>& VehicleRegistry::getAllVehicles() {
    return s_vehicles;
}

const DriverConfig& VehicleRegistry::getDriver(DriverType type) {
    size_t idx = static_cast<size_t>(type);
    if (idx < s_drivers.size()) {
        return s_drivers[idx];
    }
    return s_drivers[0];
}

const DriverConfig& VehicleRegistry::getDriver(const std::string& id) {
    for (const auto& d : s_drivers) {
        if (d.id == id) return d;
    }
    return s_drivers[0];
}

const std::vector<DriverConfig>& VehicleRegistry::getAllDrivers() {
    return s_drivers;
}

} // namespace Physics
