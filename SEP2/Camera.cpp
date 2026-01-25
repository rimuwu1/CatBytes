/* Start Header ************************************************************************/
/*!
\file Camera.cpp
\author Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par kerwinajijie.wong@digipen.edu
\date January, 25, 2026
\brief This file contains the function definitions for Camera Handling, which is used
		to control the view position in the game.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "Camera.h"
#include "AEEngine.h"

void Camera_Init(Camera& cam, float startX, float startY)
{
	cam.x = startX;
	cam.y = startY;
	AEGfxSetCamPosition(cam.x, cam.y);
}

void Camera_FollowPlayer(Camera& cam, float playerX, float playerY, float dt)
{
	float followSpeed = 10.0f;
	cam.x += (playerX - cam.x) * followSpeed * dt;
	cam.y += (playerY - cam.y) * followSpeed * dt;
}

void Camera_Apply(const Camera& cam)
{
	AEGfxSetCamPosition(cam.x, cam.y);
}