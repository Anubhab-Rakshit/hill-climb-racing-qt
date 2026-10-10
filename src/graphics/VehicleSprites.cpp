#include "VehicleSprites.h"
#include <QPainter>
#include <cmath>
#include <algorithm>

namespace Graphics {

bool VehicleSprites::s_initialized = false;
std::vector<QImage> VehicleSprites::s_chassisSprites;
std::vector<QImage> VehicleSprites::s_wheelSprites;
std::vector<QImage> VehicleSprites::s_driverSprites;
std::vector<QImage> VehicleSprites::s_driverPortraits;

// Raster drawing helpers
static inline void setPx(QImage& img, int x, int y, uint32_t col) {
    if (x >= 0 && x < img.width() && y >= 0 && y < img.height()) {
        reinterpret_cast<uint32_t*>(img.scanLine(y))[x] = col;
    }
}

static void fillR(QImage& img, int x, int y, int w, int h, uint32_t col) {
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(img.width(), x + w);
    int y1 = std::min(img.height(), y + h);
    for (int cy = y0; cy < y1; ++cy) {
        uint32_t* line = reinterpret_cast<uint32_t*>(img.scanLine(cy));
        std::fill(line + x0, line + x1, col);
    }
}

static void drawR(QImage& img, int x, int y, int w, int h, uint32_t col) {
    for (int px = x; px < x + w; ++px) {
        setPx(img, px, y, col);
        setPx(img, px, y + h - 1, col);
    }
    for (int py = y; py < y + h; ++py) {
        setPx(img, x, py, col);
        setPx(img, x + w - 1, py, col);
    }
}

static void fillC(QImage& img, int xc, int yc, int r, uint32_t col) {
    if (r <= 0) return;
    int r2 = r * r;
    int y0 = std::max(0, yc - r);
    int y1 = std::min(img.height() - 1, yc + r);
    for (int y = y0; y <= y1; ++y) {
        int dy = y - yc;
        int dx = static_cast<int>(std::sqrt(r2 - dy * dy));
        int x0 = std::max(0, xc - dx);
        int x1 = std::min(img.width() - 1, xc + dx);
        if (x0 <= x1) {
            uint32_t* line = reinterpret_cast<uint32_t*>(img.scanLine(y));
            std::fill(line + x0, line + x1 + 1, col);
        }
    }
}

static void drawC(QImage& img, int xc, int yc, int r, uint32_t col) {
    int x = 0;
    int y = r;
    int d = 1 - r;
    auto plot8 = [&](int px, int py) {
        setPx(img, xc + px, yc + py, col);
        setPx(img, xc - px, yc + py, col);
        setPx(img, xc + px, yc - py, col);
        setPx(img, xc - px, yc - py, col);
        setPx(img, xc + py, yc + px, col);
        setPx(img, xc - py, yc + px, col);
        setPx(img, xc + py, yc - px, col);
        setPx(img, xc - py, yc - px, col);
    };
    plot8(x, y);
    while (x < y) {
        ++x;
        if (d < 0) d += 2 * x + 1;
        else { --y; d += 2 * (x - y) + 1; }
        plot8(x, y);
    }
}

static void drawL(QImage& img, int x0, int y0, int x1, int y1, uint32_t col) {
    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    while (true) {
        setPx(img, x0, y0, col);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void VehicleSprites::init() {
    if (s_initialized) return;

    s_chassisSprites.resize(static_cast<size_t>(Physics::VehicleType::COUNT));
    s_wheelSprites.resize(static_cast<size_t>(Physics::VehicleType::COUNT));
    s_driverSprites.resize(static_cast<size_t>(Physics::DriverType::COUNT));
    s_driverPortraits.resize(static_cast<size_t>(Physics::DriverType::COUNT));

    auto loadSprite = [](const QString& qrcPath, const QString& diskPath, std::function<QImage()> fallback) -> QImage {
        QImage img;
        if (img.load(qrcPath) && !img.isNull()) return img;
        if (img.load(diskPath) && !img.isNull()) return img;
        return fallback();
    };

    struct VehEntry {
        Physics::VehicleType type;
        const char* id;
        std::function<QImage()> chassisFallback;
        std::function<QImage()> wheelFallback;
    };

    const VehEntry entries[] = {
        {Physics::VehicleType::OFFROADER, "offroader", createOffroaderChassis, createOffroaderWheel},
        {Physics::VehicleType::RALLY_CAR, "rallycar", createRallyChassis, createRallyWheel},
        {Physics::VehicleType::SPORTS_COUPE, "sportscoupe", createSportsCoupeChassis, createSportsCoupeWheel},
        {Physics::VehicleType::SUPERCAR, "supercar", createSupercarChassis, createSupercarWheel},
        {Physics::VehicleType::PICKUP_TRUCK, "pickuptruck", createPickupChassis, createPickupWheel},
        {Physics::VehicleType::MONSTER_TRUCK, "monstertruck", createMonsterTruckChassis, createMonsterTruckWheel},
        {Physics::VehicleType::DESERT_RAID, "desertraid", createDesertRaidChassis, createDesertRaidWheel},
        {Physics::VehicleType::FORMULA_RACER, "formularacer", createFormulaChassis, createFormulaWheel},
        {Physics::VehicleType::MUSCLE_CAR, "musclecar", createMuscleCarChassis, createMuscleCarWheel},
        {Physics::VehicleType::ELECTRIC_OFFROADER, "electricoffroader", createElectricOffroaderChassis, createElectricOffroaderWheel}
    };

    for (const auto& e : entries) {
        size_t idx = static_cast<size_t>(e.type);
        QString qrcChassis = QString(":/assets/vehicles/chassis_%1.png").arg(e.id);
        QString diskChassis = QString("assets/vehicles/chassis_%1.png").arg(e.id);
        s_chassisSprites[idx] = loadSprite(qrcChassis, diskChassis, e.chassisFallback);

        QString qrcWheel = QString(":/assets/vehicles/wheel_%1.png").arg(e.id);
        QString diskWheel = QString("assets/vehicles/wheel_%1.png").arg(e.id);
        s_wheelSprites[idx] = loadSprite(qrcWheel, diskWheel, e.wheelFallback);
    }

    s_driverSprites[static_cast<size_t>(Physics::DriverType::BILL)]  = createBillDriver();
    s_driverSprites[static_cast<size_t>(Physics::DriverType::SARAH)] = createSarahDriver();
    s_driverSprites[static_cast<size_t>(Physics::DriverType::BOB)]   = createBobDriver();
    s_driverSprites[static_cast<size_t>(Physics::DriverType::NEIL)]  = createNeilDriver();

    for (size_t i = 0; i < s_driverSprites.size(); ++i) {
        s_driverPortraits[i] = s_driverSprites[i].scaled(34, 34, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    s_initialized = true;
}

const QImage& VehicleSprites::getChassisSprite(Physics::VehicleType type) {
    init();
    size_t idx = static_cast<size_t>(type);
    return (idx < s_chassisSprites.size()) ? s_chassisSprites[idx] : s_chassisSprites[0];
}

const QImage& VehicleSprites::getWheelSprite(Physics::VehicleType type) {
    init();
    size_t idx = static_cast<size_t>(type);
    return (idx < s_wheelSprites.size()) ? s_wheelSprites[idx] : s_wheelSprites[0];
}

const QImage& VehicleSprites::getDriverSprite(Physics::DriverType type) {
    init();
    size_t idx = static_cast<size_t>(type);
    return (idx < s_driverSprites.size()) ? s_driverSprites[idx] : s_driverSprites[0];
}

const QImage& VehicleSprites::getDriverPortrait(Physics::DriverType type) {
    init();
    size_t idx = static_cast<size_t>(type);
    return (idx < s_driverPortraits.size()) ? s_driverPortraits[idx] : s_driverPortraits[0];
}

// =========================================================================
// 1. CLASSIC OFF-ROADER (256 x 128)
// Anchor: (126.4, 70.0). Rear: x=64, Front: x=189, Seat: (106, 41)
// =========================================================================
QImage VehicleSprites::createOffroaderChassis() {
    QImage img(256, 128, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);

    // Trail Green body tub
    uint32_t colBody = 0xFF436B2B;
    uint32_t colHi   = 0xFF689F38;
    uint32_t colSh   = 0xFF2E4C1E;

    fillR(img, 38, 56, 168, 22, colBody);
    fillR(img, 38, 56, 168, 4,  colHi);
    fillR(img, 38, 74, 168, 4,  colSh);

    // Front hood slope
    fillR(img, 130, 50, 74, 10, colBody);
    fillR(img, 130, 50, 74, 2,  colHi);
    fillR(img, 198, 54, 8, 18,  0xFF1B3011); // Vertical 7-slot grille face

    // Chrome winch bumper
    fillR(img, 204, 60, 6, 16, 0xFFCFD8DC);
    fillR(img, 201, 64, 5, 8,  0xFF37474F); // Winch spool
    drawR(img, 200, 58, 12, 20, 0xFF212121);
    // Round Headlight
    fillC(img, 201, 52, 4, 0xFFFFF9C4);
    drawC(img, 201, 52, 4, 0xFF212121);

    // Windshield frame & Glass glint
    fillR(img, 125, 30, 6, 26, 0xFF212121);
    for (int y = 30; y < 54; ++y) {
        int xOffset = (54 - y) / 3;
        fillR(img, 122 + xOffset, y, 4, 1, 0xBB81D4FA);
        setPx(img, 124 + xOffset, y, 0xEEFFFFFF);
    }

    // Heavy Roll-Cage (Tubular black steel)
    fillR(img, 52, 22, 5, 36, 0xFF212121);
    fillR(img, 52, 20, 76, 5, 0xFF212121);
    drawL(img, 52, 22, 126, 54, 0xFF37474F);
    drawL(img, 53, 22, 127, 54, 0xFF212121);

    // Rear Mounted Spare Tire Carrier
    fillC(img, 32, 56, 16, 0xFF1C2024);
    drawC(img, 32, 56, 16, 0xFF0D0E10);
    fillC(img, 32, 56, 6,  0xFFCFD8DC);

    // Cut out Wheel Arches (rear x=64, front x=189)
    fillC(img, 64, 76, 26, 0x00000000);
    fillC(img, 189, 76, 26, 0x00000000);

    // Rugged black plastic fender flares
    drawC(img, 64, 76, 26, 0xFF212121);
    drawC(img, 64, 76, 27, 0xFF37474F);
    drawC(img, 189, 76, 26, 0xFF212121);
    drawC(img, 189, 76, 27, 0xFF37474F);

    return img;
}

// =========================================================================
// 2. RALLY RACING CAR (256 x 128)
// Anchor: (126.4, 66.0). Rear: x=59, Front: x=192, Seat: (112, 42)
// =========================================================================
QImage VehicleSprites::createRallyChassis() {
    QImage img(256, 128, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);

    uint32_t colWhite = 0xFFF5F7FA;
    uint32_t colBlue  = 0xFF1565C0;
    uint32_t colCyan  = 0xFF00E5FF;
    uint32_t colDark  = 0xFF1C252E;

    // Hatchback body shell
    fillR(img, 36, 52, 168, 22, colWhite);
    fillR(img, 36, 52, 168, 3,  0xFFFFFFFF);
    fillR(img, 36, 71, 168, 3,  0xFFCFD8DC);

    // Cabin roof
    fillR(img, 68, 28, 82, 24, colWhite);
    fillR(img, 68, 28, 82, 3,  0xFFFFFFFF);

    // Windows
    fillR(img, 74, 33, 30, 18, 0xEE37474F);
    fillR(img, 108, 33, 38, 18, 0xEE90CAF9);
    for (int y = 30; y < 52; ++y) {
        int xOffset = (52 - y) / 2;
        fillR(img, 146 + xOffset, y, 4, 1, 0xDD90CAF9);
    }

    // Dynamic racing stripes
    for (int x = 40; x < 190; ++x) {
        if ((x + 20) % 36 < 14) fillR(img, x, 56, 1, 12, colBlue);
        else if ((x + 20) % 36 < 20) fillR(img, x, 56, 1, 12, colCyan);
    }

    // Bi-Plane Rally Wing
    fillR(img, 24, 22, 28, 4, colCyan);
    fillR(img, 22, 18, 4, 12, colBlue);
    fillR(img, 34, 26, 3, 24, colDark);

    // Roof-Mounted Ram Air Scoop
    fillR(img, 105, 23, 20, 5, colCyan);
    fillR(img, 122, 24, 4, 3,  0xFF000000);

    // Front Bumper & Quad Foglight Pod
    fillR(img, 196, 56, 12, 18, colWhite);
    fillC(img, 196, 52, 3, 0xFFFFF59D);
    fillC(img, 202, 52, 3, 0xFFFFF59D);
    drawC(img, 196, 52, 3, colDark);
    drawC(img, 202, 52, 3, colDark);

    // Red mudflaps
    fillR(img, 32, 72, 4, 14, 0xFFD50000);

    // Cut out Wheel Arches (rear x=59, front x=192)
    fillC(img, 59, 70, 25, 0x00000000);
    fillC(img, 192, 70, 25, 0x00000000);

    // Flared box arches
    drawC(img, 59, 70, 25, colDark);
    drawC(img, 59, 70, 26, colCyan);
    drawC(img, 192, 70, 25, colDark);
    drawC(img, 192, 70, 26, colCyan);

    return img;
}

// =========================================================================
// 3. MODERN SPORTS COUPE (260 x 128)
// Anchor: (130.0, 66.0). Rear: x=59, Front: x=199, Seat: (117, 46)
// =========================================================================
QImage VehicleSprites::createSportsCoupeChassis() {
    QImage img(260, 128, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);

    uint32_t colBody   = 0xFF1565C0; // Sapphire Metallic Blue
    uint32_t colHi     = 0xFF42A5F5;
    uint32_t colSh     = 0xFF0D47A1;
    uint32_t colCarbon = 0xFF212529;

    // Muscular body
    fillR(img, 36, 54, 182, 18, colBody);
    fillR(img, 36, 54, 182, 3,  colHi);
    fillR(img, 36, 69, 182, 3,  colSh);

    // Long sculpted hood line
    for (int x = 142; x < 218; ++x) {
        float t = (x - 142.0f) / 76.0f;
        int topY = static_cast<int>(46 + t * 13.0f);
        fillR(img, x, topY, 1, 68 - topY, colBody);
        setPx(img, x, topY, colHi);
    }

    // Carbon-fiber roof and fastback pillars
    fillR(img, 84, 32, 62, 5, colCarbon);
    for (int y = 32; y < 52; ++y) {
        float t = (y - 32.0f) / 20.0f;
        int startX = static_cast<int>(80 - (1.0f - t) * 6.0f);
        int endX = static_cast<int>(146 + t * 12.0f);
        fillR(img, startX, y, endX - startX, 1, 0xEE0D1B2A); // Smoked dark privacy glass
        setPx(img, startX + 4, y, 0xFF42A5F5);
        setPx(img, startX + 5, y, 0xFFFFFFFF);
    }
    drawR(img, 78, 32, 70, 20, colCarbon);

    // Sleek dual LED blade headlights
    fillR(img, 212, 58, 9, 3, 0xFFE0F7FA);
    setPx(img, 219, 58, 0xFFFFFFFF);
    fillR(img, 214, 62, 7, 2, 0xFFFFB300); // Amber signal blade

    // Front carbon splitter & side skirts
    fillR(img, 214, 68, 14, 4, colCarbon);
    fillR(img, 88, 70, 80, 2,  colCarbon);

    // Integrated ducktail rear spoiler & dual chrome exhaust tips
    fillR(img, 28, 48, 12, 6, colBody);
    fillR(img, 26, 46, 14, 2, colCarbon);
    fillR(img, 26, 66, 8, 4,  0xFFECEFF1); // Chrome exhaust
    drawR(img, 26, 66, 8, 4,  0xFF263238);

    // Cut out Wheel Arches (rear x=59, front x=199)
    fillC(img, 59, 69, 24, 0x00000000);
    fillC(img, 199, 69, 24, 0x00000000);

    drawC(img, 59, 69, 24, colCarbon);
    drawC(img, 199, 69, 24, colCarbon);

    return img;
}

// =========================================================================
// 4. SUPERCAR (264 x 128)
// Anchor: (132.0, 66.0). Rear: x=58, Front: x=203, Seat: (122, 50)
// =========================================================================
QImage VehicleSprites::createSupercarChassis() {
    QImage img(264, 128, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);

    uint32_t colBody   = 0xFFFF6D00; // Acid Neon Orange
    uint32_t colHi     = 0xFFFFD180;
    uint32_t colSh     = 0xFFE65100;
    uint32_t colCarbon = 0xFF1C2024;

    // Extreme low-slung wedge chassis
    fillR(img, 32, 54, 192, 16, colBody);
    fillR(img, 32, 54, 192, 3,  colHi);
    fillR(img, 32, 67, 192, 3,  colSh);

    // Razor-sharp sloping hood
    for (int x = 145; x < 225; ++x) {
        float t = (x - 145.0f) / 80.0f;
        int topY = static_cast<int>(46 + t * 16.0f);
        fillR(img, x, topY, 1, 68 - topY, colBody);
        setPx(img, x, topY, colHi);
    }

    // Mid-Engine Bay Glass Window showcasing V12 Engine
    fillR(img, 56, 44, 38, 12, 0xAA81D4FA); // Glass cover
    // Visible V12 Chrome Intake Manifolds & Red Cam Covers!
    fillR(img, 62, 46, 26, 3, 0xFFCFD8DC);
    fillR(img, 60, 50, 30, 3, 0xFFD50000);

    // Cockpit canopy
    for (int y = 34; y < 50; ++y) {
        float t = (y - 34.0f) / 16.0f;
        int startX = static_cast<int>(94 - (1.0f - t) * 6.0f);
        int endX = static_cast<int>(148 + t * 8.0f);
        fillR(img, startX, y, endX - startX, 1, 0xEE102027);
        setPx(img, startX + 3, y, 0xFF00E5FF);
        setPx(img, startX + 4, y, 0xFFFFFFFF);
    }
    drawR(img, 90, 34, 62, 16, colCarbon);

    // Massive side NACA air ducts
    fillR(img, 100, 57, 24, 8, 0xFF0D0F12);
    drawR(img, 99, 56, 26, 10, colCarbon);

    // High Swan-Neck Carbon Rear Wing
    fillR(img, 20, 26, 42, 4, colCarbon);
    fillR(img, 18, 22, 4, 12, colBody); // Endplates
    fillR(img, 32, 30, 3, 24, colCarbon); // Swan neck upright 1
    fillR(img, 48, 30, 3, 24, colCarbon); // Swan neck upright 2

    // Carbon front splitter with winglets
    fillR(img, 218, 68, 16, 4, colCarbon);
    fillR(img, 230, 64, 4, 8,  colCarbon);

    // Quad Titanium Exhaust Tips
    fillR(img, 22, 62, 8, 4, 0xFF90A4AE);

    // Cut out Wheel Arches (rear x=58, front x=203)
    fillC(img, 58, 68, 23, 0x00000000);
    fillC(img, 203, 68, 23, 0x00000000);

    drawC(img, 58, 68, 23, colCarbon);
    drawC(img, 203, 68, 23, colCarbon);

    return img;
}

// =========================================================================
// 5. HEAVY-DUTY PICKUP TRUCK (270 x 130)
// Anchor: (135.0, 72.0). Rear: x=58, Front: x=209, Seat: (113, 38)
// =========================================================================
QImage VehicleSprites::createPickupChassis() {
    QImage img(270, 130, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);

    uint32_t colTruck     = 0xFFB71C1C; // Deep Metallic Crimson
    uint32_t colHighlight = 0xFFE53935;
    uint32_t colShadow    = 0xFF7F0000;
    uint32_t colChrome    = 0xFFECEFF1;

    // Heavy long dual-cab body
    fillR(img, 28, 56, 206, 26, colTruck);
    fillR(img, 28, 56, 206, 4,  colHighlight);
    fillR(img, 28, 78, 206, 4,  colShadow);

    // High Cabin Roof
    fillR(img, 82, 28, 76, 28, colTruck);
    fillR(img, 82, 28, 76, 3,  colHighlight);

    // Windows
    fillR(img, 88, 33, 30, 20, 0xEE78909C);
    fillR(img, 122, 33, 32, 20, 0xEE90CAF9);
    drawR(img, 88, 33, 66, 20, 0xFF102027);
    fillR(img, 118, 33, 4, 20, colTruck);
    fillR(img, 132, 46, 5, 2,  colChrome);

    // Amber Cab Clearance Lights
    for (int cl = 90; cl < 155; cl += 14) {
        fillR(img, cl, 26, 4, 2, 0xFFFFC107);
    }

    // Heavy Chrome Billet Grille & Bumper
    fillR(img, 222, 50, 12, 30, colChrome);
    for (int g = 52; g < 76; g += 5) {
        fillR(img, 224, g, 8, 2, 0xFF263238);
    }
    fillR(img, 228, 68, 12, 14, 0xFF37474F);
    fillR(img, 228, 68, 12, 3,  colChrome);
    fillR(img, 224, 48, 6, 6,   0xFFFFF9C4);
    fillR(img, 224, 56, 6, 6,   0xFFFFB74D);

    // Open Bed with Headache Rack & Spotlights
    fillR(img, 28, 48, 54, 8, colShadow);
    fillR(img, 76, 18, 5, 38, 0xFF212121);
    fillR(img, 36, 18, 5, 38, 0xFF212121);
    fillR(img, 36, 18, 45, 4, 0xFF212121);
    for (int s = 42; s < 78; s += 9) {
        fillC(img, s, 14, 4, 0xFFFFF59D);
        drawC(img, s, 14, 4, 0xFF212121);
    }

    // Chrome Nerf Bar Steps
    fillR(img, 84, 80, 72, 3, colChrome);

    // Cut out Wheel Arches (rear x=58, front x=209)
    fillC(img, 58, 80, 28, 0x00000000);
    fillC(img, 209, 80, 28, 0x00000000);

    drawC(img, 58, 80, 28, 0xFF212121);
    drawC(img, 58, 80, 29, 0xFF37474F);
    drawC(img, 209, 80, 28, 0xFF212121);
    drawC(img, 209, 80, 29, 0xFF37474F);

    return img;
}

// =========================================================================
// 6. MONSTER TRUCK (280 x 140)
// Anchor: (140.0, 75.0). Rear: x=58, Front: x=219, Seat: (128, 31)
// =========================================================================
QImage VehicleSprites::createMonsterTruckChassis() {
    QImage img(280, 140, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);

    uint32_t colPurple = 0xFF7B1FA2; // Toxic Violet
    uint32_t colLime   = 0xFF76FF03; // Neon Lime Flames
    uint32_t colHi     = 0xFFBA68C8;
    uint32_t colChrome = 0xFFECEFF1;
    uint32_t colSteel  = 0xFF37474F;

    // High elevated pickup competition cab
    fillR(img, 72, 28, 126, 32, colPurple);
    fillR(img, 72, 28, 126, 3,  colHi);

    // Cab windshield & windows
    fillR(img, 108, 34, 48, 18, 0xEE7986CB);
    drawR(img, 108, 34, 48, 18, 0xFF212121);
    for (int y = 32; y < 52; ++y) {
        int xOffset = (52 - y) / 2;
        fillR(img, 154 + xOffset, y, 4, 1, 0xDD90CAF9);
    }

    // Neon Lime Green Tribal Flames
    drawL(img, 150, 48, 196, 44, colLime);
    drawL(img, 140, 52, 192, 50, colLime);
    drawL(img, 130, 56, 180, 54, colLime);
    fillR(img, 182, 46, 12, 8, colLime);

    // Massive Tubular Space-Frame Subframe (Under-Chassis)
    fillR(img, 54, 60, 168, 6, colSteel);
    drawL(img, 58, 64, 138, 85, colChrome);
    drawL(img, 218, 64, 138, 85, colChrome);
    drawL(img, 98, 64, 178, 85, colChrome);
    drawL(img, 178, 64, 98, 85, colChrome);

    // Central Heavy Transfer Case
    fillR(img, 128, 76, 24, 14, 0xFF212121);
    drawR(img, 128, 76, 24, 14, colChrome);

    // Upward Zoomie Exhaust Headers (Spitting flame)
    for (int i = 0; i < 4; ++i) {
        int hx = 172 + i * 5;
        drawL(img, hx, 62, hx - 4, 48, colChrome);
        fillC(img, hx - 4, 47, 2, colLime);
    }

    // Roof-mounted Quad Floodlights
    for (int fl = 104; fl < 144; fl += 10) {
        fillC(img, fl, 22, 4, 0xFFFFF59D);
        drawC(img, fl, 22, 4, 0xFF212121);
    }

    // Cut out Giant Wheel Arches (rear x=58, front x=219, r=40)
    fillC(img, 58, 85, 40, 0x00000000);
    fillC(img, 219, 85, 40, 0x00000000);

    // Lime-accented fender arches
    drawC(img, 58, 85, 40, colLime);
    drawC(img, 219, 85, 40, colLime);

    return img;
}

// =========================================================================
// 7. DESERT RALLY RAID TROPHY TRUCK (268 x 130)
// Anchor: (134.0, 70.0). Rear: x=59, Front: x=206, Seat: (118, 42)
// =========================================================================
QImage VehicleSprites::createDesertRaidChassis() {
    QImage img(268, 130, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);

    uint32_t colSand   = 0xFFD7CCC8; // Desert Tan
    uint32_t colOrange = 0xFFFF6D00; // Baja Orange
    uint32_t colDark   = 0xFF263238;
    uint32_t colAlu    = 0xFFCFD8DC;

    // Trophy Truck fiberglass bodywork
    fillR(img, 52, 54, 156, 22, colSand);
    fillR(img, 52, 54, 156, 3,  0xFFEFEBE9);
    fillR(img, 52, 73, 156, 3,  0xFF8D6E63);

    // Carbon Hood & High-vis Orange Stripes
    fillR(img, 136, 48, 68, 12, colDark);
    fillR(img, 54, 60, 152, 4,  colOrange);

    // Cab glass
    fillR(img, 98, 32, 42, 22, 0xEE37474F);
    for (int y = 32; y < 54; ++y) {
        int xOffset = (54 - y) / 2;
        fillR(img, 138 + xOffset, y, 4, 1, 0xDD81D4FA);
    }
    drawR(img, 96, 32, 50, 22, colDark);

    // Exposed Rear Tubular Trophy Bed with TWO Mounted Spares!
    fillR(img, 32, 44, 4, 30, colDark);
    fillR(img, 50, 44, 4, 30, colDark);
    drawL(img, 32, 44, 96, 32, colDark);
    // Spare Tire 1 & 2
    fillC(img, 52, 48, 14, 0xFF1C2024);
    fillC(img, 52, 48, 4,  colOrange);
    fillC(img, 72, 48, 14, 0xFF1C2024);
    fillC(img, 72, 48, 4,  colOrange);
    // Orange ratchet tie-down straps
    fillR(img, 42, 46, 38, 2, colOrange);

    // Aluminum Front Bash Skid Plate
    drawL(img, 206, 76, 228, 64, colAlu);
    drawL(img, 206, 77, 228, 65, colAlu);
    drawL(img, 206, 78, 228, 66, colAlu);

    // Curved Ultra-Bright White Roof LED Light Bar
    fillR(img, 96, 28, 48, 4, 0xFF212121);
    for (int lx = 98; lx < 142; lx += 4) {
        fillR(img, lx, 29, 3, 2, 0xFFFFFFFF);
    }

    // Cut out Wheel Arches (rear x=59, front x=206, r=29)
    fillC(img, 59, 76, 29, 0x00000000);
    fillC(img, 206, 76, 29, 0x00000000);

    drawC(img, 59, 76, 29, colDark);
    drawC(img, 206, 76, 29, colOrange);

    return img;
}

// =========================================================================
// 8. FORMULA RACING CAR (270 x 120)
// Anchor: (135.0, 66.0). Rear: x=58, Front: x=210, Seat: (131, 48)
// =========================================================================
QImage VehicleSprites::createFormulaChassis() {
    QImage img(270, 120, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);

    uint32_t colRed    = 0xFFD50000; // Rosso Corsa Red
    uint32_t colHi     = 0xFFFF1744;
    uint32_t colWhite  = 0xFFFFFFFF;
    uint32_t colCarbon = 0xFF1E2229;

    // Slender needle nose cone
    for (int x = 145; x < 232; ++x) {
        float t = (x - 145.0f) / 87.0f;
        int topY = static_cast<int>(52 + t * 9.0f);
        int botY = static_cast<int>(66 - t * 4.0f);
        fillR(img, x, topY, 1, botY - topY, colRed);
        setPx(img, x, topY, colHi);
    }
    // White nose livery stripe
    fillR(img, 155, 56, 68, 2, colWhite);

    // Front Aerodynamic Wing & Endplates
    fillR(img, 224, 62, 18, 4, colWhite);
    fillR(img, 238, 54, 4, 14, colCarbon); // Front wing vertical endplate
    fillR(img, 222, 60, 4, 2,  colCarbon);

    // Central Cockpit Tub & Driver Seat Cavity
    fillR(img, 102, 50, 48, 16, colRed);
    fillR(img, 102, 50, 48, 2,  colHi);

    // Halo Cockpit Safety Hoop (Curved titanium hoop arching over driver)
    fillR(img, 122, 38, 18, 4, colCarbon);
    drawL(img, 138, 38, 148, 52, colCarbon);
    drawL(img, 122, 38, 116, 52, colCarbon);

    // High Overhead Airbox Intake Chimney
    fillR(img, 92, 30, 24, 22, colRed);
    fillR(img, 92, 30, 24, 3,  colHi);
    fillR(img, 112, 32, 4, 6,  colCarbon); // Airbox intake opening

    // Sidepods with Radiator Inlets
    fillR(img, 78, 52, 44, 14, colRed);
    fillR(img, 106, 53, 6, 10, colCarbon); // Sidepod radiator inlet

    // High Cantilevered Rear Aerofoil Wing
    fillR(img, 22, 22, 36, 4, colWhite);
    fillR(img, 20, 18, 4, 16, colCarbon); // Endplate
    fillR(img, 36, 26, 3, 30, colCarbon); // Central swan-neck pylon
    fillC(img, 22, 46, 3, 0xFFFF1744);    // Flashing rear rain safety light

    // Exposed Pushrod Suspension Arms (Open-Wheel aesthetic)
    drawL(img, 188, 56, 210, 68, colCarbon);
    drawL(img, 194, 64, 210, 68, colCarbon);
    drawL(img, 78, 56, 58, 68,   colCarbon);
    drawL(img, 84, 64, 58, 68,   colCarbon);

    return img;
}

// =========================================================================
// 9. RETRO MUSCLE CAR (264 x 128)
// Anchor: (132.0, 68.0). Rear: x=60, Front: x=203, Seat: (114, 46)
// =========================================================================
QImage VehicleSprites::createMuscleCarChassis() {
    QImage img(264, 128, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);

    uint32_t colBlack  = 0xFF1A1C20; // Midnight Raven Black
    uint32_t colGold   = 0xFFFFC107; // Twin Gold Racing Stripes
    uint32_t colHi     = 0xFF37474F;
    uint32_t colChrome = 0xFFECEFF1;

    // Muscular long hood and fastback cabin
    fillR(img, 32, 54, 184, 20, colBlack);
    fillR(img, 32, 54, 184, 2,  colHi);

    // Fastback roofline
    fillR(img, 78, 32, 70, 22, colBlack);
    fillR(img, 78, 32, 70, 2,  colHi);

    // Side windows with chrome trim
    fillR(img, 84, 36, 26, 18, 0xEE455A64);
    fillR(img, 114, 36, 32, 18, 0xEE78909C);
    drawR(img, 82, 34, 66, 20, colChrome);

    // Dual Gold Racing Stripes
    fillR(img, 32, 56, 184, 3, colGold);
    fillR(img, 32, 61, 184, 3, colGold);

    // Massive Supercharger Blower Scoop through Hood!
    fillR(img, 156, 38, 22, 16, colChrome);
    drawR(img, 156, 38, 22, 16, 0xFF212121);
    // Triple Red Throttle Butterflies inside blower
    fillC(img, 163, 44, 3, 0xFFFF1744);
    fillC(img, 171, 44, 3, 0xFFFF1744);

    // Recessed Black Mesh Grille with Chrome Surround
    fillR(img, 214, 52, 6, 22, colBlack);
    drawR(img, 214, 52, 6, 22, colChrome);
    // Hidden quad headlights
    fillC(img, 216, 56, 2, 0xFFFFF9C4);
    fillC(img, 216, 64, 2, 0xFFFFF9C4);

    // Full-Width Mirror Chrome Front & Rear Bumpers
    fillR(img, 218, 66, 6, 8, colChrome);
    fillR(img, 26, 66, 8, 8,  colChrome);

    // Ducktail decklid spoiler
    fillR(img, 28, 48, 12, 6, colBlack);
    fillR(img, 28, 48, 12, 2, colGold);

    // Side-exit chrome exhaust tips below door sill
    fillR(img, 98, 72, 14, 3, colChrome);

    // Cut out Wheel Arches (rear x=60, front x=203)
    fillC(img, 60, 72, 26, 0x00000000);
    fillC(img, 203, 72, 26, 0x00000000);

    drawC(img, 60, 72, 26, colChrome);
    drawC(img, 203, 72, 26, colChrome);

    return img;
}

// =========================================================================
// 10. FUTURISTIC ELECTRIC OFF-ROADER (260 x 128)
// Anchor: (130.0, 68.0). Rear: x=61, Front: x=199, Seat: (118, 42)
// =========================================================================
QImage VehicleSprites::createElectricOffroaderChassis() {
    QImage img(260, 128, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);

    uint32_t colWhite   = 0xFFF5F5F5; // Pearl White Armor
    uint32_t colTitan   = 0xFF37474F; // Titanium Panels
    uint32_t colNeon    = 0xFF00E5FF; // Cyber Cyan LED
    uint32_t colMagenta = 0xFFF50057; // Battery Status Glow

    // Angular faceted cyber chassis body
    fillR(img, 38, 54, 174, 20, colWhite);
    fillR(img, 38, 54, 174, 3,  0xFFFFFFFF);
    fillR(img, 38, 71, 174, 3,  colTitan);

    // Hexagonal Solar Cell Roof Array
    fillR(img, 82, 30, 68, 6, 0xFF0D47A1);
    for (int sx = 84; sx < 148; sx += 8) {
        drawL(img, sx, 30, sx + 4, 35, colNeon);
    }

    // Aerodynamic Enclosed Electrochromic Canopy
    for (int y = 36; y < 54; ++y) {
        float t = (y - 36.0f) / 18.0f;
        int startX = static_cast<int>(80 - (1.0f - t) * 6.0f);
        int endX = static_cast<int>(152 + t * 8.0f);
        fillR(img, startX, y, endX - startX, 1, 0xEE1A237E);
        setPx(img, startX + 4, y, colNeon);
    }
    drawR(img, 76, 36, 78, 18, colTitan);

    // Continuous Full-Width Neon Cyan Brow Light Bar
    fillR(img, 154, 52, 54, 3, colNeon);
    setPx(img, 207, 53, 0xFFFFFFFF);

    // Neon Magenta Battery Charge Status Strips along door sills
    fillR(img, 92, 70, 64, 2, colMagenta);
    setPx(img, 100, 70, 0xFFFFFFFF);
    setPx(img, 120, 70, 0xFFFFFFFF);
    setPx(img, 140, 70, 0xFFFFFFFF);

    // Active Electro-Magnetic Rear Winglets
    fillR(img, 26, 38, 20, 4, colTitan);
    fillR(img, 24, 34, 4, 12, colNeon);

    // High ground clearance angular cyber skirts
    drawL(img, 208, 68, 218, 60, colNeon);

    // Cut out Wheel Arches (rear x=61, front x=199)
    fillC(img, 61, 72, 26, 0x00000000);
    fillC(img, 199, 72, 26, 0x00000000);

    drawC(img, 61, 72, 26, colNeon);
    drawC(img, 199, 72, 26, colTitan);

    return img;
}

// =========================================================================
// WHEEL SPRITES (All 74x74, Monster Truck 84x84)
// =========================================================================

// 1. Off-Roader Wheel
QImage VehicleSprites::createOffroaderWheel() {
    QImage img(74, 74, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);
    int cx = 37, cy = 37;

    fillC(img, cx, cy, 32, 0xFF1C2024);
    drawC(img, cx, cy, 32, 0xFF0D0E10);
    drawC(img, cx, cy, 30, 0xFF2B3238);

    for (int a = 0; a < 12; ++a) {
        float ang = a * (3.14159f * 2.0f / 12.0f);
        int tx = static_cast<int>(cx + std::cos(ang) * 28.0f);
        int ty = static_cast<int>(cy + std::sin(ang) * 28.0f);
        fillR(img, tx - 1, ty - 1, 3, 3, 0xFF111417);
    }

    fillC(img, cx, cy, 20, 0xFFCFD8DC);
    drawC(img, cx, cy, 20, 0xFF263238);
    for (int i = 0; i < 5; ++i) {
        float ang = i * (3.14159f * 2.0f / 5.0f);
        int hx = static_cast<int>(cx + std::cos(ang) * 12.0f);
        int hy = static_cast<int>(cy + std::sin(ang) * 12.0f);
        fillC(img, hx, hy, 4, 0xFF263238);
    }
    fillC(img, cx, cy, 6, 0xFFECEFF1);
    fillC(img, cx, cy, 3, 0xFF37474F);

    return img;
}

// 2. Rally Wheel (White OZ Multi-Spoke with Red Caliper)
QImage VehicleSprites::createRallyWheel() {
    QImage img(74, 74, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);
    int cx = 37, cy = 37;

    fillC(img, cx, cy, 31, 0xFF1C2228);
    drawC(img, cx, cy, 31, 0xFF0E1114);

    fillC(img, cx, cy, 24, 0xFF455A64); // Slotted rotor
    fillR(img, cx + 9, cy - 12, 7, 16, 0xFFD50000); // Red Brembo caliper

    // 12-Spoke White OZ Rally Rim
    for (int i = 0; i < 12; ++i) {
        float ang = i * (3.14159f * 2.0f / 12.0f);
        int sx = static_cast<int>(cx + std::cos(ang) * 22.0f);
        int sy = static_cast<int>(cy + std::sin(ang) * 22.0f);
        drawL(img, cx, cy, sx, sy, 0xFFFFFFFF);
    }
    drawC(img, cx, cy, 22, 0xFFECEFF1);
    drawC(img, cx, cy, 21, 0xFFFFFFFF);
    fillC(img, cx, cy, 5, 0xFF1C252E);
    fillC(img, cx, cy, 2, 0xFFD50000);

    return img;
}

// 3. Modern Sports Coupe Wheel (Forged Bronze Split-5-Spoke)
QImage VehicleSprites::createSportsCoupeWheel() {
    QImage img(74, 74, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);
    int cx = 37, cy = 37;

    fillC(img, cx, cy, 30, 0xFF181C20);
    drawC(img, cx, cy, 30, 0xFF0D0F12);

    fillC(img, cx, cy, 23, 0xFF546E7A);
    fillR(img, cx + 10, cy - 10, 6, 14, 0xFF212121); // Black performance caliper

    // Forged Bronze Rim
    uint32_t colBronze = 0xFFBCAAA4;
    uint32_t colDarkBr = 0xFF8D6E63;
    for (int i = 0; i < 5; ++i) {
        float ang = i * (3.14159f * 2.0f / 5.0f);
        int sx1 = static_cast<int>(cx + std::cos(ang - 0.12f) * 21.0f);
        int sy1 = static_cast<int>(cy + std::sin(ang - 0.12f) * 21.0f);
        int sx2 = static_cast<int>(cx + std::cos(ang + 0.12f) * 21.0f);
        int sy2 = static_cast<int>(cy + std::sin(ang + 0.12f) * 21.0f);
        drawL(img, cx, cy, sx1, sy1, colBronze);
        drawL(img, cx, cy, sx2, sy2, colBronze);
    }
    drawC(img, cx, cy, 22, colBronze);
    drawC(img, cx, cy, 21, colDarkBr);
    fillC(img, cx, cy, 5, 0xFF1A1A1A);
    fillC(img, cx, cy, 2, colBronze);

    return img;
}

// 4. Supercar Wheel (Center-Lock Matte Black with Cyan Caliper)
QImage VehicleSprites::createSupercarWheel() {
    QImage img(74, 74, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);
    int cx = 37, cy = 37;

    fillC(img, cx, cy, 29, 0xFF121417);
    drawC(img, cx, cy, 29, 0xFF090A0C);

    fillC(img, cx, cy, 24, 0xFF37474F); // Carbon-ceramic disc
    fillR(img, cx + 9, cy - 12, 7, 16, 0xFF00E5FF); // Bright Cyan Caliper

    // Matte Black Y-Spoke Rim
    for (int i = 0; i < 5; ++i) {
        float ang = i * (3.14159f * 2.0f / 5.0f);
        int sx = static_cast<int>(cx + std::cos(ang) * 22.0f);
        int sy = static_cast<int>(cy + std::sin(ang) * 22.0f);
        drawL(img, cx, cy, sx, sy, 0xFF455A64);
        drawL(img, cx + 1, cy, sx + 1, sy, 0xFF212529);
    }
    drawC(img, cx, cy, 22, 0xFF455A64);
    fillC(img, cx, cy, 5, 0xFFFF6D00); // Anodized Orange Center Nut
    fillC(img, cx, cy, 2, 0xFFFFFFFF);

    return img;
}

// 5. Pickup Wheel (8-Lug Polished Steel with Gold Beadlock Ring)
QImage VehicleSprites::createPickupWheel() {
    QImage img(74, 74, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);
    int cx = 37, cy = 37;

    fillC(img, cx, cy, 35, 0xFF1A1F24);
    drawC(img, cx, cy, 35, 0xFF0E1114);

    for (int a = 0; a < 14; ++a) {
        float ang = a * (3.14159f * 2.0f / 14.0f);
        int tx = static_cast<int>(cx + std::cos(ang) * 31.0f);
        int ty = static_cast<int>(cy + std::sin(ang) * 31.0f);
        fillR(img, tx - 2, ty - 2, 5, 5, 0xFF111417);
    }

    drawC(img, cx, cy, 24, 0xFFFFB300);
    drawC(img, cx, cy, 23, 0xFFFF8F00);
    fillC(img, cx, cy, 22, 0xFF263238);

    for (int i = 0; i < 8; ++i) {
        float ang = i * (3.14159f * 2.0f / 8.0f);
        int bx = static_cast<int>(cx + std::cos(ang) * 23.5f);
        int by = static_cast<int>(cy + std::sin(ang) * 23.5f);
        setPx(img, bx, by, 0xFFFFD54F);
    }

    fillC(img, cx, cy, 14, 0xFF37474F);
    fillC(img, cx, cy, 8,  0xFF212121);
    fillC(img, cx, cy, 4,  0xFFECEFF1);

    return img;
}

// 6. Monster Truck Wheel (Giant 66-Inch Chevron Tractor Tire on 84x84)
QImage VehicleSprites::createMonsterTruckWheel() {
    QImage img(84, 84, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);
    int cx = 42, cy = 42;

    fillC(img, cx, cy, 40, 0xFF15181C);
    drawC(img, cx, cy, 40, 0xFF0B0D0F);

    // Giant Chevron Tractor Lugs (16 deep angled paddles)
    for (int a = 0; a < 16; ++a) {
        float ang = a * (3.14159f * 2.0f / 16.0f);
        int px1 = static_cast<int>(cx + std::cos(ang) * 38.0f);
        int py1 = static_cast<int>(cy + std::sin(ang) * 38.0f);
        int px2 = static_cast<int>(cx + std::cos(ang + 0.15f) * 29.0f);
        int py2 = static_cast<int>(cy + std::sin(ang + 0.15f) * 29.0f);
        drawL(img, px1, py1, px2, py2, 0xFF242A30);
        drawL(img, px1 + 1, py1, px2 + 1, py2, 0xFF0D0F11);
    }

    // Neon Lime Green Beadlock Outer Ring
    drawC(img, cx, cy, 26, 0xFF76FF03);
    drawC(img, cx, cy, 25, 0xFF64DD17);

    // Planetary Gear Hub Case (Heavy industrial steel)
    fillC(img, cx, cy, 24, 0xFF263238);
    fillC(img, cx, cy, 16, 0xFF37474F);
    fillC(img, cx, cy, 9,  0xFF1B2226);
    fillC(img, cx, cy, 4,  0xFFECEFF1);

    for (int b = 0; b < 10; ++b) {
        float ang = b * (3.14159f * 2.0f / 10.0f);
        int bx = static_cast<int>(cx + std::cos(ang) * 25.5f);
        int by = static_cast<int>(cy + std::sin(ang) * 25.5f);
        setPx(img, bx, by, 0xFFFFFFFF);
    }

    return img;
}

// 7. Desert Rally Raid Wheel (Orange Beadlock with Baja Mud Lugs)
QImage VehicleSprites::createDesertRaidWheel() {
    QImage img(74, 74, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);
    int cx = 37, cy = 37;

    fillC(img, cx, cy, 36, 0xFF1C2024);
    drawC(img, cx, cy, 36, 0xFF0E1114);

    for (int a = 0; a < 12; ++a) {
        float ang = a * (3.14159f * 2.0f / 12.0f);
        int tx = static_cast<int>(cx + std::cos(ang) * 32.0f);
        int ty = static_cast<int>(cy + std::sin(ang) * 32.0f);
        fillR(img, tx - 2, ty - 2, 4, 4, 0xFF111417);
    }

    // High-visibility Orange Beadlock Ring
    drawC(img, cx, cy, 25, 0xFFFF6D00);
    drawC(img, cx, cy, 24, 0xFFFFAB40);
    fillC(img, cx, cy, 23, 0xFF212529);

    // 8 Silver Beadlock Bolts
    for (int i = 0; i < 8; ++i) {
        float ang = i * (3.14159f * 2.0f / 8.0f);
        int bx = static_cast<int>(cx + std::cos(ang) * 24.5f);
        int by = static_cast<int>(cy + std::sin(ang) * 24.5f);
        setPx(img, bx, by, 0xFFECEFF1);
    }

    fillC(img, cx, cy, 12, 0xFF37474F);
    fillC(img, cx, cy, 5,  0xFFFF6D00);

    return img;
}

// 8. Formula Wheel (Open Racing Slick with Gold Magnesium Rim)
QImage VehicleSprites::createFormulaWheel() {
    QImage img(74, 74, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);
    int cx = 37, cy = 37;

    fillC(img, cx, cy, 28, 0xFF121417);
    drawC(img, cx, cy, 28, 0xFF0A0C0E);

    // Yellow Pirelli-Style Compound Sidewall Stripe
    drawC(img, cx, cy, 26, 0xFFFFD600);

    // Golden Magnesium Multi-Spoke Center
    uint32_t colGold = 0xFFFFC107;
    fillC(img, cx, cy, 21, 0xFF212121);
    for (int i = 0; i < 10; ++i) {
        float ang = i * (3.14159f * 2.0f / 10.0f);
        int sx = static_cast<int>(cx + std::cos(ang) * 19.0f);
        int sy = static_cast<int>(cy + std::sin(ang) * 19.0f);
        drawL(img, cx, cy, sx, sy, colGold);
    }
    drawC(img, cx, cy, 19, colGold);
    fillC(img, cx, cy, 5, 0xFFD50000); // Red Center Nut
    fillC(img, cx, cy, 2, 0xFFFFFFFF);

    return img;
}

// 9. Retro Muscle Car Wheel (Torq-Thrust 5-Spoke Chrome Rim)
QImage VehicleSprites::createMuscleCarWheel() {
    QImage img(74, 74, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);
    int cx = 37, cy = 37;

    fillC(img, cx, cy, 33, 0xFF1A1C20);
    drawC(img, cx, cy, 33, 0xFF0D0F11);
    drawC(img, cx, cy, 31, 0xFF2A2E34);

    // Polished Mirror Chrome 5-Spoke Star Rim
    uint32_t colChrome = 0xFFECEFF1;
    uint32_t colShade  = 0xFF90A4AE;
    fillC(img, cx, cy, 23, 0xFF263238);

    for (int i = 0; i < 5; ++i) {
        float ang = i * (3.14159f * 2.0f / 5.0f);
        int sx = static_cast<int>(cx + std::cos(ang) * 21.0f);
        int sy = static_cast<int>(cy + std::sin(ang) * 21.0f);
        drawL(img, cx, cy, sx, sy, colChrome);
        drawL(img, cx + 1, cy, sx + 1, sy, colChrome);
        drawL(img, cx - 1, cy, sx - 1, sy, colShade);
    }
    drawC(img, cx, cy, 22, colChrome);
    drawC(img, cx, cy, 21, 0xFFFFFFFF);

    // Deep-dish chrome bullet center cap
    fillC(img, cx, cy, 7, colChrome);
    fillC(img, cx, cy, 4, 0xFFCFD8DC);
    fillC(img, cx, cy, 2, 0xFFFFFFFF);

    return img;
}

// 10. Futuristic Electric Off-Roader Wheel (Directional Turbofan Carbon Disc)
QImage VehicleSprites::createElectricOffroaderWheel() {
    QImage img(74, 74, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);
    int cx = 37, cy = 37;

    fillC(img, cx, cy, 33, 0xFF15181C);
    drawC(img, cx, cy, 33, 0xFF0D0F12);

    // Futuristic smart tread blocks
    for (int a = 0; a < 10; ++a) {
        float ang = a * (3.14159f * 2.0f / 10.0f);
        int tx = static_cast<int>(cx + std::cos(ang) * 29.0f);
        int ty = static_cast<int>(cy + std::sin(ang) * 29.0f);
        fillR(img, tx - 1, ty - 1, 3, 3, 0xFF00E5FF);
    }

    // Glowing Neon Cyan Rim Ring
    drawC(img, cx, cy, 24, 0xFF00E5FF);
    drawC(img, cx, cy, 23, 0xFF80D8FF);

    // Directional Carbon Turbofan Blades
    fillC(img, cx, cy, 22, 0xFF212529);
    for (int i = 0; i < 6; ++i) {
        float ang = i * (3.14159f * 2.0f / 6.0f);
        int px1 = static_cast<int>(cx + std::cos(ang) * 21.0f);
        int py1 = static_cast<int>(cy + std::sin(ang) * 21.0f);
        int px2 = static_cast<int>(cx + std::cos(ang + 0.35f) * 10.0f);
        int py2 = static_cast<int>(cy + std::sin(ang + 0.35f) * 10.0f);
        drawL(img, px1, py1, px2, py2, 0xFF37474F);
        drawL(img, px1, py1 + 1, px2, py2 + 1, 0xFF00E5FF);
    }

    fillC(img, cx, cy, 8, 0xFF263238);
    fillC(img, cx, cy, 5, 0xFF00E5FF);
    fillC(img, cx, cy, 2, 0xFFFFFFFF);

    return img;
}

// =========================================================================
// DRIVER SPRITES (Bill Newton, Sarah Swift, Rusty Bob, Cosmo Neil)
// =========================================================================

// 1. Bill Newton
QImage VehicleSprites::createBillDriver() {
    QImage img(90, 90, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);
    int cx = 45, cy = 48;

    fillR(img, 28, 74, 34, 16, 0xFF1565C0);
    fillR(img, 36, 74, 18, 16, 0xFFD32F2F);
    fillR(img, 39, 64, 12, 12, 0xFFFFCC80);

    fillC(img, cx, cy, 20, 0xFFFFB74D);
    fillR(img, cx - 18, cy - 2, 10, 4, 0xFF4E342E);
    fillC(img, cx + 9, cy, 3, 0xFF212121);
    setPx(img, cx + 10, cy - 1, 0xFFFFFFFF);

    drawL(img, cx + 6, cy - 6, cx + 14, cy - 8, 0xFF3E2723);
    drawL(img, cx + 7, cy + 9, cx + 16, cy + 7, 0xFFB71C1C);

    fillR(img, cx - 22, cy - 22, 44, 18, 0xFFD32F2F);
    fillR(img, cx - 22, cy - 22, 44, 4,  0xFFFF5252);
    fillR(img, cx - 36, cy - 16, 16, 6,  0xFFB71C1C);
    fillC(img, cx, cy - 22, 3, 0xFFECEFF1);

    return img;
}

// 2. Sarah Swift
QImage VehicleSprites::createSarahDriver() {
    QImage img(90, 90, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);
    int cx = 45, cy = 48;

    fillR(img, 28, 74, 34, 16, 0xFFECEFF1);
    fillR(img, 36, 74, 18, 16, 0xFF00E5FF);
    fillR(img, 40, 64, 10, 12, 0xFF263238);

    fillC(img, cx, cy, 22, 0xFFECEFF1);
    fillC(img, cx, cy, 21, 0xFFFFFFFF);

    drawL(img, cx - 20, cy - 6, cx + 20, cy - 6, 0xFF00E5FF);
    drawL(img, cx - 20, cy - 5, cx + 20, cy - 5, 0xFF00E5FF);
    drawL(img, cx - 20, cy - 4, cx + 20, cy - 4, 0xFFFFB300);

    fillR(img, cx - 2, cy - 4, 23, 14, 0xEE1A237E);
    drawR(img, cx - 2, cy - 4, 23, 14, 0xFF101418);
    drawL(img, cx + 2, cy + 6, cx + 16, cy - 2, 0xFF00E5FF);
    drawL(img, cx + 3, cy + 6, cx + 17, cy - 2, 0xFFFFFFFF);

    fillR(img, cx + 4, cy + 12, 14, 7, 0xFFCFD8DC);
    fillR(img, cx + 6, cy + 14, 3, 2,  0xFF263238);
    fillR(img, cx + 11, cy + 14, 3, 2, 0xFF263238);

    return img;
}

// 3. Rusty Bob
QImage VehicleSprites::createBobDriver() {
    QImage img(90, 90, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);
    int cx = 45, cy = 50;

    fillR(img, 28, 74, 34, 16, 0xFF3E4E38);
    fillR(img, 36, 74, 18, 8,  0xFFD7CCC8);
    fillR(img, 39, 64, 12, 12, 0xFFFFCC80);

    fillC(img, cx, cy, 20, 0xFFFFB74D);

    fillR(img, cx - 4, cy + 8, 22, 6, 0xFFECEFF1);
    drawR(img, cx - 4, cy + 8, 22, 6, 0xFFB0BEC5);

    drawL(img, cx + 6, cy - 1, cx + 12, cy - 1, 0xFF3E2723);
    drawL(img, cx + 12, cy - 2, cx + 14, cy - 4, 0xFF8D6E63);

    fillC(img, cx + 7, cy - 7, 7, 0xFF78909C);
    fillC(img, cx + 7, cy - 7, 4, 0xFF81D4FA);
    drawC(img, cx + 7, cy - 7, 7, 0xFFFFB300);
    fillR(img, cx - 18, cy - 9, 24, 4, 0xFF5D4037);

    fillR(img, cx - 22, cy - 24, 44, 16, 0xFF6D4C41);
    fillR(img, cx - 22, cy - 24, 44, 3,  0xFF8D6E63);
    fillR(img, cx + 6, cy - 14, 18, 5, 0xFF5D4037);

    return img;
}

// 4. Cosmo Neil
QImage VehicleSprites::createNeilDriver() {
    QImage img(90, 90, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);
    int cx = 45, cy = 46;

    fillR(img, 28, 74, 34, 16, 0xFFECEFF1);
    fillR(img, 32, 68, 26, 8,  0xFFCFD8DC);
    fillC(img, 45, 68, 6, 0xFF1565C0);

    fillC(img, cx, cy, 24, 0x5590CAF9);
    drawC(img, cx, cy, 24, 0xFFECEFF1);

    fillC(img, cx + 2, cy - 2, 17, 0xFFFFB300);
    fillC(img, cx + 2, cy - 2, 14, 0xFFFFD54F);

    drawL(img, cx - 10, cy, cx + 16, cy + 2, 0xFF3E2723);
    setPx(img, cx + 6, cy - 8, 0xFFFFFFFF);
    setPx(img, cx + 10, cy - 6, 0xFFFFFFFF);

    fillR(img, cx + 16, cy + 8, 6, 3, 0xFF212121);

    return img;
}

} // namespace Graphics
