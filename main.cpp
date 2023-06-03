#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include "Gui.h"

#define STB_IMAGE_IMPLEMENTATION
#include "External/stb_image.h"

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


LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ResPacker::Gui gui;

vector<char> mainFont;
vector<char> iconFont;

PakTypes::PakFile resFile = Unpacker::ParsePakFile("res.pak");

string resPassword = "res_packer_gui";

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

    // Setup Dear ImGui style
    Theme::SetupImGuiStyle();
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

    gui.unpacker.setPassword(resPassword);

    size_t encryptionOpsLimit = gui.unpacker.getEncryptionOpsLimit();
    size_t encryptionMemLimit = gui.unpacker.getEncryptionMemLimit();

    gui.unpacker.setEncryptionOpsLimit(crypto_pwhash_OPSLIMIT_MIN);
    gui.unpacker.setEncryptionMemLimit(crypto_pwhash_MEMLIMIT_MIN);

    iconFont = gui.unpacker.ExtractFileToMemory(resFile, "file1");
    mainFont = gui.unpacker.ExtractFileToMemory(resFile, "file2");
    auto icon = gui.unpacker.ExtractFileToMemory(resFile, "file3");

    gui.unpacker.setEncryptionOpsLimit(encryptionOpsLimit);
    gui.unpacker.setEncryptionMemLimit(encryptionMemLimit);

    int imageWidth, imageHeight, imageChannels;
    unsigned char* imagePixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(&icon[0]), icon.size(), &imageWidth, &imageHeight, &imageChannels, STBI_rgb_alpha);
    ID3D11Texture2D* dxTexture;
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = imageWidth;
    desc.Height = imageHeight;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA subResource;
    ZeroMemory(&subResource, sizeof(subResource));
    subResource.pSysMem = imagePixels;
    subResource.SysMemPitch = imageWidth * 4;

    g_pd3dDevice->CreateTexture2D(&desc, &subResource, &dxTexture);

    ID3D11ShaderResourceView* srv;
    g_pd3dDevice->CreateShaderResourceView(dxTexture, nullptr, &srv);
    gui.iconTexture = (ImTextureID)srv;
    gui.iconHeight = imageHeight;
    gui.iconWidth = imageWidth;
    dxTexture->Release();
    stbi_image_free(imagePixels);

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
    gui.font2 = io.Fonts->AddFontFromMemoryTTF(mainFont.data(), mainFont.size(), 28.0f);
    string SaveFileName;

    // Main loop
    while (!gui.mainLoopDone) {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                gui.mainLoopDone = true;
        }
        if (gui.mainLoopDone)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        gui.Setup();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        gui.RenderPlaceholder();
        gui.RenderMainMenu();
        gui.RenderAboutWindow();
        gui.RenderPackingProgressWindow();
        gui.RenderPackingCompleteWindow();
        gui.RenderSettingsWindow();
        gui.RenderFileWindow();
        gui.RenderEditPackedPathWindow();
        gui.RenderExtractWindow();
        gui.RenderHeaderGenerationWindow();
        gui.RenderUnpackingCompleteWindow();

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
                    gui.showFileWindow = false;
                    try {
                        gui.pakFile = Unpacker::ParsePakFile(file_path);
                        gui.showExtractWindow = true;
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

                    gui.files.emplace_back(file);
                }
            }

            DragFinish(hDrop);
            gui.showFileWindow = true;
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
