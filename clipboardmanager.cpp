#include "clipboardmanager.h"

clipboardmanager::clipboardmanager(QObject *parent)
    : QObject(parent)
{
    setClipboard(QGuiApplication::clipboard());
    setIsActive(false);
}

clipboardmanager::~clipboardmanager() {}

void clipboardmanager::start() {
    if(isActive) return;
    connect(getClipboard(), &QClipboard::changed, this, &clipboardmanager::onClipboardChanged);
    setIsActive(true);
    qDebug() << "Clipboard manager started!";
}

void clipboardmanager::stop() {
    if(!isActive) return;
    disconnect(getClipboard(), &QClipboard::changed, this, &clipboardmanager::onClipboardChanged);
    setIsActive(false);
        qDebug() << "Clipboard manager stopped!";
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

    qDebug() << "Clipboard changed -> data ready to send!";
    emit clipboardDataReady(jsonData);
}
