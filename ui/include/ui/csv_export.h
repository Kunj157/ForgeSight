#pragma once

#include <QString>
#include <QVector>

#include "ui/history_model.h"

namespace ui {

class CsvExport {
  public:
    static bool to_file(const QString& filePath, const QVector<HistoryPoint>& points);
    static QString to_string(const QVector<HistoryPoint>& points);
};

} // namespace ui
