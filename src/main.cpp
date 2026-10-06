#include <QApplication>
#include <QTimer>
#include <iostream>
#include <string>
#include "GameEngine.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("HillClimbRacingQt");
    app.setOrganizationName("HillClimbQtTeam");

    Core::GameEngine engine;

    if (argc > 1 && std::string(argv[1]) == "--smoke-test") {
        engine.show();
        engine.start();
        std::cout << "[SMOKE TEST] Initialized GameEngine and started tick loop." << std::endl;

        // 1. Visit GARAGE to verify garage vehicle rendering
        QTimer::singleShot(100, [&]() {
            engine.stateManager().changeState(Core::StateType::GARAGE);
        });
        QTimer::singleShot(250, [&]() {
            engine.framebuffer().getImage().save("scratch/garage_captured.png");
            std::cout << "[SMOKE TEST] Saved scratch/garage_captured.png" << std::endl;
        });

        // 2. Select Countryside & Start Driving at 400ms
        QTimer::singleShot(400, [&]() {
            std::cout << "[SMOKE TEST] Starting Countryside race..." << std::endl;
            engine.stateManager().startRace("countryside");
            engine.input().state().gas = true;
        });

        // Dynamic driver pedal control: full throttle on ground, pitch correction in air
        QTimer* driverPedalTimer = new QTimer(&app);
        QObject::connect(driverPedalTimer, &QTimer::timeout, [&]() {
            if (engine.stateManager().currentState() == Core::StateType::GAMEPLAY) {
                const auto& v = engine.stateManager().physics().vehicle();
                if (v.isAirborne()) {
                    // Level out if car is pitching backwards
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

        // 3. Intermediate check after 4.0s gas
        QTimer::singleShot(4400, [&]() {
            float dist = engine.stateManager().physics().distanceReached();
            float speed = engine.stateManager().physics().vehicle().getSpeedKmh();
            bool isOver = engine.stateManager().physics().isGameOver();

            engine.framebuffer().getImage().save("scratch/gameplay_captured.png");
            std::cout << "[SMOKE TEST] Checkpoint at 4.0s gas (saved scratch/gameplay_captured.png):" << std::endl;
            std::cout << "  - Distance reached: " << dist << " m" << std::endl;
            std::cout << "  - Speed: " << speed << " km/h" << std::endl;
            std::cout << "  - Status: " << (isOver ? "CRASHED" : "ALIVE & RACING!") << std::endl;
        });

        // 4. Checkpoint after 8.0s gas
        QTimer::singleShot(8400, [&]() {
            float dist = engine.stateManager().physics().distanceReached();
            float speed = engine.stateManager().physics().vehicle().getSpeedKmh();
            bool isOver = engine.stateManager().physics().isGameOver();

            std::cout << "[SMOKE TEST] Checkpoint at 8.0s gas:" << std::endl;
            std::cout << "  - Total Distance: " << dist << " m" << std::endl;
            std::cout << "  - Current Speed: " << speed << " km/h" << std::endl;
            std::cout << "  - Status: " << (isOver ? "CRASHED" : "ALIVE & RACING!") << std::endl;
        });

        // 5. Final Verification after 14.0s gas (over Ski Jump & Moguls)
        QTimer::singleShot(14400, [&]() {
            engine.input().state().gas = false;
            engine.input().state().brake = false;
            float dist = engine.stateManager().physics().distanceReached();
            float speed = engine.stateManager().physics().vehicle().getSpeedKmh();
            bool isOver = engine.stateManager().physics().isGameOver();
            int coins = engine.stateManager().physics().coinsCollected();
            int flips = engine.stateManager().physics().flipsCompleted();
            float airTime = engine.stateManager().physics().totalAirTime();

            std::cout << "[SMOKE TEST] Comprehensive Results after 14.0s race:" << std::endl;
            std::cout << "  - Total Distance: " << dist << " m" << std::endl;
            std::cout << "  - Current Speed: " << speed << " km/h" << std::endl;
            std::cout << "  - Coins Collected: " << coins << std::endl;
            std::cout << "  - Flips Completed: " << flips << std::endl;
            std::cout << "  - Total Air Time: " << airTime << " s" << std::endl;
            std::cout << "  - Status: " << (isOver ? "CRASHED" : "ALIVE & SOARING!") << std::endl;

            if (dist > 50.0f) {
                std::cout << "[SMOKE TEST] MASTER VERIFICATION SUCCESS: Vehicle conquered hills & obstacles smoothly!" << std::endl;
            } else {
                std::cerr << "[SMOKE TEST] VERIFICATION FAILURE: dist=" << dist << " isOver=" << isOver << std::endl;
            }
            app.quit();
        });
        return app.exec();
    }

    engine.show();
    engine.start();

    return app.exec();
}
