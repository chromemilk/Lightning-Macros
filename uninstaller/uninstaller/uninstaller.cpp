#include <iostream>
#include <fstream>
#include <cstdlib> // For system
#include <cstdio> // For remove

int main()
{

    std::cout << "Are you sure you want to uninstall? (y/n): ";
    char input;
    std::cin >> input;
    std::cout << "-------------------------------------------\n";
    if (input == 'y')
    {
        //Terminate the tasks first
        // Kill processes
        system("taskkill /f /im ExternalCrossHairOverlay.exe");
        system("taskkill /f /im \"Lightning Macros.exe\"");
        system("taskkill /f /im \"Lightning Launcher.exe\"");

        // Remove files
        remove(".\\ExternalCrossHairOverlay.exe");
        remove(".\\Lightning Macros.exe");
        remove(".\\Lightning Launcher.exe");

        std::cout << "Attempted to remove files\n";
	}
    else
    {
        std::cout << "Removal process aborted\n";
	}
	std::cin.get();
	return 0;
}

