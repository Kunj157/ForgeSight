#include "ui/pdf_export.h"

#include <QBuffer>
#include <QFile>
#include <QPainter>
#include <QPdfWriter>
#include <QQuickWindow>

namespace ui {

namespace {

void paint_report(QPainter& painter, int pageW, int pageH, QQuickWindow* window,
                  const QVector<HistoryPoint>& points) {
    painter.setRenderHint(QPainter::Antialiasing);

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

    // Just under the header text (y is already the first row's baseline).
    painter.drawLine(50, y - lineH + 6, pageW - 50, y - lineH + 6);

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
}

} // namespace

QByteArray PdfExport::to_bytes(QQuickWindow* window, const QVector<HistoryPoint>& points) {
    QByteArray pdf;
    QBuffer buffer(&pdf);
    buffer.open(QIODevice::WriteOnly);
    {
        // QPdfWriter is part of Qt GUI, so the same code runs on the desktop
        // and in the browser build (QPrinter is not available there).
        QPdfWriter writer(&buffer);
        // The layout below is in screen pixels (50 px margins, 20 px rows), so
        // draw at screen resolution. At the 1200 dpi QPrinter used before,
        // every row landed in the same few millimetres at the top of the page.
        writer.setResolution(96);
        writer.setPageSize(QPageSize(QPageSize::A4));
        writer.setPageMargins(QMarginsF(10, 10, 10, 10), QPageLayout::Millimeter);
        writer.setTitle(QStringLiteral("ForgeSight - Historical Data Export"));

        QPainter painter;
        if (!painter.begin(&writer))
            return {};
        paint_report(painter, writer.width(), writer.height(), window, points);
        painter.end();
    }
    return pdf;
}

bool PdfExport::export_chart(const QString& filePath, QQuickWindow* window,
                             const QVector<HistoryPoint>& points) {
    const QByteArray pdf = to_bytes(window, points);
    if (pdf.isEmpty())
        return false;
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(pdf) == pdf.size();
}

} // namespace ui
