#include "SoundManager.h"
#include <QSoundEffect>
#include <QUrl>
#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QMediaPlayer>
#include <QAudioOutput>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QAudioSink>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QAudioDevice>
#endif
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Audio {

#pragma pack(push, 1)
struct WavHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t fileSize = 0;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1; // PCM
    uint16_t numChannels = 1; // Mono
    uint32_t sampleRate = 22050;
    uint32_t byteRate = 22050 * 2;
    uint16_t blockAlign = 2;
    uint16_t bitsPerSample = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize = 0;
};
#pragma pack(pop)

static void writeWavFile(const QString& filePath, const std::vector<int16_t>& samples, uint32_t sampleRate = 22050) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return;

    WavHeader header;
    header.sampleRate = sampleRate;
    header.byteRate = sampleRate * 2;
    header.dataSize = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
    header.fileSize = sizeof(WavHeader) - 8 + header.dataSize;

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(samples.data()), header.dataSize);
    file.close();
}

static bool readWavFile(const QString& filePath, std::vector<int16_t>& outSamples, uint32_t& outSampleRate) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QByteArray data = file.readAll();
    file.close();

    if (data.size() < 44) return false;

    const char* ptr = data.constData();
    if (std::memcmp(ptr, "RIFF", 4) != 0 || std::memcmp(ptr + 8, "WAVE", 4) != 0) {
        return false;
    }

    int offset = 12;
    uint16_t channels = 1;
    uint16_t bitsPerSample = 16;
    outSampleRate = 22050;
    const char* audioDataPtr = nullptr;
    uint32_t audioDataSize = 0;

    while (offset + 8 <= data.size()) {
        char chunkId[5] = {0};
        std::memcpy(chunkId, ptr + offset, 4);
        uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(ptr + offset + 4);
        if (std::memcmp(chunkId, "fmt ", 4) == 0 && offset + 8 + 16 <= data.size()) {
            channels = *reinterpret_cast<const uint16_t*>(ptr + offset + 10);
            outSampleRate = *reinterpret_cast<const uint32_t*>(ptr + offset + 12);
            bitsPerSample = *reinterpret_cast<const uint16_t*>(ptr + offset + 22);
        } else if (std::memcmp(chunkId, "data", 4) == 0) {
            audioDataPtr = ptr + offset + 8;
            audioDataSize = std::min(chunkSize, static_cast<uint32_t>(data.size() - (offset + 8)));
            break;
        }
        offset += 8 + chunkSize;
    }

    if (!audioDataPtr || audioDataSize == 0 || bitsPerSample != 16) return false;

    int numSamples = audioDataSize / sizeof(int16_t);
    const int16_t* srcSamples = reinterpret_cast<const int16_t*>(audioDataPtr);
    if (channels == 1) {
        outSamples.assign(srcSamples, srcSamples + numSamples);
    } else if (channels == 2) {
        outSamples.resize(numSamples / 2);
        for (size_t i = 0; i < outSamples.size(); ++i) {
            int32_t mixed = (static_cast<int32_t>(srcSamples[i * 2]) + static_cast<int32_t>(srcSamples[i * 2 + 1])) / 2;
            outSamples[i] = static_cast<int16_t>(mixed);
        }
    }
    return !outSamples.empty();
}

class MixerIODevice : public QIODevice {
public:
    explicit MixerIODevice(Audio::SoundManager* owner)
        : QIODevice(nullptr)
        , m_owner(owner)
    {}

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override {
        return 22050 * 2 + QIODevice::bytesAvailable();
    }

protected:
    qint64 readData(char* data, qint64 maxlen) override {
        if (!m_owner || maxlen < 2) return 0;
        int samples = static_cast<int>(maxlen / 2);
        m_owner->renderMixedAudio(reinterpret_cast<int16_t*>(data), samples, 22050);
        return static_cast<qint64>(samples * 2);
    }
    qint64 writeData(const char*, qint64) override { return 0; }

private:
    Audio::SoundManager* m_owner;
};

static QStringList audioSearchRoots() {
    QStringList candidates;
    auto add = [&](const QString& p) {
        if (!p.isEmpty() && !candidates.contains(p)) candidates.push_back(p);
    };
    add(QDir::currentPath() + "/assets/audio");
    add(QCoreApplication::applicationDirPath() + "/assets/audio");
    add(QCoreApplication::applicationDirPath() + "/../assets/audio");
    add(QCoreApplication::applicationDirPath() + "/../../assets/audio");
    add(QCoreApplication::applicationDirPath() + "/../../../assets/audio");
    const char* src = __FILE__;
    if (src) {
        QDir srcDir = QFileInfo(QString::fromUtf8(src)).absoluteDir();
        add(srcDir.absoluteFilePath("../../assets/audio"));
        add(srcDir.absoluteFilePath("../../../assets/audio"));
    }
    return candidates;
}

static QString resolveAsset(const QString& relPath) {
    if (QFile::exists(relPath)) {
        return QFileInfo(relPath).canonicalFilePath();
    }

    QString fileName = relPath;
    int lastSlash = relPath.lastIndexOf('/');
    if (lastSlash >= 0) {
        fileName = relPath.mid(lastSlash + 1);
    }

    for (const auto& root : audioSearchRoots()) {
        if (!QDir(root).exists()) continue;

        QString exactPath = root + "/" + relPath;
        if (QFile::exists(exactPath)) {
            return QFileInfo(exactPath).canonicalFilePath();
        }

        if (relPath.startsWith("sfx/") || relPath.startsWith("vehicle/") || relPath.startsWith("impacts/") ||
            relPath.startsWith("engine/") || relPath.startsWith("environment/")) {
            QString sfxPath = root + "/sfx/" + fileName;
            if (QFile::exists(sfxPath)) return QFileInfo(sfxPath).canonicalFilePath();
        }
        if (relPath.startsWith("music/")) {
            QString musPath = root + "/music/" + fileName;
            if (QFile::exists(musPath)) return QFileInfo(musPath).canonicalFilePath();
        }

        QStringList subdirs = {"sfx", "music", "vehicle", "impacts", "engine", "environment", ""};
        for (const auto& sub : subdirs) {
            QString cand = sub.isEmpty() ? (root + "/" + fileName) : (root + "/" + sub + "/" + fileName);
            if (QFile::exists(cand)) {
                return QFileInfo(cand).canonicalFilePath();
            }
        }
    }
    return QString();
}

SoundManager::SoundManager(QObject* parent)
    : QObject(parent)
    , m_audioAvailable(true)
    , m_playbackRunning(false)
    , m_masterVolume(1.0f)
    , m_musicVolume(0.75f)
    , m_sfxVolume(0.85f)
    , m_engineRpm(1000.0f)
    , m_engineThrottle(0.0f)
    , m_enginePitch(1.0f)
    , m_engineVolume(0.0f)
    , m_currentVehicleType(Physics::VehicleType::OFFROADER)
    , m_tyreRollVolume(0.0f)
    , m_tyreSkidVolume(0.0f)
    , m_currentBiome("countryside")
    , m_isSkidding(false)
    , m_windVolume(0.0f)
    , m_menuAmbienceVol(0.0f)
    , m_garageAmbienceVol(0.0f)
    , m_raceAmbienceVol(0.0f)
    , m_suspensionVol(0.0f)
    , m_turboSpoolVol(0.0f)
    , m_turboSpoolPitch(1.0f)
    , m_lowFuelChimeVol(0.0f)
    , m_prevVehicleThrottle(0.0f)
    , m_lowFuelTimer(0.0f)
    , m_wasAirborne(false)
    , m_airborneTime(0.0f)
    , m_isAirborne(false)
    , m_lastImpactIntensity(0.0f)
    , m_cooldownImpact(0.0f)
    , m_cooldownLanding(0.0f)
    , m_cooldownJump(0.0f)
    , m_cooldownCrash(0.0f)
    , m_targetMusicTrack(MusicTrack::NONE)
    , m_activeMusicTrack(MusicTrack::NONE)
    , m_musicCurrentVol(0.0f)
    , m_menuMusicVol(0.0f)
    , m_garageMusicVol(0.0f)
    , m_raceMusicVol(0.0f)
    , m_raceDesertVol(0.0f)
    , m_raceMountainVol(0.0f)
    , m_raceMoonVol(0.0f)
    , m_gameOverMusicVol(0.0f)
    , m_coinStreak(0)
    , m_coinStreakTimer(0.0f)
    , m_sndClick(std::make_unique<QSoundEffect>(this))
    , m_sndCoin(std::make_unique<QSoundEffect>(this))
    , m_sndFuel(std::make_unique<QSoundEffect>(this))
    , m_sndStunt(std::make_unique<QSoundEffect>(this))
    , m_sndCrash(std::make_unique<QSoundEffect>(this))
    , m_sndImpactSoft(std::make_unique<QSoundEffect>(this))
    , m_sndImpactHard(std::make_unique<QSoundEffect>(this))
    , m_sndLandingSoft(std::make_unique<QSoundEffect>(this))
    , m_sndLandingHard(std::make_unique<QSoundEffect>(this))
    , m_sndJump(std::make_unique<QSoundEffect>(this))
    , m_sndMenuMusic(std::make_unique<QSoundEffect>(this))
    , m_sndGarageMusic(std::make_unique<QSoundEffect>(this))
    , m_sndRaceMusic(std::make_unique<QSoundEffect>(this))
    , m_sndRaceDesert(std::make_unique<QSoundEffect>(this))
    , m_sndRaceMountain(std::make_unique<QSoundEffect>(this))
    , m_sndRaceMoon(std::make_unique<QSoundEffect>(this))
    , m_sndGameOverMusic(std::make_unique<QSoundEffect>(this))
    , m_sndTyreRoll(std::make_unique<QSoundEffect>(this))
    , m_sndTyreSkidAsphalt(std::make_unique<QSoundEffect>(this))
    , m_sndTyreSkidDirt(std::make_unique<QSoundEffect>(this))
    , m_sndTyreSkidSand(std::make_unique<QSoundEffect>(this))
    , m_sndWind(std::make_unique<QSoundEffect>(this))
    , m_sndMenuAmbience(std::make_unique<QSoundEffect>(this))
    , m_sndGarageAmbience(std::make_unique<QSoundEffect>(this))
    , m_sndRaceAmbience(std::make_unique<QSoundEffect>(this))
    , m_sndSuspension(std::make_unique<QSoundEffect>(this))
    , m_sndTurbo(std::make_unique<QSoundEffect>(this))
    , m_sndBlowOff(std::make_unique<QSoundEffect>(this))
    , m_sndLowFuel(std::make_unique<QSoundEffect>(this))
    , m_synthEnginePhase(0.0f)
    , m_synthWindPhase(0.0f)
    , m_synthSkidPhase(0.0f)
    , m_synthTurboPhase(0.0f)
    , m_synthMenuMusicPos(0)
    , m_synthGarageMusicPos(0)
    , m_synthRaceMusicPos(0)
    , m_synthRaceDesertPos(0)
    , m_synthRaceMountainPos(0)
    , m_synthRaceMoonPos(0)
    , m_synthMenuAmbiencePos(0)
    , m_synthGarageAmbiencePos(0)
    , m_synthRaceAmbiencePos(0)
    , m_menuMusicPlayer(std::make_unique<QMediaPlayer>(this))
    , m_raceMusicPlayer(std::make_unique<QMediaPlayer>(this))
    , m_menuAudioOut(std::make_unique<QAudioOutput>(this))
    , m_raceAudioOut(std::make_unique<QAudioOutput>(this))
{
    m_assetBase = resolveAssetBase();
    for (int i = 0; i < ENGINE_TIER_COUNT; ++i) {
        m_sndEngine[i] = std::make_unique<QSoundEffect>(this);
        m_engineTierVolumes[i] = 0.0f;
    }

    initProceduralSounds();
    startPlaybackEngine();
}

QString SoundManager::resolveAssetBase() const {
    for (const auto& root : audioSearchRoots()) {
        if (QDir(root).exists()) {
            return QDir(root).canonicalPath();
        }
    }
    return QString();
}

bool SoundManager::loadValidatedWav(const QString& path, std::vector<int16_t>& outSamples, uint32_t& outRate) {
    return readWavFile(path, outSamples, outRate);
}

void SoundManager::attachEffect(QSoundEffect* effect, const QString& path, bool loop) {
    if (!effect) return;
    if (!path.isEmpty() && QFile::exists(path)) {
        effect->setSource(QUrl::fromLocalFile(path));
        effect->setLoopCount(loop ? QSoundEffect::Infinite : 1);
        m_loadedAssetCount++;
        connect(effect, &QSoundEffect::statusChanged, this, [effect, path]() {
            if (effect->status() == QSoundEffect::Error) {
                qWarning() << "[Audio::SoundManager] QSoundEffect error on" << path;
            }
        });
    } else {
        m_failedAssetCount++;
    }
}

void SoundManager::queueOneShot(const std::vector<int16_t>* bank, float volumeScale) {
    if (!bank || bank->empty()) return;
    std::lock_guard<std::mutex> lock(m_mixMutex);
    if (m_oneShots.size() < 16) {
        OneShotVoice voice;
        voice.bank = bank;
        voice.pos = 0;
        voice.volume = volumeScale;
        m_oneShots.push_back(voice);
    }
}

void SoundManager::startPlaybackEngine() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const auto outputs = QMediaDevices::audioOutputs();
    if (outputs.isEmpty()) {
        m_audioDeviceName = "None (Headless/Offscreen)";
        m_playbackRunning = false;
        return;
    }
    auto dev = QMediaDevices::defaultAudioOutput();
    if (dev.isNull()) {
        m_audioDeviceName = "None (Null Default)";
        m_playbackRunning = false;
        return;
    }
    m_audioDeviceName = dev.description();

    QAudioFormat format;
    format.setSampleRate(22050);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    if (!dev.isFormatSupported(format)) {
        format = dev.preferredFormat();
    }

    try {
        m_audioSink = std::make_unique<QAudioSink>(dev, format, this);
        m_mixerDevice = std::make_unique<MixerIODevice>(this);
        if (m_mixerDevice->open(QIODevice::ReadOnly)) {
            m_audioSink->start(m_mixerDevice.get());
            m_playbackRunning = (m_audioSink->state() != QAudio::StoppedState);
        }
    } catch (...) {
        m_playbackRunning = false;
    }
#else
    m_audioDeviceName = "Legacy Audio";
    m_playbackRunning = false;
#endif
}

SoundManager::~SoundManager() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (m_audioSink) {
        m_audioSink->stop();
    }
#endif
    if (m_menuMusicPlayer) m_menuMusicPlayer->stop();
    if (m_raceMusicPlayer) m_raceMusicPlayer->stop();
}

void SoundManager::initProceduralSounds() {
    QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (tmpDir.isEmpty()) tmpDir = "/tmp";
    const uint32_t SR = 22050;

    auto makePath = [&](const char* name) -> QString {
        return tmpDir + QString("/hcr_snd_%1.wav").arg(name);
    };

    // Helper PRNG for synthesis fallback
    uint32_t seed = 424242;
    auto rnd = [&]() -> float {
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
        return (static_cast<float>(seed) / 2147483647.0f) * 2.0f - 1.0f;
    };

    // Wire QMediaPlayer & QAudioOutput for high-reliability background music streaming
    m_menuMusicPlayer->setAudioOutput(m_menuAudioOut.get());
    m_menuMusicPlayer->setLoops(QMediaPlayer::Infinite);
    m_raceMusicPlayer->setAudioOutput(m_raceAudioOut.get());
    m_raceMusicPlayer->setLoops(QMediaPlayer::Infinite);

    connect(m_menuMusicPlayer.get(), &QMediaPlayer::errorOccurred, this, [](QMediaPlayer::Error error, const QString& errorString) {
        if (error != QMediaPlayer::NoError) {
            qWarning() << "[Audio::SoundManager] Menu music player error:" << error << errorString;
        }
    });
    connect(m_raceMusicPlayer.get(), &QMediaPlayer::errorOccurred, this, [](QMediaPlayer::Error error, const QString& errorString) {
        if (error != QMediaPlayer::NoError) {
            qWarning() << "[Audio::SoundManager] Race music player error:" << error << errorString;
        }
    });

    // 1. Click Sound
    {
        QString path = resolveAsset("sfx/click.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankClick, fileSR)) {
            attachEffect(m_sndClick.get(), path, false);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("click");
            int count = static_cast<int>(SR * 0.025f);
            m_bankClick.resize(count);
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                float env = std::exp(-t * 140.0f);
                float s = std::sin(2.0f * M_PI * 1250.0f * t) * env;
                m_bankClick[i] = static_cast<int16_t>(s * 22000.0f);
            }
            writeWavFile(fbPath, m_bankClick, SR);
            attachEffect(m_sndClick.get(), fbPath, false);
        }
    }

    // 2. Coin Sound
    {
        QString path = resolveAsset("sfx/coin.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankCoin, fileSR)) {
            attachEffect(m_sndCoin.get(), path, false);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("coin");
            int count = static_cast<int>(SR * 0.12f);
            m_bankCoin.resize(count);
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                float freq = (t < 0.05f) ? 987.77f : 1318.51f;
                float env = (t < 0.05f) ? 1.0f : std::exp(-(t - 0.05f) * 25.0f);
                float s = (std::sin(2.0f * M_PI * freq * t) + 0.25f * std::sin(4.0f * M_PI * freq * t)) * env;
                m_bankCoin[i] = static_cast<int16_t>(s * 24000.0f);
            }
            writeWavFile(fbPath, m_bankCoin, SR);
            attachEffect(m_sndCoin.get(), fbPath, false);
        }
    }

    // 3. Fuel Sound
    {
        QString path = resolveAsset("sfx/fuel.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankFuel, fileSR)) {
            attachEffect(m_sndFuel.get(), path, false);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("fuel");
            int count = static_cast<int>(SR * 0.22f);
            m_bankFuel.resize(count);
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                float freq = 280.0f + 680.0f * (t / 0.22f);
                float env = std::sin((t / 0.22f) * M_PI);
                float s = std::sin(2.0f * M_PI * freq * t) * env;
                m_bankFuel[i] = static_cast<int16_t>(s * 24000.0f);
            }
            writeWavFile(fbPath, m_bankFuel, SR);
            attachEffect(m_sndFuel.get(), fbPath, false);
        }
    }

    // 4. Stunt Fanfare
    {
        QString path = resolveAsset("sfx/stunt.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankStunt, fileSR)) {
            attachEffect(m_sndStunt.get(), path, false);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("stunt");
            int count = static_cast<int>(SR * 0.36f);
            m_bankStunt.resize(count);
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                float freq = (t < 0.09f) ? 523.25f : (t < 0.18f) ? 659.25f : (t < 0.27f) ? 783.99f : 1046.50f;
                float noteT = std::fmod(t, 0.09f);
                float env = std::exp(-noteT * 14.0f);
                float s = (std::sin(2.0f * M_PI * freq * t) + 0.35f * std::sin(4.0f * M_PI * freq * t)) * env;
                m_bankStunt[i] = static_cast<int16_t>(s * 26000.0f);
            }
            writeWavFile(fbPath, m_bankStunt, SR);
            attachEffect(m_sndStunt.get(), fbPath, false);
        }
    }

    // 5. Crash Sound
    {
        QString path = resolveAsset("sfx/crash.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankCrash, fileSR)) {
            attachEffect(m_sndCrash.get(), path, false);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("crash");
            int count = static_cast<int>(SR * 0.50f);
            m_bankCrash.resize(count);
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                float boom = std::sin(2.0f * M_PI * 55.0f * t) * std::exp(-t * 6.0f);
                float crunch = rnd() * std::exp(-t * 12.0f);
                float metal = std::sin(2.0f * M_PI * 420.0f * t) * std::exp(-t * 22.0f);
                float s = boom * 0.75f + crunch * 0.55f + metal * 0.25f;
                m_bankCrash[i] = static_cast<int16_t>(std::clamp(s, -1.0f, 1.0f) * 28000.0f);
            }
            writeWavFile(fbPath, m_bankCrash, SR);
            attachEffect(m_sndCrash.get(), fbPath, false);
        }
    }

    // 6. Soft Impact
    {
        QString path = resolveAsset("sfx/impact_soft.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankImpactSoft, fileSR)) {
            attachEffect(m_sndImpactSoft.get(), path, false);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("impact_soft");
            int count = static_cast<int>(SR * 0.16f);
            m_bankImpactSoft.resize(count);
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                float bump = std::sin(2.0f * M_PI * 88.0f * t) * std::exp(-t * 24.0f);
                float tap = rnd() * 0.3f * std::exp(-t * 40.0f);
                m_bankImpactSoft[i] = static_cast<int16_t>((bump * 0.85f + tap) * 26000.0f);
            }
            writeWavFile(fbPath, m_bankImpactSoft, SR);
            attachEffect(m_sndImpactSoft.get(), fbPath, false);
        }
    }

    // 7. Hard Impact
    {
        QString path = resolveAsset("sfx/impact_hard.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankImpactHard, fileSR)) {
            attachEffect(m_sndImpactHard.get(), path, false);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("impact_hard");
            int count = static_cast<int>(SR * 0.26f);
            m_bankImpactHard.resize(count);
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                float clang1 = std::sin(2.0f * M_PI * 195.0f * t) * std::exp(-t * 15.0f);
                float clang2 = std::sin(2.0f * M_PI * 480.0f * t) * std::exp(-t * 22.0f);
                float grit = rnd() * 0.45f * std::exp(-t * 18.0f);
                float s = clang1 * 0.55f + clang2 * 0.35f + grit;
                m_bankImpactHard[i] = static_cast<int16_t>(std::clamp(s, -1.0f, 1.0f) * 27000.0f);
            }
            writeWavFile(fbPath, m_bankImpactHard, SR);
            attachEffect(m_sndImpactHard.get(), fbPath, false);
        }
    }

    // 8. Landing Soft
    {
        QString path = resolveAsset("sfx/landing_soft.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankLandingSoft, fileSR)) {
            attachEffect(m_sndLandingSoft.get(), path, false);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("landing_soft");
            int count = static_cast<int>(SR * 0.14f);
            m_bankLandingSoft.resize(count);
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                float thump = std::sin(2.0f * M_PI * 85.0f * t) * std::exp(-t * 26.0f);
                float chirpFreq = 620.0f - 380.0f * (t / 0.14f);
                float chirp = std::sin(2.0f * M_PI * chirpFreq * t) * 0.25f * std::exp(-t * 30.0f);
                m_bankLandingSoft[i] = static_cast<int16_t>((thump * 0.85f + chirp) * 25000.0f);
            }
            writeWavFile(fbPath, m_bankLandingSoft, SR);
            attachEffect(m_sndLandingSoft.get(), fbPath, false);
        }
    }

    // 9. Landing Hard
    {
        QString path = resolveAsset("sfx/landing_hard.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankLandingHard, fileSR)) {
            attachEffect(m_sndLandingHard.get(), path, false);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("landing_hard");
            int count = static_cast<int>(SR * 0.28f);
            m_bankLandingHard.resize(count);
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                float boom = std::sin(2.0f * M_PI * 70.0f * t) * std::exp(-t * 14.0f);
                float slap = std::sin(2.0f * M_PI * 340.0f * t) * std::exp(-t * 28.0f);
                float grit = rnd() * 0.35f * std::exp(-t * 20.0f);
                float s = boom * 0.65f + slap * 0.40f + grit;
                m_bankLandingHard[i] = static_cast<int16_t>(std::clamp(s, -1.0f, 1.0f) * 28000.0f);
            }
            writeWavFile(fbPath, m_bankLandingHard, SR);
            attachEffect(m_sndLandingHard.get(), fbPath, false);
        }
    }

    // 10. Jump Takeoff
    {
        QString path = resolveAsset("sfx/jump.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankJump, fileSR)) {
            attachEffect(m_sndJump.get(), path, false);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("jump");
            int count = static_cast<int>(SR * 0.16f);
            m_bankJump.resize(count);
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                float freq = 240.0f - 120.0f * (t / 0.16f);
                float pop = std::sin(2.0f * M_PI * freq * t) * std::exp(-t * 18.0f);
                float puff = rnd() * 0.25f * std::exp(-t * 24.0f);
                m_bankJump[i] = static_cast<int16_t>((pop * 0.75f + puff) * 23000.0f);
            }
            writeWavFile(fbPath, m_bankJump, SR);
            attachEffect(m_sndJump.get(), fbPath, false);
        }
    }

    // 11. Tyre Rolling Loop
    {
        QString path = resolveAsset("sfx/tyre_roll.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankTyreRoll, fileSR)) {
            attachEffect(m_sndTyreRoll.get(), path, true);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("tyre_roll");
            int count = static_cast<int>(SR * 1.2f);
            m_bankTyreRoll.resize(count);
            float lp = 0.0f;
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                float r = rnd();
                lp += (r - lp) * 0.06f;
                float hum = std::sin(2.0f * M_PI * 55.0f * t) * 0.35f;
                float s = lp * 0.85f + hum;
                m_bankTyreRoll[i] = static_cast<int16_t>(std::clamp(s, -1.0f, 1.0f) * 22000.0f);
            }
            writeWavFile(fbPath, m_bankTyreRoll, SR);
            attachEffect(m_sndTyreRoll.get(), fbPath, true);
        }
    }

    // 12. Tyre Skid Loop
    {
        QString path = resolveAsset("sfx/skid.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankTyreSkidAsphalt, fileSR)) {
            m_bankTyreSkidDirt = m_bankTyreSkidAsphalt;
            m_bankTyreSkidSand = m_bankTyreSkidAsphalt;
            attachEffect(m_sndTyreSkidAsphalt.get(), path, true);
            attachEffect(m_sndTyreSkidDirt.get(), path, true);
            attachEffect(m_sndTyreSkidSand.get(), path, true);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("skid_asphalt");
            int count = static_cast<int>(SR * 1.5f);
            m_bankTyreSkidAsphalt.resize(count);
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                float f1 = 2200.0f + 140.0f * std::sin(2.0f * M_PI * 18.0f * t);
                float f2 = 2950.0f + 180.0f * std::cos(2.0f * M_PI * 23.0f * t);
                float s1 = std::sin(2.0f * M_PI * f1 * t);
                float s2 = std::sin(2.0f * M_PI * f2 * t);
                float noise = rnd() * 0.40f;
                float s = (s1 * 0.45f + s2 * 0.35f + noise) * 0.85f;
                m_bankTyreSkidAsphalt[i] = static_cast<int16_t>(std::clamp(s, -1.0f, 1.0f) * 24000.0f);
            }
            writeWavFile(fbPath, m_bankTyreSkidAsphalt, SR);
            attachEffect(m_sndTyreSkidAsphalt.get(), fbPath, true);
            attachEffect(m_sndTyreSkidDirt.get(), fbPath, true);
            attachEffect(m_sndTyreSkidSand.get(), fbPath, true);
        }
    }

    // 13. Environmental Wind Loop
    {
        QString path = resolveAsset("sfx/wind_loop.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankWind, fileSR)) {
            attachEffect(m_sndWind.get(), path, true);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("wind");
            int count = static_cast<int>(SR * 2.0f);
            m_bankWind.resize(count);
            float fLp = 0.0f;
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                float n = rnd();
                float cutoff = 0.08f + 0.04f * std::sin(2.0f * M_PI * 0.8f * t);
                fLp += (n - fLp) * cutoff;
                float s = fLp * 1.2f;
                m_bankWind[i] = static_cast<int16_t>(std::clamp(s, -1.0f, 1.0f) * 20000.0f);
            }
            writeWavFile(fbPath, m_bankWind, SR);
            attachEffect(m_sndWind.get(), fbPath, true);
        }
    }

    // 14. Engine Soundbank Tiers (5 Seamless Loops across RPM range)
    const char* engineAssetNames[ENGINE_TIER_COUNT] = {
        "engine_idle.wav", "engine_tier1.wav", "engine_tier2.wav", "engine_tier3.wav", "engine_tier4.wav"
    };
    const float tierFreqs[ENGINE_TIER_COUNT] = {33.33f, 73.33f, 140.0f, 206.66f, 273.33f};
    const float tierDurs[ENGINE_TIER_COUNT] = {1.20f, 1.00f, 0.85f, 0.75f, 0.70f};

    for (int k = 0; k < ENGINE_TIER_COUNT; ++k) {
        QString path = resolveAsset(QString("sfx/") + engineAssetNames[k]);
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankEngineTiers[k], fileSR)) {
            attachEffect(m_sndEngine[k].get(), path, true);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath(QString("engine_tier%1").arg(k).toLatin1().constData());
            int count = static_cast<int>(SR * tierDurs[k]);
            m_bankEngineTiers[k].resize(count);
            float freq = tierFreqs[k];

            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                float p1 = std::sin(2.0f * M_PI * freq * t);
                float p2 = std::sin(4.0f * M_PI * freq * t) * 0.65f;
                float p3 = std::sin(6.0f * M_PI * freq * t) * 0.35f;
                float sub = std::sin(M_PI * freq * t) * 0.40f;

                float s = (p1 + p2 + p3 + sub);
                s = std::tanh(s * 1.35f);

                if (k == 4) {
                    float stutter = (std::sin(2.0f * M_PI * 26.0f * t) > 0.1f) ? 1.0f : 0.25f;
                    s *= stutter;
                }

                m_bankEngineTiers[k][i] = static_cast<int16_t>(s * 25000.0f);
            }
            writeWavFile(fbPath, m_bankEngineTiers[k], SR);
            attachEffect(m_sndEngine[k].get(), fbPath, true);
        }
    }

    // 15. Menu Music (QMediaPlayer & QSoundEffect fallback)
    {
        QString menuPath = resolveAsset("music/menu_music.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!menuPath.isEmpty() && readWavFile(menuPath, m_bankMenuMusic, fileSR)) {
            attachEffect(m_sndMenuMusic.get(), menuPath, true);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("music_menu");
            int count = static_cast<int>(SR * 7.5f);
            m_bankMenuMusic.resize(count);
            float bpm = 128.0f;
            float beatDur = 60.0f / bpm;
            const float bassNotes[4] = {110.0f, 87.31f, 130.81f, 98.0f};
            const float leadMelody[16] = {
                440.0f, 440.0f, 523.25f, 659.25f, 587.33f, 523.25f, 440.0f, 392.0f,
                440.0f, 523.25f, 659.25f, 783.99f, 659.25f, 587.33f, 523.25f, 440.0f
            };

            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                int bar = static_cast<int>(t / (beatDur * 4.0f)) % 4;
                int beat16 = static_cast<int>(t / (beatDur * 0.25f)) % 16;
                float noteT = std::fmod(t, beatDur * 0.25f);
                float bFreq = bassNotes[bar];
                float bEnv = std::exp(-noteT * 12.0f);
                float bass = (std::sin(2.0f * M_PI * bFreq * t) + 0.3f * std::sin(4.0f * M_PI * bFreq * t)) * bEnv;
                float lFreq = leadMelody[beat16];
                float vib = 1.0f + 0.015f * std::sin(2.0f * M_PI * 6.0f * t);
                float lEnv = std::exp(-noteT * 6.0f);
                float phase = std::fmod(t * lFreq * vib, 1.0f);
                float lead = ((phase < 0.5f) ? 0.35f : -0.35f) * lEnv;
                int beat = static_cast<int>(t / beatDur) % 4;
                float bT = std::fmod(t, beatDur);
                float kick = (beat == 0 || beat == 2) ? std::sin(2.0f * M_PI * (120.0f - 80.0f * (bT / beatDur)) * bT) * std::exp(-bT * 22.0f) : 0.0f;
                float snare = (beat == 1 || beat == 3) ? rnd() * std::exp(-bT * 28.0f) * 0.4f : 0.0f;
                float hihat = rnd() * std::exp(-std::fmod(t, beatDur * 0.5f) * 55.0f) * 0.15f;
                float s = (bass * 0.45f + lead * 0.40f + kick * 0.50f + snare + hihat);
                m_bankMenuMusic[i] = static_cast<int16_t>(std::clamp(s, -1.0f, 1.0f) * 23000.0f);
            }
            writeWavFile(fbPath, m_bankMenuMusic, SR);
            attachEffect(m_sndMenuMusic.get(), fbPath, true);
            menuPath = fbPath;
        }
        if (m_menuMusicPlayer && !menuPath.isEmpty()) {
            m_menuMusicPlayer->setSource(QUrl::fromLocalFile(menuPath));
        }
    }

    // 16. Race Music (QMediaPlayer & QSoundEffect fallback)
    {
        QString racePath = resolveAsset("music/race_music.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!racePath.isEmpty() && readWavFile(racePath, m_bankRaceMusic, fileSR)) {
            attachEffect(m_sndRaceMusic.get(), racePath, true);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("music_race");
            int count = static_cast<int>(SR * 6.8f);
            m_bankRaceMusic.resize(count);
            float bpm = 140.0f;
            float beatDur = 60.0f / bpm;
            const float raceBass[4] = {146.83f, 116.54f, 130.81f, 146.83f};
            const float raceLead[16] = {
                587.33f, 587.33f, 698.46f, 783.99f, 880.00f, 783.99f, 698.46f, 587.33f,
                523.25f, 587.33f, 698.46f, 880.00f, 1046.5f, 880.00f, 698.46f, 587.33f
            };

            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                int bar = static_cast<int>(t / (beatDur * 4.0f)) % 4;
                int beat16 = static_cast<int>(t / (beatDur * 0.25f)) % 16;
                float noteT = std::fmod(t, beatDur * 0.25f);
                float bFreq = raceBass[bar];
                float bEnv = std::exp(-noteT * 14.0f);
                float saw = (2.0f * std::fmod(t * bFreq, 1.0f) - 1.0f) * 0.40f * bEnv;
                float lFreq = raceLead[beat16];
                float lEnv = std::exp(-noteT * 8.0f);
                float leadPh = std::fmod(t * lFreq, 1.0f);
                float lead = ((leadPh < 0.4f) ? 0.40f : -0.40f) * lEnv;
                float bT = std::fmod(t, beatDur);
                float kick = std::sin(2.0f * M_PI * (140.0f - 90.0f * (bT / beatDur)) * bT) * std::exp(-bT * 20.0f) * 0.6f;
                int beat = static_cast<int>(t / beatDur) % 4;
                float snare = (beat == 1 || beat == 3) ? rnd() * std::exp(-bT * 30.0f) * 0.5f : 0.0f;
                float hat = rnd() * std::exp(-std::fmod(t, beatDur * 0.25f) * 60.0f) * 0.18f;
                float s = (saw * 0.45f + lead * 0.45f + kick + snare + hat);
                m_bankRaceMusic[i] = static_cast<int16_t>(std::clamp(s, -1.0f, 1.0f) * 24000.0f);
            }
            writeWavFile(fbPath, m_bankRaceMusic, SR);
            attachEffect(m_sndRaceMusic.get(), fbPath, true);
            racePath = fbPath;
        }
        if (m_raceMusicPlayer && !racePath.isEmpty()) {
            m_raceMusicPlayer->setSource(QUrl::fromLocalFile(racePath));
        }
    }

    // 16b. Garage Music ("Post-Adventure Tea Party" - CC0 Zane Little Music)
    {
        QString path = resolveAsset("music/garage_music.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankGarageMusic, fileSR)) {
            attachEffect(m_sndGarageMusic.get(), path, true);
            loaded = true;
        }
        if (!loaded) {
            m_bankGarageMusic = m_bankMenuMusic;
            attachEffect(m_sndGarageMusic.get(), path, true);
        }
    }

    // 16c. Desert Race Music ("Dizzy Racing" - CC0 Zane Little Music)
    {
        QString path = resolveAsset("music/race_desert.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankRaceDesert, fileSR)) {
            attachEffect(m_sndRaceDesert.get(), path, true);
            loaded = true;
        }
        if (!loaded) {
            m_bankRaceDesert = m_bankRaceMusic;
            attachEffect(m_sndRaceDesert.get(), path, true);
        }
    }

    // 16d. Mountain & Volcano Race Music ("Chip Bit Danger" - CC0 Sudocolon)
    {
        QString path = resolveAsset("music/race_mountain.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankRaceMountain, fileSR)) {
            attachEffect(m_sndRaceMountain.get(), path, true);
            loaded = true;
        }
        if (!loaded) {
            m_bankRaceMountain = m_bankRaceMusic;
            attachEffect(m_sndRaceMountain.get(), path, true);
        }
    }

    // 16e. Moon Race Music ("Space Cadet Training Montage" - CC0 Zane Little Music)
    {
        QString path = resolveAsset("music/race_moon.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankRaceMoon, fileSR)) {
            attachEffect(m_sndRaceMoon.get(), path, true);
            loaded = true;
        }
        if (!loaded) {
            m_bankRaceMoon = m_bankRaceMusic;
            attachEffect(m_sndRaceMoon.get(), path, true);
        }
    }

    // 16f. Game Over Jingle ("Icy Game Over" - CC0 Sudocolon)
    {
        QString path = resolveAsset("music/game_over.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankGameOverMusic, fileSR)) {
            attachEffect(m_sndGameOverMusic.get(), path, false);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("game_over_jingle");
            int count = static_cast<int>(SR * 2.0f);
            m_bankGameOverMusic.resize(count);
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                float f = (t < 0.5f) ? 330.0f : (t < 1.0f ? 293.66f : 261.63f);
                float env = std::exp(-std::fmod(t, 0.5f) * 6.0f);
                float s = std::sin(2.0f * M_PI * f * t) * env * 0.5f;
                m_bankGameOverMusic[i] = static_cast<int16_t>(s * 20000.0f);
            }
            writeWavFile(fbPath, m_bankGameOverMusic, SR);
            attachEffect(m_sndGameOverMusic.get(), fbPath, false);
        }
    }

    // 17. Menu Atmosphere Ambience (Calm Outdoor Breeze, Ambient Pad & Gentle Melodic Birdsong)
    {
        QString path = resolveAsset("environment/menu_ambience.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankMenuAmbience, fileSR)) {
            attachEffect(m_sndMenuAmbience.get(), path, true);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("ambience_menu");
            int count = static_cast<int>(SR * 6.0f);
            m_bankMenuAmbience.resize(count);

            float lpState = 0.0f;
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                // Soft warm breeze: filtered noise with slow breath modulation
                float n = rnd();
                float cutoff = 0.025f + 0.012f * std::sin(2.0f * M_PI * 0.15f * t);
                lpState += (n - lpState) * cutoff;
                float breeze = lpState * 1.4f;

                // Calming ambient pad (A major / C# minor harmonics: 220Hz, 330Hz, 440Hz, 554Hz)
                float pad1 = std::sin(2.0f * M_PI * 220.0f * t) * (0.05f + 0.03f * std::sin(2.0f * M_PI * 0.22f * t));
                float pad2 = std::sin(2.0f * M_PI * 329.6f * t) * (0.04f + 0.02f * std::sin(2.0f * M_PI * 0.31f * t + 1.0f));
                float pad3 = std::sin(2.0f * M_PI * 440.0f * t) * (0.03f + 0.015f * std::sin(2.0f * M_PI * 0.18f * t + 2.0f));
                float pad4 = std::sin(2.0f * M_PI * 554.4f * t) * 0.02f;
                float pad = pad1 + pad2 + pad3 + pad4;

                // Gentle distant birdsong at t=1.4s and t=4.2s (soft sine chirps, no harsh frequencies)
                float bird = 0.0f;
                if (t >= 1.40f && t < 1.62f) {
                    float bT = t - 1.40f;
                    float freq = (bT < 0.10f) ? 2350.0f : 2800.0f;
                    float env = std::sin(static_cast<float>(M_PI) * (bT / 0.22f));
                    bird += std::sin(2.0f * M_PI * freq * t) * env * 0.12f;
                }
                if (t >= 4.20f && t < 4.45f) {
                    float bT = t - 4.20f;
                    float freq = (bT < 0.08f) ? 2200.0f : (bT < 0.16f ? 2600.0f : 3100.0f);
                    float env = std::sin(static_cast<float>(M_PI) * (bT / 0.25f));
                    bird += std::sin(2.0f * M_PI * freq * t) * env * 0.10f;
                }

                float s = breeze * 0.40f + pad * 0.45f + bird * 0.35f;

                // Smooth cosine loop boundary windowing (first & last 2048 samples)
                if (i < 2048) {
                    float win = 0.5f * (1.0f - std::cos(static_cast<float>(M_PI) * i / 2048.0f));
                    s *= win;
                } else if (i > count - 2048) {
                    float win = 0.5f * (1.0f - std::cos(static_cast<float>(M_PI) * (count - 1 - i) / 2048.0f));
                    s *= win;
                }

                m_bankMenuAmbience[i] = static_cast<int16_t>(std::clamp(s, -1.0f, 1.0f) * 20000.0f);
            }
            writeWavFile(fbPath, m_bankMenuAmbience, SR);
            attachEffect(m_sndMenuAmbience.get(), fbPath, true);
        }
    }

    // 18. Garage Atmosphere Ambience (Warm Room Tone, Soft Draft & Gentle Tool Clicks)
    {
        QString path = resolveAsset("environment/garage_ambience.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankGarageAmbience, fileSR)) {
            attachEffect(m_sndGarageAmbience.get(), path, true);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("ambience_garage");
            int count = static_cast<int>(SR * 5.0f);
            m_bankGarageAmbience.resize(count);

            float lpDraft = 0.0f;
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                // Warm indoor room hum (60Hz + 120Hz pure sine, warm acoustic warmth)
                float hum = std::sin(2.0f * M_PI * 60.0f * t) * 0.08f + std::sin(2.0f * M_PI * 120.0f * t) * 0.035f;

                // Ventilation air draft
                float n = rnd();
                lpDraft += (n - lpDraft) * 0.018f;
                float draft = lpDraft * 1.2f;

                // Distant subtle mechanic workshop ratchet click at t=2.0s
                float tool = 0.0f;
                if (t >= 2.0f && t < 2.08f) {
                    float dtClick = t - 2.0f;
                    int subPulse = static_cast<int>(dtClick / 0.016f);
                    float pT = std::fmod(dtClick, 0.016f);
                    if (subPulse < 4) {
                        tool = std::sin(2.0f * M_PI * 850.0f * pT) * std::exp(-pT * 120.0f) * 0.08f;
                    }
                }
                // Gentle soft wrench tap at t=3.8s
                if (t >= 3.80f && t < 3.88f) {
                    float dtTap = t - 3.80f;
                    tool = std::sin(2.0f * M_PI * 1175.0f * dtTap) * std::exp(-dtTap * 60.0f) * 0.09f;
                }

                float s = hum * 0.45f + draft * 0.35f + tool * 0.40f;

                if (i < 2048) {
                    float win = 0.5f * (1.0f - std::cos(static_cast<float>(M_PI) * i / 2048.0f));
                    s *= win;
                } else if (i > count - 2048) {
                    float win = 0.5f * (1.0f - std::cos(static_cast<float>(M_PI) * (count - 1 - i) / 2048.0f));
                    s *= win;
                }

                m_bankGarageAmbience[i] = static_cast<int16_t>(std::clamp(s, -1.0f, 1.0f) * 20000.0f);
            }
            writeWavFile(fbPath, m_bankGarageAmbience, SR);
            attachEffect(m_sndGarageAmbience.get(), fbPath, true);
        }
    }

    // 19. Racing Biome Ambience (Dynamic Outdoor Track Wind & Terrain Atmosphere)
    {
        QString path = resolveAsset("environment/race_ambience.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankRaceAmbience, fileSR)) {
            attachEffect(m_sndRaceAmbience.get(), path, true);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("ambience_race");
            int count = static_cast<int>(SR * 6.0f);
            m_bankRaceAmbience.resize(count);

            float lpAir = 0.0f;
            float bpFoliage = 0.0f;
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                float n = rnd();
                // Rolling outdoor wind with smooth gust swell
                float cutoff = 0.035f + 0.015f * std::sin(2.0f * M_PI * 0.18f * t);
                lpAir += (n - lpAir) * cutoff;
                float windGust = lpAir * 1.5f;

                // Subtle foliage rustle (bandpass around 1200Hz)
                float bpIn = n - lpAir;
                bpFoliage += (bpIn - bpFoliage) * 0.12f;
                float rustle = bpFoliage * 0.06f;

                // Sub-bass earth presence (48Hz)
                float sub = std::sin(2.0f * M_PI * 48.0f * t) * 0.05f;

                float s = windGust * 0.50f + rustle * 0.35f + sub * 0.30f;

                if (i < 2048) {
                    float win = 0.5f * (1.0f - std::cos(static_cast<float>(M_PI) * i / 2048.0f));
                    s *= win;
                } else if (i > count - 2048) {
                    float win = 0.5f * (1.0f - std::cos(static_cast<float>(M_PI) * (count - 1 - i) / 2048.0f));
                    s *= win;
                }

                m_bankRaceAmbience[i] = static_cast<int16_t>(std::clamp(s, -1.0f, 1.0f) * 20000.0f);
            }
            writeWavFile(fbPath, m_bankRaceAmbience, SR);
            attachEffect(m_sndRaceAmbience.get(), fbPath, true);
        }
    }

    // 20. Suspension Hydraulic Damper Breath
    {
        QString path = resolveAsset("vehicle/suspension.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankSuspension, fileSR)) {
            attachEffect(m_sndSuspension.get(), path, true);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("suspension");
            int count = static_cast<int>(SR * 0.40f);
            m_bankSuspension.resize(count);
            float bpState = 0.0f;
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                float n = rnd();
                // Sweep center frequency downward from 850Hz to 320Hz
                float sweepCoeff = 0.18f - 0.12f * (t / 0.40f);
                bpState += (n - bpState) * sweepCoeff;
                // Attack over 0.03s, exponential decay over rest
                float env = (t < 0.03f) ? (t / 0.03f) : std::exp(-(t - 0.03f) * 11.0f);
                float s = bpState * env * 1.5f;
                m_bankSuspension[i] = static_cast<int16_t>(std::clamp(s, -1.0f, 1.0f) * 18000.0f);
            }
            writeWavFile(fbPath, m_bankSuspension, SR);
            attachEffect(m_sndSuspension.get(), fbPath, true);
        }
    }

    // 21. Turbocharger Whine & Spool
    {
        QString path = resolveAsset("vehicle/turbo.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankTurbo, fileSR)) {
            attachEffect(m_sndTurbo.get(), path, true);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("turbo");
            int count = static_cast<int>(SR * 0.80f);
            m_bankTurbo.resize(count);
            float freq = 2400.0f;
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                float s1 = std::sin(2.0f * M_PI * freq * t);
                float s2 = std::sin(2.0f * M_PI * freq * 2.0f * t) * 0.20f;
                float hiss = rnd() * 0.08f;
                float s = (s1 + s2 + hiss) * 0.40f;
                if (i < 1024) {
                    s *= (0.5f * (1.0f - std::cos(static_cast<float>(M_PI) * i / 1024.0f)));
                } else if (i > count - 1024) {
                    s *= (0.5f * (1.0f - std::cos(static_cast<float>(M_PI) * (count - 1 - i) / 1024.0f)));
                }
                m_bankTurbo[i] = static_cast<int16_t>(std::clamp(s, -1.0f, 1.0f) * 18000.0f);
            }
            writeWavFile(fbPath, m_bankTurbo, SR);
            attachEffect(m_sndTurbo.get(), fbPath, true);
        }
    }

    // 22. Blow-Off Valve (BOV) Flutter
    {
        QString path = resolveAsset("vehicle/blowoff.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankBlowOff, fileSR)) {
            attachEffect(m_sndBlowOff.get(), path, false);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("blowoff");
            int count = static_cast<int>(SR * 0.35f);
            m_bankBlowOff.resize(count);
            float bpN = 0.0f;
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                float n = rnd();
                bpN += (n - bpN) * 0.25f;
                // 16Hz flutter modulation: "whoo-t-t-t-t"
                float flutter = 0.5f + 0.5f * std::cos(2.0f * M_PI * 16.0f * t);
                float env = (t < 0.02f) ? (t / 0.02f) : std::exp(-(t - 0.02f) * 12.0f);
                float carrier = std::sin(2.0f * M_PI * 1350.0f * t) * 0.35f + bpN * 0.65f;
                float s = carrier * flutter * env;
                m_bankBlowOff[i] = static_cast<int16_t>(std::clamp(s, -1.0f, 1.0f) * 22000.0f);
            }
            writeWavFile(fbPath, m_bankBlowOff, SR);
            attachEffect(m_sndBlowOff.get(), fbPath, false);
        }
    }

    // 23. Low Fuel Gentle Warning Chime
    {
        QString path = resolveAsset("vehicle/low_fuel.wav");
        uint32_t fileSR = 22050;
        bool loaded = false;
        if (!path.isEmpty() && readWavFile(path, m_bankLowFuel, fileSR)) {
            attachEffect(m_sndLowFuel.get(), path, false);
            loaded = true;
        }
        if (!loaded) {
            QString fbPath = makePath("low_fuel");
            int count = static_cast<int>(SR * 0.28f);
            m_bankLowFuel.resize(count);
            for (int i = 0; i < count; ++i) {
                float t = static_cast<float>(i) / SR;
                // C5 (523.25 Hz) + C6 (1046.5 Hz) soft warm marimba chime
                float tone1 = std::sin(2.0f * M_PI * 523.25f * t);
                float tone2 = std::sin(2.0f * M_PI * 1046.5f * t) * 0.22f;
                float env = (t < 0.005f) ? (t / 0.005f) : std::exp(-(t - 0.005f) * 15.0f);
                float s = (tone1 + tone2) * env * 0.65f;
                m_bankLowFuel[i] = static_cast<int16_t>(std::clamp(s, -1.0f, 1.0f) * 20000.0f);
            }
            writeWavFile(fbPath, m_bankLowFuel, SR);
            attachEffect(m_sndLowFuel.get(), fbPath, false);
        }
    }
}

void SoundManager::playEffectSafe(QSoundEffect* effect, float volumeScale) {
    if (!effect) return;
    float effVol = std::clamp(m_masterVolume * m_sfxVolume * volumeScale, 0.0f, 1.0f);
    effect->setVolume(effVol);
    if (effect->isLoaded()) {
        effect->stop();
        effect->play();
    }
}

void SoundManager::updateLoopSafe(QSoundEffect* effect, float targetVol, bool active) {
    if (!effect) return;
    float effVol = std::clamp(m_masterVolume * targetVol, 0.0f, 1.0f);
    effect->setVolume(effVol);

    if (active && effVol > 0.01f) {
        if (!effect->isPlaying() && effect->isLoaded()) {
            effect->play();
        }
    } else {
        if (effect->isPlaying() && effVol <= 0.005f) {
            effect->stop();
        }
    }
}

void SoundManager::setMasterVolume(float vol) {
    m_masterVolume = std::clamp(vol, 0.0f, 1.0f);
    applyVolumeScales();
}

void SoundManager::setMusicVolume(float vol) {
    m_musicVolume = std::clamp(vol, 0.0f, 1.0f);
    applyVolumeScales();
}

void SoundManager::setSfxVolume(float vol) {
    m_sfxVolume = std::clamp(vol, 0.0f, 1.0f);
    applyVolumeScales();
}

void SoundManager::applyVolumeScales() {
    // Dynamically adjust active loop volumes
    if (m_sndMenuMusic) m_sndMenuMusic->setVolume(m_masterVolume * m_musicVolume * m_menuMusicVol);
    if (m_sndGarageMusic) m_sndGarageMusic->setVolume(m_masterVolume * m_musicVolume * m_garageMusicVol);
    if (m_sndRaceMusic) m_sndRaceMusic->setVolume(m_masterVolume * m_musicVolume * m_raceMusicVol);
    if (m_sndRaceDesert) m_sndRaceDesert->setVolume(m_masterVolume * m_musicVolume * m_raceDesertVol);
    if (m_sndRaceMountain) m_sndRaceMountain->setVolume(m_masterVolume * m_musicVolume * m_raceMountainVol);
    if (m_sndRaceMoon) m_sndRaceMoon->setVolume(m_masterVolume * m_musicVolume * m_raceMoonVol);
    if (m_sndGameOverMusic) m_sndGameOverMusic->setVolume(m_masterVolume * m_musicVolume * m_gameOverMusicVol);

    float effMenuMax = std::max(m_menuMusicVol, m_garageMusicVol);
    if (m_menuAudioOut) m_menuAudioOut->setVolume(std::clamp(m_masterVolume * m_musicVolume * effMenuMax, 0.0f, 1.0f));
    float effRaceMax = std::max({m_raceMusicVol, m_raceDesertVol, m_raceMountainVol, m_raceMoonVol});
    if (m_raceAudioOut) m_raceAudioOut->setVolume(std::clamp(m_masterVolume * m_musicVolume * effRaceMax, 0.0f, 1.0f));

    if (m_sndTyreRoll) m_sndTyreRoll->setVolume(m_masterVolume * m_sfxVolume * m_tyreRollVolume);
    if (m_sndWind) m_sndWind->setVolume(m_masterVolume * m_sfxVolume * m_windVolume);
    if (m_sndTyreSkidAsphalt) m_sndTyreSkidAsphalt->setVolume(m_masterVolume * m_sfxVolume * m_tyreSkidVolume);
    if (m_sndTyreSkidDirt) m_sndTyreSkidDirt->setVolume(m_masterVolume * m_sfxVolume * m_tyreSkidVolume);
    if (m_sndTyreSkidSand) m_sndTyreSkidSand->setVolume(m_masterVolume * m_sfxVolume * m_tyreSkidVolume);

    if (m_sndMenuAmbience) m_sndMenuAmbience->setVolume(m_masterVolume * m_sfxVolume * m_menuAmbienceVol);
    if (m_sndGarageAmbience) m_sndGarageAmbience->setVolume(m_masterVolume * m_sfxVolume * m_garageAmbienceVol);
    if (m_sndRaceAmbience) m_sndRaceAmbience->setVolume(m_masterVolume * m_sfxVolume * m_raceAmbienceVol);
    if (m_sndSuspension) m_sndSuspension->setVolume(m_masterVolume * m_sfxVolume * m_suspensionVol);
    if (m_sndTurbo) m_sndTurbo->setVolume(m_masterVolume * m_sfxVolume * m_turboSpoolVol);

    for (int k = 0; k < ENGINE_TIER_COUNT; ++k) {
        if (m_sndEngine[k]) {
            m_sndEngine[k]->setVolume(m_masterVolume * m_sfxVolume * m_engineTierVolumes[k]);
        }
    }
}

void SoundManager::setTargetMusic(MusicTrack track) {
    m_targetMusicTrack = track;
}

void SoundManager::stopAllMusic() {
    m_targetMusicTrack = MusicTrack::NONE;
    m_activeMusicTrack = MusicTrack::NONE;
    m_musicCurrentVol = 0.0f;
    m_menuMusicVol = 0.0f;
    m_garageMusicVol = 0.0f;
    m_raceMusicVol = 0.0f;
    m_raceDesertVol = 0.0f;
    m_raceMountainVol = 0.0f;
    m_raceMoonVol = 0.0f;
    m_gameOverMusicVol = 0.0f;
    if (m_sndMenuMusic) m_sndMenuMusic->stop();
    if (m_sndGarageMusic) m_sndGarageMusic->stop();
    if (m_sndRaceMusic) m_sndRaceMusic->stop();
    if (m_sndRaceDesert) m_sndRaceDesert->stop();
    if (m_sndRaceMountain) m_sndRaceMountain->stop();
    if (m_sndRaceMoon) m_sndRaceMoon->stop();
    if (m_sndGameOverMusic) m_sndGameOverMusic->stop();
    if (m_menuMusicPlayer) m_menuMusicPlayer->stop();
    if (m_raceMusicPlayer) m_raceMusicPlayer->stop();
}

void SoundManager::playGameOverJingle() {
    playEffectSafe(m_sndGameOverMusic.get(), 0.90f);
    queueOneShot(&m_bankGameOverMusic, 0.90f);
}

void SoundManager::playButtonClick() {
    playEffectSafe(m_sndClick.get(), 0.75f);
    queueOneShot(&m_bankClick, 0.75f);
}

void SoundManager::playCoinPickup() {
    m_coinStreakTimer = 0.40f;
    m_coinStreak++;
    float vol = std::min(1.0f, 0.75f + m_coinStreak * 0.05f);
    playEffectSafe(m_sndCoin.get(), vol);
    queueOneShot(&m_bankCoin, vol);
}

void SoundManager::playFuelPickup() {
    playEffectSafe(m_sndFuel.get(), 0.90f);
    queueOneShot(&m_bankFuel, 0.90f);
}

void SoundManager::playStuntCheer() {
    playEffectSafe(m_sndStunt.get(), 0.95f);
    queueOneShot(&m_bankStunt, 0.95f);
}

void SoundManager::playCrash() {
    if (m_cooldownCrash > 0.0f) return;
    m_cooldownCrash = 0.50f;
    playEffectSafe(m_sndCrash.get(), 1.0f);
    queueOneShot(&m_bankCrash, 1.0f);
}

void SoundManager::playSoftImpact(float intensity) {
    if (m_cooldownImpact > 0.0f) return;
    m_cooldownImpact = 0.08f;
    m_lastImpactIntensity = intensity;
    float vol = std::clamp(intensity * 0.70f, 0.20f, 0.85f);
    playEffectSafe(m_sndImpactSoft.get(), vol);
    queueOneShot(&m_bankImpactSoft, vol);
}

void SoundManager::playHardImpact(float intensity) {
    if (m_cooldownImpact > 0.0f && intensity <= m_lastImpactIntensity) return;
    m_cooldownImpact = 0.10f;
    m_lastImpactIntensity = intensity;
    float vol = std::clamp(intensity * 0.90f, 0.50f, 1.0f);
    playEffectSafe(m_sndImpactHard.get(), vol);
    queueOneShot(&m_bankImpactHard, vol);
}

void SoundManager::playLandingSoft(float intensity) {
    if (m_cooldownLanding > 0.0f) return;
    m_cooldownLanding = 0.12f;
    float vol = std::clamp(intensity * 0.65f, 0.25f, 0.80f);
    playEffectSafe(m_sndLandingSoft.get(), vol);
    queueOneShot(&m_bankLandingSoft, vol);
}

void SoundManager::playLandingHard(float intensity) {
    if (m_cooldownLanding > 0.0f) return;
    m_cooldownLanding = 0.15f;
    float vol = std::clamp(intensity * 0.90f, 0.50f, 1.0f);
    playEffectSafe(m_sndLandingHard.get(), vol);
    queueOneShot(&m_bankLandingHard, vol);
}

void SoundManager::playJumpTakeoff() {
    if (m_cooldownJump > 0.0f) return;
    m_cooldownJump = 0.15f;
    playEffectSafe(m_sndJump.get(), 0.65f);
    queueOneShot(&m_bankJump, 0.65f);
}

void SoundManager::playTurboBlowOff(float intensity) {
    float vol = std::clamp(intensity * 0.75f, 0.20f, 0.85f);
    playEffectSafe(m_sndBlowOff.get(), vol);
    queueOneShot(&m_bankBlowOff, vol);
}

void SoundManager::playLowFuelChime(float intensity) {
    float vol = std::clamp(intensity * 0.60f, 0.20f, 0.70f);
    playEffectSafe(m_sndLowFuel.get(), vol);
    queueOneShot(&m_bankLowFuel, vol);
}

void SoundManager::updateEngineRpm(float rpm, float throttle, Physics::VehicleType type) {
    m_engineRpm = std::clamp(rpm, 1000.0f, 8500.0f);
    m_engineThrottle = std::clamp(throttle, 0.0f, 1.0f);
    m_currentVehicleType = type;

    // Pitch ratio: 0.70 at idle up to 2.50 at redline
    m_enginePitch = 0.70f + 1.80f * ((m_engineRpm - 1000.0f) / 7500.0f);

    // Throttle load adds volume & overdrive
    float loadBoost = (m_engineThrottle > 0.05f) ? (0.20f + 0.25f * m_engineThrottle) : 0.0f;
    m_engineVolume = std::clamp(0.40f + 0.25f * ((m_engineRpm - 1000.0f) / 7500.0f) + loadBoost, 0.0f, 1.0f);

    updateEngineLoops();
}

void SoundManager::updateEngineLoops() {
    // 5 Tiers: 1000, 2200, 4200, 6200, 8200 RPM
    const float tierRpm[ENGINE_TIER_COUNT] = {1000.0f, 2200.0f, 4200.0f, 6200.0f, 8200.0f};

    // Calculate crossfade distribution
    float targetWeights[ENGINE_TIER_COUNT] = {0.0f};

    if (m_engineRpm <= tierRpm[0]) {
        targetWeights[0] = 1.0f;
    } else if (m_engineRpm >= tierRpm[ENGINE_TIER_COUNT - 1]) {
        targetWeights[ENGINE_TIER_COUNT - 1] = 1.0f;
    } else {
        for (int k = 0; k < ENGINE_TIER_COUNT - 1; ++k) {
            if (m_engineRpm >= tierRpm[k] && m_engineRpm <= tierRpm[k + 1]) {
                float frac = (m_engineRpm - tierRpm[k]) / (tierRpm[k + 1] - tierRpm[k]);
                targetWeights[k] = 1.0f - frac;
                targetWeights[k + 1] = frac;
                break;
            }
        }
    }

    // Apply smoothed tier volumes
    for (int k = 0; k < ENGINE_TIER_COUNT; ++k) {
        float desiredVol = targetWeights[k] * m_engineVolume;
        m_engineTierVolumes[k] += (desiredVol - m_engineTierVolumes[k]) * 0.35f;
        updateLoopSafe(m_sndEngine[k].get(), m_sfxVolume * m_engineTierVolumes[k], m_engineTierVolumes[k] > 0.02f);
    }
}

void SoundManager::update(float dt, const Physics::Vehicle& vehicle, const Physics::Terrain& terrain, Core::StateType state) {
    // 1. Update Cooldown Timers
    m_cooldownImpact = std::max(0.0f, m_cooldownImpact - dt);
    m_cooldownLanding = std::max(0.0f, m_cooldownLanding - dt);
    m_cooldownJump = std::max(0.0f, m_cooldownJump - dt);
    m_cooldownCrash = std::max(0.0f, m_cooldownCrash - dt);
    m_coinStreakTimer = std::max(0.0f, m_coinStreakTimer - dt);
    if (m_coinStreakTimer <= 0.0f) m_coinStreak = 0;

    m_currentBiome = terrain.getBiome();

    // 2. Music State Automation & Crossfading
    if (state == Core::StateType::MAIN_MENU || state == Core::StateType::STAGE_SELECT) {
        setTargetMusic(MusicTrack::MENU);
    } else if (state == Core::StateType::GARAGE) {
        setTargetMusic(MusicTrack::GARAGE);
    } else if (state == Core::StateType::GAMEPLAY || state == Core::StateType::PAUSED) {
        MusicTrack desiredRaceTrack = MusicTrack::RACE;
        if (m_currentBiome == "desert") {
            desiredRaceTrack = MusicTrack::RACE_DESERT;
        } else if (m_currentBiome == "mountain" || m_currentBiome == "volcano") {
            desiredRaceTrack = MusicTrack::RACE_MOUNTAIN;
        } else if (m_currentBiome == "moon") {
            desiredRaceTrack = MusicTrack::RACE_MOON;
        }

        if (m_targetMusicTrack != desiredRaceTrack) {
            setTargetMusic(desiredRaceTrack);
        }
    } else if (state == Core::StateType::GAME_OVER) {
        setTargetMusic(MusicTrack::NONE);
    }

    float crossfadeRate = 4.0f * dt;
    float targetMenuVol = (m_targetMusicTrack == MusicTrack::MENU) ? 1.0f : 0.0f;
    float targetGarageVol = (m_targetMusicTrack == MusicTrack::GARAGE) ? 1.0f : 0.0f;
    float raceMult = (state == Core::StateType::PAUSED) ? 0.35f : 1.0f;
    float targetRaceVol = (m_targetMusicTrack == MusicTrack::RACE) ? raceMult : 0.0f;
    float targetDesertVol = (m_targetMusicTrack == MusicTrack::RACE_DESERT) ? raceMult : 0.0f;
    float targetMountainVol = (m_targetMusicTrack == MusicTrack::RACE_MOUNTAIN) ? raceMult : 0.0f;
    float targetMoonVol = (m_targetMusicTrack == MusicTrack::RACE_MOON) ? raceMult : 0.0f;

    m_menuMusicVol += (targetMenuVol - m_menuMusicVol) * std::min(1.0f, crossfadeRate);
    m_garageMusicVol += (targetGarageVol - m_garageMusicVol) * std::min(1.0f, crossfadeRate);
    m_raceMusicVol += (targetRaceVol - m_raceMusicVol) * std::min(1.0f, crossfadeRate);
    m_raceDesertVol += (targetDesertVol - m_raceDesertVol) * std::min(1.0f, crossfadeRate);
    m_raceMountainVol += (targetMountainVol - m_raceMountainVol) * std::min(1.0f, crossfadeRate);
    m_raceMoonVol += (targetMoonVol - m_raceMoonVol) * std::min(1.0f, crossfadeRate);

    // Determine currently dominant active music track
    float maxVol = 0.05f;
    MusicTrack dominant = MusicTrack::NONE;
    if (m_menuMusicVol > maxVol) { maxVol = m_menuMusicVol; dominant = MusicTrack::MENU; }
    if (m_garageMusicVol > maxVol) { maxVol = m_garageMusicVol; dominant = MusicTrack::GARAGE; }
    if (m_raceMusicVol > maxVol) { maxVol = m_raceMusicVol; dominant = MusicTrack::RACE; }
    if (m_raceDesertVol > maxVol) { maxVol = m_raceDesertVol; dominant = MusicTrack::RACE_DESERT; }
    if (m_raceMountainVol > maxVol) { maxVol = m_raceMountainVol; dominant = MusicTrack::RACE_MOUNTAIN; }
    if (m_raceMoonVol > maxVol) { maxVol = m_raceMoonVol; dominant = MusicTrack::RACE_MOON; }
    m_activeMusicTrack = dominant;
    m_musicCurrentVol = (dominant != MusicTrack::NONE) ? maxVol : 0.0f;

    updateLoopSafe(m_sndMenuMusic.get(), m_musicVolume * m_menuMusicVol, m_menuMusicVol > 0.01f);
    updateLoopSafe(m_sndGarageMusic.get(), m_musicVolume * m_garageMusicVol, m_garageMusicVol > 0.01f);
    updateLoopSafe(m_sndRaceMusic.get(), m_musicVolume * m_raceMusicVol, m_raceMusicVol > 0.01f);
    updateLoopSafe(m_sndRaceDesert.get(), m_musicVolume * m_raceDesertVol, m_raceDesertVol > 0.01f);
    updateLoopSafe(m_sndRaceMountain.get(), m_musicVolume * m_raceMountainVol, m_raceMountainVol > 0.01f);
    updateLoopSafe(m_sndRaceMoon.get(), m_musicVolume * m_raceMoonVol, m_raceMoonVol > 0.01f);

    float effMenuVol = std::clamp(m_masterVolume * m_musicVolume * std::max(m_menuMusicVol, m_garageMusicVol), 0.0f, 1.0f);
    float effRaceVol = std::clamp(m_masterVolume * m_musicVolume * std::max({m_raceMusicVol, m_raceDesertVol, m_raceMountainVol, m_raceMoonVol}), 0.0f, 1.0f);

    if (m_menuAudioOut) m_menuAudioOut->setVolume(effMenuVol);
    if (m_raceAudioOut) m_raceAudioOut->setVolume(effRaceVol);

    if (m_menuMusicPlayer) {
        if (effMenuVol > 0.01f) {
            if (m_menuMusicPlayer->playbackState() != QMediaPlayer::PlayingState) {
                m_menuMusicPlayer->play();
            }
        } else if (m_menuMusicPlayer->playbackState() == QMediaPlayer::PlayingState && effMenuVol <= 0.005f) {
            m_menuMusicPlayer->pause();
        }
    }

    if (m_raceMusicPlayer) {
        if (effRaceVol > 0.01f) {
            if (m_raceMusicPlayer->playbackState() != QMediaPlayer::PlayingState) {
                m_raceMusicPlayer->play();
            }
        } else if (m_raceMusicPlayer->playbackState() == QMediaPlayer::PlayingState && effRaceVol <= 0.005f) {
            m_raceMusicPlayer->pause();
        }
    }

    // 2b. State-Aware Atmospheric Ambience Automation & Seamless Crossfading
    float targetMenuAmb = 0.0f;
    float targetGarageAmb = 0.0f;
    float targetRaceAmb = 0.0f;

    if (state == Core::StateType::MAIN_MENU || state == Core::StateType::STAGE_SELECT) {
        targetMenuAmb = 0.35f;
    } else if (state == Core::StateType::GARAGE) {
        targetGarageAmb = 0.35f;
    } else if (state == Core::StateType::GAMEPLAY) {
        targetRaceAmb = 0.28f;
    } else if (state == Core::StateType::PAUSED) {
        targetRaceAmb = 0.12f; // Smoothly ducked when paused
    } else if (state == Core::StateType::GAME_OVER) {
        targetRaceAmb = 0.08f; // Quiet ambient tail during wreck sequence
    }

    // Slew rate 3.5f * dt gives a completely click-free ~0.3s exponential crossfade
    float ambSlew = std::min(1.0f, 3.5f * dt);
    m_menuAmbienceVol += (targetMenuAmb - m_menuAmbienceVol) * ambSlew;
    m_garageAmbienceVol += (targetGarageAmb - m_garageAmbienceVol) * ambSlew;
    m_raceAmbienceVol += (targetRaceAmb - m_raceAmbienceVol) * ambSlew;

    updateLoopSafe(m_sndMenuAmbience.get(), m_sfxVolume * m_menuAmbienceVol, m_menuAmbienceVol > 0.01f);
    updateLoopSafe(m_sndGarageAmbience.get(), m_sfxVolume * m_garageAmbienceVol, m_garageAmbienceVol > 0.01f);
    updateLoopSafe(m_sndRaceAmbience.get(), m_sfxVolume * m_raceAmbienceVol, m_raceAmbienceVol > 0.01f);

    // If not in active gameplay or wreck simulation, silence vehicle audio
    if (state != Core::StateType::GAMEPLAY && state != Core::StateType::GAME_OVER) {
        m_tyreRollVolume = 0.0f;
        m_tyreSkidVolume = 0.0f;
        m_windVolume = 0.0f;
        m_engineVolume = 0.0f;
        m_suspensionVol = 0.0f;
        m_turboSpoolVol = 0.0f;
        m_prevVehicleThrottle = 0.0f;
        m_lowFuelChimeVol = 0.0f;
        updateLoopSafe(m_sndTyreRoll.get(), 0.0f, false);
        updateLoopSafe(m_sndTyreSkidAsphalt.get(), 0.0f, false);
        updateLoopSafe(m_sndTyreSkidDirt.get(), 0.0f, false);
        updateLoopSafe(m_sndTyreSkidSand.get(), 0.0f, false);
        updateLoopSafe(m_sndWind.get(), 0.0f, false);
        updateLoopSafe(m_sndSuspension.get(), 0.0f, false);
        updateLoopSafe(m_sndTurbo.get(), 0.0f, false);
        for (int k = 0; k < ENGINE_TIER_COUNT; ++k) updateLoopSafe(m_sndEngine[k].get(), 0.0f, false);
        return;
    }

    // 3. Dynamic Engine Synthesis
    if (!vehicle.isDriverDown() && vehicle.fuel() > 0.0f) {
        updateEngineRpm(vehicle.getEngineRpm(), vehicle.gasInput(), vehicle.vehicleType());
    } else {
        // Passive coast / cut engine
        updateEngineRpm(1000.0f, 0.0f, vehicle.vehicleType());
        m_engineVolume *= 0.85f;
    }

    // 4. Tyre Rolling Audio
    bool onGround = vehicle.isRearOnGround() || vehicle.isFrontOnGround();
    float speedKmh = vehicle.getSpeedKmh();

    if (onGround) {
        float targetRoll = std::clamp(speedKmh / 50.0f, 0.0f, 1.0f) * 0.55f;
        m_tyreRollVolume += (targetRoll - m_tyreRollVolume) * std::min(1.0f, 12.0f * dt);
    } else {
        m_tyreRollVolume += (0.0f - m_tyreRollVolume) * std::min(1.0f, 18.0f * dt);
    }
    updateLoopSafe(m_sndTyreRoll.get(), m_sfxVolume * m_tyreRollVolume, m_tyreRollVolume > 0.02f);

    // 5. Tyre Skidding & Traction-Loss
    float slip = vehicle.maxSlipSpeed();
    bool hardBrake = (vehicle.brakeInput() > 0.25f && speedKmh > 10.0f);
    m_isSkidding = onGround && (slip > 1.25f || hardBrake);

    float targetSkid = 0.0f;
    if (m_isSkidding) {
        targetSkid = std::clamp((slip - 1.0f) / 3.2f, 0.25f, 1.0f) * 0.80f;
    }
    m_tyreSkidVolume += (targetSkid - m_tyreSkidVolume) * std::min(1.0f, 15.0f * dt);

    // Route to biome material
    bool isRock = (m_currentBiome == "mountain" || m_currentBiome == "volcano");
    bool isSand = (m_currentBiome == "desert" || m_currentBiome == "moon");
    bool isDirt = (!isRock && !isSand);

    updateLoopSafe(m_sndTyreSkidAsphalt.get(), m_sfxVolume * m_tyreSkidVolume, m_isSkidding && isRock);
    updateLoopSafe(m_sndTyreSkidSand.get(), m_sfxVolume * m_tyreSkidVolume, m_isSkidding && isSand);
    updateLoopSafe(m_sndTyreSkidDirt.get(), m_sfxVolume * m_tyreSkidVolume, m_isSkidding && isDirt);

    // 6. Environmental Wind Rush (Silent in lunar vacuum)
    m_isAirborne = vehicle.isAirborne();
    float targetWind = (m_currentBiome == "moon") ? 0.0f :
                       (std::clamp((speedKmh - 25.0f) / 75.0f, 0.0f, 1.0f) * (m_isAirborne ? 0.65f : 0.35f));
    m_windVolume += (targetWind - m_windVolume) * std::min(1.0f, 8.0f * dt);
    updateLoopSafe(m_sndWind.get(), m_sfxVolume * m_windVolume, m_windVolume > 0.02f);

    // 6b. Suspension Damper Hydraulic Breathing Audio (Smooth, organic bump response)
    float suspMotion = std::abs(vehicle.rearSuspVel()) + std::abs(vehicle.frontSuspVel());
    float targetSusp = 0.0f;
    if (onGround && suspMotion > 0.35f) {
        targetSusp = std::clamp((suspMotion - 0.35f) / 3.0f, 0.0f, 1.0f) * 0.35f;
    }
    float suspSlew = (targetSusp > m_suspensionVol) ? std::min(1.0f, 18.0f * dt) : std::min(1.0f, 9.0f * dt);
    m_suspensionVol += (targetSusp - m_suspensionVol) * suspSlew;
    updateLoopSafe(m_sndSuspension.get(), m_sfxVolume * m_suspensionVol, m_suspensionVol > 0.02f);

    // 6c. Turbocharger Whine & Blow-Off Flutter
    float curThrottle = vehicle.gasInput();
    float curRpm = vehicle.getEngineRpm();
    float targetTurbo = 0.0f;
    if (curThrottle > 0.30f && curRpm > 3200.0f && onGround) {
        float rpmRatio = std::clamp((curRpm - 3200.0f) / 4800.0f, 0.0f, 1.0f);
        targetTurbo = rpmRatio * curThrottle * 0.26f;
        m_turboSpoolPitch = 1.0f + 1.25f * rpmRatio;
    }
    m_turboSpoolVol += (targetTurbo - m_turboSpoolVol) * std::min(1.0f, 7.0f * dt);
    updateLoopSafe(m_sndTurbo.get(), m_sfxVolume * m_turboSpoolVol, m_turboSpoolVol > 0.02f);

    if (m_prevVehicleThrottle > 0.65f && curThrottle < 0.15f && m_turboSpoolVol > 0.10f) {
        playTurboBlowOff(std::clamp(m_turboSpoolVol * 2.8f, 0.35f, 0.85f));
    }
    m_prevVehicleThrottle = curThrottle;

    // 6d. Low Fuel Warning Chime (pulsing alert when fuel drops below 20%)
    float fuelPct = vehicle.fuel();
    if (fuelPct > 0.0f && fuelPct < 20.0f && !vehicle.isDriverDown()) {
        m_lowFuelTimer += dt;
        float interval = (fuelPct < 8.0f) ? 0.90f : 1.60f;
        if (m_lowFuelTimer >= interval) {
            m_lowFuelTimer = 0.0f;
            float intensity = std::clamp((20.0f - fuelPct) / 20.0f, 0.35f, 0.85f);
            playLowFuelChime(intensity);
        }
        m_lowFuelChimeVol = std::clamp((20.0f - fuelPct) / 20.0f, 0.0f, 1.0f);
    } else {
        m_lowFuelTimer = 0.0f;
        m_lowFuelChimeVol = 0.0f;
    }

    // 7. Jump Liftoff & Airborne Unweighting
    if (!m_wasAirborne && m_isAirborne) {
        // Transition to airborne
        if (speedKmh > 15.0f) {
            playJumpTakeoff();
        }
    }
    if (m_isAirborne) {
        m_airborneTime += dt;
    }

    // 8. Landing & Suspension Bottom-Out
    if (m_wasAirborne && !m_isAirborne && m_airborneTime > 0.12f) {
        float vy = -vehicle.chassisVel().y;
        float suspV = -std::min(vehicle.rearSuspVel(), vehicle.frontSuspVel());
        float intensity = std::max(vy / 4.0f, suspV / 3.0f);

        if (vy > 4.5f || suspV > 3.0f) {
            playLandingHard(intensity);
        } else if (vy > 0.8f) {
            playLandingSoft(intensity);
        }
        m_airborneTime = 0.0f;
    }
    m_wasAirborne = m_isAirborne;

    // 9. Collision & Impact Evaluation
    float impactV = vehicle.lastImpactVelocity();
    if (impactV > 0.8f) {
        if (impactV >= 7.0f || vehicle.isDriverDown()) {
            playCrash();
        } else if (impactV >= 2.5f) {
            playHardImpact(impactV / 5.5f);
        } else {
            playSoftImpact(impactV / 2.5f);
        }
    }
}

void SoundManager::renderMixedAudio(int16_t* buffer, int numSamples, int sampleRate) {
    if (!buffer || numSamples <= 0) return;

    std::lock_guard<std::mutex> lock(m_mixMutex);
    for (int i = 0; i < numSamples; ++i) {
        float sum = 0.0f;

        // A. Music Subsystem Mixing (Supports all background music tracks)
        if (m_menuMusicVol > 0.005f && !m_bankMenuMusic.empty()) {
            float s = static_cast<float>(m_bankMenuMusic[m_synthMenuMusicPos]) / 32767.0f;
            sum += s * m_musicVolume * m_menuMusicVol * 0.70f;
            m_synthMenuMusicPos = (m_synthMenuMusicPos + 1) % m_bankMenuMusic.size();
        }
        if (m_garageMusicVol > 0.005f && !m_bankGarageMusic.empty()) {
            float s = static_cast<float>(m_bankGarageMusic[m_synthGarageMusicPos]) / 32767.0f;
            sum += s * m_musicVolume * m_garageMusicVol * 0.70f;
            m_synthGarageMusicPos = (m_synthGarageMusicPos + 1) % m_bankGarageMusic.size();
        }
        if (m_raceMusicVol > 0.005f && !m_bankRaceMusic.empty()) {
            float s = static_cast<float>(m_bankRaceMusic[m_synthRaceMusicPos]) / 32767.0f;
            sum += s * m_musicVolume * m_raceMusicVol * 0.70f;
            m_synthRaceMusicPos = (m_synthRaceMusicPos + 1) % m_bankRaceMusic.size();
        }
        if (m_raceDesertVol > 0.005f && !m_bankRaceDesert.empty()) {
            float s = static_cast<float>(m_bankRaceDesert[m_synthRaceDesertPos]) / 32767.0f;
            sum += s * m_musicVolume * m_raceDesertVol * 0.70f;
            m_synthRaceDesertPos = (m_synthRaceDesertPos + 1) % m_bankRaceDesert.size();
        }
        if (m_raceMountainVol > 0.005f && !m_bankRaceMountain.empty()) {
            float s = static_cast<float>(m_bankRaceMountain[m_synthRaceMountainPos]) / 32767.0f;
            sum += s * m_musicVolume * m_raceMountainVol * 0.70f;
            m_synthRaceMountainPos = (m_synthRaceMountainPos + 1) % m_bankRaceMountain.size();
        }
        if (m_raceMoonVol > 0.005f && !m_bankRaceMoon.empty()) {
            float s = static_cast<float>(m_bankRaceMoon[m_synthRaceMoonPos]) / 32767.0f;
            sum += s * m_musicVolume * m_raceMoonVol * 0.70f;
            m_synthRaceMoonPos = (m_synthRaceMoonPos + 1) % m_bankRaceMoon.size();
        }

        // B. Continuous Engine Synthesis
        if (m_engineVolume > 0.01f) {
            float freq = 33.33f * m_enginePitch;
            m_synthEnginePhase += (freq / static_cast<float>(sampleRate));
            if (m_synthEnginePhase > 1.0f) m_synthEnginePhase -= 1.0f;

            float p1 = std::sin(2.0f * M_PI * m_synthEnginePhase);
            float p2 = std::sin(4.0f * M_PI * m_synthEnginePhase) * 0.6f;
            float engRaw = (p1 + p2);
            float engShaped = std::tanh(engRaw * (1.2f + 0.6f * m_engineThrottle));
            sum += engShaped * m_sfxVolume * m_engineVolume * 0.45f;
        }

        // C. Tyre Rolling Rumble
        if (m_tyreRollVolume > 0.01f && !m_bankTyreRoll.empty()) {
            int idx = (i * 2) % m_bankTyreRoll.size();
            float s = static_cast<float>(m_bankTyreRoll[idx]) / 32767.0f;
            sum += s * m_sfxVolume * m_tyreRollVolume * 0.40f;
        }

        // D. Tyre Skidding Screech
        if (m_tyreSkidVolume > 0.01f) {
            m_synthSkidPhase += (2400.0f / static_cast<float>(sampleRate));
            if (m_synthSkidPhase > 1.0f) m_synthSkidPhase -= 1.0f;
            float skidS = std::sin(2.0f * M_PI * m_synthSkidPhase);
            sum += skidS * m_sfxVolume * m_tyreSkidVolume * 0.50f;
        }

        // E. Wind Rush
        if (m_windVolume > 0.01f && !m_bankWind.empty()) {
            int idx = i % m_bankWind.size();
            float s = static_cast<float>(m_bankWind[idx]) / 32767.0f;
            sum += s * m_sfxVolume * m_windVolume * 0.35f;
        }

        // G. Menu & Garage Background Ambience (while not racing)
        if (m_menuAmbienceVol > 0.005f && !m_bankMenuAmbience.empty()) {
            float s = static_cast<float>(m_bankMenuAmbience[m_synthMenuAmbiencePos]) / 32767.0f;
            sum += s * m_sfxVolume * m_menuAmbienceVol * 0.38f;
            m_synthMenuAmbiencePos = (m_synthMenuAmbiencePos + 1) % m_bankMenuAmbience.size();
        }
        if (m_garageAmbienceVol > 0.005f && !m_bankGarageAmbience.empty()) {
            float s = static_cast<float>(m_bankGarageAmbience[m_synthGarageAmbiencePos]) / 32767.0f;
            sum += s * m_sfxVolume * m_garageAmbienceVol * 0.38f;
            m_synthGarageAmbiencePos = (m_synthGarageAmbiencePos + 1) % m_bankGarageAmbience.size();
        }

        // H. Racing Environment Ambience (while racing)
        if (m_raceAmbienceVol > 0.005f && !m_bankRaceAmbience.empty()) {
            float s = static_cast<float>(m_bankRaceAmbience[m_synthRaceAmbiencePos]) / 32767.0f;
            sum += s * m_sfxVolume * m_raceAmbienceVol * 0.32f;
            m_synthRaceAmbiencePos = (m_synthRaceAmbiencePos + 1) % m_bankRaceAmbience.size();
        }

        // I. Suspension Hydraulic Damper Breath
        if (m_suspensionVol > 0.01f && !m_bankSuspension.empty()) {
            int idx = (i * 2) % m_bankSuspension.size();
            float s = static_cast<float>(m_bankSuspension[idx]) / 32767.0f;
            sum += s * m_sfxVolume * m_suspensionVol * 0.28f;
        }

        // J. Turbocharger Whine
        if (m_turboSpoolVol > 0.01f) {
            float turboFreq = 2200.0f * m_turboSpoolPitch;
            m_synthTurboPhase += (turboFreq / static_cast<float>(sampleRate));
            if (m_synthTurboPhase > 1.0f) m_synthTurboPhase -= 1.0f;
            float turboS = std::sin(2.0f * M_PI * m_synthTurboPhase);
            sum += turboS * m_sfxVolume * m_turboSpoolVol * 0.22f;
        }

        // F. Event One-Shots (polyphonic software mixing)
        for (auto it = m_oneShots.begin(); it != m_oneShots.end(); ) {
            if (!it->bank || it->pos >= static_cast<int>(it->bank->size())) {
                it = m_oneShots.erase(it);
            } else {
                float s = static_cast<float>((*it->bank)[it->pos]) / 32767.0f;
                sum += s * m_sfxVolume * it->volume * 0.65f;
                it->pos++;
                ++it;
            }
        }

        // Master Volume Scaling & Anti-Clipping Limiter (Soft saturation curve)
        float masterScaled = sum * m_masterVolume;
        float limited = std::tanh(masterScaled);
        buffer[i] = static_cast<int16_t>(limited * 32000.0f);
    }
}

bool SoundManager::exportMixedAudioWav(const std::string& filePath, float durationSeconds, int sampleRate) {
    int totalSamples = static_cast<int>(durationSeconds * sampleRate);
    if (totalSamples <= 0) return false;
    std::vector<int16_t> buffer(totalSamples, 0);
    renderMixedAudio(buffer.data(), totalSamples, sampleRate);
    writeWavFile(QString::fromStdString(filePath), buffer, sampleRate);
    return true;
}

} // namespace Audio
