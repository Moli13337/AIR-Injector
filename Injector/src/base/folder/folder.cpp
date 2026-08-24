#include "folder.h"

#include <windows.h>
#include <filesystem>
#include <shlobj.h>

bool FolderManager::EnsureDirectoryExists(const std::string& path)
{
    if (!std::filesystem::exists(path)) {
        if (!CreateDirectoryA(path.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
            return false; // ! directory creation failed
        }
    }
    return true;
}

std::string FolderManager::GetDocumentsPath(const std::string& subFolder)
{
    char path[MAX_PATH];

    // Get the path to the Documents folder (CSIDL_PERSONAL refers to "My Documents")
    if (SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, path) == S_OK) {
        std::string fullPath = std::string(path) + "\\" + subFolder;

        if (!subFolder.empty() && !FolderManager::EnsureDirectoryExists(fullPath)) {
            return ""; // ! directory creation failed
        }

        return fullPath;
    }
    else {
        return "";
    }
}

std::string FolderManager::GetDllPath()
{
	// Use local AirV1.dll instead of versioned one
	return GetCurrentDir() + "\\AirV1.dll";
}

std::string FolderManager::GetVersionStringDll()
{
    std::string currentDll;
    for (const auto& entry : std::filesystem::directory_iterator(GetAirV1Folder()))
    {
        if (entry.path().extension() == ".dll" && entry.path().filename().string().starts_with("AirV1"))
        {
            currentDll = entry.path().filename().string();
            break;
        }
    }

	if (currentDll.empty())
	{
		return ""; // ! no dll found
	}

    std::string currentVersion = currentDll.substr(currentDll.find_last_of("v") + 1);
    currentVersion = currentVersion.substr(0, currentVersion.find_last_of("."));
    return currentVersion;
}

std::string FolderManager::GetAirV1Folder()
{
	return FolderManager::GetDocumentsPath("AirV1");
}

std::string FolderManager::GetCurrentDir()
{
    wchar_t buffer[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, buffer);
    std::wstring ws(buffer);
	std::string currentDirectory(ws.begin(), ws.end());
	return currentDirectory;
}