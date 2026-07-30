#include "ui/pdf_export.h"

#include <QPainter>
#include <QPrinter>
#include <QQuickWindow>
#include <QTextStream>

namespace ui {

bool PdfExport::export_chart(const QString& filePath, QQuickWindow* window,
                             const QVector<HistoryPoint>& points) {
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    printer.setPageSize(QPageSize(QPageSize::A4));

    QPainter painter(&printer);
    painter.setRenderHint(QPainter::Antialiasing);

    int pageW = printer.pageRect(QPrinter::DevicePixel).width();
    int pageH = printer.pageRect(QPrinter::DevicePixel).height();

    painter.setPen(Qt::black);
    QFont titleFont = painter.font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.drawText(50, 50, "ForgeSight - Historical Data Export");

    QFont headerFont = painter.font();
    headerFont.setPointSize(10);
    painter.setFont(headerFont);
    painter.drawText(50, 80, "Exported " + QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!points.isEmpty()) {
        QFont deviceFont = painter.font();
        deviceFont.setPointSize(9);
        painter.setFont(deviceFont);
        painter.drawText(50, 110,
                         QString("Device: %1 | Sensor: %2 | Points: %3")
                             .arg(points.first().deviceId)
                             .arg(points.first().sensor)
                             .arg(points.size()));
    }

    int y = 150;
    int lineH = 20;
    int colW = pageW / 4;

    QFont tableHeader = painter.font();
    tableHeader.setBold(true);
    painter.setFont(tableHeader);
    painter.drawText(50, y, "Timestamp");
    painter.drawText(50 + colW, y, "Value");
    painter.drawText(50 + 2 * colW, y, "Unit");
    painter.drawText(50 + 3 * colW, y, "Anomaly");
    y += lineH;

    painter.drawLine(50, y - 5, pageW - 50, y - 5);

    QFont tableFont = painter.font();
    tableFont.setBold(false);
    tableFont.setPointSize(8);
    painter.setFont(tableFont);

    int maxRows = (pageH - y) / lineH - 2;
    int count = qMin(points.size(), maxRows);

    for (int i = 0; i < count; ++i) {
        const auto& p = points[i];
        if (p.anomaly)
            painter.setPen(Qt::red);
        else
            painter.setPen(Qt::black);

        painter.drawText(50, y, p.timestamp.toString(Qt::ISODate));
        painter.drawText(50 + colW, y, QString::number(p.value, 'f', 2));
        painter.drawText(50 + 2 * colW, y, p.unit);
        painter.drawText(50 + 3 * colW, y, p.anomaly ? "Yes" : "No");
        y += lineH;
    }

    if (count < points.size()) {
        painter.setPen(Qt::black);
        painter.drawText(50, y + 10, QString("... and %1 more points").arg(points.size() - count));
    }

    if (window) {
        auto image = window->grabWindow();
        if (!image.isNull()) {
            int imgY = y + 40;
            int imgW = pageW - 100;
            int imgH = (pageH - imgY) / 2;
            painter.drawImage(QRect(50, imgY, imgW, imgH), image);
        }
    }

    painter.end();
    return true;
}

} // namespace ui
