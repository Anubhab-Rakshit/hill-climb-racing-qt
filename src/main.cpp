#include <QApplication>
#include "GameEngine.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("HillClimbRacingQt");
    app.setOrganizationName("HillClimbQtTeam");

    Core::GameEngine engine;
    engine.show();
    engine.start();

    return app.exec();
}
