#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <cstdio>
#include <GLFW/glfw3.h>
#include "Gui.h"

#define STB_IMAGE_IMPLEMENTATION
#include "External/stb_image.h"

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

ResPacker::Gui gui;

vector<char> mainFont;
vector<char> iconFont;

PakTypes::PakFile resFile = Unpacker::ParsePakFile("res.pak");

string resPassword = "res_packer_gui";

void drop_callback(GLFWwindow* window, int num_files, const char** paths);
void window_size_callback(GLFWwindow* window, int width, int height);
void window_content_scale_callback(GLFWwindow* window, float xscale, float yscale);
void window_focus_callback(GLFWwindow* window, int focused);

#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

// Main code
int main(int, char**)
{
    const int window_width = 1280;
    const int window_height = 720;

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    // Create window with graphics context
    gui.window = glfwCreateWindow(window_width, window_height, "Resource Packer", nullptr, nullptr);
    if (gui.window == nullptr)
        return 1;
    glfwMakeContextCurrent(gui.window);
    glfwSwapInterval(1); // Enable vsync

    glfwSetDropCallback(gui.window, drop_callback);
    glfwSetWindowSizeCallback(gui.window, window_size_callback);
    glfwSetWindowContentScaleCallback(gui.window, window_content_scale_callback);
    glfwSetWindowFocusCallback(gui.window, window_focus_callback);

    int windowX, windowY;

    glfwGetWindowSize(gui.window, &windowX, &windowY);
    gui.Setup(windowX, windowY);

    int work_posx, work_posy, work_width, work_height;
    glfwGetMonitorWorkarea(primary_monitor, &work_posx, &work_posy, &work_width, &work_height);

    const int window_posx = (work_width / 2) - windowX / 2;
    const int window_posy = (work_height / 2) - windowY / 2;

    glfwSetWindowPos(gui.window, window_posx, window_posy);

    // Setup Dear ImGui context
//    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    std::string iniPath = Utils::GetCurrentWorkingDirectory() + "/imgui.ini";

    io.IniFilename = iniPath.c_str();

    // Setup Dear ImGui style
    Theme::SetupImGuiStyle();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(gui.window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    float scaleX, scaleY;
    glfwGetWindowContentScale(gui.window, &scaleX, &scaleY);
    gui.scale = scaleX;

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);
//    io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 16.0f * scaleX);

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

    {
        int imageWidth, imageHeight, imageChannels;
        unsigned char* imagePixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(&icon[0]), icon.size(), &imageWidth, &imageHeight, &imageChannels, STBI_rgb_alpha);

        GLFWimage appIcon(imageWidth, imageHeight, imagePixels);
        glfwSetWindowIcon(gui.window, 1, &appIcon);

        GLuint image_texture;
        glGenTextures(1, &image_texture);
        glBindTexture(GL_TEXTURE_2D, image_texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#if defined(GL_UNPACK_ROW_LENGTH)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imagePixels);
        stbi_image_free(imagePixels);

        gui.iconTexture = image_texture;
        gui.iconHeight = imageHeight;
        gui.iconWidth = imageWidth;
    }

    float baseFontSize = 16.0f;
    io.Fonts->AddFontFromMemoryTTF(mainFont.data(), mainFont.size(), baseFontSize * scaleX);

    float iconFontSize = baseFontSize * 2.0f / 3.0f;
    static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = iconFontSize;
    io.Fonts->AddFontFromMemoryTTF(iconFont.data(), iconFont.size(), baseFontSize * scaleX, &icons_config, icons_ranges);

    gui.font2 = io.Fonts->AddFontFromMemoryTTF(mainFont.data(), mainFont.size(), 28.0f * scaleX);
    string SaveFileName;

    // Our state
#ifdef IMGUI_DEMO
    bool show_demo_window = false;
#endif
    ImVec4 clear_color = ImVec4(0.156f, 0.180f, 0.219, 1.0f);

    // Main loop
    while (!glfwWindowShouldClose(gui.window)) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        if (gui.renderingPaused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

#ifdef IMGUI_DEMO
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
#endif

        {
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
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(gui.window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(gui.window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
//    ImGui::DestroyContext();

    glfwDestroyWindow(gui.window);
    glfwTerminate();

    return 0;
}

void drop_callback(GLFWwindow* window, int num_files, const char** paths) {
    if (num_files == 1) {
        if (Paths::GetFileExtension(paths[0]) == ".pak") {
            gui.showFileWindow = false;
            try {
                gui.pakFile = Unpacker::ParsePakFile(paths[0]);
                gui.showExtractWindow = true;
            }
            catch (const exception &e) {
                MessageBoxA(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
            }

            return;
        }
    }

    for (int i = 0; i < num_files; i++) {
        vector<string> tFiles;

        Paths::getFiles(paths[i], tFiles);

        for (auto &tFile: tFiles) {
            string friendlyPath = Paths::StripPath(tFile, Paths::GetDirectory(paths[0]));
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

    gui.showFileWindow = true;
}

void window_size_callback(GLFWwindow* window, int width, int height) {
    gui.windowSize = ImVec2(width, height);
}

void window_content_scale_callback(GLFWwindow* window, float xscale, float yscale) {
    gui.scale = xscale;
}

void window_focus_callback(GLFWwindow* window, int focused) {
    if (focused == GLFW_FALSE) {
        gui.renderingPaused = true;
    } else {
        gui.renderingPaused = false;
    }
}