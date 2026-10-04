#include "MainWindow.h"
#include "AppTheme.h"
#include <QApplication>
#include <QSettings>
#include <QPushButton>
#include <QTabBar>
#include <QTemporaryDir>
#include <cstdlib>
#include <iostream>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QApplication::setStyle("Fusion");
    app.setOrganizationName("SystemMonitorTests");
    app.setApplicationName("ThemeTests");
    QTemporaryDir temporary;
    // Test real window behavior without writing the user's appearance preference.
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temporary.path());
    auto require = [](bool value, const char* message) {
        if (!value) { std::cerr << message << '\n'; std::exit(1); }
    };
    require(temporary.isValid(), "Temporary settings directory failed");
    {
        MainWindow window;
        window.show();
        auto* tabs = window.findChild<QTabBar*>("themeTabs");
        require(tabs && tabs->count() == 6 && tabs->currentIndex() == 0, "Fresh launch did not default to Blue");
        for (int index = 0; index < tabs->count(); ++index) {
            tabs->setCurrentIndex(index);
            app.processEvents();
            const auto& theme = appThemes().at(index);
            require(app.palette().color(QPalette::Base) == theme.surface, "Theme palette did not change");
            auto* graph = window.findChild<QWidget*>("cpuHistory");
            const auto image = graph->grab().toImage();
            const auto scale = image.devicePixelRatio();
            const auto background = image.pixelColor(image.width() - int(20 * scale), int(38 * scale));
            if (background != theme.surface) std::cerr << theme.id.toStdString() << ": actual=" << background.name().toStdString() << " expected=" << theme.surface.name().toStdString() << " palette=" << graph->palette().color(QPalette::Base).name().toStdString() << " scale=" << scale << '\n';
            require(background == theme.surface,
                    "Graph background did not follow the theme");
        }
        window.resize(740, 560);
        app.processEvents();
        auto* refresh = window.findChild<QPushButton*>();
        require(tabs->isVisible() && refresh && refresh->isVisible(), "Small window hid theme or refresh controls");
        require(window.rect().contains(tabs->mapTo(&window, tabs->rect().bottomRight()))
                && window.rect().contains(refresh->mapTo(&window, refresh->rect().bottomRight())),
                "Small window clipped fixed controls");
        tabs->setCurrentIndex(themeIndex("light"));
    }
    {
        MainWindow restored;
        require(restored.findChild<QTabBar*>("themeTabs")->currentIndex() == themeIndex("light"),
                "Selected theme did not persist across window creation");
    }
    QSettings{}.setValue("appearance/theme", "unknown-theme");
    {
        MainWindow recovered;
        require(recovered.findChild<QTabBar*>("themeTabs")->currentIndex() == 0,
                "Unknown saved theme did not fall back to Blue");
    }
    std::cout << "Six themes, custom graph colors, preference persistence and fallback passed\n";
}
