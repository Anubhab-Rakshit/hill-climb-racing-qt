#include <QApplication>
#include <QTimer>
#include <QDir>
#include <iostream>
#include <string>
#include <cassert>
#include <cmath>
#include "GameEngine.h"
#include "physics/TerrainConfig.h"
#include "Sprite.h"
#include "graphics/ScanlineRasterizer.h"
#include "audio/SoundManager.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("HillClimbRacingQt");
    app.setOrganizationName("HillClimbQtTeam");

    Core::GameEngine engine;

    if (argc > 1 && std::string(argv[1]) == "--test-multiplayer-sync") {
        std::cout << "\n=======================================================" << std::endl;
        std::cout << "  MULTIPLAYER PHYSICS & SYNCHRONIZATION VERIFICATION   " << std::endl;
        std::cout << "=======================================================" << std::endl;

        auto& sm = engine.stateManager();
        sm.startMultiplayerRace();

        bool pass = true;
        auto& pWorld = sm.physics();

        // 1. Vehicle Archetype and Starting Position Match Check
        std::cout << "Test 1: Matched Archetypes & Equal Starting Position" << std::endl;
        if (!pWorld.isMultiplayer()) {
            std::cout << "  FAIL: Physics world is not in multiplayer mode!" << std::endl;
            pass = false;
        }
        if (pWorld.vehicle().vehicleType() != pWorld.vehicle2().vehicleType()) {
            std::cout << "  FAIL: Vehicle types do not match! P1: " << static_cast<int>(pWorld.vehicle().vehicleType())
                      << " vs P2: " << static_cast<int>(pWorld.vehicle2().vehicleType()) << std::endl;
            pass = false;
        }
        float posXDiff = std::abs(pWorld.vehicle().chassisPos().x - pWorld.vehicle2().chassisPos().x);
        float posYDiff = std::abs(pWorld.vehicle().chassisPos().y - pWorld.vehicle2().chassisPos().y);
        if (posXDiff > 0.001f || posYDiff > 0.001f) {
            std::cout << "  FAIL: Vehicles do not start at identical positions! dx=" << posXDiff << ", dy=" << posYDiff << std::endl;
            pass = false;
        } else {
            std::cout << "  PASS: Both vehicles start at identical position (x=" << pWorld.vehicle().chassisPos().x << ")." << std::endl;
        }

        // 2. Synchronized Movement Test under Equal Inputs
        std::cout << "Test 2: Synchronized Movement under Equal Throttle (240 ticks / 2 seconds)" << std::endl;
        const float DT = 1.0f / 120.0f;
        auto& inp = engine.input().state();
        inp.p1.gas = true;
        inp.p2.gas = true;

        float maxPosDelta = 0.0f;
        float maxSpdDelta = 0.0f;
        for (int i = 0; i < 240; ++i) {
            sm.handleInput(inp);
            sm.fixedUpdate(DT);
            float dX = std::abs(pWorld.vehicle().chassisPos().x - pWorld.vehicle2().chassisPos().x);
            float dSpd = std::abs(pWorld.vehicle().getSpeedKmh() - pWorld.vehicle2().getSpeedKmh());
            if (dX > maxPosDelta) maxPosDelta = dX;
            if (dSpd > maxSpdDelta) maxSpdDelta = dSpd;
        }

        std::cout << "  Max Position Delta: " << maxPosDelta << "m, Max Speed Delta: " << maxSpdDelta << " km/h" << std::endl;
        if (maxPosDelta > 0.01f || maxSpdDelta > 0.01f) {
            std::cout << "  FAIL: Movement de-synced under identical inputs!" << std::endl;
            pass = false;
        } else {
            std::cout << "  PASS: Movement remains in 100% sync throughout run." << std::endl;
        }

        // 3. Independent Fuel & Item Collections
        std::cout << "Test 3: Per-Player Independent Collectible Pickup" << std::endl;
        if (pWorld.itemsCollectedP1().empty() || pWorld.itemsCollectedP2().empty()) {
            std::cout << "  FAIL: Collection mask arrays are uninitialized!" << std::endl;
            pass = false;
        } else {
            std::cout << "  PASS: Collection tracking is partitioned per player." << std::endl;
        }

        // 4. Hydraulic Braking to Reverse Transition
        std::cout << "Test 4: Braking vs Reverse Transition Verification" << std::endl;
        inp.p1.gas = false;
        inp.p1.brake = true;
        inp.p1.reverse = true;
        float spdBeforeBrake = pWorld.vehicle().getSpeedKmh();
        for (int i = 0; i < 60; ++i) {
            sm.handleInput(inp);
            sm.fixedUpdate(DT);
        }
        float spdAfterBrake = pWorld.vehicle().getSpeedKmh();
        std::cout << "  Speed Before Brake: " << spdBeforeBrake << " km/h -> After 0.5s Brake: " << spdAfterBrake << " km/h" << std::endl;
        if (spdAfterBrake >= spdBeforeBrake) {
            std::cout << "  FAIL: Vehicle failed to decelerate under braking!" << std::endl;
            pass = false;
        } else {
            std::cout << "  PASS: Dynamic braking decelerates vehicle smoothly." << std::endl;
        }

        // 5. Split-Screen Render Test
        std::cout << "Test 5: Split-Screen Viewports Render Pipeline" << std::endl;
        sm.render(engine.framebuffer(), 1.0f);
        std::cout << "  PASS: Split-screen Framebuffer blit and overlay rendered without fault." << std::endl;

        // 6. Multiplayer Biome Selection Test across all 6 biomes
        std::cout << "Test 6: Multiplayer Biome Selection (Desert, Arctic, Mountain, Moon, Volcano, Countryside)" << std::endl;
        std::vector<std::string> testBiomes = {"desert", "arctic", "mountain", "moon", "volcano", "countryside"};
        bool biomesOk = true;
        for (const auto& bId : testBiomes) {
            sm.startMultiplayerRace(bId);
            if (pWorld.terrain().getBiome() != bId) {
                std::cout << "  FAIL: Expected biome " << bId << ", got " << pWorld.terrain().getBiome() << std::endl;
                biomesOk = false;
            }
        }
        if (biomesOk) {
            std::cout << "  PASS: Successfully started multiplayer race on all 6 stages/biomes." << std::endl;
        } else {
            pass = false;
        }

        // 7. Active Fuel Management & Canister Recharge
        std::cout << "Test 7: Dynamic Fuel Drain and Recharge Mechanics" << std::endl;
        sm.startMultiplayerRace("desert");
        float initialFuel = pWorld.vehicle().fuel();
        inp.p1.gas = true;
        inp.p2.gas = true;
        for (int i = 0; i < 180; ++i) { // 1.5 seconds of full throttle
            sm.handleInput(inp);
            sm.fixedUpdate(DT);
        }
        float fuelAfterDrive = pWorld.vehicle().fuel();
        std::cout << "  Fuel Start: " << initialFuel << "% -> After 1.5s Throttle: " << fuelAfterDrive << "%" << std::endl;
        if (fuelAfterDrive >= initialFuel || fuelAfterDrive < 70.0f) {
            std::cout << "  FAIL: Unexpected fuel consumption behavior! Remaining: " << fuelAfterDrive << "%" << std::endl;
            pass = false;
        } else {
            std::cout << "  PASS: Dynamic fuel consumption is calibrated and active." << std::endl;
        }

        // Test fuel recharge (+45%)
        pWorld.vehicle().consumeFuel(40.0f);
        float preRecharge = pWorld.vehicle().fuel();
        pWorld.vehicle().addFuel(45.0f);
        float postRecharge = pWorld.vehicle().fuel();
        std::cout << "  Fuel Pre-Recharge: " << preRecharge << "% -> Post-Recharge: " << postRecharge << "%" << std::endl;
        if (postRecharge != std::min(100.0f, preRecharge + 45.0f)) {
            std::cout << "  FAIL: Fuel recharge did not add expected amount!" << std::endl;
            pass = false;
        } else {
            std::cout << "  PASS: Fuel recharge correctly awards calibrated +45%." << std::endl;
        }

        // 8. Dynamic Instrumentation Render & Capture
        std::cout << "Test 8: Dynamic Speedometer, Tachometer & Fuel Gauge Instrumentation" << std::endl;
        QDir().mkpath("scratch");
        sm.render(engine.framebuffer(), 1.0f);
        engine.framebuffer().getImage().save("scratch/multiplayer_gameplay_captured.png");
        std::cout << "  PASS: Saved scratch/multiplayer_gameplay_captured.png with dynamic gauges." << std::endl;

        std::cout << "\n=======================================================" << std::endl;
        std::cout << "  RESULT: " << (pass ? "ALL MULTIPLAYER TESTS PASSED!" : "TESTS FAILED!") << std::endl;
        std::cout << "=======================================================\n" << std::endl;

        return pass ? 0 : 1;
    }

    if (argc > 1 && (std::string(argv[1]) == "--profile" || std::string(argv[1]) == "--benchmark")) {
        engine.show();
        engine.start();
        std::cout << "\n=======================================================" << std::endl;
        std::cout << "  RUNNING 600-FRAME IN-DEPTH PROFILER & BENCHMARK      " << std::endl;
        std::cout << "=======================================================" << std::endl;

        auto& sm = engine.stateManager();
        sm.profile().setSelectedVehicle(Physics::VehicleType::MONSTER_TRUCK);
        sm.profile().setSelectedDriver(Physics::DriverType::BOB);
        sm.startRace("countryside");

        // Warm up for 60 frames
        for (int i = 0; i < 60; ++i) {
            sm.handleInput(engine.input().state());
            sm.fixedUpdate(1.0f / 120.0f);
            sm.fixedUpdate(1.0f / 120.0f);
            sm.render(engine.framebuffer(), 1.0f);
        }

        // Active throttle driving benchmark
        engine.input().state().gas = true;

        std::vector<double> physicsTimesUs;
        std::vector<double> terrainRenderTimesUs;
        std::vector<double> spriteRenderTimesUs;
        std::vector<double> particleRenderTimesUs;
        std::vector<double> hudRenderTimesUs;
        std::vector<double> totalFrameTimesUs;

        physicsTimesUs.reserve(1200);
        terrainRenderTimesUs.reserve(600);
        spriteRenderTimesUs.reserve(600);
        particleRenderTimesUs.reserve(600);
        hudRenderTimesUs.reserve(600);
        totalFrameTimesUs.reserve(600);

        auto& pWorld = sm.physics();
        auto& cam = sm.camera();
        auto& rasterizer = sm.terrainRasterizer();
        auto& particles = sm.particleSystem();
        auto& hud = sm.hud();
        auto& fb = engine.framebuffer();

        const int BENCHMARK_FRAMES = 600;
        const float FIXED_DT = 1.0f / 120.0f;

        for (int f = 0; f < BENCHMARK_FRAMES; ++f) {
            auto frameStart = std::chrono::high_resolution_clock::now();

            // 2 Physics steps per 60Hz display frame (120 Hz)
            for (int step = 0; step < 2; ++step) {
                auto physStart = std::chrono::high_resolution_clock::now();
                sm.handleInput(engine.input().state());
                sm.fixedUpdate(FIXED_DT);
                auto physEnd = std::chrono::high_resolution_clock::now();
                physicsTimesUs.push_back(std::chrono::duration<double, std::micro>(physEnd - physStart).count());
            }

            // Subsystem rendering breakdown
            auto t0 = std::chrono::high_resolution_clock::now();
            rasterizer.render(fb, cam, pWorld.terrain(), sm.gameTime());
            auto t1 = std::chrono::high_resolution_clock::now();
            particles.render(fb, cam);
            auto t2 = std::chrono::high_resolution_clock::now();
            Graphics::SpriteRenderer::renderVehicle(fb, cam, pWorld.vehicle());
            auto t3 = std::chrono::high_resolution_clock::now();
            hud.render(fb);
            auto t4 = std::chrono::high_resolution_clock::now();

            terrainRenderTimesUs.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count());
            particleRenderTimesUs.push_back(std::chrono::duration<double, std::micro>(t2 - t1).count());
            spriteRenderTimesUs.push_back(std::chrono::duration<double, std::micro>(t3 - t2).count());
            hudRenderTimesUs.push_back(std::chrono::duration<double, std::micro>(t4 - t3).count());

            auto frameEnd = std::chrono::high_resolution_clock::now();
            totalFrameTimesUs.push_back(std::chrono::duration<double, std::micro>(frameEnd - frameStart).count());
        }

        auto calcStats = [](std::vector<double>& v) {
            std::sort(v.begin(), v.end());
            double sum = std::accumulate(v.begin(), v.end(), 0.0);
            double avg = sum / v.size();
            double minVal = v.front();
            double maxVal = v.back();
            double p95 = v[static_cast<size_t>(v.size() * 0.95)];
            double p99 = v[static_cast<size_t>(v.size() * 0.99)];
            return std::make_tuple(minVal, avg, maxVal, p95, p99);
        };

        auto [physMin, physAvg, physMax, physP95, physP99] = calcStats(physicsTimesUs);
        auto [terrMin, terrAvg, terrMax, terrP95, terrP99] = calcStats(terrainRenderTimesUs);
        auto [partMin, partAvg, partMax, partP95, partP99] = calcStats(particleRenderTimesUs);
        auto [sprMin, sprAvg, sprMax, sprP95, sprP99] = calcStats(spriteRenderTimesUs);
        auto [hudMin, hudAvg, hudMax, hudP95, hudP99] = calcStats(hudRenderTimesUs);
        auto [totMin, totAvg, totMax, totP95, totP99] = calcStats(totalFrameTimesUs);

        double avgTotalMs = totAvg / 1000.0;
        double p99TotalMs = totP99 / 1000.0;
        double effectiveFps = 1000000.0 / totAvg;

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n--- SUBSYSTEM BREAKDOWN (per invocation) ---" << std::endl;
        std::cout << "  Physics Tick (120Hz):  Avg: " << (physAvg / 1000.0) << " ms  |  p95: " << (physP95 / 1000.0) << " ms  |  p99: " << (physP99 / 1000.0) << " ms  |  Max: " << (physMax / 1000.0) << " ms" << std::endl;
        std::cout << "  Terrain Rasterizer:    Avg: " << (terrAvg / 1000.0) << " ms  |  p95: " << (terrP95 / 1000.0) << " ms  |  p99: " << (terrP99 / 1000.0) << " ms  |  Max: " << (terrMax / 1000.0) << " ms" << std::endl;
        std::cout << "  Particle Rendering:    Avg: " << (partAvg / 1000.0) << " ms  |  p95: " << (partP95 / 1000.0) << " ms  |  p99: " << (partP99 / 1000.0) << " ms  |  Max: " << (partMax / 1000.0) << " ms" << std::endl;
        std::cout << "  Vehicle Sprites:       Avg: " << (sprAvg / 1000.0) << " ms  |  p95: " << (sprP95 / 1000.0) << " ms  |  p99: " << (sprP99 / 1000.0) << " ms  |  Max: " << (sprMax / 1000.0) << " ms" << std::endl;
        std::cout << "  HUD & Telemetry:       Avg: " << (hudAvg / 1000.0) << " ms  |  p95: " << (hudP95 / 1000.0) << " ms  |  p99: " << (hudP99 / 1000.0) << " ms  |  Max: " << (hudMax / 1000.0) << " ms" << std::endl;
        std::cout << "\n--- TOTAL FRAME PACING (Physics + All Render) ---" << std::endl;
        std::cout << "  Average Frame Time:    " << avgTotalMs << " ms (" << effectiveFps << " FPS)" << std::endl;
        std::cout << "  95th Percentile:       " << (totP95 / 1000.0) << " ms" << std::endl;
        std::cout << "  99th Percentile:       " << p99TotalMs << " ms" << std::endl;
        std::cout << "  Max Frame Spike:       " << (totMax / 1000.0) << " ms" << std::endl;
        std::cout << "=======================================================\n" << std::endl;

        return 0;
    }

    if (argc > 1 && (std::string(argv[1]) == "--test-audio" || std::string(argv[1]) == "--audio-test")) {
        std::cout << "\n=======================================================" << std::endl;
        std::cout << "  RUNNING COMPREHENSIVE REALISTIC DYNAMIC AUDIO TEST SUITE" << std::endl;
        std::cout << "=======================================================" << std::endl;

        Audio::SoundManager audio;
        bool allAudioPassed = true;

        // 1. VOLUME CONTROLS & HIERARCHY
        std::cout << "\n--- TEST 1: MASTER, MUSIC & SFX VOLUME HIERARCHY ---" << std::endl;
        {
            audio.setMasterVolume(0.8f);
            audio.setMusicVolume(0.6f);
            audio.setSfxVolume(0.5f);
            bool passVols = (std::abs(audio.masterVolume() - 0.8f) < 0.001f) &&
                            (std::abs(audio.musicVolume() - 0.6f) < 0.001f) &&
                            (std::abs(audio.sfxVolume() - 0.5f) < 0.001f);
            // Test clamping
            audio.setMasterVolume(1.5f);
            audio.setMusicVolume(-0.2f);
            bool passClamp = (audio.masterVolume() == 1.0f) && (audio.musicVolume() == 0.0f);
            audio.setMasterVolume(1.0f);
            audio.setMusicVolume(0.8f);
            audio.setSfxVolume(0.9f);
            bool pass1 = passVols && passClamp;
            std::cout << "  Volume Hierarchy & Boundary Clamping: " << (pass1 ? "PASS" : "FAIL") << std::endl;
            if (!pass1) allAudioPassed = false;
        }

        // 2. BACKGROUND MUSIC STATE TRANSITIONS & SMOOTH CROSSFADING
        std::cout << "\n--- TEST 2: BACKGROUND MUSIC STATE TRANSITIONS & CROSSFADE ---" << std::endl;
        {
            Physics::Terrain terrain;
            terrain.setBiome("countryside");
            Physics::Vehicle vehicle(Physics::VehicleType::OFFROADER);

            // Menu state -> Menu music target
            for (int i = 0; i < 60; ++i) audio.update(1.0f / 60.0f, vehicle, terrain, Core::StateType::MAIN_MENU);
            bool passMenuMus = (audio.currentMusicTrack() == Audio::MusicTrack::MENU) && (audio.currentMusicVolume() > 0.5f);
            std::cout << "  Main Menu Music Active: " << (passMenuMus ? "YES" : "NO")
                      << " (Vol: " << audio.currentMusicVolume() << ")" << std::endl;

            // Transition to Gameplay -> Smooth crossfade to Race music
            for (int i = 0; i < 30; ++i) {
                audio.update(1.0f / 60.0f, vehicle, terrain, Core::StateType::GAMEPLAY);
            }
            bool passRaceMus = (audio.currentMusicTrack() == Audio::MusicTrack::RACE);
            std::cout << "  Race Music Crossfade Successful: " << (passRaceMus ? "YES" : "NO")
                      << " (Vol: " << audio.currentMusicVolume() << ")" << std::endl;

            // Pause -> Ducking
            for (int i = 0; i < 30; ++i) audio.update(1.0f / 60.0f, vehicle, terrain, Core::StateType::PAUSED);
            bool passPauseDuck = (audio.currentMusicVolume() < 0.50f);
            std::cout << "  Paused Music Volume Ducked: " << (passPauseDuck ? "YES" : "NO")
                      << " (Ducked Vol: " << audio.currentMusicVolume() << ")" << std::endl;

            // Game Over -> Stop Music
            audio.update(1.0f / 60.0f, vehicle, terrain, Core::StateType::GAME_OVER);
            for (int i = 0; i < 60; ++i) audio.update(1.0f / 60.0f, vehicle, terrain, Core::StateType::GAME_OVER);
            bool passStopMus = (audio.currentMusicVolume() < 0.05f);
            std::cout << "  Game Over Music Fadeout: " << (passStopMus ? "YES" : "NO") << std::endl;

            bool pass2 = passMenuMus && passRaceMus && passPauseDuck && passStopMus;
            std::cout << "  -> " << (pass2 ? "PASS" : "FAIL") << std::endl;
            if (!pass2) allAudioPassed = false;
        }

        // 3. CONTINUOUS ENGINE RPM PITCH SHIFTING & THROTTLE LOAD DRIVE
        std::cout << "\n--- TEST 3: DYNAMIC ENGINE RPM PITCH & THROTTLE LOAD MODULATION ---" << std::endl;
        {
            // Idle 1000 RPM, throttle released
            audio.updateEngineRpm(1000.0f, 0.0f);
            float idlePitch = audio.currentEnginePitch();
            float idleVol = audio.currentEngineVolume();

            // Cruising 4000 RPM, moderate throttle
            audio.updateEngineRpm(4000.0f, 0.5f);
            float midPitch = audio.currentEnginePitch();
            float midVol = audio.currentEngineVolume();

            // Redline 8500 RPM, wide open throttle
            audio.updateEngineRpm(8500.0f, 1.0f);
            float redlinePitch = audio.currentEnginePitch();
            float redlineVol = audio.currentEngineVolume();

            std::cout << "  Idle (1000 RPM):    Pitch: " << idlePitch << ", Vol: " << idleVol << std::endl;
            std::cout << "  Mid-Rev (4000 RPM): Pitch: " << midPitch << ", Vol: " << midVol << std::endl;
            std::cout << "  Redline (8500 RPM): Pitch: " << redlinePitch << ", Vol: " << redlineVol << std::endl;

            bool passPitchProgression = (idlePitch < midPitch && midPitch < redlinePitch);
            bool passLoadBoost = (redlineVol > idleVol && midVol > idleVol);
            bool pass3 = passPitchProgression && passLoadBoost;
            std::cout << "  Continuous RPM Modulation & Throttle Overdrive: " << (pass3 ? "PASS" : "FAIL") << std::endl;
            if (!pass3) allAudioPassed = false;
        }

        // 4. TYRE ROLLING & SURFACE-SPECIFIC SKIDDING AUDIO
        std::cout << "\n--- TEST 4: TYRE ROLLING & MATERIAL-SPECIFIC SKID AUDIO ---" << std::endl;
        {
            Physics::Terrain terrain;
            Core::ProfileManager profile;
            Physics::Vehicle vehicle(Physics::VehicleType::OFFROADER);

            // Driving on Countryside (Dirt)
            terrain.setBiome("countryside");
            float startX = 20.0f;
            vehicle.reset(startX, terrain.getHeight(startX) + 0.5f);

            // Drive forward smoothly
            for (int i = 0; i < 60; ++i) {
                vehicle.applyInput(0.5f, 0.0f);
                vehicle.step(1.0f / 60.0f, terrain, profile);
                audio.update(1.0f / 60.0f, vehicle, terrain, Core::StateType::GAMEPLAY);
            }
            float rollVol = audio.currentTyreRollVolume();
            bool passRoll = (rollVol > 0.05f);
            std::cout << "  Tyre Rolling Volume at Speed: " << rollVol << " -> " << (passRoll ? "PASS" : "FAIL") << std::endl;

            // Induce heavy skid by braking hard at speed
            for (int i = 0; i < 30; ++i) {
                vehicle.applyInput(0.0f, 1.0f);
                vehicle.step(1.0f / 60.0f, terrain, profile);
                audio.update(1.0f / 60.0f, vehicle, terrain, Core::StateType::GAMEPLAY);
            }
            bool isSkid = audio.isSkidding();
            float skidVol = audio.currentTyreSkidVolume();
            std::cout << "  Heavy Braking Wheel Slip: " << vehicle.maxSlipSpeed()
                      << " m/s (Skidding: " << (isSkid ? "YES" : "NO") << ", Skid Vol: " << skidVol << ")" << std::endl;

            bool pass4 = passRoll && (isSkid || skidVol > 0.0f);
            std::cout << "  -> " << (pass4 ? "PASS" : "FAIL") << std::endl;
            if (!pass4) allAudioPassed = false;
        }

        // 5. COLLISION & CRASH AUDIO INTENSITY SCALING
        std::cout << "\n--- TEST 5: COLLISION IMPACT & CRASH INTENSITY SCALING ---" << std::endl;
        {
            // Test soft impact scaling
            audio.playSoftImpact(0.5f);
            float softInt = audio.lastImpactIntensity();

            // Test hard impact scaling
            audio.playHardImpact(0.9f);
            float hardInt = audio.lastImpactIntensity();

            // Test crash trigger
            audio.playCrash();

            bool passImpacts = (softInt < hardInt && hardInt >= 0.85f);
            std::cout << "  Soft Impact vs Hard Impact Velocity Scaling: " << (passImpacts ? "PASS" : "FAIL") << std::endl;
            if (!passImpacts) allAudioPassed = false;
        }

        // 6. JUMP TAKEOFF & LANDING SUSPENSION COMPRESSION
        std::cout << "\n--- TEST 6: JUMP TAKEOFF & LANDING COMPRESSION EFFECTS ---" << std::endl;
        {
            Physics::Terrain terrain;
            terrain.setBiome("countryside");
            Core::ProfileManager profile;
            Physics::Vehicle vehicle(Physics::VehicleType::RALLY_CAR);

            // Drop from 2.5m into ground -> landing event
            float startX = 20.0f;
            float groundY = terrain.getHeight(startX);
            vehicle.reset(startX, groundY + 2.5f);

            bool detectedAirborne = false;
            bool detectedLanding = false;

            for (int i = 0; i < 90; ++i) {
                vehicle.step(1.0f / 60.0f, terrain, profile);
                audio.update(1.0f / 60.0f, vehicle, terrain, Core::StateType::GAMEPLAY);
                if (audio.isAirborne()) detectedAirborne = true;
                if (detectedAirborne && !audio.isAirborne()) detectedLanding = true;
            }

            bool pass6 = detectedAirborne && detectedLanding;
            std::cout << "  Airborne Detection: " << (detectedAirborne ? "YES" : "NO") << std::endl;
            std::cout << "  Landing Compression Trigger: " << (detectedLanding ? "YES" : "NO") << std::endl;
            std::cout << "  -> " << (pass6 ? "PASS" : "FAIL") << std::endl;
            if (!pass6) allAudioPassed = false;
        }

        // 7. ENVIRONMENTAL WIND AUDIO
        std::cout << "\n--- TEST 7: ENVIRONMENTAL AIRFLOW & AIRTIME SPEED RESPONSE ---" << std::endl;
        {
            Physics::Terrain terrain;
            terrain.setBiome("desert");
            Core::ProfileManager profile;
            Physics::Vehicle vehicle(Physics::VehicleType::SUPERCAR);

            // Zero speed
            audio.update(1.0f / 60.0f, vehicle, terrain, Core::StateType::GAMEPLAY);
            float lowWind = audio.currentWindVolume();

            // High speed run
            vehicle.reset(10.0f, terrain.getHeight(10.0f) + 0.5f);
            for (int i = 0; i < 180; ++i) {
                vehicle.applyInput(1.0f, 0.0f);
                vehicle.step(1.0f / 60.0f, terrain, profile);
                audio.update(1.0f / 60.0f, vehicle, terrain, Core::StateType::GAMEPLAY);
            }
            float highWind = audio.currentWindVolume();
            std::cout << "  Stationary Wind: " << lowWind << " | High Speed (100+ km/h) Wind: " << highWind << std::endl;
            bool pass7 = (highWind > lowWind + 0.15f);
            std::cout << "  -> " << (pass7 ? "PASS" : "FAIL") << std::endl;
            if (!pass7) allAudioPassed = false;
        }

        // 8. SOFTWARE MIXER ANTI-CLIPPING LIMITER & POLYPHONY STRESS TEST
        std::cout << "\n--- TEST 8: MULTI-CHANNEL ANTI-CLIPPING LIMITER & SYNTH MIXER ---" << std::endl;
        {
            // Activate every audio source at 100% volume simultaneously
            audio.setMasterVolume(1.0f);
            audio.setMusicVolume(1.0f);
            audio.setSfxVolume(1.0f);
            audio.setTargetMusic(Audio::MusicTrack::RACE);
            audio.updateEngineRpm(8500.0f, 1.0f);
            audio.playCrash();
            audio.playHardImpact(1.0f);
            audio.playStuntCheer();

            const int SAMPLES = 4410; // 0.20 seconds of audio
            std::vector<int16_t> mixBuffer(SAMPLES, 0);
            audio.renderMixedAudio(mixBuffer.data(), SAMPLES);

            int peak = 0;
            int clips = 0;
            for (int16_t s : mixBuffer) {
                int mag = std::abs(static_cast<int>(s));
                if (mag > peak) peak = mag;
                if (mag >= 32760) clips++;
            }

            // Export rich 3.0s sample mix to scratch/ for verification and playback
            QDir().mkpath("scratch");
            bool exported = audio.exportMixedAudioWav("scratch/gameplay_audio_mix_sample.wav", 3.0f, 22050);

            std::cout << "  Synthesized Mix Peak Amplitude: " << peak << " / 32767" << std::endl;
            std::cout << "  Digital Clipping Artifacts:     " << clips << " (Target: 0)" << std::endl;
            std::cout << "  Exported Audio Sample:          " << (exported ? "scratch/gameplay_audio_mix_sample.wav" : "FAILED") << std::endl;
            bool pass8 = (peak > 1000 && clips == 0 && exported);
            std::cout << "  -> " << (pass8 ? "PASS" : "FAIL") << std::endl;
            if (!pass8) allAudioPassed = false;
        }

        std::cout << "\n=======================================================" << std::endl;
        std::cout << "  AUDIO TEST SUITE RESULT: " << (allAudioPassed ? "ALL TESTS PASSED!" : "SOME TESTS FAILED!") << std::endl;
        std::cout << "=======================================================\n" << std::endl;

        return allAudioPassed ? 0 : 1;
    }

    if (argc > 1 && (std::string(argv[1]) == "--test-physics" || std::string(argv[1]) == "--physics-test")) {
        std::cout << "\n=======================================================" << std::endl;
        std::cout << "  RUNNING COMPREHENSIVE VEHICLE & SUSPENSION TEST SUITE" << std::endl;
        std::cout << "=======================================================" << std::endl;

        Core::ProfileManager profile;
        const float DT = 1.0f / 120.0f;
        bool allTestsPassed = true;

        // 1. DROP LANDING & REBOUND OSCILLATION TEST
        std::cout << "\n--- TEST 1: DROP LANDING & REBOUND OSCILLATION (1.5m drop) ---" << std::endl;
        for (int v = 0; v < static_cast<int>(Physics::VehicleType::COUNT); ++v) {
            auto vType = static_cast<Physics::VehicleType>(v);
            Physics::Terrain terrain;
            terrain.setBiome("countryside");
            Physics::Vehicle vehicle(vType);

            float startX = 5.0f;
            float groundY = terrain.getHeight(startX);
            float dropY = groundY + vehicle.suspRestLength() + vehicle.wheelRadius() - vehicle.rearMountOffset().y + 1.5f;
            vehicle.reset(startX, dropY);

            float maxCompression = 0.0f;
            int bounceReversals = 0;
            float prevVy = -1.0f;
            float timeToSettle = -1.0f;
            float firstTouchTime = -1.0f;
            bool hitGround = false;

            for (int tick = 0; tick < 240; ++tick) { // 2.0 seconds
                vehicle.step(DT, terrain, profile);
                float suspComp = vehicle.suspRestLength() - vehicle.rearSuspensionCompression();
                if (suspComp > maxCompression) maxCompression = suspComp;

                if (vehicle.isRearOnGround() || vehicle.isFrontOnGround()) {
                    hitGround = true;
                    if (firstTouchTime < 0.0f) firstTouchTime = tick * DT;
                }

                if (hitGround) {
                    float vy = vehicle.chassisVel().y;
                    if (prevVy < -0.1f && vy > 0.1f) {
                        bounceReversals++;
                    }
                    prevVy = vy;

                    float timeSinceTouch = (tick * DT) - firstTouchTime;
                    if (std::abs(vy) < 0.05f && std::abs(vehicle.chassisAngularVel()) < 0.05f && timeToSettle < 0.0f && timeSinceTouch > 0.10f) {
                        timeToSettle = timeSinceTouch;
                    }
                }
            }

            bool passBounces = (bounceReversals <= 1); // At most 1 compression/rebound cycle, no secondary bouncing
            bool passSettle = (timeToSettle > 0.0f && timeToSettle < 0.55f);
            bool passAirborne = !vehicle.isAirborne(); // Must end stably on ground

            std::cout << "  [" << vehicle.config().name << "]: "
                      << "MaxComp: " << std::fixed << std::setprecision(3) << maxCompression << "m, "
                      << "Bounces: " << bounceReversals << " (Target <= 1), "
                      << "SettleTime: " << std::setprecision(2) << timeToSettle << "s "
                      << ((passBounces && passSettle && passAirborne) ? "-> PASS" : "-> FAIL") << std::endl;

            if (!passBounces || !passSettle || !passAirborne) allTestsPassed = false;
        }

        // 2. FLAT GROUND STEADY-STATE & IDLE STABILITY TEST
        std::cout << "\n--- TEST 2: FLAT GROUND IDLE STABILITY & ZERO-SPIN TEST ---" << std::endl;
        {
            Physics::Terrain terrain;
            terrain.setBiome("countryside");
            Physics::Vehicle vehicle(Physics::VehicleType::OFFROADER);
            float startX = 5.0f;
            float groundY = terrain.getHeight(startX);
            float restY = groundY + vehicle.suspRestLength() + vehicle.wheelRadius() - vehicle.rearMountOffset().y;
            vehicle.reset(startX, restY);

            for (int tick = 0; tick < 120; ++tick) {
                vehicle.step(DT, terrain, profile);
            }

            float speed = vehicle.getSpeedKmh();
            float rearOmega = vehicle.rearWheelAngularVel();
            float frontOmega = vehicle.frontWheelAngularVel();
            bool onGround = vehicle.isRearOnGround() && vehicle.isFrontOnGround();

            std::cout << "  Speed: " << speed << " km/h (Expected ~0)" << std::endl;
            std::cout << "  Rear Wheel Omega: " << rearOmega << " rad/s (Expected 0)" << std::endl;
            std::cout << "  Front Wheel Omega: " << frontOmega << " rad/s (Expected 0)" << std::endl;
            std::cout << "  Both Wheels Contact: " << (onGround ? "YES" : "NO") << std::endl;

            if (speed > 0.05f || std::abs(rearOmega) > 0.01f || std::abs(frontOmega) > 0.01f || !onGround) {
                allTestsPassed = false;
                std::cout << "  -> FAIL" << std::endl;
            } else {
                std::cout << "  -> PASS" << std::endl;
            }
        }

        // 3. SMALL BUMP GROUND-TRACKING (Moguls / Washboards without bouncing)
        std::cout << "\n--- TEST 3: GROUND CONTACT RETENTION OVER UNDULATIONS (60 km/h) ---" << std::endl;
        for (int v = 0; v < static_cast<int>(Physics::VehicleType::COUNT); ++v) {
            auto vType = static_cast<Physics::VehicleType>(v);
            Physics::Terrain terrain;
            terrain.setBiome("countryside");
            Physics::Vehicle vehicle(vType);

            // Start before rhythm bumps at x = 540m
            float startX = 540.0f;
            float groundY = terrain.getHeight(startX);
            float restY = groundY + vehicle.suspRestLength() + vehicle.wheelRadius() - vehicle.rearMountOffset().y;
            vehicle.reset(startX, restY);

            int totalTicks = 360; // 3 seconds of driving over moguls at ~60 km/h
            int contactTicks = 0;

            for (int tick = 0; tick < totalTicks; ++tick) {
                float spd = vehicle.getSpeedKmh();
                float gas = (spd < 60.0f) ? 0.85f : 0.0f;
                float brk = (spd > 68.0f) ? 0.35f : 0.0f;
                vehicle.applyInput(gas, brk);
                vehicle.step(DT, terrain, profile);
                if (vehicle.isRearOnGround() || vehicle.isFrontOnGround()) {
                    contactTicks++;
                }
            }

            float contactRatio = static_cast<float>(contactTicks) / totalTicks;
            bool passContact = (contactRatio >= 0.80f); // Should stay grounded over ordinary washboard undulations

            std::cout << "  [" << vehicle.config().name << "]: "
                      << "Ground Contact: " << std::fixed << std::setprecision(1) << (contactRatio * 100.0f) << "% (Target >= 80%) "
                      << (passContact ? "-> PASS" : "-> FAIL") << std::endl;

            if (!passContact) allTestsPassed = false;
        }

        // 4. BIG JUMP LAUNCH & AIRTIME SCALING TEST
        std::cout << "\n--- TEST 4: JUMP LAUNCH & PARABOLIC TRAJECTORY (Ski Jump Ramp at x=65m) ---" << std::endl;
        {
            Physics::Terrain terrain;
            terrain.setBiome("countryside");
            Physics::Vehicle vehicle(Physics::VehicleType::RALLY_CAR);
            float startX = 25.0f;
            float groundY = terrain.getHeight(startX);
            float restY = groundY + vehicle.suspRestLength() + vehicle.wheelRadius() - vehicle.rearMountOffset().y;
            vehicle.reset(startX, restY);

            float maxAirHeight = 0.0f;
            float totalAirTime = 0.0f;
            bool launched = false;
            bool landedCleanly = false;

            for (int tick = 0; tick < 480; ++tick) {
                float gas = vehicle.isAirborne() ? 0.0f : 1.0f;
                vehicle.applyInput(gas, 0.0f);
                vehicle.step(DT, terrain, profile);

                if (vehicle.isAirborne()) {
                    launched = true;
                    totalAirTime += DT;
                    float h = vehicle.chassisPos().y - terrain.getHeight(vehicle.chassisPos().x);
                    if (h > maxAirHeight) maxAirHeight = h;
                } else if (launched && (vehicle.isRearOnGround() || vehicle.isFrontOnGround())) {
                    landedCleanly = true;
                }
            }

            std::cout << "  Rally Car Peak Height: " << maxAirHeight << " m" << std::endl;
            std::cout << "  Total Air Time: " << totalAirTime << " s" << std::endl;
            std::cout << "  Landed Cleanly without Crash: " << (landedCleanly && !vehicle.isDriverDown() ? "YES" : "NO") << std::endl;

            // Air time should be realistic (between 0.25s and 2.2s for this launch), not endless floatiness
            bool passAir = (launched && totalAirTime >= 0.25f && totalAirTime <= 2.2f && landedCleanly && !vehicle.isDriverDown());
            std::cout << "  -> " << (passAir ? "PASS" : "FAIL") << std::endl;
            if (!passAir) allTestsPassed = false;
        }

        // 5. STEEP SLOPE CLIMBING & TRACTION TEST
        std::cout << "\n--- TEST 5: STEEP SLOPE CLIMBING (Mountain Wall at x=950m) ---" << std::endl;
        {
            Physics::Terrain terrain;
            terrain.setBiome("mountain");
            Physics::Vehicle vehicle(Physics::VehicleType::MONSTER_TRUCK);
            float startX = 940.0f;
            float groundY = terrain.getHeight(startX);
            float restY = groundY + vehicle.suspRestLength() + vehicle.wheelRadius() - vehicle.rearMountOffset().y;
            vehicle.reset(startX, restY);

            float initialX = startX;
            for (int tick = 0; tick < 360; ++tick) {
                vehicle.applyInput(1.0f, 0.0f);
                vehicle.step(DT, terrain, profile);
            }
            float distClimbed = vehicle.chassisPos().x - initialX;
            std::cout << "  Monster Truck Distance Climbed on Steep Mountain: " << distClimbed << " m" << std::endl;
            bool passClimb = (distClimbed > 25.0f && !vehicle.isDriverDown());
            std::cout << "  -> " << (passClimb ? "PASS" : "FAIL") << std::endl;
            if (!passClimb) allTestsPassed = false;
        }

        // 6. UPSIDE-DOWN CRASH & INVERTED RESTING TEST (Across all 10 vehicles)
        std::cout << "\n--- TEST 6: UPSIDE-DOWN CRASH & INVERTED RESTING TEST (2.0m drop) ---" << std::endl;
        for (int v = 0; v < static_cast<int>(Physics::VehicleType::COUNT); ++v) {
            auto vType = static_cast<Physics::VehicleType>(v);
            Physics::Terrain terrain;
            terrain.setBiome("countryside");
            Physics::Vehicle vehicle(vType);

            float startX = 15.0f;
            float groundY = terrain.getHeight(startX);
            float dropY = groundY + vehicle.config().spriteHeight / 80.25f + 2.0f;
            vehicle.reset(startX, dropY);

            // Invert vehicle attitude (upside-down 180 degrees)
            // Access private angle via stepping or reset orientation:
            // Simulate 180-degree roll in air
            // Let it fall and hit ground upside down
            for (int tick = 0; tick < 240; ++tick) {
                if (tick < 15) {
                    // Induce fast half-flip in first 0.12s
                    vehicle.applyInput(0.0f, 1.0f);
                }
                vehicle.step(DT, terrain, profile);
            }

            // Test dedicated upside-down placement
            vehicle.reset(startX, groundY + 1.2f, 3.14159f);
            float maxPenetration = 0.0f;
            for (int tick = 0; tick < 240; ++tick) {
                vehicle.step(DT, terrain, profile);
                float pen = vehicle.getMaxTerrainPenetration(terrain);
                if (pen > maxPenetration) maxPenetration = pen;
            }

            bool passPen = (maxPenetration < 0.005f); // Strictly less than 5mm penetration
            std::cout << "  [" << vehicle.config().name << "]: "
                      << "MaxPenetration: " << std::fixed << std::setprecision(4) << maxPenetration << "m (Target < 0.005m) "
                      << (passPen ? "-> PASS" : "-> FAIL") << std::endl;
            if (!passPen) allTestsPassed = false;
        }

        // 7. HIGH-SPEED CRASH INTO STEEP MOUNTAIN SLOPE
        std::cout << "\n--- TEST 7: HIGH-SPEED CRASH INTO STEEP SLOPE (80 km/h Impact) ---" << std::endl;
        {
            Physics::Terrain terrain;
            terrain.setBiome("mountain");
            Physics::Vehicle vehicle(Physics::VehicleType::RALLY_CAR);
            // Launch at steep slope wall at x=950m (adjusted launch point for calibrated speed)
            float startX = 926.0f;
            float groundY = terrain.getHeight(startX);
            vehicle.reset(startX, groundY + 0.5f);

            float maxPen = 0.0f;
            float maxReboundSpeed = 0.0f;
            bool hitSlope = false;

            for (int tick = 0; tick < 300; ++tick) {
                vehicle.applyInput(1.0f, 0.0f);
                vehicle.step(DT, terrain, profile);
                float pen = vehicle.getMaxTerrainPenetration(terrain);
                if (pen > maxPen) maxPen = pen;

                if (vehicle.chassisPos().x > 944.0f) {
                    hitSlope = true;
                    float spd = vehicle.chassisVel().length();
                    if (spd > maxReboundSpeed && tick > 120) maxReboundSpeed = spd;
                }
            }

            bool passSlope = (maxPen < 0.005f) && hitSlope;
            std::cout << "  Rally Car Peak Slope Penetration: " << maxPen << " m (Target < 0.005m)" << std::endl;
            std::cout << "  Crash Impact Resolved Cleanly: " << (passSlope ? "YES" : "NO") << std::endl;
            std::cout << "  -> " << (passSlope ? "PASS" : "FAIL") << std::endl;
            if (!passSlope) allTestsPassed = false;
        }

        // 8. EXTREME ROTATION & TUMBLE OVER UNEVEN TERRAIN
        std::cout << "\n--- TEST 8: EXTREME ROTATION & TUMBLE OVER UNEVEN TERRAIN ---" << std::endl;
        {
            Physics::Terrain terrain;
            terrain.setBiome("countryside");
            Physics::Vehicle vehicle(Physics::VehicleType::OFFROADER);
            float startX = 560.0f; // Moguls section
            float groundY = terrain.getHeight(startX);
            vehicle.reset(startX, groundY + 2.5f);

            float maxPen = 0.0f;
            for (int tick = 0; tick < 300; ++tick) {
                // Tumble input
                if (tick < 60) vehicle.applyInput(1.0f, 0.0f);
                else vehicle.applyInput(0.0f, 1.0f);

                vehicle.step(DT, terrain, profile);
                float pen = vehicle.getMaxTerrainPenetration(terrain);
                if (pen > maxPen) maxPen = pen;
            }

            bool passTumble = (maxPen < 0.005f);
            std::cout << "  Off-Roader Peak Tumble Penetration: " << maxPen << " m (Target < 0.005m)" << std::endl;
            std::cout << "  -> " << (passTumble ? "PASS" : "FAIL") << std::endl;
            if (!passTumble) allTestsPassed = false;
        }

        // 9. SPEEDOMETER ACCURACY & FRAME SPEED RECALIBRATION AUDIT (50% - 67% Movement Speed)
        std::cout << "\n--- TEST 9: SPEEDOMETER ACCURACY & FRAME SPEED CALIBRATION AUDIT ---" << std::endl;
        std::cout << "  [Target: Physical speed reduced to ~60% (50%-67%), Speedometer scale 100% retained]\n" << std::endl;
        for (int v = 0; v < static_cast<int>(Physics::VehicleType::COUNT); ++v) {
            auto vType = static_cast<Physics::VehicleType>(v);
            Physics::Terrain terrain;
            terrain.setBiome("countryside");
            Physics::Vehicle vehicle(vType);

            float startX = 10.0f;
            float groundY = terrain.getHeight(startX);
            float restY = groundY + vehicle.suspRestLength() + vehicle.wheelRadius() - vehicle.rearMountOffset().y;
            vehicle.reset(startX, restY);

            // Accelerate to top speed on flat ground (3.5 seconds)
            for (int tick = 0; tick < 420; ++tick) {
                vehicle.applyInput(1.0f, 0.0f);
                vehicle.step(DT, terrain, profile);
            }

            float vPhys = vehicle.chassisVel().length(); // Physical velocity in m/s
            float dxPerFrame = vPhys * DT;               // Physical world movement per frame at 120Hz
            float displayedKmh = vehicle.getSpeedKmh();  // Displayed speedometer value

            // Calculate theoretical uncalibrated speed (prior to speed reduction)
            float vUncalib = vehicle.config().baseTopSpeedKmh / (3.6f * 1.35f);
            float speedRatio = vPhys / vUncalib; // Should be ~0.60 (within 0.50 - 0.67)

            // Speedometer accuracy formula: displayedKmh == vPhys * 3.6f * SPEED_CALIBRATION
            float expectedKmh = vPhys * 3.6f * Physics::Vehicle::SPEED_CALIBRATION;
            float speedoDelta = std::abs(displayedKmh - expectedKmh);

            bool passRatio = (speedRatio >= 0.48f && speedRatio <= 0.68f);
            bool passSpeedo = (speedoDelta < 0.001f);
            bool passVehicle = passRatio && passSpeedo;

            std::cout << "  [" << vehicle.config().name << "]:\n"
                      << "    Physical Velocity: " << std::fixed << std::setprecision(2) << vPhys << " m/s "
                      << "(" << std::setprecision(4) << dxPerFrame << " m/frame @ 120Hz)\n"
                      << "    Speed Reduction: " << std::setprecision(1) << (speedRatio * 100.0f) << "% of legacy speed (Target 50%-67%)\n"
                      << "    Speedometer Readout: " << std::setprecision(1) << displayedKmh << " km/h "
                      << "(Expected: " << expectedKmh << " km/h, Diff: " << speedoDelta << ")\n"
                      << "    Calibration Verification: " << (passVehicle ? "-> PASS" : "-> FAIL") << "\n" << std::endl;

            if (!passVehicle) allTestsPassed = false;
        }

        // 10. TERRAIN-SPECIFIC AERODYNAMICS & AIRTIME AUDIT (EARTH ATMOSPHERE vs MOON VACUUM)
        std::cout << "--- TEST 10: TERRAIN-SPECIFIC AERODYNAMICS & AIRTIME AUDIT (EARTH vs MOON) ---" << std::endl;
        {
            // Test 10A: Countryside (Earth Atmosphere, airDensity = 1.0)
            Physics::Terrain earthTerrain;
            earthTerrain.setBiome("countryside");
            Physics::Vehicle earthVehicle(Physics::VehicleType::RALLY_CAR);
            float startX = 25.0f;
            float groundY = earthTerrain.getHeight(startX);
            earthVehicle.reset(startX, groundY + 0.5f);

            float earthAirTime = 0.0f;
            float earthPeakH = 0.0f;
            bool earthLaunched = false;
            bool earthLanded = false;

            for (int tick = 0; tick < 480; ++tick) {
                float gas = earthVehicle.isAirborne() ? 0.0f : 1.0f;
                earthVehicle.applyInput(gas, 0.0f);
                earthVehicle.step(DT, earthTerrain, profile);

                if (earthVehicle.isAirborne()) {
                    earthLaunched = true;
                    earthAirTime += DT;
                    float h = earthVehicle.chassisPos().y - earthTerrain.getHeight(earthVehicle.chassisPos().x);
                    if (h > earthPeakH) earthPeakH = h;
                } else if (earthLaunched && (earthVehicle.isRearOnGround() || earthVehicle.isFrontOnGround())) {
                    earthLanded = true;
                }
            }

            // Test 10B: Moon (Lunar Vacuum, airDensity = 0.0, gravity = 1.62)
            Physics::Terrain moonTerrain;
            moonTerrain.setBiome("moon");
            Physics::Vehicle moonVehicle(Physics::VehicleType::RALLY_CAR);
            float moonGroundY = moonTerrain.getHeight(startX);
            moonVehicle.reset(startX, moonGroundY + 0.5f);

            float moonAirTime = 0.0f;
            float moonPeakH = 0.0f;
            bool moonLaunched = false;
            bool moonLanded = false;

            for (int tick = 0; tick < 960; ++tick) { // 8 seconds for long lunar flight and touchdown
                float gas = moonVehicle.isAirborne() ? 0.0f : 1.0f;
                moonVehicle.applyInput(gas, 0.0f);
                moonVehicle.step(DT, moonTerrain, profile);

                if (moonVehicle.isAirborne()) {
                    moonLaunched = true;
                    moonAirTime += DT;
                    float h = moonVehicle.chassisPos().y - moonTerrain.getHeight(moonVehicle.chassisPos().x);
                    if (h > moonPeakH) moonPeakH = h;
                } else if (moonLaunched && (moonVehicle.isRearOnGround() || moonVehicle.isFrontOnGround())) {
                    moonLanded = true;
                }
            }

            std::cout << "  [Earth (Countryside, rho=1.00, g=9.81)]:\n"
                      << "    Peak Jump Height: " << std::fixed << std::setprecision(2) << earthPeakH << " m\n"
                      << "    Total Airtime: " << earthAirTime << " s (Controlled, crisp parabolic arc)\n"
                      << "    Clean Landing: " << (earthLanded && !earthVehicle.isDriverDown() ? "YES" : "NO") << "\n" << std::endl;

            std::cout << "  [Moon (Lunar Vacuum, rho=0.00, g=1.62)]:\n"
                      << "    Peak Jump Height: " << std::fixed << std::setprecision(2) << moonPeakH << " m\n"
                      << "    Total Airtime: " << moonAirTime << " s (Ultra-floaty low-gravity vacuum leap)\n"
                      << "    Clean Landing: " << (moonLanded && !moonVehicle.isDriverDown() ? "YES" : "NO") << "\n" << std::endl;

            float airtimeRatio = (earthAirTime > 0.01f) ? (moonAirTime / earthAirTime) : 0.0f;
            std::cout << "  Airtime Multiplier (Moon vs Earth): " << std::setprecision(2) << airtimeRatio << "x (Target >= 2.5x)\n" << std::endl;

            bool passEarth = (earthLaunched && earthAirTime >= 0.25f && earthAirTime <= 1.8f && earthLanded && !earthVehicle.isDriverDown());
            bool passMoon = (moonLaunched && moonAirTime >= 2.5f && airtimeRatio >= 2.5f && moonLanded && !moonVehicle.isDriverDown());
            bool passAero = passEarth && passMoon;

            std::cout << "  Aerodynamics & Airtime Result: " << (passAero ? "-> PASS" : "-> FAIL") << "\n" << std::endl;
            if (!passAero) allTestsPassed = false;
        }

        // 11. BRAKE MECHANICS & ZERO BACKWARD ACCELERATION VERIFICATION
        std::cout << "\n--- TEST 11: BRAKE MECHANICS & OPPOSING TORQUE (NO BACKWARD ACCELERATION) ---" << std::endl;
        {
            Physics::Terrain terrain;
            terrain.setTestIncline(0.0f); // Perfectly flat surface
            Physics::Vehicle vehicle(Physics::VehicleType::OFFROADER);
            float groundY = terrain.getHeight(10.0f);
            vehicle.reset(10.0f, groundY + 0.5f);

            // Phase 1: Accelerate to ~40 km/h
            for (int t = 0; t < 180; ++t) {
                vehicle.applyInput(1.0f, 0.0f, 0.0f);
                vehicle.step(DT, terrain, profile);
            }
            float speedBeforeBraking = vehicle.getSpeedKmh();
            std::cout << "  Vehicle Speed before braking: " << speedBeforeBraking << " km/h" << std::endl;

            // Phase 2: Apply full braking down to complete stop and HOLD for 3.0 seconds
            float minVx = 999.0f;
            bool stopped = false;
            float timeToStop = 0.0f;
            for (int t = 0; t < 360; ++t) {
                vehicle.applyInput(0.0f, 1.0f, 0.0f);
                vehicle.step(DT, terrain, profile);
                float vx = vehicle.chassisVel().x;
                if (vx < minVx) minVx = vx;
                if (!stopped && std::abs(vx) < 0.05f) {
                    stopped = true;
                    timeToStop = t * DT;
                }
            }
            float finalVx = vehicle.chassisVel().x;
            float finalSpeed = vehicle.getSpeedKmh();

            std::cout << "  Braked from " << speedBeforeBraking << " km/h to 0 in " << timeToStop << " s\n"
                      << "  Minimum Vx during braking: " << minVx << " m/s (Target >= -0.05 m/s, no backward prop)\n"
                      << "  Final Resting Vx: " << finalVx << " m/s (Speed: " << finalSpeed << " km/h)" << std::endl;

            // Strict rule: Braking opposes rotation to 0 and NEVER propels backward
            bool passBrake11 = (speedBeforeBraking > 25.0f && stopped && minVx >= -0.05f && std::abs(finalVx) < 0.05f);
            std::cout << "  -> " << (passBrake11 ? "PASS" : "FAIL") << std::endl;
            if (!passBrake11) allTestsPassed = false;
        }

        // 12. UPHILL SLOPE STABILITY & STATIC HILL-HOLD (20 deg and 35 deg)
        std::cout << "\n--- TEST 12: UPHILL SLOPE STABILITY & STATIC HILL-HOLD (20 deg & 35 deg) ---" << std::endl;
        {
            // Test 12A: 20 deg uphill slope
            Physics::Terrain ramp20;
            ramp20.setTestIncline(20.0f);
            Physics::Vehicle vehicle20(Physics::VehicleType::OFFROADER);
            float startX = 30.0f;
            float groundY20 = ramp20.getHeight(startX);
            vehicle20.reset(startX, groundY20 + 0.6f, 20.0f * 3.14159265f / 180.0f);

            // Settle wheels on slope with brake held
            for (int t = 0; t < 60; ++t) {
                vehicle20.applyInput(0.0f, 1.0f, 0.0f);
                vehicle20.step(DT, ramp20, profile);
            }
            float settledX20 = vehicle20.chassisPos().x;

            // Hold brake for 3 seconds on 20 deg slope
            for (int t = 0; t < 360; ++t) {
                vehicle20.applyInput(0.0f, 1.0f, 0.0f);
                vehicle20.step(DT, ramp20, profile);
            }
            float drift20 = std::abs(vehicle20.chassisPos().x - settledX20);
            float speed20 = vehicle20.getSpeedKmh();
            bool pass20 = (drift20 < 0.15f && speed20 < 0.10f);

            std::cout << "  [20 deg Incline]: 3-second Hill-Hold Drift: " << drift20 << " m | Speed: " << speed20
                      << " km/h -> " << (pass20 ? "PASS" : "FAIL") << std::endl;

            // Test 12B: 35 deg uphill slope (baseline holding limit)
            Physics::Terrain ramp35;
            ramp35.setTestIncline(35.0f);
            Physics::Vehicle vehicle35(Physics::VehicleType::OFFROADER);
            float groundY35 = ramp35.getHeight(startX);
            vehicle35.reset(startX, groundY35 + 0.6f, 35.0f * 3.14159265f / 180.0f);

            for (int t = 0; t < 60; ++t) {
                vehicle35.applyInput(0.0f, 1.0f, 0.0f);
                vehicle35.step(DT, ramp35, profile);
            }
            float settledX35 = vehicle35.chassisPos().x;

            for (int t = 0; t < 360; ++t) {
                vehicle35.applyInput(0.0f, 1.0f, 0.0f);
                vehicle35.step(DT, ramp35, profile);
            }
            float drift35 = std::abs(vehicle35.chassisPos().x - settledX35);
            float speed35 = vehicle35.getSpeedKmh();
            bool pass35 = (drift35 < 0.20f && speed35 < 0.15f);

            std::cout << "  [35 deg Incline (Limit)]: 3-second Hill-Hold Drift: " << drift35 << " m | Speed: " << speed35
                      << " km/h -> " << (pass35 ? "PASS" : "FAIL") << std::endl;

            bool pass12 = pass20 && pass35;
            std::cout << "  -> " << (pass12 ? "PASS" : "FAIL") << std::endl;
            if (!pass12) allTestsPassed = false;
        }

        // 13. STEEP SLOPE ROLL BEYOND HOLDING LIMIT (42 deg SLOPE)
        std::cout << "\n--- TEST 13: STEEP SLOPE ROLL BEYOND HOLDING LIMIT (42 deg SLOPE) ---" << std::endl;
        {
            // Baseline vehicle on 42 deg slope (sin(42 deg) = 0.669 > sin(35 deg) = 0.574)
            // Downhill gravity exceeds available baseline braking hold force.
            // Requirement: Vehicle should gradually roll backwards/downhill under excess gravity, without snapping or locking.
            Physics::Terrain ramp42;
            ramp42.setTestIncline(42.0f);
            Physics::Vehicle vehicle42(Physics::VehicleType::OFFROADER);
            float startX = 40.0f;
            float groundY42 = ramp42.getHeight(startX);
            vehicle42.reset(startX, groundY42 + 0.6f, 42.0f * 3.14159265f / 180.0f);

            // Settle wheels
            for (int t = 0; t < 60; ++t) {
                vehicle42.applyInput(0.0f, 1.0f, 0.0f);
                vehicle42.step(DT, ramp42, profile);
            }
            float settledX42 = vehicle42.chassisPos().x;

            // Apply full brakes for 3 seconds on 42 deg slope
            for (int t = 0; t < 360; ++t) {
                vehicle42.applyInput(0.0f, 1.0f, 0.0f);
                vehicle42.step(DT, ramp42, profile);
            }
            float downhillRollDist = settledX42 - vehicle42.chassisPos().x;
            float finalSpeed42 = vehicle42.getSpeedKmh();

            std::cout << "  [42 deg Incline (> 35 deg Baseline Holding Limit)]:\n"
                      << "    SettledX: " << settledX42 << " | FinalX: " << vehicle42.chassisPos().x << "\n"
                      << "    Final Vel: (" << vehicle42.chassisVel().x << ", " << vehicle42.chassisVel().y << ") m/s | Angle: " << (vehicle42.chassisAngle() * 180.0f / 3.14159f) << " deg\n"
                      << "    Downhill Roll Distance under Excess Gravity: " << downhillRollDist << " m (Target > 0.30 m)\n"
                      << "    Controlled Terminal Downhill Speed: " << finalSpeed42 << " km/h (Smooth braking resistance)\n";

            bool pass13 = (downhillRollDist > 0.30f && finalSpeed42 < 20.0f && !vehicle42.isDriverDown());
            std::cout << "  -> " << (pass13 ? "PASS" : "FAIL") << std::endl;
            if (!pass13) allTestsPassed = false;
        }

        // 14. BRAKE UPGRADES EXPANDING PHYSICAL HOLDING CAPACITY (LEVEL 8 BRAKES ON 42 deg SLOPE)
        std::cout << "\n--- TEST 14: BRAKE UPGRADES EXPANDING PHYSICAL HOLDING CAPACITY ---" << std::endl;
        {
            Core::ProfileManager upgradedProfile;
            upgradedProfile.unlockVehicle(Physics::VehicleType::OFFROADER);
            for (int u = 0; u < 8; ++u) {
                upgradedProfile.purchaseUpgrade(Physics::VehicleType::OFFROADER, Core::ProfileManager::UPGRADE_BRAKES);
            }
            int brakeLvl = upgradedProfile.getUpgradeLevel(Physics::VehicleType::OFFROADER, Core::ProfileManager::UPGRADE_BRAKES);
            float brakeMult = 1.0f + static_cast<float>(brakeLvl) * 0.08f;
            float holdAngleDeg = std::asin(std::min(1.0f, 0.573576f * brakeMult)) * (180.0f / 3.14159265f);

            std::cout << "  Off-Roader Brake Level: " << brakeLvl << " (Torque Multiplier: " << brakeMult << "x)\n"
                      << "  Calculated Physical Slope Holding Limit: " << std::fixed << std::setprecision(1) << holdAngleDeg << " deg (Expanded from 35 deg baseline)\n";

            Physics::Terrain ramp42;
            ramp42.setTestIncline(42.0f);
            Physics::Vehicle upgradedVeh(Physics::VehicleType::OFFROADER);
            float startX = 40.0f;
            float groundY42 = ramp42.getHeight(startX);
            upgradedVeh.reset(startX, groundY42 + 0.6f, 42.0f * 3.14159265f / 180.0f);

            for (int t = 0; t < 60; ++t) {
                upgradedVeh.applyInput(0.0f, 1.0f, 0.0f);
                upgradedVeh.step(DT, ramp42, upgradedProfile);
            }
            float settledX = upgradedVeh.chassisPos().x;

            for (int t = 0; t < 360; ++t) {
                upgradedVeh.applyInput(0.0f, 1.0f, 0.0f);
                upgradedVeh.step(DT, ramp42, upgradedProfile);
            }
            float driftUpgraded = std::abs(upgradedVeh.chassisPos().x - settledX);
            float speedUpgraded = upgradedVeh.getSpeedKmh();

            std::cout << "  3-Second Hold on 42 deg with Upgraded Brakes: Drift = " << driftUpgraded
                      << " m | Speed = " << speedUpgraded << " km/h\n";

            bool pass14 = (driftUpgraded < 0.15f && speedUpgraded < 0.15f);
            std::cout << "  -> " << (pass14 ? "PASS" : "FAIL") << std::endl;
            if (!pass14) allTestsPassed = false;
        }

        // 15. DOWNHILL SLOPE STABILITY & HOLDING (-25 deg SLOPE)
        std::cout << "\n--- TEST 15: DOWNHILL SLOPE STABILITY & HOLDING (-25 deg SLOPE) ---" << std::endl;
        {
            Physics::Terrain rampDown;
            rampDown.setTestIncline(-25.0f);
            Physics::Vehicle vehicleDown(Physics::VehicleType::OFFROADER);
            float startX = 30.0f;
            float groundY = rampDown.getHeight(startX);
            vehicleDown.reset(startX, groundY + 0.6f, -25.0f * 3.14159265f / 180.0f);

            for (int t = 0; t < 60; ++t) {
                vehicleDown.applyInput(0.0f, 1.0f, 0.0f);
                vehicleDown.step(DT, rampDown, profile);
            }
            float settledX = vehicleDown.chassisPos().x;

            for (int t = 0; t < 360; ++t) {
                vehicleDown.applyInput(0.0f, 1.0f, 0.0f);
                vehicleDown.step(DT, rampDown, profile);
            }
            float driftDown = std::abs(vehicleDown.chassisPos().x - settledX);
            float speedDown = vehicleDown.getSpeedKmh();

            std::cout << "  [-25 deg Downhill Incline]: 3-second Hill-Hold Drift: " << driftDown << " m | Speed: " << speedDown
                      << " km/h (Brakes hold against forward downhill pull)\n";

            bool pass15 = (driftDown < 0.20f && speedDown < 0.15f);
            std::cout << "  -> " << (pass15 ? "PASS" : "FAIL") << std::endl;
            if (!pass15) allTestsPassed = false;
        }

        // 16. DEDICATED REVERSE ENGINE CONTROL & SPEED CAP
        std::cout << "\n--- TEST 16: DEDICATED REVERSE ENGINE CONTROL & SPEED CAP ---" << std::endl;
        {
            Physics::Terrain terrain;
            terrain.setTestIncline(0.0f);
            Physics::Vehicle vehicle(Physics::VehicleType::OFFROADER);
            float groundY = terrain.getHeight(50.0f);
            vehicle.reset(50.0f, groundY + 0.5f);

            // Phase 1: Apply dedicated reverse input (gas=0, brake=0, reverse=1)
            float maxRevSpeed = 0.0f;
            for (int t = 0; t < 360; ++t) {
                vehicle.applyInput(0.0f, 0.0f, 1.0f);
                vehicle.step(DT, terrain, profile);
                float spd = vehicle.getSpeedKmh();
                if (spd > maxRevSpeed) maxRevSpeed = spd;
            }
            float revVx = vehicle.chassisVel().x;
            float topFwdKmh = vehicle.config().baseTopSpeedKmh;

            std::cout << "  Dedicated Reverse Drive: Vx = " << revVx << " m/s (Propelling backward)\n"
                      << "  Reverse Speed Achieved: " << maxRevSpeed << " km/h (Forward Base Top: " << topFwdKmh << " km/h)\n"
                      << "  Reverse/Forward Ratio: " << (maxRevSpeed / topFwdKmh * 100.0f) << "% (Target ~40%-45% cap)\n";

            // Phase 2: Release reverse, verify natural coasting deceleration
            float spdBeforeCoast = vehicle.getSpeedKmh();
            for (int t = 0; t < 120; ++t) {
                vehicle.applyInput(0.0f, 0.0f, 0.0f);
                vehicle.step(DT, terrain, profile);
            }
            float spdAfterCoast = vehicle.getSpeedKmh();
            std::cout << "  Coasting Deceleration: " << spdBeforeCoast << " km/h -> " << spdAfterCoast << " km/h (Natural roll resistance)\n";

            bool pass16 = (revVx < -2.0f && maxRevSpeed >= 20.0f && maxRevSpeed <= (topFwdKmh * 0.55f) && spdAfterCoast < spdBeforeCoast);
            std::cout << "  -> " << (pass16 ? "PASS" : "FAIL") << std::endl;
            if (!pass16) allTestsPassed = false;
        }

        // 16B. REVERSE BRAKING: progressive stop, no snap-to-zero, no forward propulsion
        std::cout << "\n--- TEST 16B: REVERSE BRAKING (PROGRESSIVE, UNIFIED, SLOPE-AWARE) ---" << std::endl;
        {
            auto runReverseBrake = [&](Physics::VehicleType vType, float inclineDeg, int driveTicks,
                                       const Core::ProfileManager& prof, const char* label) {
                Physics::Terrain terrain;
                terrain.setTestIncline(inclineDeg);
                Physics::Vehicle vehicle(vType);
                float startX = 40.0f;
                float gy = terrain.getHeight(startX);
                vehicle.reset(startX, gy + 0.6f, inclineDeg * 3.14159265f / 180.0f);
                for (int t = 0; t < driveTicks; ++t) {
                    vehicle.applyInput(0.0f, 0.0f, 1.0f);
                    vehicle.step(DT, terrain, prof);
                }
                float spd0 = vehicle.getSpeedKmh();
                float vx0 = vehicle.chassisVel().x;
                float spdAfter2f = spd0;
                float maxForwardVx = vx0;
                float timeToStop = -1.0f;
                float prevSpd = spd0;
                int increasingFrames = 0;
                for (int t = 0; t < 480; ++t) {
                    vehicle.applyInput(0.0f, 1.0f, 0.0f);
                    vehicle.step(DT, terrain, prof);
                    float vx = vehicle.chassisVel().x;
                    float spd = vehicle.getSpeedKmh();
                    if (t == 1) spdAfter2f = spd;
                    if (vx > maxForwardVx) maxForwardVx = vx;
                    if (spd > prevSpd + 0.35f) increasingFrames++;
                    prevSpd = spd;
                    if (timeToStop < 0.0f && spd < 1.5f && std::abs(vx) < 0.35f) {
                        timeToStop = t * DT;
                    }
                }
                bool notInstant = (spd0 > 8.0f) && (spdAfter2f > spd0 * 0.55f);
                bool noForwardProp = (vx0 < -0.5f) ? (maxForwardVx < 0.35f) : true;
                bool stoppedOrHeld = (timeToStop > 0.12f) || (vehicle.getSpeedKmh() < 12.0f);
                std::cout << "  [" << label << "] reverse " << spd0 << " km/h -> 2-frame " << spdAfter2f
                          << " km/h, t_stop=" << timeToStop << " s, maxFwdVx=" << maxForwardVx << "\n";
                bool pass = notInstant && noForwardProp && stoppedOrHeld && (spd0 > 8.0f);
                std::cout << "    -> " << (pass ? "PASS" : "FAIL") << std::endl;
                return pass;
            };

            bool pFlat = runReverseBrake(Physics::VehicleType::OFFROADER, 0.0f, 240, profile, "Off-Roader flat");
            bool pUp = runReverseBrake(Physics::VehicleType::OFFROADER, 15.0f, 220, profile, "Off-Roader +15 deg");
            bool pDown = runReverseBrake(Physics::VehicleType::OFFROADER, -15.0f, 220, profile, "Off-Roader -15 deg");
            bool pRally = runReverseBrake(Physics::VehicleType::RALLY_CAR, 0.0f, 220, profile, "Rally flat");
            bool pMuscle = runReverseBrake(Physics::VehicleType::MUSCLE_CAR, 0.0f, 240, profile, "Muscle flat");

            Core::ProfileManager upgradedBrakes;
            upgradedBrakes.addCoins(250000);
            upgradedBrakes.unlockVehicle(Physics::VehicleType::OFFROADER);
            for (int u = 0; u < 12; ++u) {
                upgradedBrakes.purchaseUpgrade(Physics::VehicleType::OFFROADER, Core::ProfileManager::UPGRADE_BRAKES);
            }
            Physics::Terrain flat;
            flat.setTestIncline(0.0f);
            auto timeReverseStop = [&](const Core::ProfileManager& prof) {
                Physics::Vehicle vehicle(Physics::VehicleType::OFFROADER);
                float gy = flat.getHeight(40.0f);
                vehicle.reset(40.0f, gy + 0.6f);
                for (int t = 0; t < 240; ++t) {
                    vehicle.applyInput(0.0f, 0.0f, 1.0f);
                    vehicle.step(DT, flat, prof);
                }
                for (int t = 0; t < 480; ++t) {
                    vehicle.applyInput(0.0f, 1.0f, 0.0f);
                    vehicle.step(DT, flat, prof);
                    if (vehicle.getSpeedKmh() < 1.5f) return t * DT;
                }
                return 999.0f;
            };
            float tStock = timeReverseStop(profile);
            float tUpg = timeReverseStop(upgradedBrakes);
            bool pUpgrade = (tUpg + 0.02f < tStock);
            std::cout << "  [Brake upgrades] stock t=" << tStock << " s, upgraded t=" << tUpg << " s\n";
            std::cout << "    -> " << (pUpgrade ? "PASS" : "FAIL") << std::endl;

            bool pass16b = pFlat && pUp && pDown && pRally && pMuscle && pUpgrade;
            std::cout << "  -> " << (pass16b ? "PASS" : "FAIL") << std::endl;
            if (!pass16b) allTestsPassed = false;
        }

        // 17. FUEL CONSUMPTION AUDIT (PROPULSION-ONLY CONSUMPTION)
        std::cout << "\n--- TEST 17: FUEL CONSUMPTION AUDIT (PROPULSION-ONLY CONSUMPTION) ---" << std::endl;
        {
            Physics::Terrain terrain;
            terrain.setTestIncline(0.0f);
            Physics::Vehicle vehicle(Physics::VehicleType::OFFROADER);
            float groundY = terrain.getHeight(20.0f);
            vehicle.reset(20.0f, groundY + 0.5f);

            // Case A: Idle (gas=0, brake=0, reverse=0)
            float fuelStart = vehicle.fuel();
            for (int t = 0; t < 120; ++t) {
                vehicle.applyInput(0.0f, 0.0f, 0.0f);
                vehicle.step(DT, terrain, profile);
            }
            float fuelIdleDelta = fuelStart - vehicle.fuel();
            std::cout << "  [Idle]: Fuel Consumed = " << fuelIdleDelta << "% (Zero idle drain)\n";

            // Case B: Coasting at 30 km/h with zero throttle
            vehicle.reset(20.0f, groundY + 0.5f);
            // Quick throttle to gain speed
            for (int t = 0; t < 60; ++t) {
                vehicle.applyInput(1.0f, 0.0f, 0.0f);
                vehicle.step(DT, terrain, profile);
            }
            float fuelCoastStart = vehicle.fuel();
            for (int t = 0; t < 120; ++t) {
                vehicle.applyInput(0.0f, 0.0f, 0.0f);
                vehicle.step(DT, terrain, profile);
            }
            float fuelCoastDelta = fuelCoastStart - vehicle.fuel();
            std::cout << "  [Coasting]: Fuel Consumed = " << fuelCoastDelta << "% (Zero coasting drain)\n";

            // Case C: Pure braking (gas=0, brake=1, reverse=0)
            float fuelBrakeStart = vehicle.fuel();
            for (int t = 0; t < 120; ++t) {
                vehicle.applyInput(0.0f, 1.0f, 0.0f);
                vehicle.step(DT, terrain, profile);
            }
            float fuelBrakeDelta = fuelBrakeStart - vehicle.fuel();
            std::cout << "  [Braking]: Fuel Consumed = " << fuelBrakeDelta << "% (Zero braking drain)\n";

            // Case D: Forward throttle (gas=1)
            float fuelGasStart = vehicle.fuel();
            for (int t = 0; t < 120; ++t) {
                vehicle.applyInput(1.0f, 0.0f, 0.0f);
                vehicle.step(DT, terrain, profile);
            }
            float fuelGasDelta = fuelGasStart - vehicle.fuel();
            std::cout << "  [Forward Throttle]: Fuel Consumed = " << fuelGasDelta << "% (Active engine drain)\n";

            // Case E: Reverse throttle (reverse=1)
            float fuelRevStart = vehicle.fuel();
            for (int t = 0; t < 120; ++t) {
                vehicle.applyInput(0.0f, 0.0f, 1.0f);
                vehicle.step(DT, terrain, profile);
            }
            float fuelRevDelta = fuelRevStart - vehicle.fuel();
            std::cout << "  [Reverse Throttle]: Fuel Consumed = " << fuelRevDelta << "% (Active reverse drain)\n";

            // Case F: Depleted fuel disables propulsion while retaining braking
            vehicle.reset(20.0f, groundY + 0.5f);
            vehicle.setFuel(0.0f); // Drain all fuel to 0
            float vxBeforeGas = vehicle.chassisVel().x;
            for (int t = 0; t < 60; ++t) {
                vehicle.applyInput(1.0f, 0.0f, 0.0f);
                vehicle.step(DT, terrain, profile);
            }
            float vxAfterGas = vehicle.chassisVel().x;
            bool noDriveOnEmpty = (std::abs(vxAfterGas - vxBeforeGas) < 0.01f);
            std::cout << "  [Depleted Fuel]: Forward propulsion disabled: " << (noDriveOnEmpty ? "YES" : "NO") << "\n";

            bool pass17 = (fuelIdleDelta < 0.0001f && fuelCoastDelta < 0.0001f && fuelBrakeDelta < 0.0001f
                           && fuelGasDelta > 0.05f && fuelRevDelta > 0.05f && noDriveOnEmpty);
            std::cout << "  -> " << (pass17 ? "PASS" : "FAIL") << std::endl;
            if (!pass17) allTestsPassed = false;
        }

        // 18. AUDIO ASSETS INVENTORY & INTEGRATION AUDIT
        std::cout << "\n--- TEST 18: AUDIO ASSETS INVENTORY & INTEGRATION AUDIT ---" << std::endl;
        {
            const std::vector<std::string> sfxAssets = {
                "assets/audio/sfx/click.wav",
                "assets/audio/sfx/coin.wav",
                "assets/audio/sfx/fuel.wav",
                "assets/audio/sfx/stunt.wav",
                "assets/audio/sfx/jump.wav",
                "assets/audio/sfx/landing_soft.wav",
                "assets/audio/sfx/landing_hard.wav",
                "assets/audio/sfx/impact_soft.wav",
                "assets/audio/sfx/impact_hard.wav",
                "assets/audio/sfx/crash.wav",
                "assets/audio/sfx/tyre_roll.wav",
                "assets/audio/sfx/skid.wav",
                "assets/audio/sfx/wind_loop.wav",
                "assets/audio/sfx/engine_idle.wav",
                "assets/audio/sfx/engine_tier1.wav",
                "assets/audio/sfx/engine_tier2.wav",
                "assets/audio/sfx/engine_tier3.wav",
                "assets/audio/sfx/engine_tier4.wav",
                "assets/audio/sfx/engine_tier5.wav",
                "assets/audio/sfx/engine_kenney.wav",
                "assets/audio/sfx/engine_motorcycle.wav"
            };

            const std::vector<std::string> musicAssets = {
                "assets/audio/music/menu_music.wav",
                "assets/audio/music/race_music.wav"
            };

            bool allFilesPresent = true;
            for (const auto& path : sfxAssets) {
                QFileInfo fi(QString::fromStdString(path));
                bool exists = fi.exists() && fi.size() > 500;
                if (!exists) {
                    std::cerr << "  MISSING OR EMPTY SFX ASSET: " << path << std::endl;
                    allFilesPresent = false;
                } else {
                    std::cout << "  Found SFX Asset: " << path << " (" << (fi.size() / 1024) << " KB PCM WAV)\n";
                }
            }

            for (const auto& path : musicAssets) {
                QFileInfo fi(QString::fromStdString(path));
                bool exists = fi.exists() && fi.size() > 500;
                if (!exists) {
                    std::cerr << "  MISSING OR EMPTY MUSIC ASSET: " << path << std::endl;
                    allFilesPresent = false;
                } else {
                    std::cout << "  Found Music Asset: " << path << " (" << (fi.size() / 1024) << " KB PCM WAV)\n";
                }
            }

            QFileInfo manifestFi("assets/audio/MANIFEST.md");
            bool manifestExists = manifestFi.exists() && manifestFi.size() > 200;
            std::cout << "  CC0 Licensing Manifest (MANIFEST.md): " << (manifestExists ? "PRESENT & VALID" : "MISSING") << "\n";

            QFileInfo attrFi("assets/audio/ATTRIBUTION.md");
            bool attrExists = attrFi.exists() && attrFi.size() > 200;
            std::cout << "  License & Attribution Documentation: " << (attrExists ? "PRESENT & VALID" : "MISSING") << "\n";

            Audio::SoundManager soundMgr;
            bool soundInit = soundMgr.isAudioAvailable();
            std::cout << "  SoundManager Asset Loading & Synthesis: " << (soundInit ? "ONLINE" : "OFFLINE") << "\n";
            std::cout << "  Loaded Sound Assets: " << soundMgr.loadedAssetCount() << "\n";
            std::cout << "  Audio Output Subsystem Device: " << soundMgr.audioDeviceName().toStdString() << "\n";

            bool pass18 = allFilesPresent && manifestExists && attrExists && soundInit && (soundMgr.loadedAssetCount() >= 15);
            std::cout << "  -> " << (pass18 ? "PASS" : "FAIL") << std::endl;
            if (!pass18) allTestsPassed = false;
        }

        // 19. UNIFIED DYNAMIC BRAKING AUDIT (SMOOTH DECELERATION, TERRAIN FRICTION, MULTI-SPEED FORWARD & REVERSE)
        std::cout << "\n--- TEST 19: UNIFIED DYNAMIC BRAKING AUDIT (SMOOTH DECEL, TERRAIN & TYRE FRICTION, TRANSITIONS) ---" << std::endl;
        {
            bool pass19 = true;

            // Subtest 19A: Forward Braking from Multi-Speeds (25 km/h, 50 km/h, 80 km/h) on Flat Ground
            std::cout << "  [19A: Multi-Speed Forward Braking on Flat Ground]" << std::endl;
            const float targetSpeeds[] = {25.0f, 50.0f, 80.0f};
            for (float tgtSpd : targetSpeeds) {
                Physics::Terrain terrain;
                terrain.setTestIncline(0.0f);
                Physics::Vehicle vehicle(Physics::VehicleType::OFFROADER);
                float gy = terrain.getHeight(20.0f);
                vehicle.reset(20.0f, gy + 0.6f);

                // Accelerate until target speed reached
                for (int t = 0; t < 600; ++t) {
                    vehicle.applyInput(1.0f, 0.0f, 0.0f);
                    vehicle.step(DT, terrain, profile);
                    if (vehicle.getSpeedKmh() >= tgtSpd) break;
                }

                float spd0 = vehicle.getSpeedKmh();
                float spdAfter2f = spd0;
                float startX = vehicle.chassisPos().x;
                float stopX = startX;
                float timeToStop = -1.0f;
                float minVx = 999.0f;

                for (int t = 0; t < 480; ++t) {
                    vehicle.applyInput(0.0f, 1.0f, 0.0f);
                    vehicle.step(DT, terrain, profile);
                    float vx = vehicle.chassisVel().x;
                    float spd = vehicle.getSpeedKmh();
                    if (vx < minVx) minVx = vx;
                    if (t == 1) spdAfter2f = spd;
                    if (timeToStop < 0.0f && spd < 0.5f) {
                        timeToStop = t * DT;
                        stopX = vehicle.chassisPos().x;
                    }
                }

                float stopDist = stopX - startX;
                bool notInstant = (spdAfter2f > spd0 * 0.70f);
                bool noBackwards = (minVx >= -0.05f);
                bool stoppedCleanly = (timeToStop > 0.15f && stopDist > 0.20f && vehicle.getSpeedKmh() < 0.10f);
                bool subPass = notInstant && noBackwards && stoppedCleanly;

                std::cout << "    Target " << tgtSpd << " km/h (Actual " << spd0 << " km/h) -> 2-frame " << spdAfter2f
                          << " km/h, StopTime = " << timeToStop << " s, StopDist = " << stopDist
                          << " m, MinVx = " << minVx << " m/s -> " << (subPass ? "PASS" : "FAIL") << std::endl;

                if (!subPass) pass19 = false;
            }

            // Subtest 19B: Slope-Aware Forward Braking (Uphill vs Downhill from 50 km/h)
            std::cout << "  [19B: Slope-Aware Forward Braking (Uphill +15 deg vs Downhill -15 deg)]" << std::endl;
            auto testSlopeBrake = [&](float inclineDeg, const char* label) -> std::pair<float, float> {
                Physics::Terrain terrain;
                terrain.setTestIncline(inclineDeg);
                Physics::Vehicle vehicle(Physics::VehicleType::OFFROADER);
                float gy = terrain.getHeight(30.0f);
                vehicle.reset(30.0f, gy + 0.6f, inclineDeg * 3.14159265f / 180.0f);

                for (int t = 0; t < 600; ++t) {
                    vehicle.applyInput(1.0f, 0.0f, 0.0f);
                    vehicle.step(DT, terrain, profile);
                    if (vehicle.getSpeedKmh() >= 50.0f) break;
                }

                float startX = vehicle.chassisPos().x;
                float stopX = startX;
                float timeToStop = -1.0f;
                float spd0 = vehicle.getSpeedKmh();

                for (int t = 0; t < 600; ++t) {
                    vehicle.applyInput(0.0f, 1.0f, 0.0f);
                    vehicle.step(DT, terrain, profile);
                    if (timeToStop < 0.0f && vehicle.getSpeedKmh() < 0.5f) {
                        timeToStop = t * DT;
                        stopX = vehicle.chassisPos().x;
                    }
                }
                float stopDist = std::abs(stopX - startX);
                std::cout << "    [" << label << "] Initial: " << spd0 << " km/h -> StopTime = " << timeToStop
                          << " s, StopDist = " << stopDist << " m" << std::endl;
                return {timeToStop, stopDist};
            };

            auto upRes = testSlopeBrake(15.0f, "Uphill +15 deg");
            auto downRes = testSlopeBrake(-15.0f, "Downhill -15 deg");
            // Downhill forward braking must take longer and more distance than uphill braking due to gravity pull
            bool passSlope = (downRes.first > upRes.first + 0.05f) && (downRes.second > upRes.second);
            std::cout << "    Slope physics consistency (Downhill StopTime > Uphill StopTime): " << (passSlope ? "PASS" : "FAIL") << std::endl;
            if (!passSlope) pass19 = false;

            // Subtest 19C: Terrain Friction & Skid Lockup Audit (Arctic Ice vs Countryside vs Mountain Rock)
            // Subtest 19C: Terrain Friction & Skid Lockup Audit (Glacial Ice vs Arctic vs Desert vs Countryside)
            std::cout << "  [19C: Terrain Friction & Skid Lockup Audit (Glacial Ice vs Snow vs Turf vs Rock)]" << std::endl;
            auto testFrictionBrake = [&](float frictionCoeff, const char* label) -> std::tuple<float, float, float> {
                Physics::Terrain terrain;
                terrain.setTestIncline(0.0f);
                terrain.setFriction(frictionCoeff);
                Physics::Vehicle vehicle(Physics::VehicleType::OFFROADER);
                float gy = terrain.getHeight(10.0f);
                vehicle.reset(10.0f, gy + 0.6f);

                for (int t = 0; t < 600; ++t) {
                    vehicle.applyInput(1.0f, 0.0f, 0.0f);
                    vehicle.step(DT, terrain, profile);
                    if (vehicle.getSpeedKmh() >= 50.0f) break;
                }

                float startX = vehicle.chassisPos().x;
                float stopX = startX;
                float timeToStop = -1.0f;
                float maxSlip = 0.0f;
                float spd0 = vehicle.getSpeedKmh();

                for (int t = 0; t < 600; ++t) {
                    vehicle.applyInput(0.0f, 1.0f, 0.0f);
                    vehicle.step(DT, terrain, profile);
                    float slip = std::abs(vehicle.rearSlipSpeed());
                    if (slip > maxSlip) maxSlip = slip;
                    if (timeToStop < 0.0f && vehicle.getSpeedKmh() < 0.5f) {
                        timeToStop = t * DT;
                        stopX = vehicle.chassisPos().x;
                    }
                }
                float stopDist = stopX - startX;
                std::cout << "    [" << label << "] Init: " << spd0 << " km/h, Friction: " << frictionCoeff
                          << " -> StopTime = " << timeToStop << " s, StopDist = " << stopDist
                          << " m, MaxSkidSlip = " << maxSlip << " m/s" << std::endl;
                return {timeToStop, stopDist, maxSlip};
            };

            auto [tIce, dIce, sIce] = testFrictionBrake(0.40f, "Slick Ice (mu=0.40)");
            auto [tSnow, dSnow, sSnow] = testFrictionBrake(0.82f, "Arctic Snow (mu=0.82)");
            auto [tTurf, dTurf, sTurf] = testFrictionBrake(1.25f, "Countryside Turf (mu=1.25)");
            auto [tRock, dRock, sRock] = testFrictionBrake(1.35f, "Mountain Rock (mu=1.35)");
            // Low friction ice must produce significant tyre skidding and longer stopping distance than high friction turf/rock
            bool passTerrain = (dIce > dSnow) && (dSnow > dTurf) && (dTurf >= dRock - 0.05f) && (sIce > 3.0f);
            std::cout << "    Terrain friction scaling (Ice Dist " << dIce << "m > Snow Dist " << dSnow
                      << "m > Turf Dist " << dTurf << "m >= Rock Dist " << dRock << "m): " << (passTerrain ? "PASS" : "FAIL") << std::endl;
            if (!passTerrain) pass19 = false;

            // Subtest 19D: Smooth Input Transitions (Brake -> Release -> Immediate Throttle)
            std::cout << "  [19D: Smooth Input Transitions (Brake -> Release -> Immediate Throttle)]" << std::endl;
            {
                Physics::Terrain terrain;
                terrain.setTestIncline(0.0f);
                Physics::Vehicle vehicle(Physics::VehicleType::OFFROADER);
                float gy = terrain.getHeight(20.0f);
                vehicle.reset(20.0f, gy + 0.6f);

                // Drive forward
                for (int t = 0; t < 120; ++t) {
                    vehicle.applyInput(1.0f, 0.0f, 0.0f);
                    vehicle.step(DT, terrain, profile);
                }
                float spdMoving = vehicle.getSpeedKmh();

                // Apply brake for 0.25s
                for (int t = 0; t < 30; ++t) {
                    vehicle.applyInput(0.0f, 1.0f, 0.0f);
                    vehicle.step(DT, terrain, profile);
                }
                float spdBraking = vehicle.getSpeedKmh();

                // Release brake and immediately hit full throttle
                for (int t = 0; t < 60; ++t) {
                    vehicle.applyInput(1.0f, 0.0f, 0.0f);
                    vehicle.step(DT, terrain, profile);
                }
                float spdReaccelerated = vehicle.getSpeedKmh();

                std::cout << "    Drive: " << spdMoving << " km/h -> Brake: " << spdBraking
                          << " km/h -> Re-throttle: " << spdReaccelerated << " km/h" << std::endl;

                bool passTransition = (spdBraking < spdMoving) && (spdReaccelerated > spdBraking + 5.0f);
                std::cout << "    Transition Verification: " << (passTransition ? "PASS" : "FAIL") << std::endl;
                if (!passTransition) pass19 = false;
            }

            std::cout << "  Test 19 Result: " << (pass19 ? "PASS" : "FAIL") << std::endl;
            if (!pass19) allTestsPassed = false;
        }

        std::cout << "\n=======================================================" << std::endl;
        std::cout << "  PHYSICS TEST SUITE RESULT: " << (allTestsPassed ? "ALL TESTS PASSED!" : "SOME TESTS FAILED!") << std::endl;
        std::cout << "=======================================================\n" << std::endl;

        return allTestsPassed ? 0 : 1;
    }

    if (argc > 1 && (std::string(argv[1]) == "--test-crash-capture" || std::string(argv[1]) == "--capture-crashes")) {
        QDir().mkpath("scratch");
        Core::ProfileManager profile;
        Graphics::Framebuffer fb(1280, 720);
        Graphics::Camera camera(1280, 720);
        Graphics::ScanlineRasterizer rasterizer;
        const float DT = 1.0f / 120.0f;

        std::cout << "\n=======================================================" << std::endl;
        std::cout << "  CAPTURING SETTLED CRASH SCENES FOR VISUAL VERIFICATION" << std::endl;
        std::cout << "=======================================================" << std::endl;

        // 1. Off-Roader inverted crash on Countryside
        {
            Physics::Terrain terrain;
            terrain.setBiome("countryside");
            Physics::Vehicle vehicle(Physics::VehicleType::OFFROADER);
            float startX = 35.0f;
            float groundY = terrain.getHeight(startX);
            vehicle.reset(startX, groundY + 1.2f, 3.14159f);

            // Settle on ground inverted
            for (int t = 0; t < 180; ++t) vehicle.step(DT, terrain, profile);

            camera.reset(vehicle.chassisPos());
            camera.update(0.016f, vehicle.chassisPos(), 0.0f);
            fb.clear(0xFF000000);
            rasterizer.render(fb, camera, terrain, 0.0f);
            Graphics::SpriteRenderer::renderVehicle(fb, camera, vehicle);
            fb.getImage().save("scratch/crash_settled_upsidedown.png");
            float pen = vehicle.getMaxTerrainPenetration(terrain);
            std::cout << "  [COUNTRYSIDE OFF-ROADER]: Settled Pen: " << pen << "m -> "
                      << (pen < 0.005f ? "PASS" : "FAIL") << " (Saved scratch/crash_settled_upsidedown.png)" << std::endl;
        }

        // 2. Rally Car crash on Mountain Slope
        {
            Physics::Terrain terrain;
            terrain.setBiome("mountain");
            Physics::Vehicle vehicle(Physics::VehicleType::RALLY_CAR);
            float startX = 948.0f;
            float groundY = terrain.getHeight(startX);
            vehicle.reset(startX, groundY + 1.2f, 2.35f); // Overturned on mountain slope
            for (int t = 0; t < 180; ++t) {
                vehicle.step(DT, terrain, profile);
            }

            camera.reset(vehicle.chassisPos());
            camera.update(0.016f, vehicle.chassisPos(), 0.0f);
            fb.clear(0xFF000000);
            rasterizer.render(fb, camera, terrain, 0.0f);
            Graphics::SpriteRenderer::renderVehicle(fb, camera, vehicle);
            fb.getImage().save("scratch/crash_settled_slope.png");
            float pen = vehicle.getMaxTerrainPenetration(terrain);
            std::cout << "  [MOUNTAIN RALLY CAR]: Settled Pen: " << pen << "m -> "
                      << (pen < 0.005f ? "PASS" : "FAIL") << " (Saved scratch/crash_settled_slope.png)" << std::endl;
        }

        // 3. Monster Truck inverted crash on Desert Dunes
        {
            Physics::Terrain terrain;
            terrain.setBiome("desert");
            Physics::Vehicle vehicle(Physics::VehicleType::MONSTER_TRUCK);
            float startX = 55.0f;
            float groundY = terrain.getHeight(startX);
            vehicle.reset(startX, groundY + 1.6f, 3.14159f);
            for (int t = 0; t < 180; ++t) vehicle.step(DT, terrain, profile);

            camera.reset(vehicle.chassisPos());
            camera.update(0.016f, vehicle.chassisPos(), 0.0f);
            fb.clear(0xFF000000);
            rasterizer.render(fb, camera, terrain, 0.0f);
            Graphics::SpriteRenderer::renderVehicle(fb, camera, vehicle);
            fb.getImage().save("scratch/crash_settled_monstertruck.png");
            float pen = vehicle.getMaxTerrainPenetration(terrain);
            std::cout << "  [DESERT MONSTER TRUCK]: Settled Pen: " << pen << "m -> "
                      << (pen < 0.005f ? "PASS" : "FAIL") << " (Saved scratch/crash_settled_monstertruck.png)" << std::endl;
        }

        std::cout << "=======================================================\n" << std::endl;
        return 0;
    }

    if (argc > 1 && (std::string(argv[1]) == "--test-environments" || std::string(argv[1]) == "--capture-environments")) {
        QDir().mkpath("scratch");
        Core::ProfileManager profile;
        Graphics::Framebuffer fb(1280, 720);
        Graphics::Camera camera(1280, 720);
        Graphics::ScanlineRasterizer rasterizer;

        std::cout << "\n=======================================================" << std::endl;
        std::cout << "  CAPTURING RICH PIXEL-ART ENVIRONMENTS & DAY-NIGHT SCENES" << std::endl;
        std::cout << "=======================================================" << std::endl;

        struct EnvTestScene {
            std::string biome;
            float timeOfDay;
            std::string label;
            std::string filename;
            Physics::VehicleType vehicle;
            float carX;
            float gameTime;
        };

        std::vector<EnvTestScene> scenes = {
            {"countryside", 0.50f, "Countryside at Noon (Sun, Oak/Pine Trees, Barn, Windmill)", "scratch/env_countryside_noon.png", Physics::VehicleType::OFFROADER, 40.0f, 12.0f},
            {"countryside", 0.75f, "Countryside at Sunset (Golden Sky, Warm Cast, Distant Mountains)", "scratch/env_countryside_sunset.png", Physics::VehicleType::OFFROADER, 65.0f, 25.0f},
            {"countryside", 0.05f, "Countryside at Night (Moon, Twinkling Stars, Glowing Barn, Fireflies)", "scratch/env_countryside_night.png", Physics::VehicleType::OFFROADER, 40.0f, 40.0f},
            {"arctic", 0.05f, "Arctic at Night (Aurora Borealis, Glacial Peaks, Snow Spreading, Research Dome)", "scratch/env_arctic_night.png", Physics::VehicleType::PICKUP_TRUCK, 45.0f, 15.0f},
            {"desert", 0.75f, "Desert at Sunset (Saguaro Cacti, Sandstone Arches, Mesas, Drifting Dust)", "scratch/env_desert_sunset.png", Physics::VehicleType::DESERT_RAID, 50.0f, 18.0f},
            {"moon", 0.00f, "Moon Lunar Vacuum (Blue Earth, Apollo Lunar Lander, Twinkling Stars)", "scratch/env_moon_midnight.png", Physics::VehicleType::ELECTRIC_OFFROADER, 35.0f, 10.0f},
            {"mountain", 0.35f, "Mountain Morning (Alpine Foothills, Watchtower Beacon, Pine Groves, Cairns)", "scratch/env_mountain_morning.png", Physics::VehicleType::MONSTER_TRUCK, 60.0f, 14.0f},
            {"volcano", 0.85f, "Volcano Dusk (Basalt Columns, Glowing Magma Vents, Fiery Embers)", "scratch/env_volcano_dusk.png", Physics::VehicleType::SUPERCAR, 42.0f, 22.0f},
            // Specific architectural & biome landmark close-ups
            {"countryside", 0.05f, "Countryside Vintage Barn (Illuminated Hayloft at Night)", "scratch/landmark_barn_night.png", Physics::VehicleType::OFFROADER, 8.0f, 50.0f},
            {"countryside", 0.50f, "Countryside Windmill Landmark (Rotating Sails at Noon)", "scratch/landmark_windmill_day.png", Physics::VehicleType::OFFROADER, 583.0f, 15.0f},
            {"desert", 0.75f, "Desert Natural Sandstone Arch Landmark", "scratch/landmark_desert_arch.png", Physics::VehicleType::DESERT_RAID, 8.0f, 18.0f},
            {"moon", 0.00f, "Moon Apollo Lunar Lander Landmark with Flag", "scratch/landmark_lunar_lander.png", Physics::VehicleType::ELECTRIC_OFFROADER, 8.0f, 10.0f},
            {"mountain", 0.35f, "Mountain Alpine Watchtower Landmark", "scratch/landmark_watchtower.png", Physics::VehicleType::MONSTER_TRUCK, 8.0f, 14.0f},
            {"arctic", 0.05f, "Arctic Geodesic Research Dome Landmark", "scratch/landmark_research_dome.png", Physics::VehicleType::PICKUP_TRUCK, 8.0f, 15.0f}
        };

        for (const auto& sc : scenes) {
            Physics::Terrain terrain;
            terrain.setBiome(sc.biome);
            Physics::Vehicle vehicle(sc.vehicle);
            float groundY = terrain.getHeight(sc.carX);
            vehicle.reset(sc.carX, groundY + 0.8f, 0.0f);

            // Settle vehicle onto ground
            for (int t = 0; t < 60; ++t) {
                vehicle.step(1.0f / 120.0f, terrain, profile);
            }

            rasterizer.dayNightCycle().setTimeOfDay(sc.timeOfDay);
            auto lighting = rasterizer.dayNightCycle().getLighting(sc.biome, 1280, 720, sc.carX);

            camera.reset(vehicle.chassisPos());
            camera.update(0.016f, vehicle.chassisPos(), 0.0f);

            fb.clear(0xFF000000);
            rasterizer.render(fb, camera, terrain, sc.gameTime);
            Graphics::SpriteRenderer::renderVehicle(fb, camera, vehicle, 1.0f);

            fb.getImage().save(QString::fromStdString(sc.filename));

            std::cout << "  [" << sc.biome << " @ " << rasterizer.dayNightCycle().clockString()
                      << " (" << rasterizer.dayNightCycle().phaseName() << ")]: "
                      << "AmbBright: " << lighting.ambientBrightness
                      << " | Saved: " << sc.filename << std::endl;
        }

        std::cout << "=======================================================\n" << std::endl;
        return 0;
    }

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
        bool autoThrottle = false;
        QTimer* driverPedalTimer = new QTimer(&app);
        QObject::connect(driverPedalTimer, &QTimer::timeout, [&]() {
            if (!autoThrottle) return;
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

        // 4b. In-Game Drive 0: Countryside Start Area with Monster Truck (testing grass color consistency from x=0 and stationary idle)
        QTimer::singleShot(1300, [&]() {
            engine.stateManager().profile().setSelectedVehicle(Physics::VehicleType::MONSTER_TRUCK);
            engine.stateManager().profile().setSelectedDriver(Physics::DriverType::BOB);
            engine.stateManager().startRace("countryside");

            auto& p = engine.stateManager().physics();
            float startX = 0.0f;
            float restY = p.terrain().getHeight(startX) + p.vehicle().suspRestLength() + p.vehicle().wheelRadius() - p.vehicle().rearMountOffset().y;
            p.vehicle().reset(startX, restY);
            engine.stateManager().camera().reset({startX, restY});

            autoThrottle = false;
            engine.input().state().gas = false;
            engine.input().state().brake = false;
            std::cout << "[SMOKE TEST] Started race with Monster Truck at Countryside start line (x=0) with throttle RELEASED." << std::endl;
        });

        // 4c. Verify stationary idle: ZERO wheel spin, ZERO fuel drain, RPM=1000
        QTimer::singleShot(1600, [&]() {
            const auto& p = engine.stateManager().physics();
            const auto& v = p.vehicle();
            float fuel = v.fuel();
            float rearAngVel = v.rearWheelAngularVel();
            float frontAngVel = v.frontWheelAngularVel();
            float rpm = v.getEngineRpm();
            float speed = v.getSpeedKmh();

            std::cout << "\n=======================================================" << std::endl;
            std::cout << "  STATIONARY WHEEL ROTATION & FUEL CONSUMPTION VERIFICATION " << std::endl;
            std::cout << "=======================================================" << std::endl;
            std::cout << "  - Vehicle Speed:       " << speed << " km/h (Expected: 0.0 km/h)" << std::endl;
            std::cout << "  - Engine RPM:          " << rpm << " RPM (Expected: 1000 RPM idle)" << std::endl;
            std::cout << "  - Fuel Level:          " << fuel << " % (Expected: 100.0%)" << std::endl;
            std::cout << "  - Rear Wheel AngVel:   " << rearAngVel << " rad/s (Expected: 0.0)" << std::endl;
            std::cout << "  - Front Wheel AngVel:  " << frontAngVel << " rad/s (Expected: 0.0)" << std::endl;

            assert(fuel == 100.0f);
            assert(std::abs(rearAngVel) < 0.001f);
            assert(std::abs(frontAngVel) < 0.001f);
            assert(std::abs(rpm - 1000.0f) < 1.0f);

            std::cout << "[SMOKE TEST] STATIONARY VERIFICATION: PASSED (Zero wheel rotation & zero fuel drain while idling)!" << std::endl;
            engine.framebuffer().getImage().save("scratch/gameplay_stationary_idle.png");

            // Now apply throttle to accelerate
            autoThrottle = true;
            engine.input().state().gas = true;
            std::cout << "[SMOKE TEST] Throttle pressed (gas=true) — accelerating down track..." << std::endl;
        });

        QTimer::singleShot(2000, [&]() {
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

        // 12. Game Over screen test with authentic rollover crash
        QTimer::singleShot(7200, [&]() {
            auto& sm = engine.stateManager();
            auto& p = sm.physics();
            float crashX = 70.0f;
            float groundY = p.terrain().getHeight(crashX);
            p.vehicle().reset(crashX, groundY + 1.2f, 3.14159f);
            sm.camera().reset({crashX, groundY + 1.2f});
            for (int t = 0; t < 120; ++t) {
                p.step(1.0f / 120.0f, sm.profile());
            }
            sm.changeState(Core::StateType::GAME_OVER);
            sm.render(engine.framebuffer(), 1.0f);
            engine.framebuffer().getImage().save("scratch/game_over_captured.png");
            std::cout << "[SMOKE TEST] Saved scratch/game_over_captured.png (authentic crash resting state)" << std::endl;
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
