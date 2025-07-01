#include "clipboardmanager.h"

ClipboardManager::ClipboardManager(QObject *parent)
    : QObject(parent)
{
    setClipboard(QGuiApplication::clipboard());
    setIsActive(false);
}

ClipboardManager::~ClipboardManager() {}

void ClipboardManager::start() {
    if(isActive) return;
    connect(getClipboard(), &QClipboard::changed, this, &ClipboardManager::onClipboardChanged);
    setIsActive(true);
    qDebug() << "Clipboard manager started!";
}

void ClipboardManager::stop() {
    if(!isActive) return;
    disconnect(getClipboard(), &QClipboard::changed, this, &ClipboardManager::onClipboardChanged);
    setIsActive(false);
        qDebug() << "Clipboard manager stopped!";
}

QClipboard* ClipboardManager::getClipboard()
{
    return clipboard;
}

void ClipboardManager::setClipboard(QClipboard *newClipboard)
{
    clipboard = newClipboard;
}

bool ClipboardManager::getIsActive() const
{
    return isActive;
}

void ClipboardManager::setIsActive(bool newIsActive)
{
    isActive = newIsActive;
}

void ClipboardManager::onClipboardChanged()
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

    qDebug() << "Clipboard changed -> data ready to send!";
    emit clipboardDataReady(jsonData);
}
