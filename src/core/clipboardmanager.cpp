#include "core/clipboardmanager.h"

#include <QBuffer>
#include <QClipboard>
#include <QDebug>
#include <QGuiApplication>
#include <QImage>
#include <QMimeData>

ClipboardManager::ClipboardManager(QObject *parent)
    : QObject(parent)
    , clipboard(QGuiApplication::clipboard())
{}

void ClipboardManager::start()
{
    if (isActive) {
        return;
    }
    connect(clipboard, &QClipboard::changed, this, &ClipboardManager::onClipboardChanged);
    isActive = true;
    qDebug() << "ClipboardManager: started";
}

void ClipboardManager::stop()
{
    if (!isActive) {
        return;
    }
    disconnect(clipboard, &QClipboard::changed, this, &ClipboardManager::onClipboardChanged);
    isActive = false;
    qDebug() << "ClipboardManager: stopped";
}

bool ClipboardManager::getIsActive() const
{
    return isActive;
}

void ClipboardManager::onClipboardChanged()
{
    const QMimeData *mimeData = clipboard->mimeData();
    if (!mimeData) {
        return;
    }

    QJsonObject jsonData;
    jsonData["type"] = "clipboard";

    if (mimeData->hasImage()) {
        const QImage image = qvariant_cast<QImage>(mimeData->imageData());
        if (!image.isNull()) {
            // JSON can't hold binary data, so the image travels as base64-encoded PNG.
            QByteArray bytes;
            QBuffer buffer(&bytes);
            buffer.open(QIODevice::WriteOnly);
            image.save(&buffer, "PNG");

            jsonData["dataType"] = "image_png";
            jsonData["data"] = QString::fromLatin1(bytes.toBase64());
        }
    } else if (mimeData->hasText()) {
        jsonData["dataType"] = "text";
        jsonData["data"] = mimeData->text();
    }

    emit clipboardDataReady(jsonData);
}
