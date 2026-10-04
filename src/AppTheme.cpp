#include "AppTheme.h"
#include <algorithm>

const std::vector<AppTheme>& appThemes()
{
    static const std::vector<AppTheme> themes{
        {"blue", "Blue", QColor("#111827"), QColor("#1f2937"), QColor("#e5e7eb"), QColor("#a7b0be"), QColor("#374151"), QColor("#38bdf8"), QColor("#2563eb"), Qt::white},
        {"dark", "Dark", QColor("#151515"), QColor("#242424"), QColor("#f1f1f1"), QColor("#b1b1b1"), QColor("#444444"), QColor("#dedede"), QColor("#4b4b4b"), Qt::white},
        {"light", "Light", QColor("#f3f4f6"), QColor("#ffffff"), QColor("#172033"), QColor("#526174"), QColor("#cbd5e1"), QColor("#2563eb"), QColor("#2563eb"), Qt::white},
        {"forest", "Forest", QColor("#10221b"), QColor("#1c3429"), QColor("#e8f5ed"), QColor("#a6bfaf"), QColor("#365347"), QColor("#4ade80"), QColor("#15803d"), Qt::white},
        {"plum", "Plum", QColor("#20172a"), QColor("#32243f"), QColor("#f2eaf8"), QColor("#bfaece"), QColor("#554362"), QColor("#c084fc"), QColor("#7e22ce"), Qt::white},
        {"ember", "Ember", QColor("#291c16"), QColor("#3a2a22"), QColor("#fbefe7"), QColor("#c5b2a5"), QColor("#644b3d"), QColor("#fb923c"), QColor("#c2410c"), Qt::white}
    };
    return themes;
}

int themeIndex(const QString& id)
{
    const auto& themes = appThemes();
    const auto found = std::find_if(themes.begin(), themes.end(), [&](const auto& theme) { return theme.id == id; });
    return found == themes.end() ? 0 : static_cast<int>(found - themes.begin());
}

QPalette AppTheme::palette() const
{
    QPalette p;
    p.setColor(QPalette::Window, background);
    p.setColor(QPalette::Base, surface);
    p.setColor(QPalette::AlternateBase, background);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, button);
    p.setColor(QPalette::ButtonText, buttonText);
    p.setColor(QPalette::Highlight, button);
    p.setColor(QPalette::HighlightedText, buttonText);
    p.setColor(QPalette::Mid, border);
    p.setColor(QPalette::PlaceholderText, muted);
    p.setColor(QPalette::Link, accent);
    p.setColor(QPalette::ToolTipBase, surface);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Disabled, QPalette::Text, muted);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, muted);
    return p;
}

QString AppTheme::styleSheet() const
{
    // Custom drawing uses the palette; every stylesheet color is defined here.
    return QString(R"(
        QWidget { background: %1; color: %3; font-family: 'Segoe UI'; font-size: 14px; }
        QWidget#cpuHistory { background: %2; }
        QLabel { background: transparent; }
        QLabel#pageTitle { font-size: 27px; font-weight: 700; }
        QLabel#sectionTitle { font-size: 16px; font-weight: 600; }
        QLabel#muted, QStatusBar { color: %4; }
        QFrame#metricCard { background: %2; border: 1px solid %5; border-radius: 12px; }
        QLabel#metricTitle { font-size: 15px; font-weight: 600; }
        QLabel#metricValue { font-size: 36px; font-weight: 600; }
        QProgressBar { background: %5; border: none; border-radius: 4px; }
        QProgressBar::chunk { background: %6; border-radius: 4px; }
        QPushButton { background: %7; color: %8; border: 2px solid transparent;
                      border-radius: 7px; padding: 8px 18px; font-weight: 600; }
        QPushButton:hover { background: %9; }
        QPushButton:pressed { background: %7; }
        QPushButton:focus { border-color: %6; }
        QTabBar::tab { background: %2; color: %4; border: 1px solid %5;
                      padding: 8px 15px; margin-right: 5px; border-radius: 6px; }
        QTabBar::tab:selected { color: %3; border-color: %6; }
        QTabBar::tab:hover { color: %3; }
        QTabBar::tab:focus { border-color: %6; }
        QTableView { background: %2; alternate-background-color: %1;
                     selection-background-color: %7; selection-color: %8;
                     border: 1px solid %5; border-radius: 8px; }
        QTableView::item { padding: 4px 8px; border: none; }
        QHeaderView::section { background: %2; color: %3; padding: 9px 8px; border: none;
                               border-bottom: 1px solid %5; font-weight: 600; }
        QTableView QTableCornerButton::section { background: %2; border: none; }
        QToolTip { background: %2; color: %3; border: 1px solid %5; padding: 5px; }
        QScrollBar:vertical { background: %1; width: 10px; margin: 0px; }
        QScrollBar::handle:vertical { background: %5; min-height: 28px; border-radius: 5px; }
        QScrollBar::handle:vertical:hover { background: %4; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
        QScrollBar:horizontal { background: %1; height: 10px; margin: 0px; }
        QScrollBar::handle:horizontal { background: %5; min-width: 28px; border-radius: 5px; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }
    )").arg(background.name(), surface.name(), text.name(), muted.name(), border.name(),
            accent.name(), button.name(), buttonText.name(), button.lighter(115).name());
}
