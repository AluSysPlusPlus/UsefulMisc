/*
    author  :   Alushi
    year    :   2025
    title   :   Folder_Watching_Image_Convert_Copy.cpp
*/

#include <iostream>
#include <filesystem>
#include <string>
#include <thread>
#include <chrono>
#include <windows.h>   // FILETIME, GetSystemTimeAsFileTime, GetFileAttributesExW

// Helper: convert a FILETIME into a 64-bit integer of 100-ns intervals since Jan 1 1601
static ULONGLONG filetimeToUint64(const FILETIME& ft) {
    ULARGE_INTEGER u = {};
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    return u.QuadPart;
}

int main() {
    const std::filesystem::path watchDir = R"(P:\EXAMPLE)";
    const std::filesystem::path outputFile = R"(P:\EXAMPLE\RT\input.jpg)";

    // 1) Ask for the filename to watch
    std::cout << "Enter image filename (with extension): ";
    std::string filename;
    std::getline(std::cin, filename);

    // 2) Build the full path to watch
    std::filesystem::path target = watchDir / filename;
    if (target.string().back() == '\\' || target.string().back() == '/')
        target /= filename;   // just in case user input ends with slash

    // 3) Record “now” so we only pick up files created after this point
    FILETIME startFt;
    GetSystemTimeAsFileTime(&startFt);
    ULONGLONG start64 = filetimeToUint64(startFt);

    std::cout << "Watching \"" << watchDir.string()
        << "\" for newly created \"" << filename << "\"...\n";

    // 4) Poll until we see the file with a fresh creation timestamp
    while (true) {
        if (std::filesystem::exists(target)) {
            WIN32_FILE_ATTRIBUTE_DATA data;
            if (GetFileAttributesExW(
                target.wstring().c_str(),
                GetFileExInfoStandard,
                &data))
            {
                ULONGLONG created64 = filetimeToUint64(data.ftCreationTime);
                if (created64 > start64) {
                    std::cout << "Found \"" << filename
                        << "\" at " << target.string() << "\n";

                    // --- Diagnostic & Copy Logic Below ---

                    // 5) Ensure the RT folder exists
                    std::error_code ec;
                    if (!std::filesystem::create_directories(
                        outputFile.parent_path(), ec) && ec)
                    {
                        std::cerr << "create_directories failed: ["
                            << ec.value() << "] " << ec.message() << "\n";
                        return 1;
                    }

                    // 6) Try safe copy (error_code overload)
                    bool ok = std::filesystem::copy_file(
                        target,
                        outputFile,
                        std::filesystem::copy_options::overwrite_existing,
                        ec
                    );
                    if (!ok || ec) {
                        std::cerr << "copy_file failed: ["
                            << ec.value() << "] " << ec.message() << "\n";

                        // 7) Fall back to exception‐based API for extra detail
                        try {
                            std::filesystem::copy_file(
                                target,
                                outputFile,
                                std::filesystem::copy_options::overwrite_existing
                            );
                        }
                        catch (const std::filesystem::filesystem_error& e) {
                            std::cerr << "    exception: " << e.what() << "\n";
                        }
                        return 1;
                    }

                    std::cout << "→ Copied to \"" << outputFile.string() << "\"\n";

                    // 8) List RT folder contents
                    std::cout << "RT folder now contains:\n";
                    for (auto& p : std::filesystem::directory_iterator(outputFile.parent_path())) {
                        std::cout << "  • " << p.path().filename().string();
                        if (p.path() == outputFile) std::cout << "   ← new file!";
                        std::cout << "\n";
                    }

                    // Done!
                    break;
                }
                else {
                    std::cout << "  (found, but creation-time is old; waiting for a new copy)\n";
                }
            }
            else {
                std::cerr << "Error reading attributes for \"" << target.string() << "\"\n";
            }
        }

        // wait before polling again
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
