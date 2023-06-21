#pragma once

#include <fstream>
#include <string>
#include <thread>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/archives/binary.hpp>
#include "imgui.h"
#include "imgui_internal.h"
#include <GLFW/glfw3.h>
#include "External/IconsFontAwesome6.h"
#include "Version.h"
#include "Utils.h"
#include "Paths.h"
#include "PakTypes.h"
#include "Packer.h"
#include "Unpacker.h"
#include "Theme.h"
#include "Widgets.h"

namespace ResPacker {
    class Gui {
    public:
        Gui();
        void Setup(float width, float height);
        void RenderPlaceholder() const;
        void RenderMainMenu();
        void RenderAboutWindow();
        void RenderPackingProgressWindow() const;
        void RenderPackingCompleteWindow();
        void RenderSettingsWindow();
        void RenderFileWindow();
        void RenderEditPackedPathWindow();
        void RenderExtractWindow();
        void RenderHeaderGenerationWindow();
        void RenderUnpackingCompleteWindow();

        GLFWwindow* window = nullptr;

        bool showFileWindow = false;
        bool showExtractWindow = false;

        bool renderingPaused = false;

        GLuint iconTexture;
        int iconWidth = 0, iconHeight = 0;

        float scale = 1.0f;

        ImFont *font2 = nullptr;

        ImVec2 windowSize = ImVec2(0, 0);

        PakTypes::PakFile pakFile;
        vector<PakTypes::PakFileItem> files;

        Unpacker unpacker;

        struct ProjectFile {
            unsigned int version;
            PakTypes::CompressionType compressionType;
            vector<PakTypes::PakFileItem> files;
        };

    private:
        void OpenProject();
        void SaveProject();

        void OpenProjectFile(const std::string &filename);
        static std::string SaveProjectFile(std::string filename);
        static std::string SavePakFile(std::string filename);
        static std::string SaveHeaderFile(string filename);
        void CreatePakFile(const std::string &targetPath);
        static std::string SelectFolder();

        void defaultSettings();
        void SaveSettings();
        void LoadSettings();
        void ApplySettings();

        static constexpr auto PROJECT_FILE_VERSION = 1;

        bool showSettingsWindow = false;
        bool showAboutWindow = false;
        bool showEditPackedPathWindow = false;
        bool showUnpackingCompleteWindow = false;
        bool showHeaderGenerationWindow = false;

        bool packing_files = false;
        bool packing_complete = false;

        Packer packer;

        PakTypes::CompressionType compressionType = PakTypes::CompressionType::ZSTD;
        std::string SaveFileName;
        char password[256] = "";
        std::chrono::high_resolution_clock::time_point packStart;
        std::chrono::high_resolution_clock::time_point packEnd;
        string clipboardButtonLabel = ICON_FA_CLIPBOARD " Copy to Clipboard";
        double lastClickTime = 0.0;
        char editPackedPath[256] = "";
        int editPackedPathIndex = 0;

        struct Settings {
            int zlibCompressionLevel;
            int lz4CompressionLevel;
            int zstdCompressionLevel;

            size_t encryptionOpsLimit;
            size_t encryptionMemLimit;

            template<class Archive>
            void serialize(Archive &archive) {
                archive(zlibCompressionLevel, lz4CompressionLevel, zstdCompressionLevel, encryptionOpsLimit,
                        encryptionMemLimit);
            }
        };

        Settings settings{};
    };
}
