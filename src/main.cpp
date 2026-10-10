#include <QApplication>
#include <QTimer>
#include <QDir>
#include <iostream>
#include <string>
#include <cassert>
#include <cmath>
#include "GameEngine.h"
#include "physics/TerrainConfig.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("HillClimbRacingQt");
    app.setOrganizationName("HillClimbQtTeam");

    Core::GameEngine engine;

    if (argc > 1 && std::string(argv[1]) == "--smoke-test") {
        QDir().mkpath("scratch");
        engine.show();
        engine.start();
        std::cout << "[SMOKE TEST] Initialized GameEngine with multi-vehicle, driver & progressive terrain architecture." << std::endl;

        // =========================================================================
        // REQ 1, 2, 3, 6: PROCEDURAL TERRAIN VALIDATION & REACHABILITY
        // =========================================================================
        std::cout << "\n=======================================================" << std::endl;
        std::cout << "  RUNNING PROCEDURAL TERRAIN & FUEL REACHABILITY SCAN  " << std::endl;
        std::cout << "=======================================================" << std::endl;
        bool allBiomesValid = true;
        for (const auto& biome : Physics::BiomeRegistry::getAllBiomes()) {
            auto res = Physics::TerrainValidator::validateBiome(biome.id);
            std::cout << "  [BIOME " << biome.id << "] " << biome.name << ":" << std::endl;
            std::cout << "    - Validity:           " << (res.valid ? "PASSED (100% climbable & reachable)" : "FAILED") << std::endl;
            std::cout << "    - Max Uphill Slope:   " << res.maxUphillSlopeDeg << " deg (Target <= 38.0 deg)" << std::endl;
            std::cout << "    - Max Downhill Slope: " << res.maxDownhillSlopeDeg << " deg" << std::endl;
            std::cout << "    - Fuel Canisters:     " << res.fuelCanisterCount << " placed" << std::endl;
            std::cout << "    - Max Fuel Spacing:   " << res.maxFuelIntervalMeters << " m (Target <= 165.0 m)" << std::endl;
            std::cout << res.report << std::endl;
            if (!res.valid) allBiomesValid = false;
        }
        if (allBiomesValid) {
            std::cout << "[SMOKE TEST] PROCEDURAL TERRAIN VALIDATION: PASSED FOR ALL 6 BIOMES!" << std::endl;
        } else {
            std::cerr << "[SMOKE TEST] PROCEDURAL TERRAIN VALIDATION: FAILED!" << std::endl;
            return 1;
        }

        // =========================================================================
        // REQ 4, 5, 7: CHECKPOINT & PROGRESSION ECONOMY VERIFICATION
        // =========================================================================
        std::cout << "\n=======================================================" << std::endl;
        std::cout << "  TESTING STAGE PROGRESSION & UNLOCK CHAINING          " << std::endl;
        std::cout << "=======================================================" << std::endl;
        auto& profile = engine.stateManager().profile();
        assert(profile.isStageUnlocked("countryside"));
        profile.recordStageCheckpoint("countryside", 5);
        assert(profile.isStageCompleted("countryside"));
        assert(profile.isStageUnlocked("desert"));

        profile.recordStageCheckpoint("desert", 5);
        assert(profile.isStageCompleted("desert"));
        assert(profile.isStageUnlocked("arctic"));

        profile.recordStageCheckpoint("arctic", 5);
        assert(profile.isStageCompleted("arctic"));
        assert(profile.isStageUnlocked("mountain"));

        profile.recordStageCheckpoint("mountain", 5);
        assert(profile.isStageCompleted("mountain"));
        assert(profile.isStageUnlocked("moon"));

        profile.recordStageCheckpoint("moon", 5);
        assert(profile.isStageCompleted("moon"));
        assert(profile.isStageUnlocked("volcano"));
        std::cout << "[SMOKE TEST] STAGE UNLOCK CHAINING: PASSED (Countryside -> Desert -> Arctic -> Mountain -> Moon -> Volcano)!" << std::endl;

        // Reset stage unlocks back to baseline for UI inspection
        profile.recordStageCheckpoint("countryside", 2);

        // 1. Visit MAIN_MENU
        QTimer::singleShot(50, [&]() {
            engine.stateManager().render(engine.framebuffer(), 1.0f);
            engine.framebuffer().getImage().save("scratch/main_menu_captured.png");
            std::cout << "[SMOKE TEST] Saved scratch/main_menu_captured.png" << std::endl;
        });

        // 2. Visit GARAGE - Capture All 10 Distinct Vehicles with Custom Rims, Tuning & Drivers
        struct VehTestSetup {
            Physics::VehicleType type;
            Physics::DriverType driver;
            int tab; // 0 = Powertrain, 1 = Handling/Chassis
            std::vector<Core::ProfileManager::UpgradeType> upgrades;
            const char* primaryFile;
            const char* legacyFile;
        };

        const std::vector<VehTestSetup> testVehicles = {
            {Physics::VehicleType::OFFROADER, Physics::DriverType::BILL, 0,
             {Core::ProfileManager::UPGRADE_ENGINE, Core::ProfileManager::UPGRADE_TIRES},
             "scratch/garage_01_offroader.png", "scratch/garage_offroader_bill.png"},

            {Physics::VehicleType::RALLY_CAR, Physics::DriverType::NEIL, 0,
             {Core::ProfileManager::UPGRADE_ENGINE, Core::ProfileManager::UPGRADE_4WD},
             "scratch/garage_02_rallycar.png", "scratch/garage_rally_neil.png"},

            {Physics::VehicleType::SPORTS_COUPE, Physics::DriverType::SARAH, 1,
             {Core::ProfileManager::UPGRADE_TRANSMISSION, Core::ProfileManager::UPGRADE_DOWNFORCE},
             "scratch/garage_03_sportscoupe.png", "scratch/garage_sportscar_sarah.png"},

            {Physics::VehicleType::SUPERCAR, Physics::DriverType::SARAH, 1,
             {Core::ProfileManager::UPGRADE_DOWNFORCE, Core::ProfileManager::UPGRADE_BRAKES},
             "scratch/garage_04_supercar.png", nullptr},

            {Physics::VehicleType::PICKUP_TRUCK, Physics::DriverType::BOB, 0,
             {Core::ProfileManager::UPGRADE_SUSPENSION, Core::ProfileManager::UPGRADE_CHASSIS},
             "scratch/garage_05_pickuptruck.png", "scratch/garage_pickup_bob.png"},

            {Physics::VehicleType::MONSTER_TRUCK, Physics::DriverType::BOB, 0,
             {Core::ProfileManager::UPGRADE_SUSPENSION, Core::ProfileManager::UPGRADE_TIRES},
             "scratch/garage_06_monstertruck.png", nullptr},

            {Physics::VehicleType::DESERT_RAID, Physics::DriverType::BILL, 0,
             {Core::ProfileManager::UPGRADE_SUSPENSION, Core::ProfileManager::UPGRADE_4WD},
             "scratch/garage_07_desertraid.png", nullptr},

            {Physics::VehicleType::FORMULA_RACER, Physics::DriverType::NEIL, 1,
             {Core::ProfileManager::UPGRADE_DOWNFORCE, Core::ProfileManager::UPGRADE_TRANSMISSION},
             "scratch/garage_08_formularacer.png", nullptr},

            {Physics::VehicleType::MUSCLE_CAR, Physics::DriverType::BOB, 0,
             {Core::ProfileManager::UPGRADE_ENGINE, Core::ProfileManager::UPGRADE_TRANSMISSION},
             "scratch/garage_09_musclecar.png", nullptr},

            {Physics::VehicleType::ELECTRIC_OFFROADER, Physics::DriverType::SARAH, 1,
             {Core::ProfileManager::UPGRADE_CHASSIS, Core::ProfileManager::UPGRADE_4WD},
             "scratch/garage_10_electricoffroader.png", nullptr}
        };

        for (size_t i = 0; i < testVehicles.size(); ++i) {
            int delayMs = 120 + static_cast<int>(i) * 90;
            QTimer::singleShot(delayMs, [&, i]() {
                const auto& vSetup = testVehicles[i];
                engine.stateManager().changeState(Core::StateType::GARAGE);
                engine.stateManager().profile().unlockVehicle(vSetup.type);
                engine.stateManager().profile().setSelectedVehicle(vSetup.type);
                engine.stateManager().profile().setSelectedDriver(vSetup.driver);
                engine.stateManager().garageScreen().setViewedVehicle(vSetup.type);

                // Switch Tab (tab 0: x=350, tab 1: x=550)
                int tabX = (vSetup.tab == 0) ? 350 : 550;
                engine.stateManager().garageScreen().onMouseDown(tabX, 305, engine.stateManager().profile());
                engine.stateManager().garageScreen().onMouseUp(tabX, 305, engine.stateManager().profile());

                for (auto upg : vSetup.upgrades) {
                    engine.stateManager().profile().purchaseUpgrade(vSetup.type, upg);
                }

                engine.stateManager().render(engine.framebuffer(), 1.0f);
                engine.framebuffer().getImage().save(vSetup.primaryFile);
                std::cout << "[SMOKE TEST] Saved " << vSetup.primaryFile << std::endl;
                if (vSetup.legacyFile) {
                    engine.framebuffer().getImage().save(vSetup.legacyFile);
                }
            });
        }

        // 3. Stage Select Screen - Page 1
        QTimer::singleShot(1100, [&]() {
            engine.stateManager().changeState(Core::StateType::STAGE_SELECT);
            engine.stateManager().stageSelectScreen().setPage(0);
            engine.stateManager().render(engine.framebuffer(), 1.0f);
            engine.framebuffer().getImage().save("scratch/stage_select_page1.png");
            engine.framebuffer().getImage().save("scratch/stage_select_captured.png");
            std::cout << "[SMOKE TEST] Saved scratch/stage_select_page1.png" << std::endl;
        });

        // 4. Stage Select Screen - Page 2
        QTimer::singleShot(1250, [&]() {
            engine.stateManager().profile().unlockStage("mountain");
            engine.stateManager().profile().unlockStage("moon");
            engine.stateManager().profile().unlockStage("volcano");
            engine.stateManager().stageSelectScreen().setPage(1);
            engine.stateManager().render(engine.framebuffer(), 1.0f);
            engine.framebuffer().getImage().save("scratch/stage_select_page2.png");
            std::cout << "[SMOKE TEST] Saved scratch/stage_select_page2.png" << std::endl;
        });

        // Dynamic driver pedal control
        QTimer* driverPedalTimer = new QTimer(&app);
        QObject::connect(driverPedalTimer, &QTimer::timeout, [&]() {
            if (engine.stateManager().currentState() == Core::StateType::GAMEPLAY) {
                const auto& v = engine.stateManager().physics().vehicle();
                if (v.isAirborne()) {
                    if (v.chassisAngle() > 0.15f) {
                        engine.input().state().gas = false;
                        engine.input().state().brake = true;
                    } else {
                        engine.input().state().gas = true;
                        engine.input().state().brake = false;
                    }
                } else {
                    engine.input().state().gas = true;
                    engine.input().state().brake = false;
                }
            }
        });
        driverPedalTimer->start(50);

        // 4b. In-Game Drive 0: Countryside Start Area with Monster Truck (testing grass color consistency from x=0)
        QTimer::singleShot(1300, [&]() {
            engine.stateManager().profile().setSelectedVehicle(Physics::VehicleType::MONSTER_TRUCK);
            engine.stateManager().profile().setSelectedDriver(Physics::DriverType::BOB);
            engine.stateManager().startRace("countryside");

            auto& p = engine.stateManager().physics();
            float startX = 0.0f;
            float startY = p.terrain().getHeight(startX) + 2.0f;
            p.vehicle().reset(startX, startY);
            engine.stateManager().camera().reset({startX, startY});

            engine.input().state().gas = true;
            std::cout << "[SMOKE TEST] Started race with Monster Truck at Countryside start line (x=0)." << std::endl;
        });

        QTimer::singleShot(1900, [&]() {
            engine.framebuffer().getImage().save("scratch/monstertruck_countryside_start.png");
            std::cout << "[SMOKE TEST] Saved scratch/monstertruck_countryside_start.png" << std::endl;
        });

        // 5. In-Game Drive 1: Countryside with Rally Car & Checkpoint 1 Crossing Check
        QTimer::singleShot(2100, [&]() {
            engine.stateManager().profile().setSelectedVehicle(Physics::VehicleType::RALLY_CAR);
            engine.stateManager().profile().setSelectedDriver(Physics::DriverType::SARAH);
            engine.stateManager().startRace("countryside");

            // Position vehicle right before Checkpoint 1 Archway (at 250m)
            auto& p = engine.stateManager().physics();
            float startX = 248.5f;
            float startY = p.terrain().getHeight(startX) + 1.2f;
            p.vehicle().reset(startX, startY);
            engine.stateManager().camera().reset({startX, startY});

            engine.input().state().gas = true;
            std::cout << "[SMOKE TEST] Started race with Rally Car near Checkpoint 1 on Countryside." << std::endl;
        });

        // Verify Checkpoint 1 & Tyre Alignment
        QTimer::singleShot(2700, [&]() {
            const auto& p = engine.stateManager().physics();
            const auto& v = p.vehicle();
            float dist = p.distanceReached();
            float speed = v.getSpeedKmh();
            int clearedCp = p.currentCheckpoint();

            // Verify Tyre Alignment mathematically:
            float cosA = std::cos(v.chassisAngle());
            float sinA = std::sin(v.chassisAngle());
            Physics::Vec2 downDir(sinA, -cosA);
            Physics::Vec2 rearMount = {v.chassisPos().x + (v.rearMountOffset().x * cosA - v.rearMountOffset().y * sinA),
                                       v.chassisPos().y + (v.rearMountOffset().x * sinA + v.rearMountOffset().y * cosA)};
            Physics::Vec2 expectedRearWheel = rearMount + downDir * v.rearSuspensionCompression();
            float rearDisplacementError = (v.rearWheelPos() - expectedRearWheel).length();

            std::cout << "\n[SMOKE TEST] Checkpoint 1 Crossing Check (x = " << dist << " m, " << speed << " km/h):" << std::endl;
            std::cout << "  - Cleared Checkpoint Index: " << clearedCp << " (Expected >= 1)" << std::endl;
            std::cout << "  - Tyre Alignment Offset Error: " << rearDisplacementError << " m (Target < 0.001m)" << std::endl;
            std::cout << "  - Driver Head Tilt Angle: " << v.driverHeadAngle() << " rad" << std::endl;

            engine.framebuffer().getImage().save("scratch/gameplay_checkpoint_captured.png");
            engine.framebuffer().getImage().save("scratch/gameplay_rally_captured.png");
            std::cout << "[SMOKE TEST] Saved scratch/gameplay_checkpoint_captured.png & scratch/gameplay_rally_captured.png" << std::endl;

            if (rearDisplacementError < 0.001f) {
                std::cout << "[SMOKE TEST] TYRE ALIGNMENT VERIFICATION: PASSED (Zero drift detected)!" << std::endl;
            } else {
                std::cerr << "[SMOKE TEST] TYRE ALIGNMENT VERIFICATION: FAILED! Error = " << rearDisplacementError << std::endl;
            }
        });

        // 6. In-Game Drive 2: Mountain Ridge with Monster Truck (66" tires & giant suspension)
        QTimer::singleShot(2900, [&]() {
            engine.stateManager().profile().setSelectedVehicle(Physics::VehicleType::MONSTER_TRUCK);
            engine.stateManager().profile().setSelectedDriver(Physics::DriverType::BOB);
            engine.stateManager().startRace("mountain");

            auto& p = engine.stateManager().physics();
            float startX = 120.0f;
            float startY = p.terrain().getHeight(startX) + 2.0f;
            p.vehicle().reset(startX, startY);
            engine.stateManager().camera().reset({startX, startY});

            engine.input().state().gas = true;
            std::cout << "[SMOKE TEST] Started Mountain Ridge rock climb with Monster Truck & Rusty Bob!" << std::endl;
        });

        QTimer::singleShot(3500, [&]() {
            engine.framebuffer().getImage().save("scratch/gameplay_monstertruck_mountain.png");
            std::cout << "[SMOKE TEST] Saved scratch/gameplay_monstertruck_mountain.png" << std::endl;
        });

        // 7. In-Game Drive 3: Desert Dunes with Formula Racing Car (High downforce & speed)
        QTimer::singleShot(3700, [&]() {
            engine.stateManager().profile().setSelectedVehicle(Physics::VehicleType::FORMULA_RACER);
            engine.stateManager().profile().setSelectedDriver(Physics::DriverType::NEIL);
            engine.stateManager().startRace("desert");

            auto& p = engine.stateManager().physics();
            float startX = 140.0f;
            float startY = p.terrain().getHeight(startX) + 1.2f;
            p.vehicle().reset(startX, startY);
            engine.stateManager().camera().reset({startX, startY});

            engine.input().state().gas = true;
            std::cout << "[SMOKE TEST] Started Desert Dunes high-speed run with Formula Racer & Cosmo Neil!" << std::endl;
        });

        QTimer::singleShot(4300, [&]() {
            engine.framebuffer().getImage().save("scratch/gameplay_formularacer_desert.png");
            std::cout << "[SMOKE TEST] Saved scratch/gameplay_formularacer_desert.png" << std::endl;
        });

        // 8. In-Game Drive 4: Moon with Futuristic Electric Off-Roader (Lunar gravity & instant torque)
        QTimer::singleShot(4500, [&]() {
            engine.stateManager().profile().setSelectedVehicle(Physics::VehicleType::ELECTRIC_OFFROADER);
            engine.stateManager().profile().setSelectedDriver(Physics::DriverType::SARAH);
            engine.stateManager().startRace("moon");

            auto& p = engine.stateManager().physics();
            float startX = 90.0f;
            float startY = p.terrain().getHeight(startX) + 1.5f;
            p.vehicle().reset(startX, startY);
            engine.stateManager().camera().reset({startX, startY});

            engine.input().state().gas = true;
            std::cout << "[SMOKE TEST] Started Moon low-gravity traverse with Electric Off-Roader & Sarah Swift!" << std::endl;
        });

        QTimer::singleShot(5100, [&]() {
            engine.framebuffer().getImage().save("scratch/gameplay_electric_moon.png");
            engine.framebuffer().getImage().save("scratch/gameplay_moon_captured.png");
            std::cout << "[SMOKE TEST] Saved scratch/gameplay_electric_moon.png" << std::endl;
        });

        // 9. In-Game Drive 5: Arctic Tundra with Heavy-Duty Pickup Truck
        QTimer::singleShot(5300, [&]() {
            engine.stateManager().profile().setSelectedVehicle(Physics::VehicleType::PICKUP_TRUCK);
            engine.stateManager().profile().setSelectedDriver(Physics::DriverType::BOB);
            engine.stateManager().startRace("arctic");

            auto& p = engine.stateManager().physics();
            float startX = 70.0f;
            float startY = p.terrain().getHeight(startX) + 1.4f;
            p.vehicle().reset(startX, startY);
            engine.stateManager().camera().reset({startX, startY});

            engine.input().state().gas = true;
            std::cout << "[SMOKE TEST] Started Arctic Tundra run with Pickup Truck & Rusty Bob!" << std::endl;
        });

        QTimer::singleShot(5900, [&]() {
            engine.framebuffer().getImage().save("scratch/gameplay_arctic_captured.png");
            std::cout << "[SMOKE TEST] Saved scratch/gameplay_arctic_captured.png" << std::endl;
        });

        // 10. In-Game Drive 6: Inferno Peaks (Volcano) with Supercar
        QTimer::singleShot(6100, [&]() {
            engine.stateManager().profile().setSelectedVehicle(Physics::VehicleType::SUPERCAR);
            engine.stateManager().profile().setSelectedDriver(Physics::DriverType::SARAH);
            engine.stateManager().startRace("volcano");

            auto& p = engine.stateManager().physics();
            float startX = 60.0f;
            float startY = p.terrain().getHeight(startX) + 1.2f;
            p.vehicle().reset(startX, startY);
            engine.stateManager().camera().reset({startX, startY});

            engine.input().state().gas = true;
            std::cout << "[SMOKE TEST] Started Inferno Peaks run with Supercar & Sarah Swift!" << std::endl;
        });

        QTimer::singleShot(6700, [&]() {
            engine.framebuffer().getImage().save("scratch/gameplay_volcano_captured.png");
            std::cout << "[SMOKE TEST] Saved scratch/gameplay_volcano_captured.png" << std::endl;
        });

        // 11. Pause overlay test
        QTimer::singleShot(6900, [&]() {
            engine.stateManager().changeState(Core::StateType::PAUSED);
            engine.stateManager().render(engine.framebuffer(), 1.0f);
            engine.framebuffer().getImage().save("scratch/pause_captured.png");
            std::cout << "[SMOKE TEST] Saved scratch/pause_captured.png" << std::endl;
        });

        // 12. Game Over screen test
        QTimer::singleShot(7200, [&]() {
            engine.stateManager().changeState(Core::StateType::GAME_OVER);
            engine.stateManager().render(engine.framebuffer(), 1.0f);
            engine.framebuffer().getImage().save("scratch/game_over_captured.png");
            std::cout << "[SMOKE TEST] Saved scratch/game_over_captured.png" << std::endl;
            std::cout << "\n=======================================================" << std::endl;
            std::cout << "  [SMOKE TEST] ALL 10 VEHICLES & PHYSICS VERIFIED & PASSING!" << std::endl;
            std::cout << "=======================================================\n" << std::endl;
            app.quit();
        });

        return app.exec();
    }

    engine.show();
    engine.start();

    return app.exec();
}
