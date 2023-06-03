#pragma once

#include <string>
#include <iomanip>
#include <shlobj.h>
#include <functional>
#include "Paths.h"

class Utils {
public:
    static std::string SaveFile(const wchar_t* filter, std::string(*callback)(std::string)) {
        OPENFILENAMEW ofn;
        wchar_t fileName[MAX_PATH] = L"";

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFilter = filter;
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;

        if (GetSaveFileNameW(&ofn) == TRUE) {
            std::wstring wFileName = ofn.lpstrFile;
            std::string sFileName;
            sFileName.resize(wFileName.size());
            std::use_facet<std::ctype<wchar_t>>(std::locale()).narrow(
                    wFileName.data(), wFileName.data() + wFileName.size(), '?', &sFileName[0]);
            return callback(sFileName);
        }

        return "";
    }

    static void OpenFile(const wchar_t* filter, const std::function<void(const std::string&)>& callback) {
        OPENFILENAMEW ofn;
        wchar_t fileName[MAX_PATH] = L"";

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFilter = filter;
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;

        if (GetOpenFileNameW(&ofn) == TRUE) {
            std::wstring wFileName = ofn.lpstrFile;
            std::string sFileName;
            sFileName.resize(wFileName.size());
            std::use_facet<std::ctype<wchar_t>>(std::locale()).narrow(
                    wFileName.data(), wFileName.data() + wFileName.size(), '?', &sFileName[0]);
            callback(sFileName);
        }
    }

    static std::string FormatBytes(uintmax_t bytes) {
        constexpr int scale = 1024;
        constexpr int maxUnitIndex = 4;

        static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unitIndex = 0;

        auto size = static_cast<double>(bytes);
        if (size > 0) {
            unitIndex = static_cast<int>(std::log(size) / std::log(scale));
            unitIndex = (unitIndex < maxUnitIndex) ? unitIndex : maxUnitIndex;
            size /= std::pow(scale, unitIndex);
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

    static std::string TruncatePath(const std::string &path, size_t maxLength, bool packed = false) {
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
};