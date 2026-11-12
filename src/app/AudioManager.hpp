/**
 * @file AudioManager.hpp
 * @brief Audio system for sound effects and background music.
 */

#pragma once

#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <random>

/**
 * @brief Available sound effects for gameplay and UI feedback.
 */
enum class SoundEffect {
    FireSalvo,    
    Hit,     
    Miss,       
    ShipSunk,     
    AbilityPing,  
    ButtonClick,
    PlaneFlyby    
};

/**
 * @brief Background music tracks available in the game.
 */
enum class MusicTrack {
    MainTheme, 
    BattleTheme,
    Victory,    
    Defeat      
};

/**
 * @brief Singleton manager responsible for music and sound effect playback.
 */
class AudioManager {
public:
    /**
     * @brief Returns the singleton instance.
     * @return The shared AudioManager object.
     */
    static AudioManager& instance();

    /**
     * @brief Initializes the audio subsystem and loads default state.
     */
    void init();

    /**
     * @brief Updates the music stream. Must be called each frame.
     */
    void update();

    /**
     * @brief Releases audio resources and stops playback.
     */
    void close();

    /**
     * @brief Plays a single sound effect.
     * @param effect Sound effect to play.
     */
    void playSFX(SoundEffect effect);

    /**
     * @brief Plays a background music track.
     * @param track Music track to play.
     * @param restart If true, restart the track from the beginning.
     */
    void playMusic(MusicTrack track, bool restart = false);

    /**
     * @brief Stops the currently playing music track.
     */
    void stopMusic();

    /**
     * @brief Sets the music volume.
     * @param volume Volume in the range 0.0f (mute) to 1.0f (max).
     */
    void setMusicVolume(float volume);

    /**
     * @brief Sets the sound effect volume.
     * @param volume Volume in the range 0.0f (mute) to 1.0f (max).
     */
    void setSFXVolume(float volume);

    /**
     * @brief Loads a single sound effect file in a safe manner.
     * @param id Sound effect identifier.
     * @param path Audio file path.
     */
    void loadSoundSafe(SoundEffect id, const std::string& path);

    /**
     * @brief Loads multiple variations for a sound effect in a safe way.
     * @param id Sound effect identifier.
     * @param paths List of audio file paths for variations.
     */
    void loadSoundVariationsSafe(SoundEffect id, const std::vector<std::string>& paths);

    /**
     * @brief Loads a music track from disk.
     * @param id Music track identifier.
     * @param path Audio file path.
     */
    void loadMusicSafe(MusicTrack id, const std::string& path);

private:
    
    AudioManager();

    
    ~AudioManager() = default;

    bool m_initialized{false}; ///< Whether the audio subsystem is initialized.
    float m_musicVolume{0.7f}; ///< Current master music volume.
    float m_sfxVolume{0.8f}; ///< Current master sound-effect volume.

    MusicTrack m_currentTrack{MusicTrack::MainTheme}; ///< Currently active music track.
    bool m_isMusicPlaying{false}; ///< Whether music is currently playing.

    std::unordered_map<SoundEffect, std::vector<Sound>> m_sounds; ///< Sound pools for effects with variations.
    std::unordered_map<MusicTrack, Music> m_music; ///< Loaded music tracks.

    std::mt19937 m_rng; ///< Random number generator used for variation selection.
};