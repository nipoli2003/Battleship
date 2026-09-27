#include "view/AudioManager.hpp"
#include <iostream>

AudioManager& AudioManager::instance() {
    static AudioManager s_instance;
    return s_instance;
}

// Initialize the audio device if not already initialized
void AudioManager::init() {
    if (m_initialized) return;
    InitAudioDevice();

    // Map audio assets (ensure files exist in assets/audio/)
    m_sounds[SoundEffect::FireSalvo]   = LoadSound("assets/audio/fire.wav");
    m_sounds[SoundEffect::Hit]         = LoadSound("assets/audio/hit.wav");
    m_sounds[SoundEffect::Miss]        = LoadSound("assets/audio/miss.wav");
    m_sounds[SoundEffect::ShipSunk]    = LoadSound("assets/audio/sunk.wav");
    m_sounds[SoundEffect::AbilityPing] = LoadSound("assets/audio/sonar.wav");
    m_sounds[SoundEffect::ButtonClick] = LoadSound("assets/audio/click.wav");

    m_music[MusicTrack::MainTheme]     = LoadMusicStream("assets/audio/main_theme.mp3");
    m_music[MusicTrack::BattleTheme]   = LoadMusicStream("assets/audio/battle_theme.mp3");

    m_initialized = true;
}

// Update the audio manager each frame to stream music buffers
void AudioManager::update() {
    if (m_initialized && m_isMusicPlaying) {
        UpdateMusicStream(m_music[m_currentTrack]);
    }
}

// Close the audio device and unload all sounds and music
void AudioManager::close() {
    if (!m_initialized) return;

    for (auto& [_, sound] : m_sounds) {
        if (IsSoundValid(sound)) UnloadSound(sound); // Unload the sound if it's valid
    }

    for (auto& [_, music] : m_music) {
        if (IsMusicValid(music)) UnloadMusicStream(music); // Unload the music if it's valid
    }

    m_sounds.clear();
    m_music.clear();

    CloseAudioDevice();
    m_initialized = false;
}

// Play a sound effect if it exists and is valid
void AudioManager::playSFX(SoundEffect effect) {
    if (!m_initialized) return;

    auto it = m_sounds.find(effect); // Find the sound effect in the map
    if (it != m_sounds.end() && IsSoundValid(it->second)) {
        SetSoundVolume(it->second, m_sfxVolume);
        PlaySound(it->second);
    } else {
        std::cout << "[AudioManager] Sound effect not found or invalid: " << static_cast<int>(effect) << std::endl;
    }
}

// Play a music track if it exists and is valid, optionally restarting it
void AudioManager::playMusic(MusicTrack track, bool restart) {
    if (!m_initialized) return;

    auto it = m_music.find(track);
    if (it != m_music.end() && IsMusicValid(it->second)) {
        if (restart || !m_isMusicPlaying || m_currentTrack != track) {
            StopMusicStream(m_music[m_currentTrack]); // Stop current music
            m_currentTrack = track;
            SetMusicVolume(m_music[m_currentTrack], m_musicVolume);
            PlayMusicStream(m_music[m_currentTrack]);
            m_isMusicPlaying = true;
        }
    } else {
        std::cout << "[AudioManager] Music track not found or invalid: " << static_cast<int>(track) << std::endl;
    }
}

void AudioManager::stopMusic() {
    if (!m_initialized || !m_isMusicPlaying) return;

    StopMusicStream(m_music[m_currentTrack]);
    m_isMusicPlaying = false;
}

void AudioManager::setMusicVolume(float volume) {
    m_musicVolume = std::min(std::max(volume, 0.0f), 1.0f); // Clamp volume between 0.0 and 1.0

    if (m_initialized && m_isMusicPlaying) {
        SetMusicVolume(m_music[m_currentTrack], m_musicVolume);
    }
}

void AudioManager::setSFXVolume(float volume) {
    m_sfxVolume = std::min(std::max(volume, 0.0f), 1.0f); // Clamp volume between 0.0 and 1.0
}

void AudioManager::loadSoundSafe(SoundEffect id, const std::string& path) {
    if (FileExists(path.c_str())) {
        Sound snd = LoadSound(path.c_str());
        if (IsSoundValid(snd)) {
            m_sounds[id] = snd;
            return;
        }
    }
    // Fallback: File is missing or invalid; avoid storing a garbage struct
    std::cout << "[AudioManager] Asset not found (skipping): " << path << std::endl;
}

void AudioManager::loadMusicSafe(MusicTrack id, const std::string& path) {
    if (FileExists(path.c_str())) {
        Music mus = LoadMusicStream(path.c_str());
        if (IsMusicValid(mus)) {
            m_music[id] = mus;
            return;
        }
    }
    std::cout << "[AudioManager] Music not found (skipping): " << path << std::endl;
}
