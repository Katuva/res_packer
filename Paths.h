#pragma once
#include <filesystem>
#include <vector>
#include <string>

using namespace std;

class Paths
{
public:
    static bool IsSlash(const char c)
    {
        if (c == '\\')
            return true;
        return c == '/';
    }

    static std::string GetFileName(const std::string& filePath)
    {
        return std::filesystem::path(filePath).filename().string();
    }

    static std::string GetDirectory(const std::string& filePath)
    {
        return std::filesystem::path(filePath).parent_path().filename().string();
    }

    static std::string GetFileNameWithoutExtension(const std::string& filePath)
    {
        std::string fileName = GetFileName(filePath);
        size_t dotPos = fileName.find_last_of('.');

        if (dotPos == std::string::npos)
            return fileName;
        return fileName.substr(0, dotPos);
    }

    static uintmax_t GetFileSize(const std::string& filePath)
    {
        return std::filesystem::file_size(filePath);
    }

    static std::string GetFileExtension(const std::string& filePath)
    {
        return std::filesystem::path(filePath).extension().string();
    }

    static void ReplaceFileExtension(std::string& filePath, const std::string& newExtension)
    {
        size_t dotPos = filePath.find_last_of('.');

        if (dotPos == std::string::npos)
            return;

        filePath.replace(dotPos, filePath.length() - dotPos, newExtension);
    }

    static std::string GetDirectoryPath(const std::string& filePath)
    {
        std::string parent = GetDirectory(filePath);

        return StripPath(filePath, parent);
    }

    static std::string GetPathWithoutFilename(const std::string& filePath)
    {
        return std::filesystem::path(filePath).parent_path().string();
    }

    static void CreateDirectories(const std::string& path)
    {
        std::filesystem::create_directories(path);
    }

    static void ReplaceSlashes(std::string& path)
    {
        std::replace(path.begin(), path.end(), '\\', '/');
    }

    static std::string StripPath(const std::string& path, const std::string& parentPath)
    {
        return path.substr(path.find(parentPath) + parentPath.length() + 1);
    }

    static void getFiles(const string& path, vector<string>& files)
    {
        if (filesystem::is_directory(path)) {
            for (const auto& entry : filesystem::recursive_directory_iterator(path)) {
                if (filesystem::is_regular_file(entry)) {
                    files.push_back(entry.path().string());
                }
            }
        }
        else {
            files.push_back(path);
        }
    }
};