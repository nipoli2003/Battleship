#include "app/AudioManager.hpp"
#include <chrono>
#include <iostream>

AudioManager::AudioManager()
    : m_rng(static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count())) {}

AudioManager& AudioManager::instance() {
    static AudioManager s_instance;
    return s_instance;
}

void AudioManager::init() {
    if (m_initialized) return;
    InitAudioDevice();

    // 1. Load multi-variation sound pools (e.g. 4 salvo variations)
    loadSoundVariationsSafe(SoundEffect::FireSalvo, {
        "assets/audio/fire_1.wav",
        "assets/audio/fire_2.wav",
        "assets/audio/fire_3.wav",
        "assets/audio/fire_4.wav"
    });

    // 2. Load standard SFXs (either via loadSoundSafe or loadSoundVariationsSafe)
    loadSoundSafe(SoundEffect::Hit,         "assets/audio/hit.wav");
    loadSoundSafe(SoundEffect::Miss,        "assets/audio/miss.wav");
    loadSoundSafe(SoundEffect::ShipSunk,    "assets/audio/sunk.wav");
    loadSoundSafe(SoundEffect::AbilityPing, "assets/audio/sonar.wav");
    loadSoundSafe(SoundEffect::ButtonClick, "assets/audio/click.wav");
    loadSoundSafe(SoundEffect::PlaneFlyby,  "assets/audio/plane_flyby.wav");

    // 3. Load all background music tracks
    loadMusicSafe(MusicTrack::MainTheme,   "assets/audio/main_theme.mp3");
    loadMusicSafe(MusicTrack::BattleTheme, "assets/audio/battle_theme.mp3");
    loadMusicSafe(MusicTrack::Victory,     "assets/audio/victory.mp3");
    loadMusicSafe(MusicTrack::Defeat,      "assets/audio/defeat.mp3");

    m_initialized = true;
}

void AudioManager::loadSoundSafe(SoundEffect id, const std::string& path) {
    loadSoundVariationsSafe(id, {path});
}

void AudioManager::loadSoundVariationsSafe(SoundEffect id, const std::vector<std::string>& paths) {
    for (const auto& path : paths) {
        if (FileExists(path.c_str())) {
            Sound snd = LoadSound(path.c_str());
            if (IsSoundValid(snd)) {
                m_sounds[id].push_back(snd);
                continue;
            }
        }
        // Missing files are safely skipped without throwing errors or corrupting memory
        std::cout << "[AudioManager] Asset missing/skipped: " << path << std::endl;
    }
}

void AudioManager::loadMusicSafe(MusicTrack id, const std::string& path) {
    if (FileExists(path.c_str())) {
        Music mus = LoadMusicStream(path.c_str());
        if (IsMusicValid(mus)) {
            m_music[id] = mus;
            return;
        }
    }
    std::cout << "[AudioManager] Music missing/skipped: " << path << std::endl;
}

void AudioManager::update() {
    if (!m_initialized || !m_isMusicPlaying) return;

    auto it = m_music.find(m_currentTrack);
    if (it != m_music.end() && IsMusicValid(it->second)) {
        UpdateMusicStream(it->second);
    }
}

void AudioManager::playSFX(SoundEffect effect) {
    if (!m_initialized) return;

    auto it = m_sounds.find(effect);
    if (it == m_sounds.end() || it->second.empty()) {
        return; // No files exist for this action yet; fails silently
    }

    const auto& soundPool = it->second;
    std::uniform_int_distribution<size_t> dist(0, soundPool.size() - 1);
    size_t idx = dist(m_rng);

    const Sound& snd = soundPool[idx];
    if (IsSoundValid(snd)) {
        SetSoundVolume(snd, m_sfxVolume);

        // Subtle pitch variation (0.96x to 1.04x) for natural shot variety
        std::uniform_real_distribution<float> pitchDist(0.96f, 1.04f);
        SetSoundPitch(snd, pitchDist(m_rng));

        PlaySound(snd);
    }
}

void AudioManager::playMusic(MusicTrack track, bool restart) {
    if (!m_initialized) return;

    auto it = m_music.find(track);
    if (it == m_music.end() || !IsMusicValid(it->second)) {
        m_isMusicPlaying = false;
        return;
    }

    if (m_isMusicPlaying && m_currentTrack == track && !restart) return;

    // Stop current track if switching
    if (m_isMusicPlaying) {
        auto curIt = m_music.find(m_currentTrack);
        if (curIt != m_music.end() && IsMusicValid(curIt->second)) {
            StopMusicStream(curIt->second);
        }
    }

    m_currentTrack = track;
    SetMusicVolume(it->second, m_musicVolume);
    PlayMusicStream(it->second);
    m_isMusicPlaying = true;
}

void AudioManager::stopMusic() {
    if (!m_initialized || !m_isMusicPlaying) return;

    auto it = m_music.find(m_currentTrack);
    if (it != m_music.end() && IsMusicValid(it->second)) {
        StopMusicStream(it->second);
    }
    m_isMusicPlaying = false;
}

void AudioManager::setMusicVolume(float volume) {
    m_musicVolume = volume;
    if (m_isMusicPlaying) {
        auto it = m_music.find(m_currentTrack);
        if (it != m_music.end() && IsMusicValid(it->second)) {
            SetMusicVolume(it->second, m_musicVolume);
        }
    }
}

void AudioManager::setSFXVolume(float volume) {
    m_sfxVolume = volume;
}

void AudioManager::close() {
    if (!m_initialized) return;

    // Unload all pooled sounds safely
    for (auto& [_, variations] : m_sounds) {
        for (auto& snd : variations) {
            if (IsSoundValid(snd)) {
                UnloadSound(snd);
            }
        }
    }
    m_sounds.clear();

    // Unload music streams safely
    for (auto& [_, mus] : m_music) {
        if (IsMusicValid(mus)) {
            UnloadMusicStream(mus);
        }
    }
    m_music.clear();

    CloseAudioDevice();
    m_initialized = false;
}