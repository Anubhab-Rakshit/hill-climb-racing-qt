# Audio Asset Manifest & Licensing Specification

All audio files in this project are licensed under **Creative Commons Zero (CC0 1.0 Universal) Public Domain Dedication**, permitting free commercial and non-commercial usage, modification, and distribution without copyright restrictions or royalties.

Every asset is validated as a standard uncompressed 16-bit PCM RIFF WAV file, natively decodable across Linux, macOS, and Windows by Qt 6 Multimedia (`QMediaPlayer` / `QSoundEffect`).

---

## 1. Background Music (`assets/audio/music/`)

| Asset File | Creator | Source URL | Format | Channels | Sample Rate | Duration | License | Purpose / Description |
|---|---|---|---|---|---|---|---|---|
| `music/menu_music.wav` | TinyWorlds | [OpenGameArt: Happy Adventure Loop](https://opengameart.org/content/happy-adventure-loop) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 46.81 s | CC0 1.0 | Upbeat retro chiptune theme for Main Menu and Stage Select screens. |
| `music/garage_music.wav` | Zane Little Music | [OpenGameArt: Post-Adventure Tea Party](https://opengameart.org/content/10-free-chiptune-tracks-a-bag-of-chips) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 225.04 s | CC0 1.0 | Relaxing, cozy retro chiptune groove for the vehicle Garage screen. |
| `music/race_music.wav` | Sudocolon | [OpenGameArt: Hyperton](https://opengameart.org/content/hyperton) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 96.05 s | CC0 1.0 | High-adrenaline arcade electronic racing track for Countryside & Arctic stages. |
| `music/race_desert.wav` | Zane Little Music | [OpenGameArt: Dizzy Racing](https://opengameart.org/content/10-free-chiptune-tracks-a-bag-of-chips) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 136.50 s | CC0 1.0 | Energetic, fast-paced racing theme for the Desert stage. |
| `music/race_moon.wav` | Zane Little Music | [OpenGameArt: Space Cadet Training Montage](https://opengameart.org/content/10-free-chiptune-tracks-a-bag-of-chips) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 73.02 s | CC0 1.0 | Cosmic sci-fi chiptune synthwave track for the Moon stage. |
| `music/race_mountain.wav` | Sudocolon | [OpenGameArt: Chip Bit Danger](https://opengameart.org/content/chip-bit-danger) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 59.04 s | CC0 1.0 | Driving, action-packed chiptune rock for Mountain and Volcano stages. |
| `music/game_over.wav` | Sudocolon | [OpenGameArt: Icy Game Over](https://opengameart.org/content/icy-game-over) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 8.31 s | CC0 1.0 | Melancholic 8-bit defeat and wreck sequence jingle. |

---

## 2. Sound Effects (`assets/audio/sfx/`)

| Asset File | Creator | Source URL | Format | Channels | Sample Rate | Duration | License | Purpose / Description |
|---|---|---|---|---|---|---|---|---|
| `sfx/click.wav` | KenneyNL | [OpenGameArt: 51 UI Sound Effects](https://opengameart.org/content/51-ui-sound-effects-buttons-switches-and-clicks) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 0.09 s | CC0 1.0 | Tactile mechanical button click for all UI controls and upgrades. |
| `sfx/coin.wav` | WobbleBoxx | [OpenGameArt: Level up, power up, Coin get](https://opengameart.org/content/level-up-power-up-coin-get-13-sounds) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 2.55 s | CC0 1.0 | Resonant melodic coin pickup chime with pitch combo multiplier. |
| `sfx/fuel.wav` | WobbleBoxx | [OpenGameArt: Level up, power up, Coin get](https://opengameart.org/content/level-up-power-up-coin-get-13-sounds) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 1.76 s | CC0 1.0 | Upward sweeping powerup chime for fuel canister refills. |
| `sfx/stunt.wav` | WobbleBoxx | [OpenGameArt: Level up, power up, Coin get](https://opengameart.org/content/level-up-power-up-coin-get-13-sounds) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 2.38 s | CC0 1.0 | Major arpeggio fanfare for backflips, frontflips, wheelies, and air time. |
| `sfx/jump.wav` | BMacZero | [OpenGameArt: Metal Impact Sounds](https://opengameart.org/content/metal-impact-sounds) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 0.72 s | CC0 1.0 | Pneumatic suspension unweighting pop on airborne ramp takeoff. |
| `sfx/landing_soft.wav` | MentalSanityOff | [OpenGameArt: Jump Landing Sound](https://opengameart.org/content/jump-landing-sound) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 0.41 s | CC0 1.0 | Soft tyre touchdown and suspension compression thud. |
| `sfx/landing_hard.wav` | BMacZero | [OpenGameArt: Metal Impact Sounds](https://opengameart.org/content/metal-impact-sounds) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 0.58 s | CC0 1.0 | Violent suspension bottom-out bump-stop slap on severe drops. |
| `sfx/impact_soft.wav` | KenneyNL | [Kenney: Starter Kit Racing](https://raw.githubusercontent.com/KenneyNL/Starter-Kit-Racing/main/audio/impact.ogg) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 0.65 s | CC0 1.0 | Low-speed chassis bump and ground scraping. |
| `sfx/impact_hard.wav` | BMacZero | [OpenGameArt: Metal Impact Sounds](https://opengameart.org/content/metal-impact-sounds) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 0.31 s | CC0 1.0 | High-velocity metal collision and obstacle slam. |
| `sfx/crash.wav` | qubodup | [OpenGameArt: Crash Collision](https://opengameart.org/content/crash-collision) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 0.58 s | CC0 1.0 | Fatal rollover impact boom and vehicle wreck crash. |
| `sfx/tyre_roll.wav` | Project / CC0 | [Kenney: Starter Kit Racing](https://opengameart.org/) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 1.60 s | CC0 1.0 | Continuous ground surface rolling rumble scaled to vehicle ground speed. |
| `sfx/skid.wav` | KenneyNL | [Kenney: Starter Kit Racing](https://raw.githubusercontent.com/KenneyNL/Starter-Kit-Racing/main/audio/skid.ogg) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 3.51 s | CC0 1.0 | High-friction tyre screech and brake lock skid loop. |
| `sfx/wind_loop.wav` | SketchMan3 | [OpenGameArt: Wind Whoosh Loop](https://opengameart.org/content/wind-whoosh-loop) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 5.96 s | CC0 1.0 | Aerodynamic wind rush loop scaled to velocity (muted in lunar vacuum). |
| `sfx/engine_idle.wav` | domasx2 | [OpenGameArt: Racing Car Engine Loops](https://opengameart.org/content/racing-car-engine-sound-loops) | PCM WAV 16-bit | 1 (Mono) | 44,100 Hz | 0.86 s | CC0 1.0 | Idle engine rumble (800–1200 RPM). |
| `sfx/engine_tier1.wav` | domasx2 | [OpenGameArt: Racing Car Engine Loops](https://opengameart.org/content/racing-car-engine-sound-loops) | PCM WAV 16-bit | 1 (Mono) | 44,100 Hz | 0.67 s | CC0 1.0 | Low RPM acceleration loop (1200–2500 RPM). |
| `sfx/engine_tier2.wav` | domasx2 | [OpenGameArt: Racing Car Engine Loops](https://opengameart.org/content/racing-car-engine-sound-loops) | PCM WAV 16-bit | 1 (Mono) | 44,100 Hz | 0.68 s | CC0 1.0 | Mid RPM cruising loop (2500–4500 RPM). |
| `sfx/engine_tier3.wav` | domasx2 | [OpenGameArt: Racing Car Engine Loops](https://opengameart.org/content/racing-car-engine-sound-loops) | PCM WAV 16-bit | 1 (Mono) | 44,100 Hz | 0.62 s | CC0 1.0 | High-mid RPM climbing loop (4500–6200 RPM). |
| `sfx/engine_tier4.wav` | domasx2 | [OpenGameArt: Racing Car Engine Loops](https://opengameart.org/content/racing-car-engine-sound-loops) | PCM WAV 16-bit | 1 (Mono) | 44,100 Hz | 0.63 s | CC0 1.0 | High RPM sports rev loop (6200–7600 RPM). |
| `sfx/engine_tier5.wav` | domasx2 | [OpenGameArt: Racing Car Engine Loops](https://opengameart.org/content/racing-car-engine-sound-loops) | PCM WAV 16-bit | 1 (Mono) | 44,100 Hz | 0.63 s | CC0 1.0 | Rev-limiter redline scream loop (7600–8500 RPM). |
| `sfx/engine_kenney.wav` | KenneyNL | [Kenney: Starter Kit Racing](https://raw.githubusercontent.com/KenneyNL/Starter-Kit-Racing/main/audio/engine.ogg) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 8.00 s | CC0 1.0 | Bespoke Off-Roader and Pickup truck 4x4 engine loop. |
| `sfx/engine_motorcycle.wav` | KenneyNL | [Kenney: Starter Kit Racing](https://raw.githubusercontent.com/KenneyNL/Starter-Kit-Racing/main/audio/engine-motorcycle.ogg) | PCM WAV 16-bit | 1 (Mono) | 22,050 Hz | 8.01 s | CC0 1.0 | High-rev electric and racing sports powertrain loop. |
