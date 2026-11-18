// launcher.h -- the launcher window: choose the game folder, the graphics and the sound, the game's own
// switches, the window options and the Enhanced picture, then Play. Game settings > Keys and gamepad
// shows the keys and the Enhanced gear keys and gamepad options in the window.
#pragma once

#include <wx/frame.h>

#include "game.h"

class wxButton;
class wxCheckBox;
class wxChoice;
class wxRadioBox;
class wxSimplebook;
class wxSpinCtrl;
class wxStaticBitmap;
class wxStaticText;
class wxTextCtrl;

extern const char* const APP_TITLE;

class LauncherFrame : public wxFrame {
public:
    LauncherFrame();

private:
    wxWindow* CreateMainPage(wxWindow* parent);
    wxWindow* CreateKeysPage(wxWindow* parent);
    void CreateMenus();
#ifdef __WXMSW__
    WXLRESULT MSWWindowProc(WXUINT msg, WXWPARAM wParam, WXLPARAM lParam) override;
#endif
    void FitPage();
    // The Keys page in place of the main one, and back.
    void OpenKeys();
    void CloseKeys();
    void Preferences();
    void SetTopmost(bool on);

    // Checks the folder and the program again and enables Play and the controls to match.
    void UpdateState();
    // Enables the Enhanced controls to match Graphics (enhanced / original) and the gear keys box.
    void UpdateEnhanced();
    // Lists the anti-aliasing choices the resolution allows (res-scale x aa <= 16), keeping `aa` if it can.
    void FillAntiAliasing(int aa);
    // The options as the window shows them.
    GameOptions EditedOptions() const;
    void BrowseFolder();
    void BrowseProgram();
    void Play();
    void About();
    void Save();
    wxTextCtrl* folder_ = nullptr;
    wxTextCtrl* program_ = nullptr;
    wxStaticBitmap* statusIcon_ = nullptr;
    wxStaticText* statusNote_ = nullptr;
    wxRadioBox* graphics_ = nullptr;   // the video menu's choices
    wxRadioBox* sound_ = nullptr;      // the sound options menu's choices
    wxCheckBox* demo_ = nullptr;
    wxCheckBox* noMouse_ = nullptr;
    wxCheckBox* noUemem_ = nullptr;
    wxChoice* scale_ = nullptr;
    wxCheckBox* fullscreen_ = nullptr;
    wxButton* play_ = nullptr;
    // Enhanced: Picture
    wxChoice* picture_ = nullptr;      // enhanced / original (--classic)
    wxChoice* resolution_ = nullptr;
    wxChoice* aa_ = nullptr;
    wxChoice* motion_ = nullptr;
    wxSpinCtrl* drawDistance_ = nullptr;
    wxSpinCtrl* haze_ = nullptr;
    wxChoice* aspect_ = nullptr;
    wxCheckBox* integerScale_ = nullptr;
    // Enhanced: Keys and gamepad page
    wxCheckBox* gearKeys_ = nullptr;
    wxChoice* shiftUp_ = nullptr;
    wxChoice* shiftDown_ = nullptr;
    wxCheckBox* padExtras_ = nullptr;

    wxSimplebook* book_ = nullptr;
    bool topmost_ = false;
    GameFolder game_;
    bool loading_ = true;  // no edits are recorded while the window is built
};
