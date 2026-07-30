#pragma once

#include <QString>
#include <QVector>

#include "ui/history_model.h"

QT_BEGIN_NAMESPACE
class QQuickWindow;
QT_END_NAMESPACE

namespace ui {

class PdfExport {
  public:
    static bool export_chart(const QString& filePath, QQuickWindow* window,
                             const QVector<HistoryPoint>& points);
};

} // namespace ui
