#pragma once

#include <string>
#include <memory>

class QSoundEffect;

namespace Audio {

/**
 * @brief Manages sound effects, engine pitch shifting, and UI click audio.
 * Synthesizes 100% procedural 8-bit retro arcade sounds at startup.
 */
class SoundManager {
public:
    SoundManager();
    ~SoundManager();

    void playButtonClick();
    void playCoinPickup();
    void playFuelPickup();
    void playStuntCheer();
    void playCrash();

    void updateEngineRpm(float rpm);

    bool isAudioAvailable() const { return m_audioAvailable; }

private:
    bool m_audioAvailable;
    std::unique_ptr<QSoundEffect> m_sndClick;
    std::unique_ptr<QSoundEffect> m_sndCoin;
    std::unique_ptr<QSoundEffect> m_sndFuel;
    std::unique_ptr<QSoundEffect> m_sndStunt;
    std::unique_ptr<QSoundEffect> m_sndCrash;

    void initProceduralSounds();
};

} // namespace Audio
