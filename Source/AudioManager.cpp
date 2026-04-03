/* Start Header ************************************************************************/
/*!
\file AudioManager.cpp
\author Tse Xuan Qi Tristin, tse.x, 2503757
\par tse.x@digipen.edu
\date January, 24, 2026
\brief Implements audio management including loading, caching, playback,
volume control, and unloading of audio resources.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "AudioManager.h"
#include <iostream>

// Clamps a value to the range [0.0f, 1.0f] for safe volume control
static float Clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

// Returns the singleton instance of the AudioManager.
AudioManager& AudioManager::Get()
{
    static AudioManager instance;
    return instance;
}

// Initializes audio groups for music and SFX and sets default volume levels
AudioManager::AudioManager()
{
    musicGroup = AEAudioCreateGroup();
    sfxGroup = AEAudioCreateGroup();
    musicVolume = 1.0f;
    sfxVolume = 1.0f;

    AEAudioSetGroupVolume(musicGroup, musicVolume);
    AEAudioSetGroupVolume(sfxGroup, sfxVolume);
}


// Loads an audio file and caches it
// if already loaded, returns the cached audio handle instead of reloading
AEAudio AudioManager::LoadAudio(const std::string& filepath, bool isMusic)
{
    auto it = audioMap.find(filepath);
    if (it != audioMap.end())//return cached audio if already loaded
        return it->second;

    AEAudio handle{};

    // load as music or sound depending on flag
    if (isMusic)
        handle = AEAudioLoadMusic(filepath.c_str());
    else
        handle = AEAudioLoadSound(filepath.c_str());

    audioMap[filepath] = handle;

    std::cout << "Loaded audio: " << filepath << std::endl;
    return handle;
}

// Loads multiple audio entries from JSON configuration, mapping string keys to audio handles
void AudioManager::LoadFromJson(const rapidjson::Value& audioConfig)
{
    if (!audioConfig.IsObject())
        return;

    for (auto it = audioConfig.MemberBegin(); it != audioConfig.MemberEnd(); ++it)
    {
        const std::string key = it->name.GetString();

        if (!it->value.IsString())
            continue;

        const std::string filepath = it->value.GetString();

        // detect music tracks by checking if the key contains "music".
        const bool isMusic = (key.find("music") != std::string::npos);

        AEAudio handle = LoadAudio(filepath, isMusic);
        audioKeyMap[key] = handle;
    }
}

// Retrieves an audio handle by key, returns an invalid handle if not found.
AEAudio AudioManager::GetAudio(const std::string& key) const
{
    auto it = audioKeyMap.find(key);
    if (it != audioKeyMap.end())
        return it->second;

    return AEAudio{};
}

// Unloads a specific audio resource and removes it from all internal maps
void AudioManager::UnloadAudio(AEAudio audioHandle)
{
    AEAudioUnloadAudio(audioHandle);

    for (auto it = audioMap.begin(); it != audioMap.end(); )
    {
        // match audio handles using underlying FMOD pointer to remove correct entry
        if (it->second.fmod_sound == audioHandle.fmod_sound)
            it = audioMap.erase(it);
        else
            ++it;
    }

    for (auto it = audioKeyMap.begin(); it != audioKeyMap.end(); )
    {
        if (it->second.fmod_sound == audioHandle.fmod_sound)
            it = audioKeyMap.erase(it);
        else
            ++it;
    }
}

// Unloads all audio resources and clears internal caches and audio groups.
void AudioManager::UnloadAll()
{
    for (auto it = audioMap.begin(); it != audioMap.end(); ++it)
    {
        AEAudioUnloadAudio(it->second);
    }

    audioMap.clear();
    audioKeyMap.clear();

    if (AEAudioIsValidGroup(musicGroup))
        AEAudioUnloadAudioGroup(musicGroup);

    if (AEAudioIsValidGroup(sfxGroup))
        AEAudioUnloadAudioGroup(sfxGroup);
}

// Plays an audio handle, automatically selecting the correct audio group
// (music or SFX) and applying the corresponding volume.
void AudioManager::PlayAudio(AEAudio audioHandle, bool loop)
{
    if (!AEAudioIsValidAudio(audioHandle))
        return;

    AEAudioGroup group = sfxGroup;
    float volume = sfxVolume;

    // determine whether this audio belongs to music or SFX based on its key.
    for (const auto& pair : audioKeyMap)
    {
        if (pair.second.fmod_sound == audioHandle.fmod_sound)
        {
            // key must contain "music" to be treated as music
            if (pair.first.find("music") != std::string::npos)
            {
                group = musicGroup;
                volume = musicVolume;
            }
            else
            {
                group = sfxGroup;
                volume = sfxVolume;
            }
            break;
        }
    }

    // loop indefinitely if loop == true, otherwise play once
    AEAudioPlay(audioHandle, group, volume, 1.0f, loop ? -1 : 0);
}

// Plays audio using a string key by retrieving the corresponding handle.
void AudioManager::PlayAudio(const std::string& key, bool loop)
{
    PlayAudio(GetAudio(key), loop);
}

// Sets music volume (clamped between 0 and 1) and updates the audio group.
void AudioManager::SetMusicVolume(float volume)
{
    musicVolume = Clamp01(volume);
    AEAudioSetGroupVolume(musicGroup, musicVolume);
}

// Sets SFX volume (clamped between 0 and 1) and updates the audio group.
void AudioManager::SetSFXVolume(float volume)
{
    sfxVolume = Clamp01(volume);
    AEAudioSetGroupVolume(sfxGroup, sfxVolume);
}

// Returns current music volume.
float AudioManager::GetMusicVolume() const
{
    return musicVolume;
}

// Returns current SFX volume.
float AudioManager::GetSFXVolume() const
{
    return sfxVolume;
}

// Stops playback of audio by stopping its associated audio group.
void AudioManager::StopAudio(AEAudio audioHandle)
{
    if (!AEAudioIsValidAudio(audioHandle))
        return;

    // determine whether this audio belongs to music or SFX based on its key.
    for (const auto& pair : audioKeyMap)
    {
        if (pair.second.fmod_sound == audioHandle.fmod_sound)
        {
            if (pair.first.find("music") != std::string::npos)//needs to have 'music' in string to be in music group
                AEAudioStopGroup(musicGroup);
            else
                AEAudioStopGroup(sfxGroup);
            return;
        }
    }

    // fallback: stop sfx group if unknown
    AEAudioStopGroup(sfxGroup);
}