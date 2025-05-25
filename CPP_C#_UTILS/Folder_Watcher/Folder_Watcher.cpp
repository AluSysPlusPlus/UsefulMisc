/*
    author  :   Alushi
    year    :   2025
    title   :   Folder_Watching.cpp                         */


#include <iostream>
#include <filesystem>
#include <string>
#include <thread>
#include <chrono>
#include <windows.h>   // for FILETIME, GetSystemTimeAsFileTime, GetFileAttributesExW

// Helper: convert a FILETIME into a 64-bit integer of 100-ns intervals since Jan 1 1601
static ULONGLONG filetimeToUint64(const FILETIME& ft) {
    ULARGE_INTEGER u = {};
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    return u.QuadPart;
}

int main() {
    const std::filesystem::path watchDir = R"(P:\EXAMPLE)";

    std::cout << "Enter image filename (with extension): ";
    std::string filename;
    std::getline(std::cin, filename);

    std::filesystem::path target = watchDir / filename;
    if (target.string().back() == '\\' || target.string().back() == '/')
        target /= filename;   // just in case

    // 1) Record “now” as a FILETIME
    FILETIME startFt;
    GetSystemTimeAsFileTime(&startFt);
    ULONGLONG start64 = filetimeToUint64(startFt);

    std::cout << "Watching \"" << watchDir.string()
        << "\" for newly created \"" << filename << "\"...\n";

    // 2) Poll until we find a file whose creation-time > start-time
    while (true) {
        if (std::filesystem::exists(target)) {
            // fetch its file attributes
            WIN32_FILE_ATTRIBUTE_DATA data;
            if (GetFileAttributesExW(
                target.wstring().c_str(),
                GetFileExInfoStandard,
                &data))
            {
                ULONGLONG created64 = filetimeToUint64(data.ftCreationTime);
                if (created64 > start64) {
                    std::cout << "Found \"" << filename
                        << "\" (created just now) at "
                        << target.string() << "\n";
                    break;
                }
                else {
                    std::cout << "  (found, but creation-time is old; waiting for new copy)\n";
                }
            }
            else {
                std::cerr << "Error reading attributes for “"
                    << target.string() << "”\n";
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
