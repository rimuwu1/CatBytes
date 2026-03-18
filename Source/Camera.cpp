/* Start Header ************************************************************************/
/*!
\file	Camera.cpp
\author Kerwin Wong Jia Jie, kerwinjiajie.wong, 2502740
		Peh Yu Xuan, Lovette, p.yuxuanlovette, 2502079
		Joash ng, joash.ng, 2502780
\par	kerwinjiajie.wong@digipen.edu
		p.yuxuanlovette@digipen.edu
		joash.ng@digipen.edu
\date	January, 25, 2026
\brief	This file contains the function definitions for Camera Handling, which is used
		to control the view position in the game.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#include "Camera.h"
#include "AEEngine.h"
#include <cmath>
#include <algorithm>

Camera globalCam;
float camTrauma   = 0.0f;
float camShakeTime = 0.0f;

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

	//// Fixed camera y positon
	//if (targetY > cam.y)
	//{
	//	cam.y += (targetY - cam.y) * followSpeed * dt;
	//}

	cam.y += (targetY - cam.y) * followSpeed * dt;

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

	Camera_Apply(cam);

}

void Camera_AddTrauma(float amount) {
    camTrauma = (std::min)(camTrauma + amount, 1.0f);
}

void Camera_UpdateShake(Camera& cam, float dt) {
    if (camTrauma <= 0.0f) return;
    camTrauma -= dt * 1.5f;
    if (camTrauma < 0.0f) camTrauma = 0.0f;
    float shake = camTrauma * camTrauma;
    camShakeTime += dt * 30.0f;
    cam.x += sinf(camShakeTime * 1.1f) * shake * 20.0f;
    cam.y += cosf(camShakeTime * 0.9f) * shake * 20.0f;
}

// Camera pan sequence state
bool  g_camSequenceActive   = false;
float g_camSequenceTimer    = 0.0f;
float g_camSequenceDuration = 0.6f;
float g_camHoldDuration     = 1.0f;
float g_camTargetY          = 0.0f;
float g_camReturnY          = 0.0f;

void Camera_StartSequence(float targetY, float currentY,
                          float panDuration, float holdDuration)
{
    g_camTargetY          = targetY;
    g_camReturnY          = currentY;
    g_camSequenceTimer    = 0.0f;
    g_camSequenceDuration = panDuration;
    g_camHoldDuration     = holdDuration;
    g_camSequenceActive   = true;
}

bool Camera_UpdateSequence(Camera& cam, float dt)
{
    if (!g_camSequenceActive) return false;

    g_camSequenceTimer += dt;
    float panDur  = g_camSequenceDuration;
    float holdDur = g_camHoldDuration;
    float totalDur = panDur + holdDur + panDur;

    cam.x = 0.0f; // always locked

    if (g_camSequenceTimer < panDur) {
        // phase 1: pan to target
        float t = g_camSequenceTimer / panDur;
        t = t * t * (3.0f - 2.0f * t); // smoothstep
        cam.y = g_camReturnY + (g_camTargetY - g_camReturnY) * t;

    } else if (g_camSequenceTimer < panDur + holdDur) {
        // phase 2: hold at target
        cam.y = g_camTargetY;

    } else {
        // phase 3: pan back to return position
        float t = (g_camSequenceTimer - panDur - holdDur) / panDur;
        t = t * t * (3.0f - 2.0f * t);
        cam.y = g_camTargetY + (g_camReturnY - g_camTargetY) * t;
    }

    if (g_camSequenceTimer >= totalDur) {
        g_camSequenceActive = false;
        cam.y = g_camReturnY;
        return false;
    }
    return true;
}