#include <iostream>
#include <fstream>

int main()
{
	//Terminate the tasks first
	system("taskkill /f /im ExternalCrossHairOverlay.exe");
	system("taskkill /f /im Lightning Macros.exe");
	system("taskkill /f /im Lightning Launcher.exe");

	remove(".\\ExternalCrossHairOverlay.exe");
	remove(".\\Lightning Macros.exe");
	remove(".\\Lightning Launcher.exe");
	std::cout << "Attempted to remove files\n";
	std::cin.get();
}

