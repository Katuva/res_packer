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

    static std::string FormatBuildDate(const char* buildDate) {
        std::string dateStr(buildDate);

        std::string month = dateStr.substr(0, 3);
        std::string day = dateStr.substr(4, 2);
        std::string year = dateStr.substr(7, 4);

        if (month == "Jan") month = "01";
        else if (month == "Feb") month = "02";
        else if (month == "Mar") month = "03";
        else if (month == "Apr") month = "04";
        else if (month == "May") month = "05";
        else if (month == "Jun") month = "06";
        else if (month == "Jul") month = "07";
        else if (month == "Aug") month = "08";
        else if (month == "Sep") month = "09";
        else if (month == "Oct") month = "10";
        else if (month == "Nov") month = "11";
        else if (month == "Dec") month = "12";

        day.erase(std::remove(day.begin(), day.end(), ' '), day.end());

        if (day.length() == 1) {
            day = "0" + day;
        }

        return year + "/" + month + "/" + day;
    }
};