#include <iostream>
#include <fstream>
#include <cstdlib> // For system
#include <cstdio>  // For remove

int main() {
    std::cout << "Are you sure you want to uninstall? (y/n): ";
    char input;
    std::cin >> input;
    std::cout << "-------------------------------------------\n";
    if (input == 'y')
    {
        // Terminate tasks with error checking
        if (system( "taskkill /f /im ExternalCrossHairOverlay.exe" ) != 0)
            std::cerr << "Failed to terminate ExternalCrossHairOverlay.exe\n";
        if (system( "taskkill /f /im \"Lightning Macros.exe\"" ) != 0)
            std::cerr << "Failed to terminate Lightning Macros.exe\n";
        if (system( "taskkill /f /im \"Lightning Launcher.exe\"" ) != 0)
            std::cerr << "Failed to terminate Lightning Launcher.exe\n";

        // Attempt to remove files with feedback
        if (remove( ".\\ExternalCrossHairOverlay.exe" ) != 0)
            std::cerr << "Failed to remove ExternalCrossHairOverlay.exe\n";
        if (remove( ".\\Lightning Macros.exe" ) != 0)
            std::cerr << "Failed to remove Lightning Macros.exe\n";
        if (remove( ".\\Lightning Launcher.exe" ) != 0)
            std::cerr << "Failed to remove Lightning Launcher.exe\n";
        else
            std::cout << "Attempted to remove files\n";
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
