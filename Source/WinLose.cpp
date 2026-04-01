/* Start Header *****/
/*!
\file       WinLose.cpp
\author     Sim Hui Min, Huimin, s.huimin, 2503506
            Tse Xuan Qi Tristin, tse.x, 2503757
\par        s.huimin@digipen.edu
            tse.x@digipen.edu
\date       February 01 2026
\brief      Implements the Win/Lose screen and the new-game intro cutscene.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header *****/

#include "pch.h"
#include "GameStateManager.h"
#include "GameStateList.h"
#include "WinLose.h"
#include "Fonts.h"
#include "AudioManager.h"
#include "TextureManager.h"
#include "MeshManager.h"
#include <fstream>
#include <string>
#include <vector>
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

// ============================================================
// external state
// ============================================================
const char* textScreenMessage = "You Lose";
extern bool g_playerDiedBefore;

// ============================================================
// textures
// ============================================================
static std::vector<AEGfxTexture*> s_SlideTex;
static AEGfxTexture*              s_WinTex  = nullptr;
static AEGfxTexture*              s_LoseTex = nullptr;
static std::vector<std::string>   s_SlideTexPaths;
static std::string                s_WinTexPath;
static std::string                s_LoseTexPath;

// ============================================================
// audio
// ============================================================
static AEAudio s_WinSound{};
static AEAudio s_LoseSound{};
static bool    s_SoundPlayed = false;

// ============================================================
// screen mode
// ============================================================
enum class WLMode { Cutscene, Win, Lose };
static WLMode s_Mode = WLMode::Lose;

// ============================================================
// slide definitions
// ============================================================
struct CutsceneSlide
{
    int         bgIdx;
    const char* lines[4];  
    int         lineCount;
};

static const float TEXT_TOP_Y     = -0.73f; // y axis of text from center
static const float TEXT_LINE_STEP =  0.08f; // spacing

static const CutsceneSlide SLIDES[] = // cutscene texts
{
    { 0, { "A year ago, someone precious was taken away from me.", nullptr, nullptr, nullptr }, 1 },
    { 1, { "My beautiful wife, a brilliant computer scientist,", "was captured by an evil tech corporation.", nullptr, nullptr }, 2 },
    { 2, { "I've searched city...", nullptr, nullptr, nullptr }, 1 },
    { 3, { "I've searched city...", "after city...", nullptr, nullptr }, 2 },
    { 4, { "I've searched city...", "after city...", "after city...", nullptr }, 3},
    { 4, { "Until I finally found her.", nullptr, nullptr }, 2 },
    { 5, { "In the city above the clouds..", nullptr, nullptr, nullptr }, 1 }, 
    { 5, { "at the top of that skyscraper.", nullptr, nullptr }, 2 },
    { 6, { "It won't be an easy climb.", "I'll have to make it to the top to reach it..", nullptr, nullptr }, 2 },
    { 6, { "Defeat whoever runs that greedy corporation to get her back.", nullptr, nullptr }, 2 },
    { 7, { "My intel tells me their top level is locked off.", nullptr, nullptr, nullptr }, 1 },
    { 7, { "I'll need to find a Computer Terminal to unlock it --", "one of it is specially made for the boss room.", nullptr, nullptr }, 2 },
    { 7, { "I'll find it, get her back.", nullptr, nullptr, nullptr }, 1 },
    { 7, { "Starting now.", nullptr, nullptr, nullptr }, 1 },
};
static const int SLIDE_COUNT = (int)(sizeof(SLIDES) / sizeof(SLIDES[0]));

static int  s_slideIdx     = 0;
static bool s_cutsceneDone = false;
static bool  s_fading    = false;
static float s_fadeAlpha = 0.0f;
static const float FADE_SPEED = 2.0f;

// ============================================================
// helpers
// ============================================================

static void DrawBG(AEGfxTexture* tex)
{
    if (!tex) return;
    float w = static_cast<float>(AEGfxGetWindowWidth());
    float h = static_cast<float>(AEGfxGetWindowHeight());
    // exactly as MainMenu draws g_MainMenuBG
    MeshManager::Get().DrawTexturedSquare(tex, 0.0f, 0.0f, w, h, 1.0f);
}

static void DrawFade(float alpha)
{
    if (alpha <= 0.0f) return;
    float w = static_cast<float>(AEGfxGetWindowWidth());
    float h = static_cast<float>(AEGfxGetWindowHeight());
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    MeshManager::Get().DrawSquare(0.0f, 0.0f, w, h, 0, 0, 0, alpha);
}

static void DrawCutsceneText(int idx)
{
    if (idx < 0 || idx >= SLIDE_COUNT) return;
    const CutsceneSlide& slide = SLIDES[idx];
    s8 font = FontManager::Get().GetMediumFont();

    for (int i = 0; i < slide.lineCount; ++i)
    {
        if (!slide.lines[i]) break;
        FontManager::Get().PrintCentered(font,
            slide.lines[i],
            0.0f, TEXT_TOP_Y - (float)i * TEXT_LINE_STEP, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f);
    }

    // skip hint -- upper left
    /*FontManager::Get().PrintCentered(font,
        "LSHIFT to skip || Space/Click to continue",
        -0.55f, 0.88f, 0.6f,
        0.5f, 0.5f, 0.5f, 0.8f);*/


    // upper left skip hint - separated for the color emphasis
    FontManager::Get().PrintCentered(font,
        "LSHIFT",
        -0.85f, 0.88f, 0.6f,
        0.5f, 0.4f, 0.75f, 0.9f);   // teal

    FontManager::Get().PrintCentered(font,
        "to skip  -",
        -0.68f, 0.88f, 0.6f,
        0.5f, 0.5f, 0.5f, 0.8f);    // grey

    FontManager::Get().PrintCentered(font,
        "Space/Click",
        -0.49f, 0.88f, 0.6f,
        1.0f, 0.6f, 0.75f, 0.9f);   // pink

    FontManager::Get().PrintCentered(font,
        "to continue",
        -0.25f, 0.88f, 0.6f,
        0.5f, 0.5f, 0.5f, 0.8f);    // grey
}

// ============================================================
// WinLose_Load -- one-time, just read paths from config
// ============================================================
void WinLose_Load()
{
    // defaults matching GameConfig.json cutscene_images block
    s_WinTexPath  = "Assets/Images/F8.png";
    s_LoseTexPath = "Assets/Images/F9.png";
    s_SlideTexPaths.clear();
    for (int i = 0; i < 8; ++i)
        s_SlideTexPaths.push_back("Assets/Images/F" + std::to_string(i) + ".png");

    std::ifstream ifs("Assets/Data/GameConfig.json");
    if (ifs.is_open())
    {
        rapidjson::IStreamWrapper isw(ifs);
        rapidjson::Document doc;
        doc.ParseStream(isw);
        if (!doc.HasParseError() && doc.IsObject()
            && doc.HasMember("cutscene_images")
            && doc["cutscene_images"].IsObject())
        {
            const auto& ci = doc["cutscene_images"];
            if (ci.HasMember("slides") && ci["slides"].IsArray())
            {
                s_SlideTexPaths.clear();
                for (const auto& v : ci["slides"].GetArray())
                    if (v.IsString())
                        s_SlideTexPaths.push_back(v.GetString());
            }
            if (ci.HasMember("win")  && ci["win"].IsString())  s_WinTexPath  = ci["win"].GetString();
            if (ci.HasMember("lose") && ci["lose"].IsString()) s_LoseTexPath = ci["lose"].GetString();
        }
    }
}

// ============================================================
// WinLose_Initialize -- called every entry, mirrors MainMenu_Initialize
// ============================================================
void WinLose_Initialize()
{
    s_SoundPlayed  = false;
    s_slideIdx     = 0;
    s_cutsceneDone = false;
    s_fading       = false;
    s_fadeAlpha    = 0.0f;

    // load audio bank 
    {
        auto tryLoad = [](const char* path) -> bool {
            std::ifstream f(path);
            if (!f.is_open()) return false;
            rapidjson::IStreamWrapper isw(f);
            rapidjson::Document doc;
            doc.ParseStream(isw);
            if (doc.HasParseError() || !doc.IsObject()) return false;
            if (!doc.HasMember("audio") || !doc["audio"].IsObject()) return false;
            AudioManager::Get().LoadFromJson(doc["audio"]);
            return true;
            };
        if (!tryLoad("Assets/Data/GameSave.json"))
            tryLoad("Assets/Data/GameConfig.json");
    }

    s_WinSound  = AudioManager::Get().GetAudio("win_effect");
    s_LoseSound = AudioManager::Get().GetAudio("lose_effect");

    // load textures every entry -- mirrors MainMenu_Initialize loading 
    for (int i = 0; i < (int)s_SlideTex.size(); ++i)
        if (s_SlideTex[i]) TextureManager::Get().UnloadTexture(s_SlideTexPaths[i].c_str());
    s_SlideTex.clear();

    if (s_WinTex)  { TextureManager::Get().UnloadTexture(s_WinTexPath.c_str());  s_WinTex  = nullptr; }
    if (s_LoseTex) { TextureManager::Get().UnloadTexture(s_LoseTexPath.c_str()); s_LoseTex = nullptr; }

    for (const auto& path : s_SlideTexPaths)
        s_SlideTex.push_back(TextureManager::Get().LoadTexture(path.c_str()));

    s_WinTex  = TextureManager::Get().LoadTexture(s_WinTexPath.c_str());
    s_LoseTex = TextureManager::Get().LoadTexture(s_LoseTexPath.c_str());

    if (strcmp(textScreenMessage, "Cutscene") == 0)
        s_Mode = WLMode::Cutscene;
    else if (strcmp(textScreenMessage, "You Win") == 0)
        s_Mode = WLMode::Win;
    else
        s_Mode = WLMode::Lose;
}

// ============================================================
// WinLose_Update
// ============================================================
void WinLose_Update()
{
    float dt = (float)AEFrameRateControllerGetFrameTime();

    if (s_Mode == WLMode::Cutscene)
    {
        if (s_fading)
        {
            s_fadeAlpha += dt * FADE_SPEED;
            if (s_fadeAlpha >= 1.0f)
            {
                textScreenMessage = "You Lose";
                GameStateManager::Get().next = GS_MAINGAME;
            }
            return;
        }

        if (AEInputCheckTriggered(AEVK_LSHIFT))
        {
            s_cutsceneDone = true;
            s_fading       = true;
            s_fadeAlpha    = 0.0f;
            return;
        }

        if (AEInputCheckTriggered(AEVK_SPACE) || AEInputCheckTriggered(VK_LBUTTON))
        {
            s_slideIdx++;
            if (s_slideIdx >= SLIDE_COUNT)
            {
                s_cutsceneDone = true;
                s_fading       = true;
                s_fadeAlpha    = 0.0f;
            }
        }
        return;
    }

    if (!s_SoundPlayed)
    {
        if (s_Mode == WLMode::Win)
            AudioManager::Get().PlayAudio(s_WinSound, false);
        else
            AudioManager::Get().PlayAudio(s_LoseSound, false);
        s_SoundPlayed = true;
    }

    if (AEInputCheckTriggered(AEVK_SPACE) || AEInputCheckTriggered(AEVK_RETURN))
        GameStateManager::Get().next = GS_MAINMENU;
}

// ============================================================
// WinLose_Draw
// ============================================================
void WinLose_Draw()
{
    AESysFrameStart();
    AEGfxSetCamPosition(0.0f, 0.0f);

    s8 medFont   = FontManager::Get().GetMediumFont();
    s8 largeFont = FontManager::Get().GetLargeFont();

    if (s_Mode == WLMode::Cutscene)
    {
        int safeIdx = (s_slideIdx < SLIDE_COUNT) ? s_slideIdx : SLIDE_COUNT - 1;
        int bgIdx   = SLIDES[safeIdx].bgIdx;

        if (bgIdx >= 0 && bgIdx < (int)s_SlideTex.size())
            DrawBG(s_SlideTex[bgIdx]);

        if (!s_cutsceneDone)
            DrawCutsceneText(safeIdx);

        DrawFade(s_fadeAlpha);

        AESysFrameEnd();
        return;
    }

    if (s_Mode == WLMode::Win)
    {
        DrawBG(s_WinTex);

        FontManager::Get().PrintCentered(largeFont,
            "You Win",
            0.0f, TEXT_TOP_Y + 1.45f, 1.0f,
            1.0f, 0.6f, 0.7f, 1.0f);

        FontManager::Get().PrintCentered(medFont,
            "Press Space to return",
            0.0f, TEXT_TOP_Y - TEXT_LINE_STEP, 0.7f,
            0.5f, 0.5f, 0.5f, 0.8f);

        AESysFrameEnd();
        return;
    }

    // lose
    DrawBG(s_LoseTex);

    FontManager::Get().PrintCentered(medFont,
        "You lost all your lives.. but you can't give up.",
        0.0f, TEXT_TOP_Y, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f);

    FontManager::Get().PrintCentered(medFont,
        "You won't.",
        0.0f, TEXT_TOP_Y - TEXT_LINE_STEP, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f);

    FontManager::Get().PrintCentered(medFont,
        "Press Space to return",
        0.0f, TEXT_TOP_Y - TEXT_LINE_STEP * 2.0f, 0.7f,
        0.5f, 0.5f, 0.5f, 0.8f);

    AESysFrameEnd();
}

// ============================================================
// WinLose_Free
// ============================================================
void WinLose_Free()
{
}

// ============================================================
// WinLose_Unload
// ============================================================
void WinLose_Unload()
{
    for (int i = 0; i < (int)s_SlideTex.size(); ++i)
        if (s_SlideTex[i]) 
            TextureManager::Get().UnloadTexture(s_SlideTexPaths[i].c_str());
    s_SlideTex.clear();
    s_SlideTexPaths.clear();

    if (s_WinTex)  { TextureManager::Get().UnloadTexture(s_WinTexPath.c_str());  s_WinTex  = nullptr; }
    if (s_LoseTex) { TextureManager::Get().UnloadTexture(s_LoseTexPath.c_str()); s_LoseTex = nullptr; }
}