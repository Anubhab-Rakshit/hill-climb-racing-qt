#include "SoundManager.h"
#include <QSoundEffect>
#include <QUrl>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <cmath>
#include <vector>
#include <cstdint>

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

SoundManager::SoundManager()
    : m_audioAvailable(true)
    , m_sndClick(std::make_unique<QSoundEffect>())
    , m_sndCoin(std::make_unique<QSoundEffect>())
    , m_sndFuel(std::make_unique<QSoundEffect>())
    , m_sndStunt(std::make_unique<QSoundEffect>())
    , m_sndCrash(std::make_unique<QSoundEffect>())
{
    initProceduralSounds();
}

SoundManager::~SoundManager() = default;

void SoundManager::initProceduralSounds() {
    QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const uint32_t SR = 22050;

    // 1. Click Sound (25ms subtle high-pitch click)
    {
        QString path = tmpDir + "/hcr_snd_click.wav";
        int count = static_cast<int>(SR * 0.025f);
        std::vector<int16_t> samples(count);
        for (int i = 0; i < count; ++i) {
            float t = static_cast<float>(i) / SR;
            float env = std::exp(-t * 120.0f);
            float s = std::sin(2.0f * M_PI * 1200.0f * t) * env;
            samples[i] = static_cast<int16_t>(s * 22000.0f);
        }
        writeWavFile(path, samples, SR);
        m_sndClick->setSource(QUrl::fromLocalFile(path));
        m_sndClick->setVolume(0.7f);
    }

    // 2. Coin Sound (120ms dual chime: B5 -> E6)
    {
        QString path = tmpDir + "/hcr_snd_coin.wav";
        int count = static_cast<int>(SR * 0.12f);
        std::vector<int16_t> samples(count);
        for (int i = 0; i < count; ++i) {
            float t = static_cast<float>(i) / SR;
            float freq = (t < 0.05f) ? 987.77f : 1318.51f; // B5 -> E6
            float env = (t < 0.05f) ? 1.0f : std::exp(-(t - 0.05f) * 25.0f);
            float s = std::sin(2.0f * M_PI * freq * t) * env;
            samples[i] = static_cast<int16_t>(s * 25000.0f);
        }
        writeWavFile(path, samples, SR);
        m_sndCoin->setSource(QUrl::fromLocalFile(path));
        m_sndCoin->setVolume(0.85f);
    }

    // 3. Fuel Sound (220ms upward powerup sweep)
    {
        QString path = tmpDir + "/hcr_snd_fuel.wav";
        int count = static_cast<int>(SR * 0.22f);
        std::vector<int16_t> samples(count);
        for (int i = 0; i < count; ++i) {
            float t = static_cast<float>(i) / SR;
            float freq = 280.0f + 650.0f * (t / 0.22f);
            float env = std::sin((t / 0.22f) * M_PI);
            float s = std::sin(2.0f * M_PI * freq * t) * env;
            samples[i] = static_cast<int16_t>(s * 24000.0f);
        }
        writeWavFile(path, samples, SR);
        m_sndFuel->setSource(QUrl::fromLocalFile(path));
        m_sndFuel->setVolume(0.85f);
    }

    // 4. Stunt Fanfare (320ms C-E-G arpeggio)
    {
        QString path = tmpDir + "/hcr_snd_stunt.wav";
        int count = static_cast<int>(SR * 0.32f);
        std::vector<int16_t> samples(count);
        for (int i = 0; i < count; ++i) {
            float t = static_cast<float>(i) / SR;
            float freq = (t < 0.09f) ? 523.25f : (t < 0.18f) ? 659.25f : 783.99f;
            float noteT = std::fmod(t, 0.09f);
            float env = std::exp(-noteT * 12.0f);
            float s = (std::sin(2.0f * M_PI * freq * t) + 0.3f * std::sin(4.0f * M_PI * freq * t)) * env;
            samples[i] = static_cast<int16_t>(s * 26000.0f);
        }
        writeWavFile(path, samples, SR);
        m_sndStunt->setSource(QUrl::fromLocalFile(path));
        m_sndStunt->setVolume(0.9f);
    }

    // 5. Crash Sound (400ms heavy crunchy impact noise)
    {
        QString path = tmpDir + "/hcr_snd_crash.wav";
        int count = static_cast<int>(SR * 0.40f);
        std::vector<int16_t> samples(count);
        uint32_t seed = 12345;
        for (int i = 0; i < count; ++i) {
            float t = static_cast<float>(i) / SR;
            seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
            float noise = (static_cast<float>(seed) / 2147483647.0f) * 2.0f - 1.0f;
            float boom = std::sin(2.0f * M_PI * 65.0f * t) * std::exp(-t * 8.0f);
            float crunch = noise * std::exp(-t * 14.0f);
            float s = (boom * 0.7f + crunch * 0.5f);
            samples[i] = static_cast<int16_t>(std::clamp(s, -1.0f, 1.0f) * 28000.0f);
        }
        writeWavFile(path, samples, SR);
        m_sndCrash->setSource(QUrl::fromLocalFile(path));
        m_sndCrash->setVolume(0.95f);
    }
}

void SoundManager::playButtonClick() {
    if (m_sndClick && m_sndClick->isLoaded()) {
        m_sndClick->stop();
        m_sndClick->play();
    }
}

void SoundManager::playCoinPickup() {
    if (m_sndCoin && m_sndCoin->isLoaded()) {
        m_sndCoin->stop();
        m_sndCoin->play();
    }
}

void SoundManager::playFuelPickup() {
    if (m_sndFuel && m_sndFuel->isLoaded()) {
        m_sndFuel->stop();
        m_sndFuel->play();
    }
}

void SoundManager::playStuntCheer() {
    if (m_sndStunt && m_sndStunt->isLoaded()) {
        m_sndStunt->stop();
        m_sndStunt->play();
    }
}

void SoundManager::playCrash() {
    if (m_sndCrash && m_sndCrash->isLoaded()) {
        m_sndCrash->stop();
        m_sndCrash->play();
    }
}

void SoundManager::updateEngineRpm(float /*rpm*/) {
    // Engine pitch hook
}

} // namespace Audio
