#include "clipboardmanager.h"

clipboardmanager::clipboardmanager() {
    setClipboard(QGuiApplication::clipboard());
    setIsActive(false);
}

clipboardmanager::~clipboardmanager() {

}

void clipboardmanager::start() {
    connect(getClipboard(), &QClipboard::changed, this, &clipboardmanager::onClipboardChanged);
    setIsActive(true);
}

void clipboardmanager::stop() {
    disconnect(getClipboard(), &QClipboard::changed, this, &clipboardmanager::onClipboardChanged);
    setIsActive(false);
}

QClipboard* clipboardmanager::getClipboard()
{
    return clipboard;
}

void clipboardmanager::setClipboard(QClipboard *newClipboard)
{
    clipboard = newClipboard;
}

bool clipboardmanager::getIsActive() const
{
    return isActive;
}

void clipboardmanager::setIsActive(bool newIsActive)
{
    isActive = newIsActive;
}

void clipboardmanager::onClipboardChanged()
{
    const QMimeData* mimeData = getClipboard()->mimeData();
    if(!mimeData) return;

    QJsonObject jsonData;
    jsonData["type"] = "clipboard";

    if(mimeData->hasImage()) {
        QImage image = qvariant_cast<QImage>(mimeData->imageData());
        if(!image.isNull()) {
            QByteArray bytes;
            QBuffer buffer(&bytes);

            buffer.open(QIODevice::WriteOnly);
            image.save(&buffer, "PNG");

            jsonData["dataType"] = "image_png";
            jsonData["data"] = QString::fromLatin1(bytes.toBase64());
        }
    } else if(mimeData->hasText()) {
        jsonData["dataType"] = "text";
        jsonData["data"] = mimeData->text();
    }

    emit clipboardDataReady(jsonData);
}
