#include <iostream>
#include <fstream>
#include <cstdlib> // For system
#include <cstdio>  // For remove
#include <filesystem> // For deleting all .txt files (C++17)

namespace fs = std::filesystem;

int main() {
    std::cout << "Are you sure you want to uninstall? (y/n): ";
    char input;
    std::cin >> input;
    std::cout << "-------------------------------------------\n";
    if (input == 'y')
    {
        // Terminate tasks with error checking
        if (system( "taskkill /f /im ExternalCrossHairOverlay.exe" ) != 0)
            std::cerr << "Failed to terminate ExternalCrossHairOverlay.exe [Try running as admin]:\n";
        if (system( "taskkill /f /im \"Lightning Macros.exe\"" ) != 0)
            std::cerr << "Failed to terminate Lightning Macros.exe [Try running as admin]:\n";
        if (system( "taskkill /f /im \"Lightning Launcher.exe\"" ) != 0)
            std::cerr << "Failed to terminate Lightning Launcher.exe [Try running as admin]:\n";

        // Attempt to remove specific files with feedback
        if (remove( ".\\ExternalCrossHairOverlay.exe" ) != 0)
            std::cerr << "Failed to remove ExternalCrossHairOverlay.exe [Try running as admin]:\n";
        if (remove( ".\\Lightning Macros.exe" ) != 0)
            std::cerr << "Failed to remove Lightning Macros.exe [Try running as admin]:\n";
        if (remove( ".\\Lightning Launcher.exe" ) != 0)
            std::cerr << "Failed to remove Lightning Launcher.exe [Try running as admin]:\n";

        // Delete all .txt files in the current directory
        std::cout << "Attempting to delete all created config files in the current directory...\n";
        for (const auto &entry : fs::directory_iterator( "." ))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".txt")
            {
                if (fs::remove( entry.path() ))
                    std::cout << "Deleted: " << entry.path().filename() << '\n';
                else
                    std::cerr << "Failed to delete [Try running as admin]: " << entry.path().filename() << '\n';
            }
        }
    }
    else
    {
        std::cout << "Removal process aborted\n";
    }

    // Keep the window open by waiting for an extra "Enter" input
    std::cout << "Press Enter to exit...";
    std::cin.ignore(); // Ignore leftover newline from the previous input
    std::cin.get();    // Wait for the final Enter press

    return 0;
}
