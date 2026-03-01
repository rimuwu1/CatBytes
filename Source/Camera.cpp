/* Start Header ************************************************************************/
/*!
\file Camera.cpp
\author Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
\par kerwinjiajie.wong@digipen.edu
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

Camera globalCam;

void Camera_Init(Camera& cam, float startX, float startY)
{
	cam.x = startX;
	cam.y = startY;
	AEGfxSetCamPosition(cam.x, cam.y);
}

void Camera_FollowPlayer(Camera& cam, float /*playerX*/, float playerY, float dt)
{
	//cam.x += (playerX - cam.x) * followSpeed * dt;
	cam.x = 0.0f; // Fixed player cam.x

	const float followSpeed = 10.0f;
	const float verticalOffset = 50.0f;

	float targetY = playerY + verticalOffset;

	if (targetY > cam.y)
	{
		cam.y += (targetY - cam.y) * followSpeed * dt;
	}

	const float ground = -350.0f;
	const float groundHeight = 50.0f;
	const float halfScreenHeight = 900.0f * 0.5f;

	float groundTop = ground + groundHeight * 0.5f;
	float camera_start_y = groundTop + halfScreenHeight - groundHeight;

	if (cam.y < camera_start_y)
	{
		cam.y = camera_start_y;
	}
	
}

void Camera_Apply(const Camera& cam)
{
	AEGfxSetCamPosition(cam.x, cam.y);
}

void Camera_Debug(Camera& cam, float dt) {

	const float camSpeed = 500.0f;

	if (AEInputCheckCurr(AEVK_UP)) {

		cam.y += camSpeed * dt;

	}

	if (AEInputCheckCurr(AEVK_DOWN)) {

		cam.y -= camSpeed * dt;

	}

	if (AEInputCheckCurr(AEVK_LEFT)) {

		cam.x -= camSpeed * dt;

	}

	if (AEInputCheckCurr(AEVK_RIGHT)) {

		cam.x += camSpeed * dt;

	}

	Camera_Apply(cam);

}