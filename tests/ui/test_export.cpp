#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryFile>
#include <QTextStream>

#include "ui/csv_export.h"
#include "ui/pdf_export.h"
#include "ui/history_model.h"

class ExportTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        static int argc = 0;
        static char* argv[] = {nullptr};
        static QCoreApplication app(argc, argv);
    }

    QVector<ui::HistoryPoint> sample_data() {
        return {
            {"pump-001", "temperature", 65.0, "°C",
             QDateTime::fromString("2026-07-26T10:00:00Z", Qt::ISODate), false},
            {"pump-001", "temperature", 70.0, "°C",
             QDateTime::fromString("2026-07-26T10:01:00Z", Qt::ISODate), true},
            {"pump-001", "temperature", 68.0, "°C",
             QDateTime::fromString("2026-07-26T10:02:00Z", Qt::ISODate), false},
        };
    }
};

TEST_F(ExportTest, CsvToString) {
    auto data = sample_data();
    QString csv = ui::CsvExport::to_string(data);

    EXPECT_TRUE(csv.startsWith("DeviceId,Sensor,Value,Unit,Timestamp,Anomaly"));
    EXPECT_TRUE(csv.contains("pump-001"));
    EXPECT_TRUE(csv.contains("temperature"));
    EXPECT_TRUE(csv.contains("65"));
    EXPECT_TRUE(csv.contains("true"));
}

TEST_F(ExportTest, CsvToFile) {
    auto data = sample_data();
    QTemporaryFile tmpFile;
    tmpFile.open();
    QString path = tmpFile.fileName();
    tmpFile.close();

    EXPECT_TRUE(ui::CsvExport::to_file(path, data));

    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = file.readAll();
    file.remove();

    EXPECT_TRUE(content.contains("DeviceId,Sensor,Value,Unit,Timestamp,Anomaly"));
}

TEST_F(ExportTest, CsvEmptyData) {
    QVector<ui::HistoryPoint> empty;
    QString csv = ui::CsvExport::to_string(empty);
    EXPECT_TRUE(csv.contains("DeviceId,Sensor,Value,Unit,Timestamp,Anomaly"));
    EXPECT_EQ(csv.count('\n'), 1);
}

TEST_F(ExportTest, CsvHandlesSpecialChars) {
    QVector<ui::HistoryPoint> data = {
        {"device,\"a\"", "sensor,1", 42.5, "unit",
         QDateTime::fromString("2026-07-26T10:00:00Z", Qt::ISODate), false},
    };
    QString csv = ui::CsvExport::to_string(data);
    EXPECT_TRUE(csv.contains("\"device,\"\"a\"\"\""));
}
