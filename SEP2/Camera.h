/* Start Header ************************************************************************/
/*!
\file Camera.h
\author Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par kerwinajijie.wong@digipen.edu
\date January, 25, 2026
\brief This file contains the function declarations for Camera Handling, which is used
		to control the view position in the game.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once

struct Camera
{
	float x;
	float y;
};

void Camera_Init(Camera& cam, float startX, float startY);
void Camera_FollowPlayer(Camera& cam, float playerX, float playerY, float dt);
void Camera_Apply(const Camera& cam);