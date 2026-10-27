#pragma once

#include "Framebuffer.h"
#include "RetroButton.h"
#include "ProfileManager.h"
#include "VehicleConfig.h"
#include <functional>

namespace UI {

/**
 * @brief Interactive Vehicle Garage, Driver Selector & Multi-Vehicle Tuning Shop.
 * Allows unlocking and upgrading 4 unique vehicles across 8 mechanical categories
 * with 4 distinct drivers and live hydraulic lift previews.
 */
class GarageScreen {
public:
    GarageScreen();

    void setDimensions(int width, int height);

    void setOnBack(std::function<void()> cb) { m_btnBack.setOnClick(cb); }
    void setOnDrive(std::function<void()> cb) { m_btnDrive.setOnClick(cb); }
    void setButtonSound(std::function<void()> cb);

    void onMouseMove(int px, int py);
    bool onMouseDown(int px, int py, Core::ProfileManager& profile);
    void onMouseUp(int px, int py, Core::ProfileManager& profile);

    void update(float dt);
    void render(Graphics::Framebuffer& fb, Core::ProfileManager& profile);

    Physics::VehicleType viewedVehicle() const { return m_viewedVehicle; }
    void setViewedVehicle(Physics::VehicleType type) { m_viewedVehicle = type; }

    bool isMultiplayerMode() const { return m_isMultiplayer; }
    void setMultiplayerMode(bool mp);
    int activeRacer() const { return m_activeRacer; }
    void setActiveRacer(int racer) { m_activeRacer = racer; }
    Physics::VehicleType p2Vehicle() const { return m_p2Vehicle; }
    void setP2Vehicle(Physics::VehicleType type) { m_p2Vehicle = type; }
    Physics::DriverType p2Driver() const { return m_p2Driver; }
    void setP2Driver(Physics::DriverType type) { m_p2Driver = type; }

private:
    int m_width;
    int m_height;
    float m_animTime;
    float m_carBounceY;
    float m_carBounceVel;
    int m_flashCardIdx;
    float m_flashTimer;

    Physics::VehicleType m_viewedVehicle;
    int m_activeTab; // 0 = Powertrain & Traction, 1 = Handling & Chassis

    bool m_isMultiplayer = false;
    int m_activeRacer = 0; // 0 = Player 1, 1 = Player 2
    Physics::VehicleType m_p2Vehicle = Physics::VehicleType::OFFROADER;
    Physics::DriverType m_p2Driver = Physics::DriverType::SARAH;
    RetroButton m_btnRacerToggle;

    // Navigation & Actions
    RetroButton m_btnBack;
    RetroButton m_btnDrive;

    // Vehicle Carousel
    RetroButton m_btnPrevVeh;
    RetroButton m_btnNextVeh;
    RetroButton m_btnUnlockVeh;

    // Driver Carousel
    RetroButton m_btnPrevDriver;
    RetroButton m_btnNextDriver;

    // Tuning Category Tabs
    RetroButton m_btnTabPowertrain;
    RetroButton m_btnTabChassis;

    // 8 Upgrade Buttons
    RetroButton m_btnUpgrades[Core::ProfileManager::UPGRADE_COUNT];

    void drawVehiclePreview(Graphics::Framebuffer& fb, int cx, int cy, const Core::ProfileManager& profile);
};

} // namespace UI
