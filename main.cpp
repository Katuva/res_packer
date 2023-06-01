#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <vector>
#include <string>
#include <thread>
#include <format>
#include <chrono>
#include "Paths.h"
#include "Packer.h"
#include "Unpacker.h"
#include "PakTypes.h"
#include "Utils.h"
#include "Widgets.h"
#include <shlobj.h>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/archives/binary.hpp>
#include "External/IconsFontAwesome6.h"
#include "Version.h"
#include "ui.h"

using namespace std;

#define PROJECT_FILE_VERSION 1

// Data
static ID3D11Device *g_pd3dDevice = nullptr;
static ID3D11DeviceContext *g_pd3dDeviceContext = nullptr;
static IDXGISwapChain *g_pSwapChain = nullptr;
static ID3D11RenderTargetView *g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);

void CleanupDeviceD3D();

void CreateRenderTarget();

void CleanupRenderTarget();

void CreatePakFile(const std::string &targetPath);

void SaveSettings();

void LoadSettings();

void ApplySettings();

string SelectFolder();

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

vector<char> mainFont;
vector<char> iconFont;

PakTypes::PakFile resFile = Unpacker::ParsePakFile("res.pak");

string resPassword = "res_packer_gui";

Unpacker unpacker;
Packer packer;

vector<PakTypes::PakFileItem> files;
PakTypes::PakFile pakFile;

bool showFileWindow = false;
bool showExtractWindow = false;
bool showSettingsWindow = false;
bool showAboutModal = false;
bool showUnpackingComplete = false;
bool showEditPackedPath = false;
bool packing_files = false;
bool packing_complete = false;

PakTypes::CompressionType compressionType = PakTypes::CompressionType::ZSTD;

char password[256] = "";

std::chrono::high_resolution_clock::time_point packStart;
std::chrono::high_resolution_clock::time_point packEnd;

struct ProjectFile {
    unsigned int version;
    PakTypes::CompressionType compressionType;
    vector<PakTypes::PakFileItem> files;
};

string SavePakFile(string filename) {
    if (Paths::GetFileExtension(filename).empty())
        filename.append(".pak");
    else if (Paths::GetFileExtension(filename) != ".pak")
        Paths::ReplaceFileExtension(filename, ".pak");
    return filename;
}

string SaveProjectFile(string filename) {
    if (Paths::GetFileExtension(filename).empty())
        filename.append(".pakproj");
    else if (Paths::GetFileExtension(filename) != ".pakproj")
        Paths::ReplaceFileExtension(filename, ".pakproj");
    return filename;
}

string SaveHeaderFile(string filename) {
    if (Paths::GetFileExtension(filename).empty())
        filename.append(".h");
    else if (Paths::GetFileExtension(filename) != ".h")
        Paths::ReplaceFileExtension(filename, ".h");
    return filename;
}

void OpenProjectFile(const string &filename) {
    ProjectFile projectFile;
    std::ifstream is(filename, std::ios::binary);
    cereal::BinaryInputArchive archive(is);
    archive(projectFile);

    if (projectFile.version != PROJECT_FILE_VERSION)
        throw std::runtime_error("Invalid project file version");

    compressionType = projectFile.compressionType;
    files = projectFile.files;
    showFileWindow = true;
}

std::string TruncatePath(const std::string &path, size_t maxLength, bool packed = false) {
    std::filesystem::path fsPath(path);

    std::string dir = fsPath.parent_path().string();

    if (dir.length() > maxLength) {
        dir = "..." + dir.substr(dir.length() - maxLength);
    }

    if (packed) {
        dir = dir + "/" + fsPath.filename().string();
    }

    return dir;
}

template<class Archive>
void serialize(Archive &archive, PakTypes::PakFileItem &item) {
    archive(item.name, item.path, item.packedPath, item.size, item.compressed, item.encrypted);
}

template<class Archive>
void serialize(Archive &archive, ProjectFile &project) {
    archive(project.version, project.compressionType, project.files);
}

struct Settings {
    int zlibCompressionLevel = packer.getZlibCompressionLevel();
    int lz4CompressionLevel = packer.getLz4CompressionLevel();
    int zstdCompressionLevel = packer.getZstdCompressionLevel();

    size_t encryptionOpsLimit = packer.getEncryptionOpsLimit();
    size_t encryptionMemLimit = packer.getEncryptionMemLimit();

    template<class Archive>
    void serialize(Archive &archive) {
        archive(zlibCompressionLevel, lz4CompressionLevel, zstdCompressionLevel, encryptionOpsLimit,
                encryptionMemLimit);
    }
};

Settings settings;

// Main code
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

int main(int, char **) {
    const int window_width = 1280;
    const int window_height = 800;

    const int windowX = (GetSystemMetrics(SM_CXSCREEN) / 2) - window_width / 2;
    const int windowY = (GetSystemMetrics(SM_CYSCREEN) / 2) - window_height / 2;

    // Create application window
//    ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = {sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(101)), nullptr, nullptr,
                      nullptr, L"Resource Packer", nullptr};
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Resource Packer...", WS_OVERLAPPEDWINDOW, windowX, windowY,
                                window_width, window_height, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    DragAcceptFiles(hwnd, true);

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
//    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    UI::SetupImGuiStyle();
    //ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    unpacker.setPassword(resPassword);

    iconFont = unpacker.ExtractFileToMemory(resFile, "file1");
    mainFont = unpacker.ExtractFileToMemory(resFile, "file2");

    // Our state
    bool show_demo_window = false;
    ImVec4 clear_color = ImVec4(0.156f, 0.180f, 0.219, 1.0f);
//    io.Fonts->AddFontDefault();
    float baseFontSize = 16.0f;
//    io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", baseFontSize);
    io.Fonts->AddFontFromMemoryTTF(mainFont.data(), mainFont.size(), baseFontSize);

    float iconFontSize = baseFontSize * 2.0f / 3.0f;
    static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = iconFontSize;
//    io.Fonts->AddFontFromMemoryTTF((void*)fa_solid_900, sizeof(fa_solid_900), iconFontSize, &icons_config, icons_ranges);
//    io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, 16.0f, &icons_config, icons_ranges);
    io.Fonts->AddFontFromMemoryTTF(iconFont.data(), iconFont.size(), 16.0f, &icons_config, icons_ranges);

//    io.Fonts->AddFontFromFileTTF("Roboto-Regular.ttf", 16.0f);
//    ImFont* font2 = io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 28.0f);
    ImFont *font2 = io.Fonts->AddFontFromMemoryTTF(mainFont.data(), mainFont.size(), 28.0f);
    string SaveFileName = "";

    LoadSettings();

    string clipboardButtonLabel = ICON_FA_CLIPBOARD " Copy to Clipboard";
    double lastClickTime = 0.0;
    char editPackedPath[256];
    int editPackedPathIndex;

    // Main loop
    bool done = false;
    while (!done) {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        ImVec2 windowSize = ImGui::GetIO().DisplaySize;

        if (!showFileWindow && !showExtractWindow) {
            const ImVec2 center(windowSize.x * 0.5f, windowSize.y * 0.5f);

            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(400, 44));
            ImGui::Begin("DragDrop", nullptr,
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);
            ImGui::PushFont(font2);
            ImVec2 wSize = ImGui::GetWindowSize();
            float textWidth = ImGui::CalcTextSize("Drag and drop files here...").x;
            float x = (wSize.x - textWidth) / 2;
            ImGui::SetCursorPosX(x);
            ImGui::Text("Drag and drop files here...");
            ImGui::PopFont();
            ImGui::End();
        }

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(windowSize.x, 65));
        ImGui::Begin("Toolbar", nullptr,
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoTitleBar);

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open Project")) {
                    Utils::OpenFile("Pak Project (*.pakproj)\0*.pakproj\0",
                                    reinterpret_cast<void (*)(string)>(OpenProjectFile));
                }
                ImGui::Dummy(ImVec2(0.0f, 2.0f));
                if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK "  Save Project")) {
                    string ProjectFileName = Utils::SaveFile("Pak Project (*.pakproj)\0*.pakproj\0", SaveProjectFile);
                    if (!ProjectFileName.empty()) {
                        {
                            ProjectFile projectFile;
                            projectFile.version = PROJECT_FILE_VERSION;
                            projectFile.compressionType = compressionType;
                            projectFile.files = files;
                            std::ofstream os(ProjectFileName, std::ios::binary);
                            cereal::BinaryOutputArchive archive(os);
                            archive(projectFile);
                        }
                    }
                }
                ImGui::Dummy(ImVec2(0.0f, 2.0f));
                if (ImGui::MenuItem(ICON_FA_XMARK "   Exit")) {
                    done = true;
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem(ICON_FA_GEAR " Settings"))
                    showSettingsWindow = true;
                ImGui::Dummy(ImVec2(0.0f, 2.0f));
                ImGui::MenuItem("Show File Window", nullptr, &showFileWindow, !files.empty());

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem(ICON_FA_CIRCLE_INFO " About")) {
                    showAboutModal = true;
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        if (ImGui::Button(ICON_FA_FOLDER_OPEN " Open Project", ImVec2(0, 26))) {
            Utils::OpenFile("Pak Project (*.pakproj)\0*.pakproj\0",
                            reinterpret_cast<void (*)(string)>(OpenProjectFile));
        }

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save Project", ImVec2(0, 26))) {
            string ProjectFileName = Utils::SaveFile("Pak Project (*.pakproj)\0*.pakproj\0", SaveProjectFile);
            if (!ProjectFileName.empty()) {
                {
                    ProjectFile projectFile;
                    projectFile.version = PROJECT_FILE_VERSION;
                    projectFile.compressionType = compressionType;
                    projectFile.files = files;
                    std::ofstream os(ProjectFileName, std::ios::binary);
                    cereal::BinaryOutputArchive archive(os);
                    archive(projectFile);
                }
            }
        }

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(2.0f, 0.0f));
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(2.0f, 0.0f));

        ImGui::SameLine();
        ImGui::BeginDisabled(files.empty());
        if (ImGui::Button(ICON_FA_BOX_ARCHIVE " Pack Files", ImVec2(0, 26))) {
            bool hasEncryptedItem = std::any_of(files.begin(), files.end(), [](const PakTypes::PakFileItem &item) {
                return item.encrypted;
            });
            string pass = password;
            if (pass.empty() && hasEncryptedItem) {
                MessageBoxA(nullptr, "Please enter an encryption password", "Error", MB_OK | MB_ICONERROR);
            } else {
                SaveFileName = Utils::SaveFile("Pak Files (*.pak)\0*.pak\0", SavePakFile);
                if (!SaveFileName.empty()) {
                    packing_files = true;
                    ImGui::OpenPopup("Packing Progress");
                    thread(CreatePakFile, SaveFileName).detach();
                }
            }
        }
        ImGui::EndDisabled();

        if (showAboutModal) {
            ImGui::OpenPopup(ICON_FA_CIRCLE_INFO " About");
        }

        if (ImGui::BeginPopupModal(ICON_FA_CIRCLE_INFO " About", &showAboutModal, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::SeparatorText("Build");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::Text("Resource Packer v%d.%d.%d",
                        RES_PACKER_GUI_VERSION_MAJOR,
                        RES_PACKER_GUI_VERSION_MINOR,
                        RES_PACKER_GUI_VERSION_PATCH);
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::Text("Built on %s at %s", Utils::FormatBuildDate(__DATE__).c_str(), __TIME__);
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::SeparatorText("Credits");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, UI::text_prominent);
            ImGui::Text("Dear ImGui");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("github.com/ocornut/imgui");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, UI::text_prominent);
            ImGui::Text("Zstandard");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("github.com/facebook/zstd");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, UI::text_prominent);
            ImGui::Text("LZ4");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("github.com/lz4/lz4");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, UI::text_prominent);
            ImGui::Text("Miniz");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("github.com/richgel999/miniz");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, UI::text_prominent);
            ImGui::Text("cereal");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("github.com/USCiLab/cereal");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, UI::text_prominent);
            ImGui::Text("Sodium");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("github.com/jedisct1/libsodium");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            if (ImGui::Button(ICON_FA_XMARK " Close")) {
                ImGui::CloseCurrentPopup();
                showAboutModal = false;
            }
            ImGui::EndPopup();
        }

        // Status bar code
//        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
//        float height = ImGui::GetFrameHeight();
//
//        if (ImGui::BeginViewportSideBar("##MainStatusBar", nullptr, ImGuiDir_Down, height, window_flags)) {
//            if (ImGui::BeginMenuBar()) {
//                ImGui::Text("Happy status bar");
//                ImGui::EndMenuBar();
//            }
//        }
//        ImGui::End();

        if (ImGui::BeginPopupModal("Packing Progress", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Packing files...");
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            Widgets::IndeterminateProgressBar(ImVec2(300, 20));
            if (!packing_files) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (packing_complete) {
            ImGui::OpenPopup("Packing Result");
        }

        if (ImGui::BeginPopupModal("Packing Result", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            std::chrono::duration<double> elapsed = packEnd - packStart;
            ImGui::Text("Packing has been completed");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::Text("Packed file: %s", SaveFileName.c_str());
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::Text("Packed file size: %s", Utils::FormatBytes(Paths::GetFileSize(SaveFileName)).c_str());
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::Text("Packing finished in: %.2f seconds", elapsed.count());
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            if (ImGui::Button(ICON_FA_XMARK " Close")) {
                packing_complete = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::End();

        if (showSettingsWindow) {
            ImGui::OpenPopup(ICON_FA_GEAR " Settings");
        }

        if (ImGui::BeginPopupModal(ICON_FA_GEAR " Settings", &showSettingsWindow, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::SeparatorText("Compression Settings");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::SliderInt("Zlib Compression Level", reinterpret_cast<int *>(&settings.zlibCompressionLevel), 1, 9);
            ImGui::SameLine();
            ImGui::Text(ICON_FA_CIRCLE_QUESTION);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(
                        "Lower values mean faster compression, higher values mean better compression, default is 9");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));

            ImGui::SliderInt("LZ4 Compression Level", reinterpret_cast<int *>(&settings.lz4CompressionLevel), 1, 16);
            ImGui::SameLine();
            ImGui::Text(ICON_FA_CIRCLE_QUESTION);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(
                        "Lower values mean faster compression, higher values mean better compression, default is 8");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));

            ImGui::SliderInt("ZSTD Compression Level", reinterpret_cast<int *>(&settings.zstdCompressionLevel), 1, 16);
            ImGui::SameLine();
            ImGui::Text(ICON_FA_CIRCLE_QUESTION);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(
                        "Lower values mean faster compression, higher values mean better compression, default is 8");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));

            ImGui::SeparatorText("Encryption Settings");
            ImGui::PushStyleColor(ImGuiCol_Text, UI::error_colour);
            ImGui::Text(ICON_FA_CIRCLE_EXCLAMATION);
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("It is best to leave these alone unless you know what you are doing");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));

            ImGui::SliderInt("Encryption Ops Limit", reinterpret_cast<int *>(&settings.encryptionOpsLimit), 1, 16);
            ImGui::SameLine();
            ImGui::Text(ICON_FA_CIRCLE_QUESTION);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(
                        "Lower values mean faster encryption, higher values mean more secure encryption, default is 1");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));

            ImGui::Text("Encryption Memory Limit");
            ImGui::SameLine();
            ImGui::Text(ICON_FA_CIRCLE_QUESTION);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Higher values mean more secure encryption, default is 8192b");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            if (ImGui::RadioButton("Min", settings.encryptionMemLimit == crypto_pwhash_MEMLIMIT_MIN)) {
                settings.encryptionMemLimit = crypto_pwhash_MEMLIMIT_MIN;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Medium", settings.encryptionMemLimit == crypto_pwhash_MEMLIMIT_INTERACTIVE)) {
                settings.encryptionMemLimit = crypto_pwhash_MEMLIMIT_INTERACTIVE;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Max", settings.encryptionMemLimit == crypto_pwhash_MEMLIMIT_MAX)) {
                settings.encryptionMemLimit = crypto_pwhash_MEMLIMIT_MAX;
            }
            ImGui::Dummy(ImVec2(0.0f, 2.0f));

            float button_width = ImGui::CalcTextSize(ICON_FA_WRENCH " Reset Defaults").x + ImGui::GetStyle().ItemSpacing.x * 4.5f;

            if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save Settings")) {
                SaveSettings();
                ImGui::CloseCurrentPopup();
                showSettingsWindow = false;
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_XMARK " Cancel")) {
                ImGui::CloseCurrentPopup();
                showSettingsWindow = false;
            }
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - button_width, 0));
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_WRENCH " Reset Defaults")) {
                settings = Settings();
            }

            ImGui::EndPopup();
        }

        if (showFileWindow) {
            ImGui::Begin("Files", &showFileWindow, ImGuiWindowFlags_NoCollapse);

            if (ImGui::Button(ICON_FA_BROOM " Clear All")) {
                files.clear();
            }

            ImGui::SameLine();
            if (ImGui::Button("Toggle Compression")) {
                for (auto &file: files) {
                    file.compressed = !file.compressed;
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Toggle Encryption")) {
                for (auto &file: files) {
                    file.encrypted = !file.encrypted;
                }
            }

            ImGui::SameLine();
            ImGui::Dummy(ImVec2(2.0f, 0.0f));
            ImGui::SameLine();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(2.0f, 0.0f));

            ImGui::SameLine();
            ImGui::Text("Compression Type:");

            for (int i = 0; i < PakTypes::CompressionCount; i++) {
                ImGui::SameLine();
                if (ImGui::RadioButton(Packer::CompressionTypeToString((PakTypes::CompressionType) i),
                                       (int *) &compressionType, i)) {
                    compressionType = (PakTypes::CompressionType) i;
                }
            }

            ImGui::SameLine();
            ImGui::Dummy(ImVec2(2.0f, 0.0f));
            ImGui::SameLine();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(2.0f, 0.0f));

            ImGui::SameLine();
            ImGui::Text("Encryption Password:");
            ImGui::SameLine();
            ImGui::InputText("##password", password, IM_ARRAYSIZE(password), ImGuiInputTextFlags_Password);

            ImGui::BeginTable("my_table", 8, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings);

            ImGui::TableSetupColumn("#");
            ImGui::TableSetupColumn("Filename");
            ImGui::TableSetupColumn("Original Path");
            ImGui::TableSetupColumn("Packed Path");
            ImGui::TableSetupColumn("Size");
            ImGui::TableSetupColumn("Compressed");
            ImGui::TableSetupColumn("Encrypted");
            ImGui::TableSetupColumn("");

            ImGui::TableHeadersRow();

            for (int i = 0; i < files.size(); i++) {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%04d", i + 1);

                ImGui::TableNextColumn();
                ImGui::Text("%s", files[i].name.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", TruncatePath(files[i].path, 40).c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", TruncatePath(files[i].packedPath, 40, true).c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", Utils::FormatBytes(files[i].size).c_str());

                ImGui::TableNextColumn();
                ImGui::PushID(i);
                ImGui::Checkbox("##compressed", &files[i].compressed);

                ImGui::TableNextColumn();
                ImGui::Checkbox("##encrypted", &files[i].encrypted);

                ImGui::TableNextColumn();
                if (ImGui::Button(ICON_FA_PEN)) {
                    std::memcpy(editPackedPath, files[i].packedPath.c_str(), files[i].packedPath.size() + 1);
                    editPackedPathIndex = i;

                    showEditPackedPath = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Edit");
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_TRASH_CAN)) {
                    files.erase(files.begin() + i);
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Remove");
                ImGui::PopID();
            }

            ImGui::EndTable();

            ImGui::End();
        }

        if (showEditPackedPath) {
            ImGui::OpenPopup("Edit Packed Path");
        }

        if (ImGui::BeginPopupModal("Edit Packed Path", &showEditPackedPath, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("Packed Path", editPackedPath, IM_ARRAYSIZE(editPackedPath));
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save Path")) {
                files[editPackedPathIndex].packedPath = editPackedPath;
                editPackedPathIndex = -1;
                editPackedPath[0] = '\0';
                ImGui::CloseCurrentPopup();
                showEditPackedPath = false;
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_XMARK " Close")) {
                editPackedPathIndex = -1;
                editPackedPath[0] = '\0';
                ImGui::CloseCurrentPopup();
                showEditPackedPath = false;
            }

            ImGui::EndPopup();
        }

        if (showExtractWindow) {
            ImGui::Begin("Packed Files", &showExtractWindow, ImGuiWindowFlags_NoCollapse);

            if (ImGui::Button(ICON_FA_BOX_OPEN " Unpack All")) {
                string savePath = SelectFolder();
                if (!savePath.empty()) {
                    bool error = false;
                    packStart = std::chrono::high_resolution_clock::now();
                    for (auto &file: pakFile.FileEntries) {
                        string outPath = Paths::GetPathWithoutFilename(savePath + "/" + file.FilePath);
                        Paths::CreateDirectories(outPath);
                        try {
                            string pwd(password);
                            unpacker.setPassword(pwd);
                            unpacker.ExtractFileToDisk(pakFile, outPath, file.FilePath);
                        } catch (const std::exception &e) {
                            MessageBoxA(nullptr, e.what(), "Error", MB_ICONERROR | MB_OK);
                            error = true;
                            break;
                        }
                    }
                    packEnd = std::chrono::high_resolution_clock::now();

                    if (!error)
                        showUnpackingComplete = true;
                }
            }

            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_CODE " Generate Include File")) {
                ImGui::OpenPopup("Include File Generator");
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Generate a C++ header file with the packed files defined as constexpr references");

            if (ImGui::BeginPopupModal("Include File Generator", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Header File");
                ImGui::Dummy(ImVec2(0.0f, 2.0f));

                string headerFile = "#pragma once\n\nnamespace Resources {\n";

                int i = 1;

                for (auto &file: pakFile.FileEntries) {
                    string filePath = file.FilePath;
                    headerFile += std::format("    constexpr auto FILE_{} = \"{}\";\n", i, filePath);

                    i++;
                }

                headerFile += "}";

                ImGui::InputTextMultiline("##header", const_cast<char *>(headerFile.c_str()), headerFile.size(),
                                          ImVec2(450, ImGui::GetTextLineHeight() * 16), ImGuiInputTextFlags_ReadOnly);
                ImGui::Dummy(ImVec2(0.0f, 2.0f));

                float button_width = ImGui::CalcTextSize(ICON_FA_FLOPPY_DISK " Save File").x + ImGui::GetStyle().ItemSpacing.x * 4.5f;

                if (ImGui::Button(clipboardButtonLabel.c_str())) {
                    ImGui::SetClipboardText(headerFile.c_str());
                    clipboardButtonLabel = ICON_FA_CHECK " Copied";
                    lastClickTime = ImGui::GetTime();
                }
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_XMARK " Close")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - button_width, 0));
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save File")) {
                    string HeaderFileName = Utils::SaveFile("C++ Header File (*.h)\0*.h\0", SaveHeaderFile);
                    if (!HeaderFileName.empty()) {
                        {
                            std::ofstream out(HeaderFileName);
                            out << headerFile;
                            out.close();
                        }
                    }
                }

                if (lastClickTime > 0.0 && ImGui::GetTime() - lastClickTime >= 2.0) {
                    clipboardButtonLabel = ICON_FA_CLIPBOARD " Copy to Clipboard";
                    lastClickTime = 0.0;
                }

                ImGui::EndPopup();
            }

            ImGui::SameLine();
            ImGui::Dummy(ImVec2(2.0f, 0.0f));
            ImGui::SameLine();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(2.0f, 0.0f));

            ImGui::SameLine();
            ImGui::Text("Password:");
            ImGui::SameLine();
            ImGui::InputText("##password", password, IM_ARRAYSIZE(password), ImGuiInputTextFlags_Password);

            ImGui::BeginTable("packed_files", 8, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings);

            ImGui::TableSetupColumn("#");
            ImGui::TableSetupColumn("Packed Path");
            ImGui::TableSetupColumn("Original Size");
            ImGui::TableSetupColumn("Packed Size");
            ImGui::TableSetupColumn("Compressed");
            ImGui::TableSetupColumn("Compression Type");
            ImGui::TableSetupColumn("Encrypted");
            ImGui::TableSetupColumn("");

            ImGui::TableHeadersRow();

            for (int i = 0; i < pakFile.FileEntries.size(); i++) {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%03d", i + 1);

                ImGui::TableNextColumn();
                ImGui::Text("%s", pakFile.FileEntries[i].FilePath);

                ImGui::TableNextColumn();
                ImGui::Text("%s", Utils::FormatBytes(pakFile.FileEntries[i].OriginalSize).c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", Utils::FormatBytes(pakFile.FileEntries[i].PackedSize).c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", pakFile.FileEntries[i].Compressed ? "Yes" : "No");

                ImGui::TableNextColumn();
                ImGui::Text("%s", pakFile.FileEntries[i].Compressed ? Packer::CompressionTypeToString(
                        pakFile.FileEntries[i].CompressionType) : "None");

                ImGui::TableNextColumn();
                ImGui::Text("%s", pakFile.FileEntries[i].Encrypted ? "Yes" : "No");

                ImGui::TableNextColumn();
                ImGui::PushID(i);
                if (ImGui::Button(ICON_FA_BOX_OPEN)) {
                    string outPath = SelectFolder();
                    if (!outPath.empty()) {
                        try {
                            packStart = std::chrono::high_resolution_clock::now();
                            string pwd(password);
                            unpacker.setPassword(pwd);
                            unpacker.ExtractFileToDisk(pakFile, outPath, pakFile.FileEntries[i].FilePath);
                            packEnd = std::chrono::high_resolution_clock::now();

                            showUnpackingComplete = true;
                        } catch (const std::exception &e) {
                            MessageBoxA(nullptr, e.what(), "Error", MB_ICONERROR | MB_OK);
                        }
                    }
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Unpack");
                ImGui::PopID();
            }

            ImGui::EndTable();

            if (showUnpackingComplete) {
                ImGui::OpenPopup("Unpacking Complete");
            }

            if (ImGui::BeginPopupModal("Unpacking Complete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                std::chrono::duration<double> elapsed = packEnd - packStart;
                ImGui::Text("Unpacking has been completed");
                ImGui::Dummy(ImVec2(0.0f, 2.0f));
                ImGui::Text("Unpacking finished in: %.2f seconds", elapsed.count());
                ImGui::Dummy(ImVec2(0.0f, 2.0f));
                if (ImGui::Button("OK")) {
                    ImGui::CloseCurrentPopup();
                    showUnpackingComplete = false;
                }
                ImGui::EndPopup();
            }

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = {clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                                                 clear_color.z * clear_color.w, clear_color.w};
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync
    }

    // Cleanup
//    io.Fonts->Clear();
//    resFile.File.close();
//    resFile.FileEntries.clear();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
//    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

void CreatePakFile(const std::string &targetPath) {
    packStart = std::chrono::high_resolution_clock::now();
    string pwd(password);
    packer.setPassword(pwd);
    if (!packer.CreatePakFile(files, targetPath, compressionType)) {
        MessageBoxA(nullptr, "Failed to create PAK file", "Error", MB_ICONERROR | MB_OK);
    }
    packEnd = std::chrono::high_resolution_clock::now();
    packing_files = false;
    packing_complete = true;
}

void SaveSettings() {
    std::ofstream os("settings", std::ios::binary);
    cereal::BinaryOutputArchive archive(os);
    archive(settings);
    os.close();

    ApplySettings();
}

void LoadSettings() {
    std::ifstream is("settings", std::ios::binary);
    if (is.good()) {
        cereal::BinaryInputArchive archive(is);
        archive(settings);

        ApplySettings();
    }
    is.close();
}

void ApplySettings() {
    packer.setZlibCompressionLevel(settings.zlibCompressionLevel);
    packer.setZstdCompressionLevel(settings.zstdCompressionLevel);
    packer.setLz4CompressionLevel(settings.lz4CompressionLevel);
    packer.setEncryptionOpsLimit(settings.encryptionOpsLimit);
    packer.setEncryptionMemLimit(settings.encryptionMemLimit);

    unpacker.setEncryptionOpsLimit(settings.encryptionOpsLimit);
    unpacker.setEncryptionMemLimit(settings.encryptionMemLimit);
}

bool CreateDeviceD3D(HWND hWnd) {
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0,};
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
                                                featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
                                                &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
                                            featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice,
                                            &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) {
        g_pSwapChain->Release();
        g_pSwapChain = nullptr;
    }
    if (g_pd3dDeviceContext) {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = nullptr;
    }
    if (g_pd3dDevice) {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
}

void CreateRenderTarget() {
    ID3D11Texture2D *pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
        case WM_DROPFILES: {
            auto hDrop = (HDROP) wParam;
            UINT num_files = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);

            if (num_files == 1) {
                TCHAR file_path[MAX_PATH];
                DragQueryFile(hDrop, 0, file_path, MAX_PATH);

                if (Paths::GetFileExtension(file_path) == ".pak") {
                    showFileWindow = false;
                    try {
                        pakFile = Unpacker::ParsePakFile(file_path);
                        showExtractWindow = true;
                    }
                    catch (const exception &e) {
                        MessageBoxA(hWnd, e.what(), "Error", MB_OK | MB_ICONERROR);
                    }
                    break;
                }
            }

            for (UINT i = 0; i < num_files; i++) {
                TCHAR file_path[MAX_PATH];
                DragQueryFile(hDrop, i, file_path, MAX_PATH);

                vector<string> tFiles;

                Paths::getFiles(file_path, tFiles);

                for (auto &tFile: tFiles) {
                    string friendlyPath = Paths::StripPath(tFile, Paths::GetDirectory(file_path));
                    Paths::ReplaceSlashes(friendlyPath);

                    PakTypes::PakFileItem file = {
                            .name = Paths::GetFileName(tFile),
                            .path = tFile,
                            .packedPath = friendlyPath,
                            .size = Paths::GetFileSize(tFile),
                            .compressed = false
                    };

                    files.emplace_back(file);
                }
            }

            DragFinish(hDrop);
            showFileWindow = true;
            break;
        }
        case WM_SIZE:
            if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, (UINT) LOWORD(lParam), (UINT) HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
            }
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
                return 0;
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

string SelectFolder() {
    BROWSEINFO bi = {nullptr};
    bi.lpszTitle = "Select a folder:";
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

    if (pidl != nullptr) {
        TCHAR path[MAX_PATH];
        if (SHGetPathFromIDList(pidl, path)) {
            return path;
        }
    }

    return "";
}
