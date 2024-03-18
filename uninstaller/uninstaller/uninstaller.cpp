#include <iostream>
#include <fstream>

int main()
{
	remove(".\\ExternalCrossHairOverlay.exe");
	remove(".\\Lightning Macros.exe");
	remove(".\\Lightning Launcher.exe");
	std::cout << "Attempted to remove files\n";
	return 0;
}

