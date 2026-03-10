/* Start Header ************************************************************************/
/*!
\file SpriteSheet.cpp
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 19/02/2026
\brief Implements the SpriteSheet class. Loads a spritesheet via TextureManager
       and cycles through animation frames using UV offsets derived from the
       configured row/column layout.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "SpriteSheet.h"

// -----------------------------------------------------------------------------
SpriteSheet::SpriteSheet(const std::string& filepath,
    u32 rows, u32 cols, u32 maxSprites, f32 frameDuration)
{
    this->rows = rows;
    this->cols = cols;
    this->maxSprites = (maxSprites == 0) ? rows * cols : maxSprites;
    this->frameDuration = frameDuration;
    this->animTimer = 0.f;
    this->currentFrame = 0;
    this->uvOffsetX = 0.f;
    this->uvOffsetY = 0.f;
    this->isPaused = false;

    spriteUVWidth = 1.f / static_cast<f32>(cols);
    spriteUVHeight = 1.f / static_cast<f32>(rows);

    pTexture = TextureManager::Get().LoadTexture(filepath);
    std::string defaultName = "default";
    clips[defaultName] = { 0, maxSprites - 1, frameDuration, true };
    Play(defaultName);

    RecalculateUV();
}

// -----------------------------------------------------------------------------
SpriteSheet::SpriteSheet(const SpriteSheet& other)
    : rows(other.rows), cols(other.cols), maxSprites(other.maxSprites),
    frameDuration(other.frameDuration), animTimer(0.f), currentFrame(0),
    currentClip(""), playing(false), isPaused(false),
    spriteUVWidth(other.spriteUVWidth), spriteUVHeight(other.spriteUVHeight),
    uvOffsetX(0.f), uvOffsetY(0.f), pTexture(other.pTexture)
{
    clips = other.clips;   // copy all clip definitions
    // Animation is intentionally reset; caller must Play() the desired clip.
}

// -----------------------------------------------------------------------------
float SpriteSheet::GetClipDuration(const std::string& name) const {
    auto it = clips.find(name);
    if (it != clips.end()) {
        return it->second.frameDuration;
    }
    return 0.f;
}

// -----------------------------------------------------------------------------
float SpriteSheet::GetClipTotalDuration(const std::string& name) const {
    auto it = clips.find(name);
    if (it != clips.end()) {
        const Clip& clip = it->second;
        float numFrames = static_cast<float>(clip.endFrame - clip.startFrame + 1);
        return numFrames * clip.frameDuration;
    }
    return 0.f;
}

//add clip from overall spritesheet
void SpriteSheet::AddClip(const std::string& name, u32 start, u32 end,
    float duration, bool loop) {
    clips[name] = { start, end, duration, loop };
}

//play selected clip
void SpriteSheet::Play(const std::string& name, bool forceRestart) {
    auto it = clips.find(name);
    if (it == clips.end()) return;

    if (forceRestart || currentClip != name) {
        currentClip = name;
        currentFrame = it->second.startFrame;
        animTimer = 0.f;
        playing = true;
        RecalculateUV();
    }
}

void SpriteSheet::Stop() {
    playing = false;
}

// -----------------------------------------------------------------------------
void SpriteSheet::Update(f32 dt) {
    if (!playing || isPaused) return;

    auto it = clips.find(currentClip);
    if (it == clips.end()) return;

    const Clip& clip = it->second;
    animTimer += dt;

    if (animTimer >= clip.frameDuration) {
        animTimer = 0.f;
        currentFrame++;

        if (currentFrame > clip.endFrame) {
            if (clip.loop) {
                currentFrame = clip.startFrame;
            }
            else {
                currentFrame = clip.endFrame;  // hold last frame
                playing = false;                // optional: stop animation
            }
        }
        RecalculateUV();
    }
}

// -----------------------------------------------------------------------------
void SpriteSheet::Reset()
{
    animTimer = 0.f;
    currentFrame = 0;
    RecalculateUV();
}

// -----------------------------------------------------------------------------
void SpriteSheet::RecalculateUV()
{
    u32 row = currentFrame / cols;
    u32 col = currentFrame % cols;

    uvOffsetX = spriteUVWidth * static_cast<f32>(col);
    uvOffsetY = spriteUVHeight * static_cast<f32>(row);
}