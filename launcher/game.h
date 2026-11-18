// game.h -- what a game folder holds, where sr2-enhanced is, its options and starting it.
#pragma once

#include <vector>

#include <wx/string.h>

// A game folder as the launcher sees it.
struct GameFolder {
    wxString missing;       // the first of the game's files that isn't there, empty if all are
    int saves = 0;          // saved games (HOTROD0..14.SAV)
};

GameFolder ReadGameFolder(const wxString& dir);

inline constexpr const char* SETTINGS_SECTION = "Game";  // settings.ini section

// The folder the launcher runs from.
wxString LauncherDir();

// Where things are by default: "Game" and sr2-enhanced(.exe) beside the launcher.
wxString DefaultGameDir();
wxString DefaultProgram();

// The original's video menu choices, in its order. Only VGA and EGA (run as VGA) exist in the port.
enum class Graphics { Cga, Ega, Vga, Hercules, Tandy };
bool GraphicsSupported(Graphics g);
// The sound options menu's two choices.
enum class Sound { AdLib, Normal };

// The Enhanced picture's shape (--aspect), in the launcher's order.
enum class Aspect { A4x3, A16x10, A16x9, Window };
inline constexpr int ASPECT_COUNT = 4;

// The Enhanced option ranges (ENHANCED.md 3.9).
inline constexpr int MIN_RES_SCALE = 1, MAX_RES_SCALE = 8, MIN_AA = 1, MAX_AA = 4, MAX_SAMPLES = 16;
inline constexpr int MIN_DRAW_DISTANCE = 14, MAX_DRAW_DISTANCE = 120, MIN_HAZE = 0, MAX_HAZE = 100;

struct GameOptions {
    wxString program;       // sr2-enhanced(.exe)
    wxString gameDir;       // --game-dir
    Graphics graphics = Graphics::Vga;  // the switch e / v: skips the video menu
    Sound sound = Sound::AdLib;         // the switch adlib / speaker: skips the sound options menu
    bool demo = false;      // the game's "demo" switch: it plays itself
    bool noMouse = false;   // the game's "nomouse" switch
    bool noUemem = false;   // the game's "nouemem" switch: no EMS/UMB parking of pictures
    int scale = 3;          // --scale: the window is 320x240 times this
    bool fullscreen = false;  // --fullscreen

    // Enhanced: the picture.
    bool classic = false;   // --classic: every enhancement off, the original's picture
    int resScale = 4;       // --res-scale: the race view is 320x200 times this
    int aa = 2;             // --aa: anti-aliasing, aa x aa samples per pixel (resScale * aa <= 16)
    bool smoothMotion = true;  // false: --motion-delay 0 (the game's own frames, no interpolation)
    int drawDistance = 60;  // --draw-distance: road segments ahead (14 = the original's)
    int haze = 30;          // --haze: percent of the sky colour at the far end
    Aspect aspect = Aspect::A4x3;  // --aspect
    bool integerScale = false;     // --integer-scale
    // Enhanced: the controls.
    bool gearKeys = true;   // false: --no-gear-keys
    wxString shiftUp = "A";    // --key-shift-up
    wxString shiftDown = "Z";  // --key-shift-down
    bool padExtras = true;  // false: --no-pad-extras
};

// The keys offered for shifting (names SDL knows; the engine refuses the grey keys).
const std::vector<wxString>& ShiftKeyNames();

// The arguments after the program name.
std::vector<wxString> GameArguments(const GameOptions& options);
// The whole command line as one text (for --print-command).
wxString CommandLineText(const GameOptions& options);

// The options the launcher saved last time (or the defaults).
GameOptions OptionsFromSettings();

// Starts the game. On failure returns false and says why in `error`.
bool LaunchGame(const GameOptions& options, wxString& error);
