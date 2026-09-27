#pragma once

#include <raylib.h>
#include <string>
#include <unordered_map>

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
    static constexpr bool ENABLE_AUDIO = true;

    void init() {
        if constexpr (!ENABLE_AUDIO) return;
        InitAudioDevice();
        m_initialized = true;
    }
    void update(); // Must be called each frame to stream music buffers
    void close();

    void playSFX(SoundEffect effect);
    void playMusic(MusicTrack track, bool restart = false);
    void stopMusic();
    void setMusicVolume(float volume); // Volume range: 0.0f (mute) to 1.0f (max)
    void setSFXVolume(float volume);   // Volume range: 0.0f (mute) to 1.0f (max)

    void loadSoundSafe(SoundEffect id, const std::string& path);
    void loadMusicSafe(MusicTrack id, const std::string& path);

private:
    AudioManager() = default;
    ~AudioManager() = default;

    bool m_initialized{false};
    float m_musicVolume{0.7f};
    float m_sfxVolume{0.8f};

    MusicTrack m_currentTrack{MusicTrack::MainTheme};
    bool m_isMusicPlaying{false};

    std::unordered_map<SoundEffect, Sound> m_sounds;
    std::unordered_map<MusicTrack, Music> m_music;
};