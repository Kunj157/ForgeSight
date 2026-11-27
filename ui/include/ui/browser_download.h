#pragma once

#include <QByteArray>
#include <QString>

namespace ui {

// In the browser build, writing to a path only reaches the page's in-memory
// file system, so exports are handed to the browser as a download instead.
// Returns false outside the browser build.
bool browser_download(const QByteArray& data, const QString& fileName, const QString& mimeType);

} // namespace ui
