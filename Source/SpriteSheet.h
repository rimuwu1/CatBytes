/* Start Header ************************************************************************/
/*!
\file SpriteSheet.h
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 19/02/2026
\brief Declares the SpriteSheet class for managing and animating spritesheets.
       Uses TextureManager to load textures and cycles through frames using
       row/column UV offsets.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once

#include "AEEngine.h"
#include "TextureManager.h"
#include <string>
#include <unordered_map>

// -----------------------------------------------------------------------------
// SpriteSheet class
// 
// Loads a texture containing multiple frames arranged in a grid (rows x cols).
// Provides clip-based animation: define named clips with start/end frames,
// then play them. The class calculates UV offsets for the current frame so
// that MeshManager can render just that portion of the texture.
// 
// Typical usage:
//   1. Create a SpriteSheet with image path, rows, cols.
//   2. Add clips (animation sequences) via AddClip().
//   3. Call Play("clipname") to start an animation.
//   4. Every frame, call Update(dt) to advance the frame.
//   5. In rendering, use GetTexture(), GetUVOffsetX/Y(), GetSpriteUVWidth/Height()
//      to set up the correct texture coordinates.
// 
// MeshManager::DrawSpriteSheet() handles step 5 automatically.
// -----------------------------------------------------------------------------
class SpriteSheet
{
public:
    // -------------------------------------------------------------------------
    // Clip: defines a single animation sequence within the spritesheet.
    // -------------------------------------------------------------------------
    struct Clip {
        u32 startFrame;       // first frame index (0‑based)
        u32 endFrame;         // last frame index (inclusive)
        float frameDuration;  // seconds each frame is displayed
        bool loop;            // whether to restart after endFrame
    };

    // -------------------------------------------------------------------------
    // Constructor
    // filepath        - path to the spritesheet image (loaded via TextureManager)
    // rows            - number of rows in the spritesheet grid
    // cols            - number of columns in the spritesheet grid
    // maxSprites      - total number of valid frames; if 0, defaults to rows*cols.
    //                   Use this if the spritesheet has empty cells.
    // frameDuration   - default seconds per frame (can be overridden per clip)
    // -------------------------------------------------------------------------
    SpriteSheet(const std::string& filepath,
        u32 rows,
        u32 cols,
        u32 maxSprites = 0,
        f32 frameDuration = 0.1f);

    // -------------------------------------------------------------------------
    // Update the animation timer and advance to the next frame if enough time
    // has elapsed. Call once per frame while the animation is playing.
    // -------------------------------------------------------------------------
    void Update(f32 dt);

    // -------------------------------------------------------------------------
    // Add a named animation clip.
    // name           - identifier used to play this clip later
    // start, end     - first and last frame indices (inclusive)
    // duration       - time per frame for this clip
    // loop           - true to loop, false to stop at endFrame
    // -------------------------------------------------------------------------
    void AddClip(const std::string& name, u32 start, u32 end,
        float duration = 0.1f, bool loop = true);

    // -------------------------------------------------------------------------
    // Switch to a named clip.
    // forceRestart   - if true, always restart from startFrame even if the
    //                  same clip is already playing. If false, a currently
    //                  playing clip will continue uninterrupted.
    // -------------------------------------------------------------------------
    void Play(const std::string& name, bool forceRestart = false);

    // -------------------------------------------------------------------------
    // Stop animation playback; the current frame is frozen.
    // -------------------------------------------------------------------------
    void Stop();

    // -------------------------------------------------------------------------
    // Reset the current clip to its first frame and restart playback.
    // -------------------------------------------------------------------------
    void Reset();

    // --- Getters used by MeshManager / draw calls ---
    AEGfxTexture* GetTexture()       const { return pTexture; }
    f32           GetUVOffsetX()     const { return uvOffsetX; }
    f32           GetUVOffsetY()     const { return uvOffsetY; }
    f32           GetSpriteUVWidth() const { return spriteUVWidth; }
    f32           GetSpriteUVHeight()const { return spriteUVHeight; }
    u32           GetCurrentFrame()  const { return currentFrame; }
    float         GetClipDuration(const std::string& name) const;          // duration per frame
    float         GetClipTotalDuration(const std::string& name) const;    // total time of clip
    std::string   GetCurrentClip()   const { return currentClip; }
    bool          IsPlaying()        const { return playing; }

    // --- Speed control ---
    void SetFrameDuration(f32 duration) { frameDuration = duration; }
    f32  GetFrameDuration()         const { return frameDuration; }

    // --- Pause / resume ---
    void SetPaused(bool paused) { isPaused = paused; }
    bool IsPaused()             const { return isPaused; }

private:
    AEGfxTexture* pTexture;          // texture loaded by TextureManager

    u32 rows;                         // number of rows in grid
    u32 cols;                         // number of columns in grid
    u32 maxSprites;                    // total valid frames

    f32 spriteUVWidth;                 // 1.0f / cols  (width of one cell in UV space)
    f32 spriteUVHeight;                // 1.0f / rows  (height of one cell in UV space)

    f32 animTimer;                      // accumulated time for current frame
    f32 frameDuration;                  // default seconds per frame
    u32 currentFrame;                   // current frame index (0‑based)

    f32 uvOffsetX;                      // u offset of current frame
    f32 uvOffsetY;                      // v offset of current frame

    bool isPaused;                      // if true, Update does not advance frames

    // Clip management
    std::unordered_map<std::string, Clip> clips;   // named clips
    std::string currentClip;                        // name of active clip
    bool playing;                                   // whether animation is playing

    // -------------------------------------------------------------------------
    // Recalculate uvOffsetX/Y from currentFrame, rows, cols.
    // Called automatically when the frame changes.
    // -------------------------------------------------------------------------
    void RecalculateUV();
};