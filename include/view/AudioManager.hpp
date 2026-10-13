#pragma once

#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <random>

enum class SoundEffect {
    FireSalvo,
    Hit,
    Miss,
    ShipSunk,
    AbilityPing,
    ButtonClick,
    PlaneFlyby
};

enum class MusicTrack {
    MainTheme,
    BattleTheme,
    Victory,
    Defeat
};

class AudioManager {
public:
    static AudioManager& instance();

    void init();
    void update(); // Must be called each frame to stream music buffers
    void close();

    void playSFX(SoundEffect effect);
    void playMusic(MusicTrack track, bool restart = false);
    void stopMusic();
    void setMusicVolume(float volume); // Volume range: 0.0f (mute) to 1.0f (max)
    void setSFXVolume(float volume);   // Volume range: 0.0f (mute) to 1.0f (max)

    // Single-file safe loader (backward-compatible)
    void loadSoundSafe(SoundEffect id, const std::string& path);

    // Multi-file variation safe loader (e.g. 4+ firing sounds)
    void loadSoundVariationsSafe(SoundEffect id, const std::vector<std::string>& paths);

    void loadMusicSafe(MusicTrack id, const std::string& path);

private:
    AudioManager();
    ~AudioManager() = default;

    bool m_initialized{false};
    float m_musicVolume{0.7f};
    float m_sfxVolume{0.8f};

    MusicTrack m_currentTrack{MusicTrack::MainTheme};
    bool m_isMusicPlaying{false};

    // Stored as pools/vectors to support 1 to N audio variations per SFX action
    std::unordered_map<SoundEffect, std::vector<Sound>> m_sounds;
    std::unordered_map<MusicTrack, Music> m_music;

    std::mt19937 m_rng;
};