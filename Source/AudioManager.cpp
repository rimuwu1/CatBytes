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

AudioManager& AudioManager::Get()
{
    static AudioManager instance;
    return instance;
}

AudioManager::AudioManager()
{
    musicGroup = AEAudioCreateGroup();  //create music group
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

    AEAudio handle;

    if (isMusic)
        handle = AEAudioLoadMusic(filepath.c_str());
    else
        handle = AEAudioLoadSound(filepath.c_str());

    audioMap[filepath] = handle;

    std::cout << "Loaded audio: " << filepath << std::endl;
    return handle;
}

void AudioManager::UnloadAudio(AEAudio audioHandle)
{
    AEAudioUnloadAudio(audioHandle);

    // remove from map
    for (auto it = audioMap.begin(); it != audioMap.end(); )
    {
        if (it->second.fmod_sound  == audioHandle.fmod_sound)
        {
            it = audioMap.erase(it); //erase returns next iterator
        }
        else
        {
            ++it;
        }
    }
}

void AudioManager::UnloadAll()
{
    for (auto it = audioMap.begin(); it != audioMap.end(); ++it)
    {
        AEAudioUnloadAudio(it->second);
    }
    audioMap.clear();
}

void AudioManager::PlayAudio(AEAudio audioHandle, bool loop)
{
    AEAudioPlay(audioHandle, musicGroup, 1.0f, 1.0f, loop ? -1 : 0);
}

void AudioManager::StopAudio(AEAudio /*audioHandle*/)
{
    AEAudioStopGroup(musicGroup);// stops entire group
}