/* Start Header ************************************************************************/
/*!
\file AudioManager.h
\author Tse Xuan Qi Tristin, tse.x, 2503757
\par tse.x@digipen.edu
\date January, 24, 2026
\brief Manages loading, caching, playback, and unloading of audio resources.
Ensures each audio file is created once and centrally owned.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

#include <map>
#include <string>
#include "AEAudio.h"
#include "rapidjson/document.h"

class AudioManager
{
public:
    static AudioManager& Get();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // Audio resource management, load/unload audio by filepath
    AEAudio LoadAudio(const std::string& filepath, bool isMusic = false);
    void UnloadAudio(AEAudio audioHandle);
    void UnloadAll();

    // Configuration loading, oad audio from JSON
    void LoadFromJson(const rapidjson::Value& audioConfig);

    // Audio retrieval, access by key
    AEAudio GetAudio(const std::string& key) const;

    // Playback control
    void PlayAudio(AEAudio audioHandle, bool loop = false);
    void PlayAudio(const std::string& key, bool loop = false);
    void StopAudio(AEAudio audioHandle);

    // Volume control
    void SetMusicVolume(float volume);
    void SetSFXVolume(float volume);

    float GetMusicVolume() const;
    float GetSFXVolume() const;

private:
    AudioManager();

    std::map<std::string, AEAudio> audioMap;// maps file paths to loaded audio handles
    std::map<std::string, AEAudio> audioKeyMap;//maps JSON keys to audio handles for easy lookup

    AEAudioGroup musicGroup;
    AEAudioGroup sfxGroup;

    float musicVolume;
    float sfxVolume;
};