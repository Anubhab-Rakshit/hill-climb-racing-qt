#pragma once

#include <string>

namespace Audio {

/**
 * @brief Manages sound effects, engine pitch shifting, and UI click audio.
 */
class SoundManager {
public:
    SoundManager();

    void playButtonClick();
    void playCoinPickup();
    void playFuelPickup();
    void playStuntCheer();
    void playCrash();

    void updateEngineRpm(float rpm);

    bool isAudioAvailable() const { return m_audioAvailable; }

private:
    bool m_audioAvailable;
};

} // namespace Audio
