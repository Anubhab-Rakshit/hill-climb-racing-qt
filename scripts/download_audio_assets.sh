#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

mkdir -p "${ROOT_DIR}/assets/audio/music"
mkdir -p "${ROOT_DIR}/assets/audio/engine"
mkdir -p "${ROOT_DIR}/assets/audio/vehicle"
mkdir -p "${ROOT_DIR}/assets/audio/impacts"
mkdir -p "${ROOT_DIR}/assets/audio/environment"

TMP_DIR="$(mktemp -d /tmp/hcr_audio_download.XXXXXX)"
trap 'rm -rf "${TMP_DIR}"' EXIT

convert_ogg_to_wav() {
    local src="$1"
    local dst="$2"
    echo "Converting ${src} -> ${dst}..."
    gst-launch-1.0 -q filesrc location="${src}" ! oggdemux ! vorbisdec ! audioconvert ! audioresample ! "audio/x-raw,rate=22050,channels=1,format=S16LE" ! wavenc ! filesink location="${dst}"
}

convert_mp3_to_wav() {
    local src="$1"
    local dst="$2"
    echo "Converting ${src} -> ${dst}..."
    gst-launch-1.0 -q filesrc location="${src}" ! mpegaudioparse ! mpg123audiodec ! audioconvert ! audioresample ! "audio/x-raw,rate=22050,channels=1,format=S16LE" ! wavenc ! filesink location="${dst}"
}

download_file() {
    local url="$1"
    local dest="$2"
    echo "Downloading ${url} -> ${dest}..."
    curl -sSL --retry 3 --connect-timeout 10 "${url}" -o "${dest}"
    if [ ! -s "${dest}" ]; then
        echo "Error: Failed to download ${url}" >&2
        return 1
    fi
}

echo "=== Downloading OpenGameArt Engine Loops (CC0 by domasx2) ==="
download_file "https://opengameart.org/sites/default/files/loop_0.wav" "${ROOT_DIR}/assets/audio/engine/engine_idle.wav"
download_file "https://opengameart.org/sites/default/files/loop_1_0.wav" "${ROOT_DIR}/assets/audio/engine/engine_tier1.wav"
download_file "https://opengameart.org/sites/default/files/loop_2_0.wav" "${ROOT_DIR}/assets/audio/engine/engine_tier2.wav"
download_file "https://opengameart.org/sites/default/files/loop_3_0.wav" "${ROOT_DIR}/assets/audio/engine/engine_tier3.wav"
download_file "https://opengameart.org/sites/default/files/loop_4_0.wav" "${ROOT_DIR}/assets/audio/engine/engine_tier4.wav"
download_file "https://opengameart.org/sites/default/files/loop_5_0.wav" "${ROOT_DIR}/assets/audio/engine/engine_tier5.wav"

echo "=== Downloading Kenney Racing Assets (CC0 by KenneyNL) ==="
download_file "https://raw.githubusercontent.com/KenneyNL/Starter-Kit-Racing/main/audio/engine.ogg" "${TMP_DIR}/engine_kenney.ogg"
convert_ogg_to_wav "${TMP_DIR}/engine_kenney.ogg" "${ROOT_DIR}/assets/audio/engine/engine_kenney.wav"

download_file "https://raw.githubusercontent.com/KenneyNL/Starter-Kit-Racing/main/audio/engine-motorcycle.ogg" "${TMP_DIR}/engine_motorcycle.ogg"
convert_ogg_to_wav "${TMP_DIR}/engine_motorcycle.ogg" "${ROOT_DIR}/assets/audio/engine/engine_motorcycle.wav"

download_file "https://raw.githubusercontent.com/KenneyNL/Starter-Kit-Racing/main/audio/skid.ogg" "${TMP_DIR}/skid.ogg"
convert_ogg_to_wav "${TMP_DIR}/skid.ogg" "${ROOT_DIR}/assets/audio/vehicle/skid.wav"

download_file "https://raw.githubusercontent.com/KenneyNL/Starter-Kit-Racing/main/audio/impact.ogg" "${TMP_DIR}/impact_soft.ogg"
convert_ogg_to_wav "${TMP_DIR}/impact_soft.ogg" "${ROOT_DIR}/assets/audio/impacts/impact_soft.wav"

echo "=== Downloading Crash & Collision SFX (CC0 by qubodup) ==="
download_file "https://opengameart.org/sites/default/files/qubodup-crash.ogg" "${TMP_DIR}/crash.ogg"
convert_ogg_to_wav "${TMP_DIR}/crash.ogg" "${ROOT_DIR}/assets/audio/impacts/crash.wav"

echo "=== Downloading Wind Loop (CC0 by SketchMan3) ==="
download_file "https://opengameart.org/sites/default/files/wind%20woosh%20loop.ogg" "${TMP_DIR}/wind.ogg"
convert_ogg_to_wav "${TMP_DIR}/wind.ogg" "${ROOT_DIR}/assets/audio/environment/wind_loop.wav"

echo "=== Downloading Menu & Gameplay Music (CC0 by TinyWorlds & Sudocolon) ==="
download_file "https://opengameart.org/sites/default/files/happy_adveture.mp3" "${TMP_DIR}/menu_music.mp3"
convert_mp3_to_wav "${TMP_DIR}/menu_music.mp3" "${ROOT_DIR}/assets/audio/music/menu_music.wav"

download_file "https://opengameart.org/sites/default/files/Hyperton.mp3" "${TMP_DIR}/race_music.mp3"
convert_mp3_to_wav "${TMP_DIR}/race_music.mp3" "${ROOT_DIR}/assets/audio/music/race_music.wav"

echo "=== All audio assets downloaded and converted successfully! ==="
