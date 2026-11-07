#ifndef CLIPBOARDMANAGER_H
#define CLIPBOARDMANAGER_H

#include <QJsonObject>
#include <QObject>

class QClipboard;

// Watches the PC clipboard and packs every change (text or image) into a JSON message for the phone.
class ClipboardManager : public QObject
{
    Q_OBJECT

public:
    explicit ClipboardManager(QObject *parent = nullptr);

    void start();
    void stop();
    bool getIsActive() const;

signals:
    // {"type": "clipboard", "dataType": "text" | "image_png", "data": <text or base64 PNG>}
    void clipboardDataReady(const QJsonObject &jsonData);

private slots:
    void onClipboardChanged();

private:
    QClipboard *clipboard;
    bool isActive = false;
};

#endif // CLIPBOARDMANAGER_H
