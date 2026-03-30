/* Start Header ************************************************************************/
/*!
\file AudioManager.cpp
\author Tse Xuan Qi Tristin, tse.x, 2503757
\par tse.x@digipen.edu
\date Junuary, 24, 2026
\brief Implements the audio cache logic and automatically releases all loaded audio

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "AudioManager.h"
#include <iostream>

static float Clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

AudioManager& AudioManager::Get()
{
    static AudioManager instance;
    return instance;
}

AudioManager::AudioManager()
{
    musicGroup = AEAudioCreateGroup();
    sfxGroup = AEAudioCreateGroup();
    musicVolume = 1.0f;
    sfxVolume = 1.0f;

    AEAudioSetGroupVolume(musicGroup, musicVolume);
    AEAudioSetGroupVolume(sfxGroup, sfxVolume);
}

AudioManager::~AudioManager()
{
    //UnloadAll();
    //AEAudioExit();
}

AEAudio AudioManager::LoadAudio(const std::string& filepath, bool isMusic)
{
    auto it = audioMap.find(filepath);
    if (it != audioMap.end())
        return it->second;

    AEAudio handle{};

    if (isMusic)
        handle = AEAudioLoadMusic(filepath.c_str());
    else
        handle = AEAudioLoadSound(filepath.c_str());

    audioMap[filepath] = handle;

    std::cout << "Loaded audio: " << filepath << std::endl;
    return handle;
}

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

        // crude music detection by key name
        const bool isMusic = (key.find("music") != std::string::npos);

        AEAudio handle = LoadAudio(filepath, isMusic);
        audioKeyMap[key] = handle;
    }
}

AEAudio AudioManager::GetAudio(const std::string& key) const
{
    auto it = audioKeyMap.find(key);
    if (it != audioKeyMap.end())
        return it->second;

    return AEAudio{};
}

void AudioManager::UnloadAudio(AEAudio audioHandle)
{
    AEAudioUnloadAudio(audioHandle);

    for (auto it = audioMap.begin(); it != audioMap.end(); )
    {
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

void AudioManager::PlayAudio(AEAudio audioHandle, bool loop)
{
    if (!AEAudioIsValidAudio(audioHandle))
        return;

    AEAudioGroup group = sfxGroup;
    float volume = sfxVolume;

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

    AEAudioPlay(audioHandle, group, volume, 1.0f, loop ? -1 : 0);
}

void AudioManager::PlayAudio(const std::string& key, bool loop)
{
    PlayAudio(GetAudio(key), loop);
}

void AudioManager::SetMusicVolume(float volume)
{
    musicVolume = Clamp01(volume);
    AEAudioSetGroupVolume(musicGroup, musicVolume);
}

void AudioManager::SetSFXVolume(float volume)
{
    sfxVolume = Clamp01(volume);
    AEAudioSetGroupVolume(sfxGroup, sfxVolume);
}

float AudioManager::GetMusicVolume() const
{
    return musicVolume;
}

float AudioManager::GetSFXVolume() const
{
    return sfxVolume;
}

void AudioManager::StopAudio(AEAudio audioHandle)
{
    if (!AEAudioIsValidAudio(audioHandle))
        return;

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