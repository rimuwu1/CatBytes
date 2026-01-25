/* Start Header ************************************************************************/
/*!
\file       Background.cpp
\author     Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
\par        p.yuxuanlovette@digipen.edu
\date       January 24 2026
\brief      

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "pch.h"

#include "AEEngine.h"
#include "Background.h"
#include "Fonts.h"


// array to store level indicators
static const char* levelIndicator[backgroundSections]{

	"Level 1",
	"Level 2",
	"Level 3",
	"BOSS"

};

// variables for text display
static float textTimer = 0.0f;
static bool showText = false;
static int currentSection = Background_CurrentSection();
static int previousSection = -1;


void LevelIndicator_Initialize() {

	showText = false;
	textTimer = 0.0f;

}

void LevelIndicator_Show(int sectionIndex) {

	currentSection = sectionIndex;
	previousSection = sectionIndex;
	showText = true;
	textTimer = 0.0f;

}

void LevelIndicator_Update(float dt) {

	if (!showText) {

		return;

	}

	// update text timer
	if (showText) {

		textTimer += dt;

		// show text for 2 seconds
		if (textTimer >= 2.0f) {

			showText = false;

		}

	}

}

void LevelIndicator_Draw() {

	// draw text + text fade in/out
	if (showText && g_FontMedium != -1 && previousSection >= 0) {

		float totalTime = 2.0f;
		float fadeTime = 0.5f;

		float a = 1.0f;

		// level indicator fade in
		if (textTimer < fadeTime) {

			a = AEClamp(textTimer / fadeTime, 0.0f, 1.0f);

		}
		// level indicator fade out
		else if (textTimer > totalTime - fadeTime) {

			a = AEClamp((totalTime - textTimer) / fadeTime, 0.0f, 1.0f);

		}

		AEGfxPrint(g_FontMedium, levelIndicator[previousSection], 0.0f, 0.65f, 1.0f, 1.0f, 1.0f, 1.0f, a);

	}


}