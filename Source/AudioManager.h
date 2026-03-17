/* Start Header ************************************************************************/
/*!
\file AudioManager.h
\author Tse Xuan Qi Tristin, tse.x, 2503757
\par tse.x@digipen.edu
\date Junuary, 24, 2026
\brief handles loading, caching, and unloading of audio resources and ensures each audio file 
is created once and owned in a single location.

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

    // Load/unload audio by filepath
    AEAudio LoadAudio(const std::string& filepath, bool isMusic = false);
    void UnloadAudio(AEAudio audioHandle);
    void UnloadAll();

    // Load audio from JSON
    void LoadFromJson(const rapidjson::Value& audioConfig);

    // Access by key
    AEAudio GetAudio(const std::string& key) const;

    // Play/Stop
    void PlayAudio(AEAudio audioHandle, bool loop = false);
    void PlayAudio(const std::string& key, bool loop = false);
    void StopAudio(AEAudio audioHandle);

private:
    AudioManager();
    ~AudioManager();

    std::map<std::string, AEAudio> audioMap;// filepath, AEAudio
    std::map<std::string, AEAudio> audioKeyMap; // json key, AEAudio
    AEAudioGroup musicGroup;
};