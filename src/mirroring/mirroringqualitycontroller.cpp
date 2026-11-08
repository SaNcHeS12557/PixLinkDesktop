#include "mirroring/mirroringqualitycontroller.h"

#include <QDebug>

#include <algorithm>
#include <array>

namespace {
// From best to most economical. Changing the bitrate is instant on the phone; changing the size or
// frame rate restarts its encoder (a short pause and a new keyframe), which is another reason to
// switch levels rarely.
constexpr std::array<MirroringQualityController::Level, 5> kLevels = {{
    {"High", 1920, 60, 8000},
    {"Balanced", 1600, 60, 5000},
    {"Medium", 1280, 30, 2500},
    {"Low", 960, 30, 1200},
    {"Minimal", 720, 20, 600},
}};

// Thresholds for one second of statistics.
constexpr double kBadQueueDelayMs = 150;    // frames arrive this much later than the network allows
constexpr double kSevereQueueDelayMs = 500; // ... badly enough to skip a level
constexpr double kBadRttMs = 250;
constexpr double kSevereRttMs = 800;
constexpr double kGoodQueueDelayMs = 50;
constexpr double kGoodRttMs = 100;
constexpr double kBadPhoneDropRatio = 0.05;
// Decoder busy for this share of the time = the PC can't decode as fast as frames arrive.
// (A queue of waiting packets alone isn't a reliable sign: late frames often arrive in a burst.)
constexpr double kBadDecoderLoad = 0.85;
constexpr double kGoodDecoderLoad = 0.6;

constexpr int kWarmupSeconds = 3; // start of a stream: keyframe bursts, ignore
constexpr int kBadSecondsToStepDown = 2;
constexpr int kMinSecondsBetweenChanges = 3;
constexpr int kBaseGoodSecondsToStepUp = 10;
constexpr int kMaxGoodSecondsToStepUp = 60;
constexpr int kFailedStepUpWindowS = 15; // trouble this soon after stepping up = the step up failed

// Starting level by round-trip time (ms) measured before mirroring starts.
int initialLevelForRtt(double rttMs)
{
    if (rttMs < 0) {
        return 1; // unknown yet
    }
    if (rttMs < 20) {
        return 0;
    }
    if (rttMs < 60) {
        return 1;
    }
    if (rttMs < 150) {
        return 2;
    }
    return 3;
}
} // namespace

int MirroringQualityController::levelCount()
{
    return int(kLevels.size());
}

MirroringQualityController::Level MirroringQualityController::level(int index)
{
    return kLevels[std::clamp(index, 0, levelCount() - 1)];
}

MirroringQualityController::MirroringQualityController(QObject *parent)
    : QObject(parent)
    , requiredGoodSeconds(kBaseGoodSecondsToStepUp)
{}

QJsonObject MirroringQualityController::prepareSession(double rttMs)
{
    levelIndex = manualLevel >= 0 ? manualLevel : initialLevelForRtt(rttMs);
    qDebug() << "MirroringQualityController: starting at" << describe() << "- RTT" << rttMs << "ms";
    emit levelChanged();
    return configFor(levelIndex);
}

void MirroringQualityController::startSession(int protocolVersion)
{
    sessionActive = true;
    phoneProtocolVersion = protocolVersion;
    secondsInSession = 0;
    badStreak = 0;
    goodStreak = 0;
    requiredGoodSeconds = kBaseGoodSecondsToStepUp;
    sinceLastChange.start();
    sinceLastStepUp.invalidate();
}

void MirroringQualityController::stopSession()
{
    sessionActive = false;
}

void MirroringQualityController::setManualLevel(int index)
{
    manualLevel = index < 0 ? -1 : std::clamp(index, 0, levelCount() - 1);
    if (manualLevel >= 0) {
        applyLevel(manualLevel, "chosen by user");
    } else {
        emit levelChanged(); // back to automatic; adaptation continues from the current level
    }
}

int MirroringQualityController::currentLevel() const
{
    return levelIndex;
}

bool MirroringQualityController::isAutomatic() const
{
    return manualLevel < 0;
}

QString MirroringQualityController::describe() const
{
    const Level l = level(levelIndex);
    return QString("%1 (%2) - %3 px, %4 fps, %5 Mbps")
        .arg(l.name, isAutomatic() ? "auto" : "fixed")
        .arg(l.maxSize)
        .arg(l.fps)
        .arg(l.bitrateKbps / 1000.0, 0, 'f', 1);
}

void MirroringQualityController::onStats(const MirroringStats &stats)
{
    if (!sessionActive || manualLevel >= 0) {
        return;
    }
    if (++secondsInSession <= kWarmupSeconds) {
        return;
    }

    const bool knownDelay = stats.queueDelayMs >= 0;
    const bool knownRtt = stats.rttMs >= 0;
    const double framesTotal = stats.receivedFps + stats.phoneDropped;
    const double phoneDropRatio = framesTotal > 0 ? stats.phoneDropped / framesTotal : 0;

    const bool severe =
        (knownDelay && stats.queueDelayMs > kSevereQueueDelayMs) || (knownRtt && stats.rttMs > kSevereRttMs);
    const bool bad = severe || (knownDelay && stats.queueDelayMs > kBadQueueDelayMs) ||
                     (knownRtt && stats.rttMs > kBadRttMs) || phoneDropRatio > kBadPhoneDropRatio ||
                     stats.decoderLoad > kBadDecoderLoad;
    const bool good = !bad && (!knownDelay || stats.queueDelayMs < kGoodQueueDelayMs) &&
                      (!knownRtt || stats.rttMs < kGoodRttMs) && stats.phoneDropped == 0 &&
                      stats.decoderLoad < kGoodDecoderLoad;

    if (bad) {
        badStreak++;
        goodStreak = 0;
    } else if (good) {
        goodStreak++;
        badStreak = 0;
    } else {
        badStreak = 0;
        goodStreak = 0;
    }

    const bool mayChange = sinceLastChange.elapsed() >= kMinSecondsBetweenChanges * 1000;

    if (badStreak >= kBadSecondsToStepDown && mayChange && levelIndex < levelCount() - 1) {
        // Trouble right after stepping up: that level is too much for now, wait longer next time.
        if (sinceLastStepUp.isValid() && sinceLastStepUp.elapsed() < kFailedStepUpWindowS * 1000) {
            requiredGoodSeconds = std::min(requiredGoodSeconds * 2, kMaxGoodSecondsToStepUp);
        }
        const QString reason = QString("queue delay %1 ms, RTT %2 ms, phone drops %3, decoder load %4%")
                                   .arg(stats.queueDelayMs, 0, 'f', 0)
                                   .arg(stats.rttMs, 0, 'f', 0)
                                   .arg(stats.phoneDropped)
                                   .arg(stats.decoderLoad * 100, 0, 'f', 0);
        applyLevel(std::min(levelIndex + (severe ? 2 : 1), levelCount() - 1), reason);
    } else if (goodStreak >= requiredGoodSeconds && mayChange && levelIndex > 0) {
        applyLevel(levelIndex - 1, QString("stable for %1 s").arg(goodStreak));
        sinceLastStepUp.start();
    }
}

void MirroringQualityController::applyLevel(int index, const QString &reason)
{
    if (index == levelIndex) {
        emit levelChanged();
        return;
    }
    levelIndex = index;
    badStreak = 0;
    goodStreak = 0;
    sinceLastChange.start();
    qDebug() << "MirroringQualityController: ->" << describe() << "-" << reason;

    if (sessionActive && phoneProtocolVersion >= 2) {
        emit configChanged(configFor(levelIndex));
    }
    emit levelChanged();
}

QJsonObject MirroringQualityController::configFor(int index) const
{
    const Level l = level(index);
    return QJsonObject{
        {"level", l.name},
        {"maxSize", l.maxSize},
        {"fps", l.fps},
        {"bitrateKbps", l.bitrateKbps},
    };
}
