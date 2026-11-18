// launcher.cpp -- the launcher window.
#include "launcher.h"

#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/hyperlink.h>
#include <wx/radiobox.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/settings.h>
#include <wx/simplebook.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#ifdef __WXMSW__
#include <wx/msw/wrapcctl.h>
#include <shellapi.h>
#endif

#include "icon.h"
#include "settings.h"
#include "version.h"

const char* const APP_TITLE = "Street Rod 2 Enhanced";

namespace {

const char* const WEBSITE = "https://kkania.com";
const char* const SUPPORT = "https://buymeacoffee.com/krzysztofkania";
const char* const SECTION = SETTINGS_SECTION;

const int MIN_SCALE = 1, MAX_SCALE = 6;
const int ID_KEYS = wxID_HIGHEST + 1, ID_ABOUT = wxID_HIGHEST + 2;
// The book's pages.
const int PAGE_MAIN = 0, PAGE_KEYS = 1;

#ifdef __WXMSW__
HRESULT CALLBACK AboutCallback(HWND hwnd, UINT msg, WPARAM, LPARAM lp, LONG_PTR) {
    if (msg == TDN_HYPERLINK_CLICKED)
        ShellExecuteW(hwnd, L"open", reinterpret_cast<LPCWSTR>(lp), nullptr, nullptr, SW_SHOWNORMAL);
    return S_OK;
}
#else
// A label and a link on one line, for the portable About box.
void AddLink(wxWindow* parent, wxSizer* sizer, const wxString& label, const wxString& text, const wxString& url) {
    auto* line = new wxBoxSizer(wxHORIZONTAL);
    line->Add(new wxStaticText(parent, wxID_ANY, label + " "), 0, wxALIGN_CENTER_VERTICAL);
    line->Add(new wxHyperlinkCtrl(parent, wxID_ANY, text, url), 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(line);
}
#endif

wxStaticText* GreyText(wxWindow* parent, const wxString& text = wxEmptyString) {
    auto* label = new wxStaticText(parent, wxID_ANY, text);
    label->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    return label;
}

// A label, a text field and a Browse button in a row of a two-column grid.
wxTextCtrl* PathRow(wxWindow* parent, wxFlexGridSizer* grid, const wxString& label, const wxString& tip,
                    wxButton** browse) {
    grid->Add(new wxStaticText(parent, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
    auto* row = new wxBoxSizer(wxHORIZONTAL);
    auto* text = new wxTextCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(parent->FromDIP(300), -1));
    text->SetToolTip(tip);
    *browse = new wxButton(parent, wxID_ANY, "B&rowse...");
    row->Add(text, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, parent->FromDIP(8));
    row->Add(*browse, 0, wxALIGN_CENTER_VERTICAL);
    grid->Add(row, 1, wxEXPAND);
    return text;
}

wxString SavesText(int n) {
    return n == 0 ? wxString("no saved games") : n == 1 ? wxString("1 saved game") : wxString::Format("%d saved games", n);
}

}  // namespace

LauncherFrame::LauncherFrame()
    : wxFrame(nullptr, wxID_ANY, APP_TITLE, wxDefaultPosition, wxDefaultSize,
              wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX)) {
    SetIcons(AppIcons());
    topmost_ = settings::GetInt("Preferences", "AlwaysOnTop", 0) != 0;
    if (topmost_) SetWindowStyleFlag(GetWindowStyleFlag() | wxSTAY_ON_TOP);

    // The launcher and, in its place while it is open, the Keys page (PAGE_*).
    book_ = new wxSimplebook(this);
    book_->AddPage(CreateMainPage(book_), "Game");
    book_->AddPage(CreateKeysPage(book_), "Keys");
    auto* all = new wxBoxSizer(wxVERTICAL);
    all->Add(book_, 1, wxEXPAND);
    SetSizer(all);
    CreateMenus();

    // Settings from the last run (validated by OptionsFromSettings).
    const GameOptions saved = OptionsFromSettings();
    const wxString dir = settings::GetString(SECTION, "GameFolder", "");
    const wxString program = settings::GetString(SECTION, "Program", "");
    folder_->ChangeValue(dir.empty() ? DefaultGameDir() : dir);
    program_->ChangeValue(program.empty() ? DefaultProgram() : program);
    graphics_->SetSelection(static_cast<int>(saved.graphics));
    sound_->SetSelection(saved.sound == Sound::Normal ? 1 : 0);
    demo_->SetValue(saved.demo);
    noMouse_->SetValue(saved.noMouse);
    noUemem_->SetValue(saved.noUemem);
    scale_->SetSelection(saved.scale - MIN_SCALE);
    fullscreen_->SetValue(saved.fullscreen);
    picture_->SetSelection(saved.classic ? 1 : 0);
    resolution_->SetSelection(saved.resScale - MIN_RES_SCALE);
    FillAntiAliasing(saved.aa);
    motion_->SetSelection(saved.smoothMotion ? 0 : 1);
    drawDistance_->SetValue(saved.drawDistance);
    haze_->SetValue(saved.haze);
    aspect_->SetSelection(static_cast<int>(saved.aspect));
    integerScale_->SetValue(saved.integerScale);
    gearKeys_->SetValue(saved.gearKeys);
    shiftUp_->SetStringSelection(saved.shiftUp);
    shiftDown_->SetStringSelection(saved.shiftDown);
    padExtras_->SetValue(saved.padExtras);
    loading_ = false;
    UpdateState();
    UpdateEnhanced();

    Bind(wxEVT_ACTIVATE, [this](wxActivateEvent& event) {
        if (event.GetActive()) UpdateState();  // files may have been copied in, games saved meanwhile
        event.Skip();
    });
    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&) {
        Save();
        Destroy();
    });
    // Esc leaves the Keys page.
    Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& event) {
        if (event.GetKeyCode() == WXK_ESCAPE && book_->GetSelection() == PAGE_KEYS)
            CloseKeys();
        else
            event.Skip();
    });

    FitPage();
    if (!settings::RestoreWindowPosition(SECTION, this)) Centre();
}

wxWindow* LauncherFrame::CreateMainPage(wxWindow* parent) {
    auto* page = new wxPanel(parent);
    const int margin = FromDIP(12), gap = FromDIP(8), small = FromDIP(4);

    // Game files
    auto* filesBox = new wxStaticBoxSizer(wxVERTICAL, page, "Game files");
    wxWindow* fb = filesBox->GetStaticBox();
    auto* filesGrid = new wxFlexGridSizer(2, gap, gap);
    filesGrid->AddGrowableCol(1);
    wxButton* browseFolder = nullptr;
    wxButton* browseProgram = nullptr;
    folder_ = PathRow(fb, filesGrid, "&Folder:",
                      "The folder with the original game's files (SR.EXE, LIB1, LIB2, LIB3 and SR2_DATA).",
                      &browseFolder);
    program_ = PathRow(fb, filesGrid, "&Program:", "sr2-enhanced, the game.", &browseProgram);
    auto* statusRow = new wxBoxSizer(wxHORIZONTAL);
    statusIcon_ = new wxStaticBitmap(fb, wxID_ANY, wxArtProvider::GetBitmapBundle(wxART_WARNING, wxART_MENU));
    statusNote_ = new wxStaticText(fb, wxID_ANY, wxEmptyString);
    statusRow->Add(statusIcon_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, small);
    statusRow->Add(statusNote_, 1, wxALIGN_CENTER_VERTICAL);
    filesBox->Add(filesGrid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, gap);
    filesBox->Add(statusRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, gap);
    filesBox->Add(GreyText(fb, "The game saves its games (HOTROD0..14.SAV) in this folder,\n"
                               "so it must be writable (not under Program Files)."),
                  0, wxALL, gap);
    folder_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
        if (!loading_) UpdateState();
    });
    program_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
        if (!loading_) UpdateState();
    });
    browseFolder->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { BrowseFolder(); });
    browseProgram->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { BrowseProgram(); });

    // Graphics and sound: the original's video menu and sound options menu, answered here so that
    // sr2-enhanced skips both text screens. CGA, Hercules and Tandy are listed as in the original but greyed
    // out: the port has no driver for them; EGA runs through the same driver as VGA.
    auto* startBox = new wxStaticBoxSizer(wxHORIZONTAL, page, "Graphics and sound");
    wxWindow* sb = startBox->GetStaticBox();
    wxArrayString gfx;
    gfx.Add("&CGA (not supported by the port)");
    gfx.Add("&EGA (shown as VGA)");
    gfx.Add("&VGA");
    gfx.Add("&Hercules (not supported by the port)");
    gfx.Add("&Tandy (not supported by the port)");
    graphics_ = new wxRadioBox(sb, wxID_ANY, "Video", wxDefaultPosition, wxDefaultSize, gfx, 1, wxRA_SPECIFY_COLS);
    for (int n = 0; n < 5; ++n) graphics_->Enable(n, GraphicsSupported(static_cast<Graphics>(n)));
    graphics_->SetToolTip("The original's video menu. The port draws the VGA 16-colour screen.");
    wxArrayString snd;
    snd.Add("&AdLib (music and effects)");
    snd.Add("&Normal (PC speaker)");
    sound_ = new wxRadioBox(sb, wxID_ANY, "Sound", wxDefaultPosition, wxDefaultSize, snd, 1, wxRA_SPECIFY_COLS);
    sound_->SetToolTip("The original's sound options menu: AdLib (emulated) or the PC speaker.");
    startBox->Add(graphics_, 1, wxEXPAND | wxALL, gap);
    startBox->Add(sound_, 1, wxEXPAND | wxTOP | wxRIGHT | wxBOTTOM, gap);

    // Options: the window, and the game's own switches
    auto* optionsBox = new wxStaticBoxSizer(wxVERTICAL, page, "Options");
    wxWindow* ob = optionsBox->GetStaticBox();
    auto* grid = new wxFlexGridSizer(2, gap, gap);
    grid->Add(new wxStaticText(ob, wxID_ANY, "&Window size:"), 0, wxALIGN_CENTER_VERTICAL);
    scale_ = new wxChoice(ob, wxID_ANY);
    for (int s = MIN_SCALE; s <= MAX_SCALE; ++s) scale_->Append(wxString::Format(L"%d × %d", 320 * s, 240 * s));
    scale_->SetToolTip("The window's size when the game starts. Alt+Enter switches to full screen.");
    grid->Add(scale_, 0, wxALIGN_CENTER_VERTICAL);
    optionsBox->Add(grid, 0, wxLEFT | wxRIGHT | wxTOP, gap);
    fullscreen_ = new wxCheckBox(ob, wxID_ANY, "Start in f&ull screen (Alt+Enter switches)");
    optionsBox->Add(fullscreen_, 0, wxLEFT | wxRIGHT | wxTOP, gap);
    demo_ = new wxCheckBox(ob, wxID_ANY, "&Demo: the game plays itself (Esc ends it)");
    demo_->SetToolTip("The original's \"demo\" switch: a self-running game.");
    noMouse_ = new wxCheckBox(ob, wxID_ANY, "No &mouse (keyboard and joystick only)");
    noMouse_->SetToolTip("The original's \"nomouse\" switch.");
    noUemem_ = new wxCheckBox(ob, wxID_ANY, "No &upper memory (nouemem)");
    noUemem_->SetToolTip("The original's \"nouemem\" switch: the pictures aren't parked in upper/expanded memory.");
    optionsBox->Add(demo_, 0, wxLEFT | wxRIGHT | wxTOP, gap);
    optionsBox->Add(noMouse_, 0, wxLEFT | wxRIGHT | wxTOP, gap);
    optionsBox->Add(noUemem_, 0, wxALL, gap);

    // Enhanced: the picture of the races (ENHANCED.md 3.9). Menus and the garage keep the original's.
    auto* pictureBox = new wxStaticBoxSizer(wxVERTICAL, page, "Picture");
    wxWindow* pb = pictureBox->GetStaticBox();
    auto* pgrid = new wxFlexGridSizer(2, gap, gap);
    const auto label = [pb, pgrid](const wxString& text) {
        pgrid->Add(new wxStaticText(pb, wxID_ANY, text), 0, wxALIGN_CENTER_VERTICAL);
    };
    label("P&icture:");
    picture_ = new wxChoice(pb, wxID_ANY);
    picture_->Append("Enhanced");
    picture_->Append("Original (classic)");
    picture_->SetToolTip("Original: every enhancement off (--classic), the game exactly as the faithful port.");
    pgrid->Add(picture_, 0, wxEXPAND);
    label("Re&solution:");
    resolution_ = new wxChoice(pb, wxID_ANY);
    for (int r = MIN_RES_SCALE; r <= MAX_RES_SCALE; ++r)
        resolution_->Append(wxString::Format(L"%d × %d", 320 * r, 200 * r));
    resolution_->SetToolTip("The race view's resolution (--res-scale): 320 x 200 times 1..8.");
    pgrid->Add(resolution_, 0, wxEXPAND);
    label("Anti-a&liasing:");
    aa_ = new wxChoice(pb, wxID_ANY);
    aa_->SetToolTip("Samples per pixel (--aa). Resolution times samples is at most 5120 x 3200.");
    pgrid->Add(aa_, 0, wxEXPAND);
    label("M&otion:");
    motion_ = new wxChoice(pb, wxID_ANY);
    motion_->Append("Smooth (between the game's frames)");
    motion_->Append("The game's own frames");
    motion_->SetToolTip("Smooth draws the road between the game's frames, one game frame behind (--motion-delay).");
    pgrid->Add(motion_, 0, wxEXPAND);
    label("Draw distan&ce:");
    auto* ddRow = new wxBoxSizer(wxHORIZONTAL);
    drawDistance_ = new wxSpinCtrl(pb, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(FromDIP(70), -1),
                                   wxSP_ARROW_KEYS, MIN_DRAW_DISTANCE, MAX_DRAW_DISTANCE, 60);
    drawDistance_->SetToolTip("Road segments drawn ahead (--draw-distance); 14 is the original's.");
    ddRow->Add(drawDistance_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, small);
    ddRow->Add(GreyText(pb, "segments (14 = original)"), 0, wxALIGN_CENTER_VERTICAL);
    pgrid->Add(ddRow);
    label("&Haze:");
    auto* hazeRow = new wxBoxSizer(wxHORIZONTAL);
    haze_ = new wxSpinCtrl(pb, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(FromDIP(70), -1),
                           wxSP_ARROW_KEYS, MIN_HAZE, MAX_HAZE, 30);
    haze_->SetToolTip("How much of the sky colour the far end fades into (--haze); 0 is off.");
    hazeRow->Add(haze_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, small);
    hazeRow->Add(GreyText(pb, "% (0 = off)"), 0, wxALIGN_CENTER_VERTICAL);
    pgrid->Add(hazeRow);
    label("Aspec&t:");
    aspect_ = new wxChoice(pb, wxID_ANY);
    aspect_->Append("4:3 (original)");
    aspect_->Append("16:10");
    aspect_->Append("16:9");
    aspect_->Append("Fill the window");
    aspect_->SetToolTip("The picture's shape (--aspect). Wider shapes stretch the picture; menus and the mouse "
                        "still line up.");
    pgrid->Add(aspect_, 0, wxEXPAND);
    pictureBox->Add(pgrid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, gap);
    integerScale_ = new wxCheckBox(pb, wxID_ANY, "Whole-num&ber scaling (black bars)");
    integerScale_->SetToolTip("Scale the picture by a whole number only (--integer-scale).");
    pictureBox->Add(integerScale_, 0, wxLEFT | wxRIGHT | wxTOP, gap);
    pictureBox->Add(GreyText(pb, "The races are drawn enhanced; menus and the\ngarage keep the original's picture."),
                    0, wxALL, gap);
    picture_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { UpdateEnhanced(); });
    resolution_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { FillAntiAliasing(aa_->GetSelection() + MIN_AA); });

    // Buttons
    auto* buttons = new wxBoxSizer(wxHORIZONTAL);
    play_ = new wxButton(page, wxID_ANY, "&Play");
    auto* close = new wxButton(page, wxID_CLOSE, "Close");
    buttons->AddStretchSpacer();
    buttons->Add(play_, 0, wxRIGHT, gap);
    buttons->Add(close);
    play_->SetDefault();
    play_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Play(); });
    close->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Close(); });

    // The original's choices on the left, the Enhanced picture on the right.
    auto* left = new wxBoxSizer(wxVERTICAL);
    left->Add(startBox, 0, wxEXPAND);
    left->Add(optionsBox, 1, wxEXPAND | wxTOP, margin);
    auto* columns = new wxBoxSizer(wxHORIZONTAL);
    columns->Add(left, 1, wxEXPAND | wxRIGHT, margin);
    columns->Add(pictureBox, 1, wxEXPAND);

    auto* all = new wxBoxSizer(wxVERTICAL);
    all->Add(filesBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, margin);
    all->Add(columns, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, margin);
    all->Add(buttons, 0, wxEXPAND | wxALL, margin);
    page->SetSizer(all);
    return page;
}

wxWindow* LauncherFrame::CreateKeysPage(wxWindow* parent) {
    auto* page = new wxPanel(parent);
    const int margin = FromDIP(12), gap = FromDIP(8), small = FromDIP(4);

    auto* heading = new wxStaticText(page, wxID_ANY, "Keys and gamepad");
    heading->SetFont(GetFont().Bold().Scaled(1.2f));
    auto* intro = GreyText(page, "The game's own keys can't be changed. The Enhanced controls below are\n"
                                 "ignored with the original picture: A / Z shift, no gamepad.");

    auto* keysBox = new wxStaticBoxSizer(wxVERTICAL, page, "Keys");
    wxWindow* kb = keysBox->GetStaticBox();
    auto* keys = new wxFlexGridSizer(2, small, FromDIP(16));
    const char* const KEYS[][2] = {
        {"Mouse", "point and click (the game draws its own pointer)"},
        {"Arrows / keypad", "move the pointer"},
        {"Space  Ins  keypad 0", "click"},
        {"", ""},
        {"Races:", ""},
        {"Up  Down", "gas, brake"},
        {"Left  Right", "steer"},
        {"Space  Ins", "shift (down when braking, else up)"},
        {"A  Z", "shift up, down (the gear keys below)"},
        {"", ""},
        {"Esc", "skip the drive through town; end the demo"},
        {"M", "music on / off"},
        {"Ctrl", "sound on / off"},
        {"Alt+Enter", "full screen"},
    };
    for (const auto& k : KEYS) {
        keys->Add(new wxStaticText(kb, wxID_ANY, k[0]));
        keys->Add(GreyText(kb, k[1]));
    }
    keysBox->Add(keys, 0, wxALL, gap);

    // Enhanced: the gear keys and the gamepad's extra buttons (S3.1).
    auto* controlsBox = new wxStaticBoxSizer(wxVERTICAL, page, "Enhanced controls");
    wxWindow* cb = controlsBox->GetStaticBox();
    gearKeys_ = new wxCheckBox(cb, wxID_ANY, "Gear &keys in races");
    gearKeys_->SetToolTip("Off: only Space / Ins shift (--no-gear-keys). Menus are not affected.");
    controlsBox->Add(gearKeys_, 0, wxLEFT | wxRIGHT | wxTOP, gap);
    auto* gearGrid = new wxFlexGridSizer(4, small, gap);
    wxArrayString keyNames;
    for (const wxString& k : ShiftKeyNames()) keyNames.Add(k);
    gearGrid->Add(new wxStaticText(cb, wxID_ANY, "Shift &up:"), 0, wxALIGN_CENTER_VERTICAL);
    shiftUp_ = new wxChoice(cb, wxID_ANY, wxDefaultPosition, wxDefaultSize, keyNames);
    shiftUp_->SetToolTip("The key that shifts up (--key-shift-up); A by default.");
    gearGrid->Add(shiftUp_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, gap);
    gearGrid->Add(new wxStaticText(cb, wxID_ANY, "Shift &down:"), 0, wxALIGN_CENTER_VERTICAL);
    shiftDown_ = new wxChoice(cb, wxID_ANY, wxDefaultPosition, wxDefaultSize, keyNames);
    shiftDown_->SetToolTip("The key that shifts down (--key-shift-down); Z by default.");
    gearGrid->Add(shiftDown_, 0, wxALIGN_CENTER_VERTICAL);
    controlsBox->Add(gearGrid, 0, wxLEFT | wxRIGHT | wxTOP, gap);
    padExtras_ = new wxCheckBox(cb, wxID_ANY, "Gamepad &shoulders, triggers and Start");
    padExtras_->SetToolTip("Right / left shoulder shift up / down, right / left trigger gas / brake, Start = Esc. "
                           "Off: --no-pad-extras (stick, D-pad and A still work).");
    controlsBox->Add(padExtras_, 0, wxLEFT | wxRIGHT | wxTOP, gap);
    controlsBox->Add(GreyText(cb, "Gamepad: stick or D-pad steers, gas and brake; A clicks and shifts."), 0, wxALL,
                     gap);
    gearKeys_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { UpdateEnhanced(); });

    auto* buttons = new wxBoxSizer(wxHORIZONTAL);
    auto* back = new wxButton(page, wxID_OK, "&Back");
    back->SetDefault();
    buttons->AddStretchSpacer();
    buttons->Add(back);
    back->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { CloseKeys(); });

    auto* all = new wxBoxSizer(wxVERTICAL);
    all->Add(heading, 0, wxLEFT | wxRIGHT | wxTOP, margin);
    all->Add(intro, 0, wxLEFT | wxRIGHT | wxTOP, small);
    all->Add(keysBox, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, margin);
    all->Add(controlsBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, margin);
    all->Add(buttons, 0, wxEXPAND | wxALL, margin);
    page->SetSizer(all);
    return page;
}

void LauncherFrame::CreateMenus() {
    auto* file = new wxMenu;
    file->Append(wxID_PREFERENCES, wxString::FromUTF8("&Preferences…"));
    file->AppendSeparator();
    file->Append(wxID_EXIT, "E&xit\tAlt+F4");
    auto* settingsMenu = new wxMenu;
    settingsMenu->Append(ID_KEYS, wxString::FromUTF8("&Keys and gamepad…"));

    auto* bar = new wxMenuBar;
    bar->Append(file, "&File");
    bar->Append(settingsMenu, "&Game settings");
#ifndef __WXMSW__
    auto* about = new wxMenu;
    about->Append(ID_ABOUT, wxString::FromUTF8("&About ") + APP_TITLE + wxString::FromUTF8("…"));
    bar->Append(about, "A&bout");
#endif
    SetMenuBar(bar);
#ifdef __WXMSW__
    // A top-level About item that acts straight away, which wxWidgets menus
    // can't express; MSWWindowProc handles its command.
    const HMENU native = static_cast<HMENU>(bar->GetHMenu());
    AppendMenuW(native, MF_STRING, ID_ABOUT, L"A&bout");
    DrawMenuBar(static_cast<HWND>(GetHWND()));
#endif

    Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(); }, wxID_EXIT);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { Preferences(); }, wxID_PREFERENCES);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { OpenKeys(); }, ID_KEYS);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { About(); }, ID_ABOUT);
}

#ifdef __WXMSW__
WXLRESULT LauncherFrame::MSWWindowProc(WXUINT msg, WXWPARAM wParam, WXLPARAM lParam) {
    if (msg == WM_COMMAND && LOWORD(wParam) == ID_ABOUT && lParam == 0) {
        CallAfter([this] { About(); });
        return 0;
    }
    return wxFrame::MSWWindowProc(msg, wParam, lParam);
}
#endif

void LauncherFrame::FitPage() { SetClientSize(book_->GetCurrentPage()->GetBestSize()); }

void LauncherFrame::OpenKeys() {
    if (book_->GetSelection() == PAGE_KEYS) return;
    book_->ChangeSelection(PAGE_KEYS);
    FitPage();
    GetMenuBar()->Enable(ID_KEYS, false);
    SetDefaultItem(book_->GetPage(PAGE_KEYS)->FindWindow(wxID_OK));
}

void LauncherFrame::CloseKeys() {
    book_->ChangeSelection(PAGE_MAIN);
    FitPage();
    GetMenuBar()->Enable(ID_KEYS, true);
    SetDefaultItem(play_);
    play_->SetFocus();
}

void LauncherFrame::Preferences() {
    wxDialog dialog(this, wxID_ANY, "Preferences");
    auto* all = new wxBoxSizer(wxVERTICAL);
    auto* topmost = new wxCheckBox(&dialog, wxID_ANY, "Always on &top");
    topmost->SetValue(topmost_);
    topmost->SetToolTip("Keep the launcher above other windows.");
    all->Add(topmost, 0, wxALL, dialog.FromDIP(12));
    all->Add(dialog.CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM,
             dialog.FromDIP(12));
    dialog.SetSizerAndFit(all);
    dialog.SetMinSize(wxSize(dialog.FromDIP(300), -1));
    dialog.Fit();
    dialog.CentreOnParent();
    topmost->SetFocus();
    if (dialog.ShowModal() != wxID_OK) return;
    if (topmost->GetValue() != topmost_) SetTopmost(topmost->GetValue());
}

void LauncherFrame::SetTopmost(bool on) {
    topmost_ = on;
    const long style = GetWindowStyleFlag();
    SetWindowStyleFlag(on ? style | wxSTAY_ON_TOP : style & ~wxSTAY_ON_TOP);
    settings::SetInt("Preferences", "AlwaysOnTop", on ? 1 : 0);
}

void LauncherFrame::UpdateState() {
    const wxString dir = folder_->GetValue(), program = program_->GetValue();
    game_ = ReadGameFolder(dir);
    const bool haveProgram = wxFileName::FileExists(program);

    wxString note;
    if (!game_.missing.empty())
        note = wxString::Format("This folder needs the game's files: %s is missing.", game_.missing);
    else if (!haveProgram)
        note = wxString::Format("%s isn't there.", wxFileName(program).GetFullName());
    else
        note = wxString::Format("Found Street Rod 2, %s.", SavesText(game_.saves));
    const bool ok = haveProgram && game_.missing.empty();
    statusIcon_->Show(!ok);
    statusNote_->SetLabel(note);
    play_->Enable(ok);
    play_->GetParent()->Layout();
}

void LauncherFrame::UpdateEnhanced() {
    const bool enhanced = picture_->GetSelection() == 0;
    for (wxWindow* w : {static_cast<wxWindow*>(resolution_), static_cast<wxWindow*>(aa_),
                        static_cast<wxWindow*>(motion_), static_cast<wxWindow*>(drawDistance_),
                        static_cast<wxWindow*>(haze_), static_cast<wxWindow*>(aspect_),
                        static_cast<wxWindow*>(integerScale_), static_cast<wxWindow*>(gearKeys_),
                        static_cast<wxWindow*>(padExtras_)})
        w->Enable(enhanced);
    shiftUp_->Enable(enhanced && gearKeys_->GetValue());
    shiftDown_->Enable(enhanced && gearKeys_->GetValue());
}

void LauncherFrame::FillAntiAliasing(int aa) {
    const int res = resolution_->GetSelection() + MIN_RES_SCALE;
    const int most = wxMin(MAX_AA, MAX_SAMPLES / res);
    aa_->Clear();
    for (int a = MIN_AA; a <= most; ++a) aa_->Append(a == 1 ? wxString("Off") : wxString::Format(L"%d × %d", a, a));
    aa_->SetSelection(wxMax(MIN_AA, wxMin(most, aa)) - MIN_AA);
}

GameOptions LauncherFrame::EditedOptions() const {
    GameOptions o;
    o.program = wxFileName(program_->GetValue()).GetFullPath();
    o.gameDir = wxFileName(folder_->GetValue()).GetFullPath();
    o.graphics = static_cast<Graphics>(graphics_->GetSelection());
    o.sound = sound_->GetSelection() == 1 ? Sound::Normal : Sound::AdLib;
    o.demo = demo_->GetValue();
    o.noMouse = noMouse_->GetValue();
    o.noUemem = noUemem_->GetValue();
    o.scale = scale_->GetSelection() + MIN_SCALE;
    o.fullscreen = fullscreen_->GetValue();
    o.classic = picture_->GetSelection() == 1;
    o.resScale = resolution_->GetSelection() + MIN_RES_SCALE;
    o.aa = aa_->GetSelection() + MIN_AA;
    o.smoothMotion = motion_->GetSelection() == 0;
    o.drawDistance = drawDistance_->GetValue();
    o.haze = haze_->GetValue();
    o.aspect = static_cast<Aspect>(aspect_->GetSelection());
    o.integerScale = integerScale_->GetValue();
    o.gearKeys = gearKeys_->GetValue();
    o.shiftUp = shiftUp_->GetStringSelection();
    o.shiftDown = shiftDown_->GetStringSelection();
    o.padExtras = padExtras_->GetValue();
    return o;
}

void LauncherFrame::BrowseFolder() {
    wxDirDialog dialog(this, "Choose the folder with the game's files", folder_->GetValue(),
                       wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    if (dialog.ShowModal() == wxID_OK) folder_->SetValue(dialog.GetPath());  // raises wxEVT_TEXT
}

void LauncherFrame::BrowseProgram() {
    wxFileName current(program_->GetValue());
#ifdef __WXMSW__
    const char* const filter = "Programs (*.exe)|*.exe|All files (*.*)|*.*";
#else
    const char* const filter = "All files|*";
#endif
    wxFileDialog dialog(this, "Choose sr2-enhanced", current.GetPath(), current.GetFullName(), filter,
                        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dialog.ShowModal() == wxID_OK) program_->SetValue(dialog.GetPath());  // raises wxEVT_TEXT
}

void LauncherFrame::Play() {
    Save();
    const GameOptions options = EditedOptions();
    wxString error;
    if (!LaunchGame(options, error)) wxMessageBox(error, APP_TITLE, wxOK | wxICON_ERROR, this);
}

void LauncherFrame::Save() {
    // A folder or program left at its default is stored empty, so it follows the launcher if it moves.
    const wxString dir = folder_->GetValue(), program = program_->GetValue();
    settings::SetString(SECTION, "GameFolder",
                        wxFileName(dir).SameAs(wxFileName(DefaultGameDir())) ? wxString() : dir);
    settings::SetString(SECTION, "Program",
                        wxFileName(program).SameAs(wxFileName(DefaultProgram())) ? wxString() : program);
    settings::SetInt(SECTION, "Graphics", graphics_->GetSelection());
    settings::SetInt(SECTION, "Sound", sound_->GetSelection());
    settings::SetInt(SECTION, "Demo", demo_->GetValue() ? 1 : 0);
    settings::SetInt(SECTION, "NoMouse", noMouse_->GetValue() ? 1 : 0);
    settings::SetInt(SECTION, "NoUemem", noUemem_->GetValue() ? 1 : 0);
    settings::SetInt(SECTION, "Scale", scale_->GetSelection() + MIN_SCALE);
    settings::SetInt(SECTION, "Fullscreen", fullscreen_->GetValue() ? 1 : 0);
    const GameOptions o = EditedOptions();
    settings::SetInt(SECTION, "Classic", o.classic ? 1 : 0);
    settings::SetInt(SECTION, "ResScale", o.resScale);
    settings::SetInt(SECTION, "AntiAliasing", o.aa);
    settings::SetInt(SECTION, "SmoothMotion", o.smoothMotion ? 1 : 0);
    settings::SetInt(SECTION, "DrawDistance", o.drawDistance);
    settings::SetInt(SECTION, "Haze", o.haze);
    settings::SetInt(SECTION, "Aspect", static_cast<int>(o.aspect));
    settings::SetInt(SECTION, "IntegerScale", o.integerScale ? 1 : 0);
    settings::SetInt(SECTION, "GearKeys", o.gearKeys ? 1 : 0);
    settings::SetString(SECTION, "ShiftUp", o.shiftUp);
    settings::SetString(SECTION, "ShiftDown", o.shiftDown);
    settings::SetInt(SECTION, "PadExtras", o.padExtras ? 1 : 0);
    settings::SaveWindowPosition(SECTION, this);
}

void LauncherFrame::About() {
    const wxString title = wxString("About ") + APP_TITLE;
    const wxString heading = wxString(APP_TITLE) + " " + APP_VERSION_TEXT;
    const wxString blurb = "Starts sr2-enhanced, the enhanced SDL3 port of Street Rod 2: The Next Generation "
                           "(California Dreams, 1991).";
#ifdef __WXMSW__
    // The Windows task dialog.
    const wxString content = wxString::Format(
        "%s\n\n"
        "Author: Krzysztof Kania\n"
        "Website: <a href=\"%s\">kkania.com</a>\n"
        "Support: <a href=\"%s\">buymeacoffee.com/krzysztofkania</a>",
        blurb, WEBSITE, SUPPORT);
    wxIcon icon;
    icon.CopyFromBitmap(AppBitmap(FromDIP(32)));
    TASKDIALOGCONFIG dialog{};
    dialog.cbSize = sizeof dialog;
    dialog.hwndParent = static_cast<HWND>(GetHWND());
    dialog.dwFlags = TDF_ENABLE_HYPERLINKS | TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION |
                     TDF_POSITION_RELATIVE_TO_WINDOW;
    dialog.dwCommonButtons = TDCBF_OK_BUTTON;
    dialog.pszWindowTitle = title.wc_str();
    dialog.hMainIcon = static_cast<HICON>(icon.GetHICON());
    dialog.pszMainInstruction = heading.wc_str();
    dialog.pszContent = content.wc_str();
    dialog.pfCallback = AboutCallback;
    TaskDialogIndirect(&dialog, nullptr, nullptr, nullptr);
#else
    wxDialog dialog(this, wxID_ANY, title);
    auto* body = new wxBoxSizer(wxHORIZONTAL);
    body->Add(new wxStaticBitmap(&dialog, wxID_ANY, AppBitmap(dialog.FromDIP(48))), 0, wxALL, dialog.FromDIP(12));

    auto* text = new wxBoxSizer(wxVERTICAL);
    auto* headingText = new wxStaticText(&dialog, wxID_ANY, heading);
    headingText->SetFont(dialog.GetFont().Bold().Scaled(1.3f));
    text->Add(headingText, 0, wxBOTTOM, dialog.FromDIP(8));
    text->Add(new wxStaticText(&dialog, wxID_ANY, blurb), 0, wxBOTTOM, dialog.FromDIP(12));
    text->Add(new wxStaticText(&dialog, wxID_ANY, "Author: Krzysztof Kania"));
    AddLink(&dialog, text, "Website:", "kkania.com", WEBSITE);
    AddLink(&dialog, text, "Support:", "buymeacoffee.com/krzysztofkania", SUPPORT);
    body->Add(text, 1, wxTOP | wxRIGHT | wxBOTTOM, dialog.FromDIP(12));

    auto* all = new wxBoxSizer(wxVERTICAL);
    all->Add(body, 1, wxEXPAND);
    all->Add(dialog.CreateStdDialogButtonSizer(wxOK), 0, wxEXPAND | wxALL, dialog.FromDIP(8));
    dialog.SetSizerAndFit(all);
    dialog.CentreOnParent();
    dialog.ShowModal();
#endif
}
