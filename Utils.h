#pragma once

#include <string>
#include <iomanip>
#include "Paths.h"

class Utils {
public:
    static std::string SaveFile(const char *filter, std::string (*callback)(std::string)) {
        OPENFILENAME ofn;
        wchar_t fileName[MAX_PATH] = L"";

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFilter = reinterpret_cast<LPCSTR>(filter);
        ofn.lpstrFile = reinterpret_cast<LPSTR>(fileName);
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;

        if (GetSaveFileName(&ofn) == TRUE) {
            string sFileName = ofn.lpstrFile;
            return callback(sFileName);
        }

        return "";
    }

    static void OpenFile(const char *filter, void (*callback)(std::string)) {
        OPENFILENAME ofn;
        wchar_t fileName[MAX_PATH] = L"";

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFilter = reinterpret_cast<LPCSTR>(filter);
        ofn.lpstrFile = reinterpret_cast<LPSTR>(fileName);
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;

        if (GetOpenFileName(&ofn) == TRUE) {
            std::string sFileName = ofn.lpstrFile;
            callback(sFileName);
        }
    }


    static std::string FormatBytes(uintmax_t bytes) {
        const int scale = 1024;
        static const char *units[] = {"B", "KB", "MB", "GB", "TB"};
        int unitIndex = 0;

        double size = bytes;
        while (size >= scale && unitIndex < sizeof(units) / sizeof(units[0]) - 1) {
            size /= scale;
            ++unitIndex;
        }

        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << size << ' ' << units[unitIndex];
        return ss.str();
    }
};