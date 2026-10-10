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
