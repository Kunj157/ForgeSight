#include "ui/browser_download.h"

#ifdef Q_OS_WASM
#include <emscripten/val.h>
#endif

namespace ui {

bool browser_download(const QByteArray& data, const QString& fileName, const QString& mimeType) {
#ifdef Q_OS_WASM
    using emscripten::val;
    // Copy the bytes into a Blob and click a temporary download link, the
    // same thing a page does for a "Save" button.
    val bytes = val::global("Uint8Array").new_(data.size());
    bytes.call<void>("set",
                     val(emscripten::typed_memory_view(
                         data.size(), reinterpret_cast<const unsigned char*>(data.constData()))));
    val parts = val::array();
    parts.call<void>("push", bytes);
    val options = val::object();
    options.set("type", mimeType.toStdString());
    val blob = val::global("Blob").new_(parts, options);
    val url = val::global("URL").call<val>("createObjectURL", blob);

    val document = val::global("document");
    val link = document.call<val>("createElement", std::string("a"));
    link.set("href", url);
    link.set("download", fileName.toStdString());
    document["body"].call<void>("appendChild", link);
    link.call<void>("click");
    link.call<void>("remove");
    // The URL is not revoked: revoking right after click() can cancel the
    // download in some browsers, and the page holds at most a few exports.
    return true;
#else
    Q_UNUSED(data)
    Q_UNUSED(fileName)
    Q_UNUSED(mimeType)
    return false;
#endif
}

} // namespace ui
