#include "pch.h"
#include "MainMenu.h"
#include "GameStateManager.h"
#include "AEEngine.h"
#include "Fonts.h"

extern int current;
extern int previous;
extern int next;

static int frameCounter = 0;
static int g_hoveredButton = 0;

// main menu buttons
const float BUTTON_X = -0.9f;           // left-aligned with title
const float BUTTON_START_Y = 0.3f;      
const float BUTTON_SPACING = 0.15f;     // space between buttons
const float BUTTON_WIDTH = 0.4f;        // approximate width for click detection
const float BUTTON_HEIGHT = 0.1f;

void MainMenu_Load()
{
}

void MainMenu_Initialize()
{
    frameCounter = 0;
}

void MainMenu_Update()
{ 
    frameCounter++;

    // get mouse position
    s32 mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);
    float windowWidth = (float)AEGfxGetWindowWidth();
    float windowHeight = (float)AEGfxGetWindowHeight();
    float normalizedX = ((float)mouseX / windowWidth) * 2.0f - 1.0f;
    float normalizedY = 1.0f - ((float)mouseY / windowHeight) * 2.0f;
    normalizedX *= 0.5f;
    normalizedY *= 0.5f;

    // hover effect
    g_hoveredButton = 0;

    // PLAY button
    if (normalizedX >= -0.9f && normalizedX <= -0.3f &&
        normalizedY >= 0.03f && normalizedY <= 0.10f) // manually adjust the width n range here -> diff is the range (currently 0.07 diff)
    {
        g_hoveredButton = 1;  
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            next = GS_LEVEL1;
        }
    }
    // CONTROLS button
    else if (normalizedX >= -0.9f && normalizedX <= -0.3f &&
        normalizedY >= -0.06f && normalizedY <= 0.01f) // manually adjust the width n range here -> diff is the range
    {
        g_hoveredButton = 2;  
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            // TODO: Controls state
        }
    }
    // CREDITS button
    else if (normalizedX >= -0.9f && normalizedX <= -0.3f &&
        normalizedY >= -0.14f && normalizedY <= -0.07f) // manually adjust the width n range here -> diff is the range
    {
        g_hoveredButton = 3; 
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            // TODO: Credits state
        }
    }
    // EXIT button
    else if (normalizedX >= -0.9f && normalizedX <= -0.3f &&
        normalizedY >= -0.20f && normalizedY <= -0.13f) // manually adjust the width n range here -> diff is the range
    {
        g_hoveredButton = 4; 
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            next = GS_QUIT;
        }
    }
}

void MainMenu_Draw()
{
    AESysFrameStart();
    AEGfxSetBackgroundColor(0.5f, 0.5f, 0.5f);  

    if (g_FontLarge != -1 && g_FontMedium != -1)
    {
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        // awesome title #noproblem
        AEGfxPrint(g_FontLarge, "POGMA", BUTTON_X, 0.3f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

        // PLAY - white if hovered, grey otherwise
        float playR = (g_hoveredButton == 1) ? 1.0f : 0.6f;
        float playG = (g_hoveredButton == 1) ? 1.0f : 0.6f;
        float playB = (g_hoveredButton == 1) ? 1.0f : 0.6f;
        AEGfxPrint(g_FontMedium, "PLAY", BUTTON_X, 0.1f, 1.0f, playR, playG, playB, 1.0f);

        // CONTROLS
        float controlR = (g_hoveredButton == 2) ? 1.0f : 0.6f;
        float controlG = (g_hoveredButton == 2) ? 1.0f : 0.6f;
        float controlB = (g_hoveredButton == 2) ? 1.0f : 0.6f;
        AEGfxPrint(g_FontMedium, "CONTROLS", BUTTON_X, -0.05f, 1.0f, controlR, controlG, controlB, 1.0f);

        // CREDITS
        float creditR = (g_hoveredButton == 3) ? 1.0f : 0.6f;
        float creditG = (g_hoveredButton == 3) ? 1.0f : 0.6f;
        float creditB = (g_hoveredButton == 3) ? 1.0f : 0.6f;
        AEGfxPrint(g_FontMedium, "CREDITS", BUTTON_X, -0.2f, 1.0f, creditR, creditG, creditB, 1.0f);

        // EXIT
        float exitR = (g_hoveredButton == 4) ? 1.0f : 0.6f;
        float exitG = (g_hoveredButton == 4) ? 1.0f : 0.6f;
        float exitB = (g_hoveredButton == 4) ? 1.0f : 0.6f;
        AEGfxPrint(g_FontMedium, "EXIT", BUTTON_X, -0.35f, 1.0f, exitR, exitG, exitB, 1.0f);
    }

    AESysFrameEnd();
}

void MainMenu_Free()
{
}

void MainMenu_Unload()
{
}