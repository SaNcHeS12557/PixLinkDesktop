#ifndef MIRRORINGQUALITYCONTROLLER_H
#define MIRRORINGQUALITYCONTROLLER_H

#include "mirroring/mirroringstats.h"

#include <QElapsedTimer>
#include <QJsonObject>
#include <QObject>

// Chooses the stream quality (resolution, frame rate, bitrate) the phone should send, and adapts it
// to the network and to how fast this PC decodes.
//
// It steps down quickly when frames start to queue up (a few seconds of trouble) and steps up slowly
// (only after a longer good period), so the picture doesn't keep flipping between levels. If stepping
// up causes trouble again soon after, the next attempt waits longer.
//
// The user can also pin a fixed level; then the controller only reports, it doesn't adapt.
class MirroringQualityController : public QObject
{
    Q_OBJECT

public:
    struct Level
    {
        const char *name;
        int maxSize; // longest side of the video, in pixels
        int fps;
        int bitrateKbps;
    };

    static int levelCount();
    static Level level(int index);

    explicit MirroringQualityController(QObject *parent = nullptr);

    // Picks the starting level from the measured round-trip time and returns the settings to put into
    // the "init_mirroring_request" command.
    QJsonObject prepareSession(double rttMs);
    // The phone has started streaming. Configuration updates are only sent to v2 phones.
    void startSession(int phoneProtocolVersion);
    void stopSession();

    // index = fixed level, -1 = automatic.
    void setManualLevel(int index);
    int currentLevel() const;
    bool isAutomatic() const;
    QString describe() const;

public slots:
    void onStats(const MirroringStats &stats);

signals:
    // Settings to send to the phone as a "mirroring_config" command.
    void configChanged(const QJsonObject &config);
    void levelChanged();

private:
    int levelIndex = 1;
    int manualLevel = -1;
    bool sessionActive = false;
    int phoneProtocolVersion = 1;

    int secondsInSession = 0;
    int badStreak = 0;
    int goodStreak = 0;
    int requiredGoodSeconds;
    QElapsedTimer sinceLastChange;
    QElapsedTimer sinceLastStepUp;

    void applyLevel(int index, const QString &reason);
    QJsonObject configFor(int index) const;
};

#endif // MIRRORINGQUALITYCONTROLLER_H
