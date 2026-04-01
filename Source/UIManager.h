/* Start Header ************************************************************************/
/*!
\file UIManager.h
\author Joash ng, joash.ng, 2502780
\par joash.ng@digipen.edu
\date 03/03/2026
\brief This file declares the UIManager class which is declared as a singleton instance and handles popup and pause menus that overlay other gamestates

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/* End Header **************************************************************************/
#pragma once
#include <string>
#include <functional>

class UIManager {
public:
    static UIManager& Get() {
        static UIManager instance;
        return instance;
    }

    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;

    void ShowConfirmation(const std::string& title,
        const std::string& message,
        std::function<void()> onConfirm,
        std::function<void()> onCancel);

    void ShowPause();
    void UIManager::HidePause() { m_PauseActive = false; }
    bool UIManager::IsPauseActive() const { return m_PauseActive; }
    bool UIManager::IsActive() const { return m_PopupActive || m_PauseActive; }

    bool Update(float camX, float camY);
    void Draw(float camX, float camY);
    void Reset();

private:
    UIManager() = default;

    struct PopupData {
        std::string title;
        std::string message;
        std::function<void()> onConfirm;
        std::function<void()> onCancel;
    };
    PopupData m_Popup;
    bool m_PopupActive = false;

    bool m_PauseActive = false;
    int  m_PauseHovered = 0;
    int  m_PausePrevHov = 0;

    bool IsMouseOverButton(float btnX, float btnY, float btnW, float btnH, float camX, float camY) const;
    void UpdateConfirmation(float camX, float camY, bool& consumed);
    void UpdatePause(float camX, float camY, bool& consumed);
    void DrawConfirmation(float camX, float camY);
    void DrawPause(float camX, float camY);
};