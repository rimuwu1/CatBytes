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

#include "Background.h"
#include "AEEngine.h"

// !! FOR TESTING BACKGROUND; MIGHT REMOVE IN FUTURE !!
static float minY = 0.0f;
static float maxY = 800.0;

float debugCamY = 0.0f;
int previousSection = -1;

// --------------------------------------------------BACKGROUND SECTIONS START--------------------------------------------------

float sectionHeight[backgroundSections]{

	// level 1 height limit
	500.0f,

	// level 2 height limit
	1000.0f,

	// level 3 height limit
	1500.0f,

	// level 4 height limit
	2000.0f

};

static int currentSection = 0;

// --------------------------------------------------BACKGROUND SECTIONS END--------------------------------------------------

// --------------------------------------------------BACKGROUND COLOURS START--------------------------------------------------

// helper function to normalise RGB values
static constexpr Colour rgb(unsigned int r, unsigned int g, unsigned int b) {

	return Colour{

		static_cast<float>(r) / 255.0f,
		static_cast<float>(g) / 255.0f,
		static_cast<float>(b) / 255.0f,
		1.0f

	};

}

// array to store background colours
static Colour backgroundColours[backgroundSections] = {

	// level 1: pink
	rgb(255, 213, 196),

	// level 2: yellow
	rgb(255, 227, 187),

	// level 3: blue
	rgb(81, 97, 145),

	// level 4: purple
	rgb(60, 48, 89)

};

// current background colour (starts with pink)
static Colour currentColour = backgroundColours[0];

// function for smooth colour blending
static Colour blendColours(const Colour& first, const Colour& second, float fraction) {

	return Colour{

		// red colour blending
		first.r + (second.r - first.r) * fraction,

		// green colour blending
		first.g + (second.g - first.g) * fraction,

		// blue colour blending
		first.b + (second.b - first.b) * fraction,

		// alpha
		first.a + (second.a - first.a) * fraction

	};

}

// --------------------------------------------------BACKGROUND COLOURS END--------------------------------------------------

// initialise background system
void Background_Initialise() {

	// initial background colour: pink
	currentColour = backgroundColours[0];

}

// update background based on camera panning's y-axis
void Background_Update(float cameraYAxis) {

	// !!	FAKE "CAMERA"; TO BE EDITED ONCE CAMERA IS IN !!
	static float fakeCam = minY;

	// find current section based on cam
	int index = 0;

	for (int i = 0; i < backgroundSections; i++) {

		if (cameraYAxis <= sectionHeight[i]) {

			index = i;
			break;

		}

	}

	currentSection = index;

	// find lower & upper bound of section
	float lowerBound = (index == 0) ? minY : sectionHeight[index - 1];
	float upperBound = sectionHeight[index];

	// find fraction for colour blend
	float colourBlend = (cameraYAxis - lowerBound) / (upperBound - lowerBound);

	// blend colours
	if (index < backgroundSections - 1) {

		currentColour = blendColours(backgroundColours[index],
									 backgroundColours[index + (index < backgroundSections - 1 ? 1 : 0)], // ternary operator: prevents array overflow
									 colourBlend);

	}
	else {

		// no blending needed for level 4
		currentColour = backgroundColours[index];

	}

}

// draw background
void Background_Draw() {

	// draw in colours
	AEGfxSetBackgroundColor(currentColour.r, currentColour.g, currentColour.b);

}

// free background resources
void Background_Free() {

}

void Background_Reset(float startCamY, int startSection) {

	debugCamY = startCamY;
	currentSection = startSection;
	previousSection = -1;
	currentColour = backgroundColours[startSection];

}

// get current section
int Background_CurrentSection() {

	return currentSection;

}