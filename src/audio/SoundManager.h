#pragma once

#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <mutex>
#include <QtGlobal>
#include <QString>
#include "VehicleConfig.h"
#include "Vehicle.h"
#include "Terrain.h"
#include "GameState.h"

class QSoundEffect;
class QMediaPlayer;
class QAudioOutput;
class QIODevice;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
class QAudioSink;
#endif

#include <QObject>

namespace Audio {

enum class MusicTrack {
    NONE = 0,
    MENU,
    RACE,
    POKEMON_CHIPTUNE,
    SOOTHING_PASTORAL,
    EDGY_SYNTHWAVE,
    ENERGETIC_ARCADE
};

/**
 * @brief Realistic, Dynamic Game Audio Subsystem for Hill Climb Racing.
 * Synthesizes procedural retro 8/16-bit soundbank at startup and provides:
 * - Dynamic RPM engine audio with continuous pitch modulation and throttle load drive.
 * - Physics-driven rolling, skidding, and traction-loss tyre audio with surface material response.
 * - Multi-tiered collision and crash audio scaled by relative impact velocity.
 * - Jump liftoff and suspension landing impacts with bottom-out detection.
 * - Velocity- and airborne-responsive environmental wind audio.
 * - State-aware background music with seamless cross-fading.
 * - Master, Music, and SFX volume controls with anti-clipping software mixing.
 */
class SoundManager : public QObject {
    Q_OBJECT
public:
    explicit SoundManager(QObject* parent = nullptr);
    ~SoundManager() override;

    // High-Level Frame Update (Driven directly from physics and game state)
    void update(float dt, const Physics::Vehicle& vehicle, const Physics::Terrain& terrain, Core::StateType state);

    // Event One-Shots (with debouncing, pitch variety, and intensity scaling)
    void playButtonClick();
    void playCoinPickup();
    void playFuelPickup();
    void playStuntCheer();
    void playCrash();
    void playNeckCrunch();
    void playSoftImpact(float intensity = 1.0f);
    void playHardImpact(float intensity = 1.0f);
    void playLandingSoft(float intensity = 1.0f);
    void playLandingHard(float intensity = 1.0f);
    void playJumpTakeoff();

    // Volume Controls (Master, Music, SFX)
    float masterVolume() const { return m_masterVolume; }
    void setMasterVolume(float vol);

    float musicVolume() const { return m_musicVolume; }
    void setMusicVolume(float vol);

    float sfxVolume() const { return m_sfxVolume; }
    void setSfxVolume(float vol);

    // Dynamic Engine State Hook (Backward compatibility & explicit driving)
    void updateEngineRpm(float rpm, float throttle = 0.0f, Physics::VehicleType type = Physics::VehicleType::OFFROADER);

    // Music Control
    void setTargetMusic(MusicTrack track);
    void stopAllMusic();

    // Telemetry & Verification Getters
    float currentEngineRpm() const { return m_engineRpm; }
    float currentEnginePitch() const { return m_enginePitch; }
    float currentEngineVolume() const { return m_engineVolume; }
    float currentTyreRollVolume() const { return m_tyreRollVolume; }
    float currentTyreSkidVolume() const { return m_tyreSkidVolume; }
    float currentWindVolume() const { return m_windVolume; }
    MusicTrack currentMusicTrack() const { return m_activeMusicTrack; }
    float currentMusicVolume() const { return m_musicCurrentVol; }
    float lastImpactIntensity() const { return m_lastImpactIntensity; }
    bool isSkidding() const { return m_isSkidding; }
    bool isAirborne() const { return m_isAirborne; }

    bool isAudioAvailable() const { return m_audioAvailable; }
    QString assetBasePath() const { return m_assetBase; }
    QString audioDeviceName() const { return m_audioDeviceName; }
    int loadedAssetCount() const { return m_loadedAssetCount; }
    int failedAssetCount() const { return m_failedAssetCount; }
    bool isPlaybackEngineRunning() const { return m_playbackRunning; }

    // Offline / Diagnostic Audio Rendering (renders mixed PCM samples without clipping)
    void renderMixedAudio(int16_t* buffer, int numSamples, int sampleRate = 22050);
    bool exportMixedAudioWav(const std::string& filePath, float durationSeconds = 3.0f, int sampleRate = 22050);

private:
    friend class MixerIODevice;

    bool m_audioAvailable;
    bool m_playbackRunning = false;
    QString m_assetBase;
    QString m_audioDeviceName;
    int m_loadedAssetCount = 0;
    int m_failedAssetCount = 0;

    // Volume Settings (0.0 to 1.0)
    float m_masterVolume;
    float m_musicVolume;
    float m_sfxVolume;

    // Dynamic Engine State
    float m_engineRpm;
    float m_engineThrottle;
    float m_enginePitch;
    float m_engineVolume;
    Physics::VehicleType m_currentVehicleType;

    // Dynamic Tyre State
    float m_tyreRollVolume;
    float m_tyreSkidVolume;
    std::string m_currentBiome;
    bool m_isSkidding;

    // Environmental Wind
    float m_windVolume;

    // Airborne & Landing State Tracking
    bool m_wasAirborne;
    float m_airborneTime;
    bool m_isAirborne;

    // Collision Tracking
    float m_lastImpactIntensity;

    // Cooldown Timers (debouncing to prevent machine gun sound triggering)
    float m_cooldownImpact;
    float m_cooldownLanding;
    float m_cooldownJump;
    float m_cooldownCrash;

    // Music State & Crossfading
    MusicTrack m_targetMusicTrack;
    MusicTrack m_activeMusicTrack;
    float m_musicCurrentVol;
    float m_menuMusicVol;
    float m_raceMusicVol;

    // Coin Combo Streak Pitch Counter
    int m_coinStreak;
    float m_coinStreakTimer;

    // QSoundEffect Handles (Qt Multimedia)
    // One-Shots
    std::unique_ptr<QSoundEffect> m_sndClick;
    std::unique_ptr<QSoundEffect> m_sndCoin;
    std::unique_ptr<QSoundEffect> m_sndFuel;
    std::unique_ptr<QSoundEffect> m_sndStunt;
    std::unique_ptr<QSoundEffect> m_sndCrash;
    std::unique_ptr<QSoundEffect> m_sndImpactSoft;
    std::unique_ptr<QSoundEffect> m_sndImpactHard;
    std::unique_ptr<QSoundEffect> m_sndLandingSoft;
    std::unique_ptr<QSoundEffect> m_sndLandingHard;
    std::unique_ptr<QSoundEffect> m_sndJump;

    // Music Loops
    std::unique_ptr<QSoundEffect> m_sndMenuMusic;
    std::unique_ptr<QSoundEffect> m_sndRaceMusic;

    // Continuous Environmental & Tyre Loops
    std::unique_ptr<QSoundEffect> m_sndTyreRoll;
    std::unique_ptr<QSoundEffect> m_sndTyreSkidAsphalt;
    std::unique_ptr<QSoundEffect> m_sndTyreSkidDirt;
    std::unique_ptr<QSoundEffect> m_sndTyreSkidSand;
    std::unique_ptr<QSoundEffect> m_sndWind;

    // Multi-tier Engine Soundbank Loops (Cross-faded seamlessly across RPM spectrum)
    static constexpr int ENGINE_TIER_COUNT = 5;
    std::unique_ptr<QSoundEffect> m_sndEngine[ENGINE_TIER_COUNT];
    float m_engineTierVolumes[ENGINE_TIER_COUNT];

    // Pre-synthesized PCM buffers for software mixer & headless verification
    std::vector<int16_t> m_bankMenuMusic;
    std::vector<int16_t> m_bankRaceMusic;
    std::vector<int16_t> m_bankPokemonMusic;
    std::vector<int16_t> m_bankSoothingMusic;
    std::vector<int16_t> m_bankEdgyMusic;
    std::vector<int16_t> m_bankEnergeticMusic;
    std::vector<int16_t> m_bankNeckCrunch;
    std::vector<int16_t> m_bankEngineTiers[ENGINE_TIER_COUNT];
    std::vector<int16_t> m_bankTyreRoll;
    std::vector<int16_t> m_bankTyreSkidAsphalt;
    std::vector<int16_t> m_bankTyreSkidDirt;
    std::vector<int16_t> m_bankTyreSkidSand;
    std::vector<int16_t> m_bankWind;
    std::vector<int16_t> m_bankClick;
    std::vector<int16_t> m_bankCoin;
    std::vector<int16_t> m_bankFuel;
    std::vector<int16_t> m_bankStunt;
    std::vector<int16_t> m_bankCrash;
    std::vector<int16_t> m_bankImpactSoft;
    std::vector<int16_t> m_bankImpactHard;
    std::vector<int16_t> m_bankLandingSoft;
    std::vector<int16_t> m_bankLandingHard;
    std::vector<int16_t> m_bankJump;

    struct OneShotVoice {
        const std::vector<int16_t>* bank = nullptr;
        int pos = 0;
        float volume = 1.0f;
    };
    std::vector<OneShotVoice> m_oneShots;
    mutable std::mutex m_mixMutex;

    // Software Synth Phase Accumulators (for real-time sample mixing)
    float m_synthEnginePhase;
    float m_synthWindPhase;
    float m_synthSkidPhase;
    int m_synthMenuMusicPos;
    int m_synthRaceMusicPos;

    std::unique_ptr<QMediaPlayer> m_menuMusicPlayer;
    std::unique_ptr<QMediaPlayer> m_raceMusicPlayer;
    std::unique_ptr<QAudioOutput> m_menuAudioOut;
    std::unique_ptr<QAudioOutput> m_raceAudioOut;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    std::unique_ptr<QAudioSink> m_audioSink;
#endif
    std::unique_ptr<QIODevice> m_mixerDevice;

    void initProceduralSounds();
    void startPlaybackEngine();
    void updateEngineLoops();
    void applyVolumeScales();
    void playEffectSafe(QSoundEffect* effect, float volumeScale);
    void updateLoopSafe(QSoundEffect* effect, float targetVol, bool active);
    void queueOneShot(const std::vector<int16_t>* bank, float volumeScale);
    QString resolveAssetBase() const;
    bool loadValidatedWav(const QString& path, std::vector<int16_t>& outSamples, uint32_t& outRate);
    void attachEffect(QSoundEffect* effect, const QString& path, bool loop);
};

} // namespace Audio
