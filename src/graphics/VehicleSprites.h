#pragma once

#include <QImage>
#include <vector>
#include "VehicleConfig.h"

namespace Graphics {

/**
 * @brief High-precision Software Pixel-Art Sprite Engine.
 * Generates and caches pixel-art vehicle chassis, wheels, and drivers.
 */
class VehicleSprites {
public:
    static void init();

    static const QImage& getChassisSprite(Physics::VehicleType type);
    static const QImage& getWheelSprite(Physics::VehicleType type);
    static const QImage& getDriverSprite(Physics::DriverType type);

    // Mini icon/portrait for UI buttons and selection
    static const QImage& getDriverPortrait(Physics::DriverType type);

private:
    static bool s_initialized;
    static std::vector<QImage> s_chassisSprites;
    static std::vector<QImage> s_wheelSprites;
    static std::vector<QImage> s_driverSprites;
    static std::vector<QImage> s_driverPortraits;

    static QImage createOffroaderChassis();
    static QImage createRallyChassis();
    static QImage createSportsCoupeChassis();
    static QImage createSupercarChassis();
    static QImage createPickupChassis();
    static QImage createMonsterTruckChassis();
    static QImage createDesertRaidChassis();
    static QImage createFormulaChassis();
    static QImage createMuscleCarChassis();
    static QImage createElectricOffroaderChassis();

    static QImage createOffroaderWheel();
    static QImage createRallyWheel();
    static QImage createSportsCoupeWheel();
    static QImage createSupercarWheel();
    static QImage createPickupWheel();
    static QImage createMonsterTruckWheel();
    static QImage createDesertRaidWheel();
    static QImage createFormulaWheel();
    static QImage createMuscleCarWheel();
    static QImage createElectricOffroaderWheel();

    static QImage createBillDriver();
    static QImage createSarahDriver();
    static QImage createBobDriver();
    static QImage createNeilDriver();
};

} // namespace Graphics
