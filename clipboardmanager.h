#ifndef CLIPBOARDMANAGER_H
#define CLIPBOARDMANAGER_H

#include <QGuiApplication>
#include <QClipboard>
#include <QObject>
#include <QJsonObject>
#include <QMimeData>
#include <QImage>
#include <QBuffer>

class clipboardmanager : public QObject
{
    Q_OBJECT

public:
    clipboardmanager(QObject *parent = nullptr);
    ~clipboardmanager();

    void start();
    void stop();

    QClipboard *getClipboard();
    void setClipboard(QClipboard *newClipboard);
    bool getIsActive() const;
    void setIsActive(bool newIsActive);

private:
    QClipboard *clipboard;
    bool isActive;

signals:
    void clipboardDataReady(const QJsonObject &jsonData);

private slots:
    void onClipboardChanged();
};

#endif // CLIPBOARDMANAGER_H
