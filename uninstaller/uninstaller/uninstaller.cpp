#include <iostream>
#include <fstream>
#include <cstdlib> // For system
#include <cstdio> // For remove

int main()
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
	std::cin.get();
}

