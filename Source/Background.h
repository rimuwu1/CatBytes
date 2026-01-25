/* Start Header ************************************************************************/
/*!
\file       Background.h
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

#pragma once

constexpr int backgroundSections = 4;

extern float sectionHeight[];

extern float fakeCamY;

extern int previousSection;

struct Colour {

	float r, g, b, a;

};

// initialise background system
void Background_Initialise();

// update background based on camera panning's y-axis
void Background_Update(float cameraYAxis);

// draw background
void Background_Draw();

// free background resources
void Background_Free();

// get current section
int Background_CurrentSection();