#include "msauth/FilePicker.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <commdlg.h>
#endif

namespace msauth {

std::optional<std::filesystem::path> pickJsonFile()
{
#ifdef _WIN32
    char filename[MAX_PATH] = {};
    OPENFILENAMEA dialog = {};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = nullptr;
    dialog.lpstrFilter = "JSON files\0*.json\0All files\0*.*\0";
    dialog.lpstrFile = filename;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameA(&dialog)) {
        return std::nullopt;
    }
    return std::filesystem::path(filename);
#else
    return std::nullopt;
#endif
}

} // namespace msauth
