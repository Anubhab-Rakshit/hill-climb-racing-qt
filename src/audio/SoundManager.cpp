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
    , m_raceMusicVol(0.0f)
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
    , m_sndRaceMusic(std::make_unique<QSoundEffect>(this))
    , m_sndTyreRoll(std::make_unique<QSoundEffect>(this))
    , m_sndTyreSkidAsphalt(std::make_unique<QSoundEffect>(this))
    , m_sndTyreSkidDirt(std::make_unique<QSoundEffect>(this))
    , m_sndTyreSkidSand(std::make_unique<QSoundEffect>(this))
    , m_sndWind(std::make_unique<QSoundEffect>(this))
    , m_synthEnginePhase(0.0f)
    , m_synthWindPhase(0.0f)
    , m_synthSkidPhase(0.0f)
    , m_synthMenuMusicPos(0)
    , m_synthRaceMusicPos(0)
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
    if (m_sndRaceMusic) m_sndRaceMusic->setVolume(m_masterVolume * m_musicVolume * m_raceMusicVol);
    if (m_menuAudioOut) m_menuAudioOut->setVolume(std::clamp(m_masterVolume * m_musicVolume * m_menuMusicVol, 0.0f, 1.0f));
    if (m_raceAudioOut) m_raceAudioOut->setVolume(std::clamp(m_masterVolume * m_musicVolume * m_raceMusicVol, 0.0f, 1.0f));

    if (m_sndTyreRoll) m_sndTyreRoll->setVolume(m_masterVolume * m_sfxVolume * m_tyreRollVolume);
    if (m_sndWind) m_sndWind->setVolume(m_masterVolume * m_sfxVolume * m_windVolume);
    if (m_sndTyreSkidAsphalt) m_sndTyreSkidAsphalt->setVolume(m_masterVolume * m_sfxVolume * m_tyreSkidVolume);
    if (m_sndTyreSkidDirt) m_sndTyreSkidDirt->setVolume(m_masterVolume * m_sfxVolume * m_tyreSkidVolume);
    if (m_sndTyreSkidSand) m_sndTyreSkidSand->setVolume(m_masterVolume * m_sfxVolume * m_tyreSkidVolume);

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
    m_menuMusicVol = 0.0f;
    m_raceMusicVol = 0.0f;
    if (m_sndMenuMusic) m_sndMenuMusic->stop();
    if (m_sndRaceMusic) m_sndRaceMusic->stop();
    if (m_menuMusicPlayer) m_menuMusicPlayer->stop();
    if (m_raceMusicPlayer) m_raceMusicPlayer->stop();
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
    if (state == Core::StateType::MAIN_MENU || state == Core::StateType::GARAGE || state == Core::StateType::STAGE_SELECT) {
        setTargetMusic(MusicTrack::MENU);
    } else if (state == Core::StateType::GAMEPLAY || state == Core::StateType::PAUSED) {
        setTargetMusic(MusicTrack::RACE);
    } else if (state == Core::StateType::GAME_OVER) {
        setTargetMusic(MusicTrack::NONE);
    }

    float crossfadeRate = 4.0f * dt;
    float targetMenuVol = (m_targetMusicTrack == MusicTrack::MENU) ? 1.0f : 0.0f;
    float targetRaceVol = (m_targetMusicTrack == MusicTrack::RACE) ? ((state == Core::StateType::PAUSED) ? 0.35f : 1.0f) : 0.0f;

    m_menuMusicVol += (targetMenuVol - m_menuMusicVol) * std::min(1.0f, crossfadeRate);
    m_raceMusicVol += (targetRaceVol - m_raceMusicVol) * std::min(1.0f, crossfadeRate);

    m_activeMusicTrack = (m_raceMusicVol > m_menuMusicVol && m_raceMusicVol > 0.05f) ? MusicTrack::RACE :
                         (m_menuMusicVol > 0.05f) ? MusicTrack::MENU : MusicTrack::NONE;
    m_musicCurrentVol = std::max(m_menuMusicVol, m_raceMusicVol);

    updateLoopSafe(m_sndMenuMusic.get(), m_musicVolume * m_menuMusicVol, m_menuMusicVol > 0.01f);
    updateLoopSafe(m_sndRaceMusic.get(), m_musicVolume * m_raceMusicVol, m_raceMusicVol > 0.01f);

    float effMenuVol = std::clamp(m_masterVolume * m_musicVolume * m_menuMusicVol, 0.0f, 1.0f);
    float effRaceVol = std::clamp(m_masterVolume * m_musicVolume * m_raceMusicVol, 0.0f, 1.0f);

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

    // If not in active gameplay or wreck simulation, silence vehicle audio
    if (state != Core::StateType::GAMEPLAY && state != Core::StateType::GAME_OVER) {
        m_tyreRollVolume = 0.0f;
        m_tyreSkidVolume = 0.0f;
        m_windVolume = 0.0f;
        m_engineVolume = 0.0f;
        updateLoopSafe(m_sndTyreRoll.get(), 0.0f, false);
        updateLoopSafe(m_sndTyreSkidAsphalt.get(), 0.0f, false);
        updateLoopSafe(m_sndTyreSkidDirt.get(), 0.0f, false);
        updateLoopSafe(m_sndTyreSkidSand.get(), 0.0f, false);
        updateLoopSafe(m_sndWind.get(), 0.0f, false);
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

        // A. Music Subsystem Mixing
        if (m_activeMusicTrack == MusicTrack::MENU && !m_bankMenuMusic.empty()) {
            float s = static_cast<float>(m_bankMenuMusic[m_synthMenuMusicPos]) / 32767.0f;
            sum += s * m_musicVolume * m_menuMusicVol * 0.70f;
            m_synthMenuMusicPos = (m_synthMenuMusicPos + 1) % m_bankMenuMusic.size();
        } else if (m_activeMusicTrack == MusicTrack::RACE && !m_bankRaceMusic.empty()) {
            float s = static_cast<float>(m_bankRaceMusic[m_synthRaceMusicPos]) / 32767.0f;
            sum += s * m_musicVolume * m_raceMusicVol * 0.70f;
            m_synthRaceMusicPos = (m_synthRaceMusicPos + 1) % m_bankRaceMusic.size();
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
