#include "gui/MainWindow.h"
#include <QApplication>
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("ConcurrentSimulator Preview");
    app.setStyle("Fusion");
    gui::MainWindow window;
    // Composición futura: window.attachEngine(makeLogicalEngine(config), "Motor lógico");
    // Añadir fábricas reales y selector cuando pasen G1–G4, sin cambiar widgets.
    window.show();
    return app.exec();
}
