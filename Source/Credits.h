/* Start Header ************************************************************************/
/*!
\file Credits.h
\author Tse Xuan Qi Tristin, tse.x, 2503757
\par tse.x@digipen.edu
\date January, 24, 2026
\brief Declares functions for the Credits screen, including loading,
initialization, update, rendering, and cleanup of the credits scene.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

// Initialization and cleanup
void Credits_Load();
void Credits_Initialize();
void Credits_Free();
void Credits_Unload();

// Update and rendering
void Credits_Update();
void Credits_Draw();