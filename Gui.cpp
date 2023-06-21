#include "Gui.h"

namespace cereal {
    template<class Archive>
    void serialize(Archive &archive, PakTypes::PakFileItem &item) {
        archive(item.name, item.path, item.packedPath, item.size, item.compressed, item.encrypted);
    }

    template<class Archive>
    void serialize(Archive &archive, ResPacker::Gui::ProjectFile &project) {
        archive(project.version, project.compressionType, project.files);
    }
}

namespace ResPacker {
    Gui::Gui() {
        LoadSettings();
    }

    void Gui::Setup(float width, float height) {
        windowSize = ImVec2(width, height);
    }

    void Gui::RenderPlaceholder() const {
        if (showFileWindow || showExtractWindow) {
            return;
        }

        const ImVec2 placeHolderWindowSize(400 * scale, 44 * scale);
        const std::string placeholderText = "Drag and drop files here...";

        ImGui::SetNextWindowPos(ImVec2(windowSize.x * 0.5f, windowSize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(placeHolderWindowSize);
        ImGui::Begin("DragDrop", nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings);

        ImGui::PushFont(font2);
        ImGui::SetCursorPosX((placeHolderWindowSize.x - ImGui::CalcTextSize(placeholderText.c_str()).x) / 2);
        ImGui::SetCursorPosY((placeHolderWindowSize.y - ImGui::CalcTextSize(placeholderText.c_str()).y) / 2);
        ImGui::Text("%s", placeholderText.c_str());
        ImGui::PopFont();

        ImGui::End();
    }

    void Gui::RenderMainMenu() {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        if (scale > 1)
            ImGui::SetNextWindowSize(ImVec2(windowSize.x, 56 * scale));
        else
            ImGui::SetNextWindowSize(ImVec2(windowSize.x, 67));
        ImGui::Begin("Toolbar", nullptr,
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoTitleBar);

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open Project")) {
                    Gui::OpenProject();
                }
                ImGui::Dummy(ImVec2(0.0f, 2.0f));
                if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK "  Save Project", nullptr, nullptr, !files.empty())) {
                    Gui::SaveProject();
                }
                ImGui::Dummy(ImVec2(0.0f, 2.0f));
                if (ImGui::MenuItem(ICON_FA_XMARK "   Exit")) {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }

                ImGui::EndMenu();
            }

            ImGui::Dummy(ImVec2(2.0f, 0.0f));

            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem(ICON_FA_GEAR " Settings"))
                    showSettingsWindow = true;
                ImGui::Dummy(ImVec2(0.0f, 2.0f));
                ImGui::MenuItem("Show File Window", nullptr, &showFileWindow, !files.empty());

                ImGui::EndMenu();
            }

            ImGui::Dummy(ImVec2(2.0f, 0.0f));

            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem(ICON_FA_CIRCLE_INFO " About")) {
                    showAboutWindow = true;
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        if (ImGui::Button(ICON_FA_FOLDER_OPEN " Open Project")) {
            Gui::OpenProject();
        }

        ImGui::SameLine();
        ImGui::BeginDisabled(files.empty());
        if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save Project")) {
            Gui::SaveProject();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(2.0f, 0.0f));
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(2.0f, 0.0f));

        ImGui::SameLine();
        ImGui::BeginDisabled(files.empty());
        if (ImGui::Button(ICON_FA_BOX_ARCHIVE " Pack Files")) {
            bool hasEncryptedItem = std::any_of(files.begin(), files.end(), [](const PakTypes::PakFileItem &item) {
                return item.encrypted;
            });
            string pass = password;
            if (pass.empty() && hasEncryptedItem) {
                MessageBoxA(nullptr, "Please enter an encryption password", "Error", MB_OK | MB_ICONERROR);
            } else {
                SaveFileName = Utils::SaveFile(L"Pak Files (*.pak)\0*.pak\0", SavePakFile);
                if (!SaveFileName.empty()) {
                    packing_files = true;
                    ImGui::OpenPopup("Packing Progress");
                    std::thread(&Gui::CreatePakFile, this, SaveFileName).detach();
                }
            }
        }
        ImGui::EndDisabled();
        ImGui::End();
    }

    void Gui::RenderAboutWindow() {
        if (!showAboutWindow) {
            return;
        }

        ImGui::OpenPopup(ICON_FA_CIRCLE_INFO " About");

        if (ImGui::BeginPopupModal(ICON_FA_CIRCLE_INFO " About", &showAboutWindow, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::SeparatorText("Build");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::BeginTable("build_table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            if (scale > 1)
                ImGui::Image((void*)(intptr_t)iconTexture, ImVec2(iconWidth / 1.5, iconHeight / 1.5));
            else
                ImGui::Image((void*)(intptr_t)iconTexture, ImVec2(iconWidth / 2, iconHeight / 2));
            ImGui::TableNextColumn();
            ImGui::Text("Resource Packer v%d.%d.%d",
                        RES_PACKER_GUI_VERSION_MAJOR,
                        RES_PACKER_GUI_VERSION_MINOR,
                        RES_PACKER_GUI_VERSION_PATCH);
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::Text("Built on %s at %s", Utils::FormatBuildDate(__DATE__).c_str(), __TIME__);
            ImGui::EndTable();
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::SeparatorText("Credits");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_prominent);
            ImGui::Text("Dear ImGui");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("github.com/ocornut/imgui");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_prominent);
            ImGui::Text("Zstandard");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("github.com/facebook/zstd");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_prominent);
            ImGui::Text("LZ4");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("github.com/lz4/lz4");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_prominent);
            ImGui::Text("Miniz");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("github.com/richgel999/miniz");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_prominent);
            ImGui::Text("cereal");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("github.com/USCiLab/cereal");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::text_prominent);
            ImGui::Text("Sodium");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text("github.com/jedisct1/libsodium");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            if (ImGui::Button(ICON_FA_XMARK " Close")) {
                ImGui::CloseCurrentPopup();
                showAboutWindow = false;
            }
            ImGui::EndPopup();
        }
    }

    void Gui::RenderPackingProgressWindow() const {
        if (!packing_files) {
            return;
        }

        ImGui::OpenPopup("Packing Progress");

        if (ImGui::BeginPopupModal("Packing Progress", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::Text("Packing files...");
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            Widgets::IndeterminateProgressBar(ImVec2(300 * scale, 20 * scale));
            if (!packing_files) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void Gui::RenderPackingCompleteWindow() {
        if (!packing_complete) {
            return;
        }

        ImGui::OpenPopup("Packing Result");

        if (ImGui::BeginPopupModal("Packing Result", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
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
    }

    void Gui::RenderSettingsWindow() {
        if (!showSettingsWindow) {
            return;
        }

        ImGui::OpenPopup(ICON_FA_GEAR " Settings");

        if (ImGui::BeginPopupModal(ICON_FA_GEAR " Settings", &showSettingsWindow, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
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
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::error_colour);
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
                Gui::SaveSettings();
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
                Gui::defaultSettings();
            }

            ImGui::EndPopup();
        }
    }

    void Gui::RenderFileWindow() {
        if (!showFileWindow) {
            return;
        }

        ImGui::Begin("Files To Pack", &showFileWindow, ImGuiWindowFlags_NoCollapse);

        if (ImGui::Button(ICON_FA_BROOM " Clear All")) {
            files.clear();
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_FILE_ZIPPER " Toggle Compression")) {
            for (auto &file: files) {
                file.compressed = !file.compressed;
            }
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_LOCK " Toggle Encryption")) {
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

        ImGui::Dummy(ImVec2(0.0f, 2.0f));

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
            ImGui::Text("%s", Utils::TruncatePath(files[i].path, 40).c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s", Utils::TruncatePath(files[i].packedPath, 40, true).c_str());

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

                showEditPackedPathWindow = true;
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

    void Gui::RenderEditPackedPathWindow() {
        if (!showEditPackedPathWindow) {
            return;
        }

        ImGui::OpenPopup("Edit Packed Path");

        if (ImGui::BeginPopupModal("Edit Packed Path", &showEditPackedPathWindow, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::InputText("Packed Path", editPackedPath, IM_ARRAYSIZE(editPackedPath));
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save Path")) {
                files[editPackedPathIndex].packedPath = editPackedPath;
                editPackedPathIndex = -1;
                editPackedPath[0] = '\0';
                ImGui::CloseCurrentPopup();
                showEditPackedPathWindow = false;
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_XMARK " Close")) {
                editPackedPathIndex = -1;
                editPackedPath[0] = '\0';
                ImGui::CloseCurrentPopup();
                showEditPackedPathWindow = false;
            }

            ImGui::EndPopup();
        }
    }

    void Gui::RenderExtractWindow() {
        if (!showExtractWindow) {
            return;
        }

        ImGui::Begin("Packed Files", &showExtractWindow, ImGuiWindowFlags_NoCollapse);

        if (ImGui::Button(ICON_FA_BOX_OPEN " Unpack All")) {
            string savePath = Gui::SelectFolder();
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
                    showUnpackingCompleteWindow = true;
            }
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_CODE " Generate Include File")) {
            showHeaderGenerationWindow = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Generate a C++ header file with the packed files defined as constexpr references");

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

        ImGui::Dummy(ImVec2(0.0f, 2.0f));

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

                        showUnpackingCompleteWindow = true;
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
        ImGui::End();
    }

    void Gui::RenderHeaderGenerationWindow() {
        if (!showHeaderGenerationWindow) {
            return;
        }

        ImGui::OpenPopup("Include File Generator");

        if (ImGui::BeginPopupModal("Include File Generator", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
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
                                      ImVec2(450 * scale, ImGui::GetTextLineHeight() * 16), ImGuiInputTextFlags_ReadOnly);
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
                showHeaderGenerationWindow = false;
            }
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - button_width, 0));
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_FLOPPY_DISK " Save File")) {
                string HeaderFileName = Utils::SaveFile(L"C++ Header File (*.h)\0*.h\0", SaveHeaderFile);
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
    }

    void Gui::RenderUnpackingCompleteWindow() {
        if (!showUnpackingCompleteWindow) {
            return;
        }

        ImGui::OpenPopup("Unpacking Complete");

        if (ImGui::BeginPopupModal("Unpacking Complete", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
            std::chrono::duration<double> elapsed = packEnd - packStart;
            ImGui::Text("Unpacking has been completed");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::Text("Unpacking finished in: %.2f seconds", elapsed.count());
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            if (ImGui::Button(ICON_FA_XMARK " Close")) {
                ImGui::CloseCurrentPopup();
                showUnpackingCompleteWindow = false;
            }
            ImGui::EndPopup();
        }
    }

    void Gui::CreatePakFile(const std::string &targetPath) {
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

    void Gui::OpenProjectFile(const std::string &filename) {
        ProjectFile projectFile;
        {
            std::ifstream is(filename, std::ios::binary);
            cereal::BinaryInputArchive archive(is);
            archive(projectFile);
        }

        if (projectFile.version != PROJECT_FILE_VERSION)
            throw std::runtime_error("Invalid project file version");

        compressionType = projectFile.compressionType;
        files = projectFile.files;
        showFileWindow = true;
    }

    std::string Gui::SaveProjectFile(std::string filename) {
        if (Paths::GetFileExtension(filename).empty())
            filename.append(".pakproj");
        else if (Paths::GetFileExtension(filename) != ".pakproj")
            Paths::ReplaceFileExtension(filename, ".pakproj");
        return filename;
    }

    std::string Gui::SavePakFile(std::string filename) {
        if (Paths::GetFileExtension(filename).empty())
            filename.append(".pak");
        else if (Paths::GetFileExtension(filename) != ".pak")
            Paths::ReplaceFileExtension(filename, ".pak");
        return filename;
    }

    std::string Gui::SaveHeaderFile(string filename) {
        if (Paths::GetFileExtension(filename).empty())
            filename.append(".h");
        else if (Paths::GetFileExtension(filename) != ".h")
            Paths::ReplaceFileExtension(filename, ".h");
        return filename;
    }

    std::string Gui::SelectFolder() {
        BROWSEINFO bi{};
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

    void Gui::OpenProject() {
        Utils::OpenFile(L"Pak Project (*.pakproj)\0*.pakproj\0", [this](const std::string& filename) {
            this->OpenProjectFile(filename);
        });
    }

    void Gui::SaveProject() {
        std::string projectFileName = Utils::SaveFile(L"Pak Project (*.pakproj)\0*.pakproj\0", SaveProjectFile);
        if (!projectFileName.empty()) {
            ProjectFile projectFile {
                .version = PROJECT_FILE_VERSION,
                .compressionType = compressionType,
                .files = files
            };

            std::ofstream os(projectFileName, std::ios::binary);
            cereal::BinaryOutputArchive archive(os);
            archive(projectFile);
        }
    }

    void Gui::defaultSettings() {
        settings.zlibCompressionLevel = MZ_BEST_COMPRESSION;
        settings.lz4CompressionLevel = 8;
        settings.zstdCompressionLevel = 8;
        settings.encryptionOpsLimit = crypto_pwhash_OPSLIMIT_MIN;
        settings.encryptionMemLimit = crypto_pwhash_MEMLIMIT_MIN;
    }

    void Gui::SaveSettings() {
        {
            std::ofstream os("settings", std::ios::binary);
            cereal::BinaryOutputArchive archive(os);
            archive(settings);
        }

        ApplySettings();
    }

    void Gui::LoadSettings() {
        if (std::ifstream is("settings", std::ios::binary); is.good()) {
            cereal::BinaryInputArchive archive(is);
            archive(settings);

            Gui::ApplySettings();
        } else {
            Gui::defaultSettings();
        }
    }

    void Gui::ApplySettings() {
        packer.setZlibCompressionLevel(settings.zlibCompressionLevel);
        packer.setZstdCompressionLevel(settings.zstdCompressionLevel);
        packer.setLz4CompressionLevel(settings.lz4CompressionLevel);
        packer.setEncryptionOpsLimit(settings.encryptionOpsLimit);
        packer.setEncryptionMemLimit(settings.encryptionMemLimit);

        unpacker.setEncryptionOpsLimit(settings.encryptionOpsLimit);
        unpacker.setEncryptionMemLimit(settings.encryptionMemLimit);
    }
}