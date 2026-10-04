#pragma once
#include <QPalette>
#include <QString>
#include <vector>

struct AppTheme {
    QString id, name;
    QColor background, surface, text, muted, border, accent, button, buttonText;
    QPalette palette() const;
    QString styleSheet() const;
};
// Stable IDs keep saved preferences independent of the tab order.
const std::vector<AppTheme>& appThemes();
int themeIndex(const QString& id);
