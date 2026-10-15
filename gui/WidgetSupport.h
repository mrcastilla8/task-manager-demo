#pragma once
#include "gui/Presentation.h"
#include <QColor>
#include <QString>
namespace gui {
inline QString number(std::int64_t value) { return QString::number(static_cast<qlonglong>(value)); }
inline QColor pidColor(int pid) { return QColor::fromHsv((pid * 47LL % 360 + 360) % 360, 135, 215); }
inline QString priority(api::Prioridad value) { return "P" + QString::number(static_cast<int>(value)); }
}
