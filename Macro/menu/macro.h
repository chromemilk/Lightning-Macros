#include <vector>
#include <thread>
#include <iostream>
#include <fstream>
#include <windows.h>
#include <iostream>
#include <algorithm>
#include <Windows.h>
#include <string>
#include <fstream>
#include <sstream>
#include <ShlObj.h>
#define DEFAULT 0
#define PISTOL 1
#define DMR 2
#define RIFLE 3

namespace No_recoil {
	inline bool adsOnly = false;
	inline int strengthX = 0;
	// neg value means left, pos means right
	inline int strengthY = 0;
	inline bool active = false;
	inline float smoothing = 1.f;
	inline int pull_delay = 0;
	// Uses the mouse_event function to move the mouse
	inline bool within_program = false;
	inline float multiplier = 1.0f;
	// Makes the mouse movement more human like by adding small increments to the mouse movement to mess with the aim
	inline bool humanize = false;
}
// The next few namespaces are for the combo boxes in the gui and the hotkeys
namespace Preset {
	inline int CurrentPreset = 0;
	inline std::vector<int> PresetList{ DEFAULT, PISTOL, DMR, RIFLE};
	inline void SetPreset(int Index)
	{
		CurrentPreset = PresetList.at(Index);
	}
}


namespace Bhop {
	inline int HotKey = 0;
	inline std::vector<int> HotKeyList{ VK_LMENU, VK_LBUTTON, VK_RBUTTON, VK_XBUTTON1, VK_XBUTTON2, VK_CAPITAL, VK_LSHIFT, VK_LCONTROL };

	inline void SetHotKey(int Index)
	{
		HotKey = HotKeyList.at(Index);
	}
}

namespace Crouch {
	inline int HotKey = 0;
	inline std::vector<int> HotKeyList{ VK_LMENU, VK_LBUTTON, VK_RBUTTON, VK_XBUTTON1, VK_XBUTTON2, VK_CAPITAL, VK_LSHIFT, VK_LCONTROL };

	inline void SetHotKey(int Index)
	{
		HotKey = HotKeyList.at(Index);
	}
}

namespace CrosshairConfig
{
	inline bool executed = false;
	inline bool triedToExecute = false;
}

namespace AimAssistConfig {
	// How many steps it will take to reach the target
	inline int assistStrength = 0;
	inline bool assistActive = false;
	// The fov on the x-axis
	inline float assistAngle = 0.0f;
	// The smoothness of the aim assist
	inline float assistSmooth = 1.0f;
}


namespace Misc {
	inline bool fpsCounter = false;
	inline double currentFps;
	inline void Run() {
		// This is a simple fps counter that counts the frames per second
		if (fpsCounter) {
			static std::chrono::time_point<std::chrono::steady_clock> oldTime = std::chrono::high_resolution_clock::now();
			static int fps; fps++;

			if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - oldTime) >= std::chrono::seconds{ 1 }) {
				oldTime = std::chrono::high_resolution_clock::now();
				Misc::currentFps = fps;
				fps = 0;
			}
		}
		else {
			currentFps = 0;
		}
		
	}

}

//TODO : Add controller support
namespace Controller {
	inline bool hasController = false;
	inline bool xbox = false;
	inline bool ps4 = false;
	inline bool ps5 = false;
	inline void ControllerRecoil() {

	}
	inline void Run() {
		if (hasController) {
			if (xbox) {
				ControllerRecoil();
			}
			else if (ps4) {
				ControllerRecoil();
			}
			else if (ps5) {
				ControllerRecoil();
			}
		}
	}
}
	
//TODO: Fix the macros
namespace Macros {
	inline bool bhop = false;
	inline bool rapid_fire = false;
	inline bool turbo_crouch = false;
	inline bool isActive = false;
	inline void Run() {
		if (Macros::bhop && Macros::isActive) {
			if (GetAsyncKeyState(Bhop::HotKey)) {
				keybd_event(VK_SPACE, 0, 0, 0); // Key down
				Sleep(100);                // Short delay
				keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0); // Key up
				Sleep(10);
			}
		}
		if (Macros::turbo_crouch && Macros::isActive) {
			if (GetAsyncKeyState(Crouch::HotKey)) {
				keybd_event('C', 0, 0, 0); // Key down
				Sleep(100);                     // Short delay
				keybd_event('C', 0, KEYEVENTF_KEYUP, 0); // Key up
			}
		}
		if (Macros::rapid_fire && Macros::isActive) {
				if (GetAsyncKeyState(VK_LBUTTON)) {
					mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
					Sleep(1000);
					mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
					Sleep(100);
				}
		}
	}
}





namespace PullMouse {
	inline void Run(float SMOOTHING_FACTOR, int DRAG_RATE_X, int DRAG_RATE_Y) {
		// Set local scope smoothing
		float smoothingX = SMOOTHING_FACTOR;
		float smoothingY = SMOOTHING_FACTOR;
		// If the drag rate is 0 then the smoothing should also be zero 
		if (DRAG_RATE_X == 0) {
			smoothingX = 0;
		}
		if (DRAG_RATE_Y == 0) {
			smoothingY = 0;
		}
		// If the LMB is pressed, we get the current position of the cursor and then add to it depending on the smoothing, and drag rate
		if (GetAsyncKeyState(VK_LBUTTON)) {
			POINT currentPos;
			GetCursorPos(&currentPos);
			// handling right click
			if (No_recoil::adsOnly == true && GetAsyncKeyState(VK_RBUTTON) == false) {
				return;
			}
			if (SMOOTHING_FACTOR >= 0) {
				// If we have a delay, wait for up to one second
				if (No_recoil::pull_delay > 0) {
					Sleep(No_recoil::pull_delay);
				}
				// Higher level/eaiser way to set cursor position
				if (No_recoil::within_program == false) {
					// Taking the drag rate and then multiplying it by a smooting factor, then multiplying it by a user selected multiplier
					SetCursorPos((currentPos.x + ((int)std::ceil(((DRAG_RATE_X * smoothingX) * No_recoil::multiplier)))), (currentPos.y + ((int)std::ceil((DRAG_RATE_Y * smoothingY) * No_recoil::multiplier))));
				}
				else if (No_recoil::within_program == true) {
					//int newX = currentPos.x + static_cast<int>(std::ceil(DRAG_RATE_X * smoothingX));
					// newY = currentPos.y + static_cast<int>(std::ceil(DRAG_RATE_Y * smoothingY));
					// The mouse_event function is absolute position based so instead of using our relative position, we just directly send commands to mouse the mouse down at a pace of deltaX and deltaY
					int deltaX = static_cast<int>(std::ceil((DRAG_RATE_X * smoothingX) * No_recoil::multiplier));
					int deltaY = static_cast<int>(std::ceil((DRAG_RATE_Y * smoothingY) * No_recoil::multiplier));
					//mouse_event(MOUSEEVENTF_MOVE, deltaX, deltaY, NULL, NULL);
					if (No_recoil::humanize == true) {
						mouse_event(MOUSEEVENTF_MOVE, deltaX + 2, deltaY - 3, NULL, NULL);

					}
					else {
						mouse_event(MOUSEEVENTF_MOVE, deltaX, deltaY, NULL, NULL);
					}
				}
				Sleep(20);
			}
		}
	}
}

namespace Assist {
	// We interpolate the mouse movement and then move the mouse to the interpolated position. THe fov is basically a start and end position
	inline void Run(int strength, float fov) {
		while (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
			int steps = static_cast<int>(std::ceil(fov * AimAssistConfig::assistSmooth));
			int dx = steps;
			int dy = static_cast<int>(std::ceil((No_recoil::strengthY * No_recoil::smoothing) * No_recoil::multiplier));;

			for (int i = 0; i <= steps; ++i) {
				if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
					break;  // Exit the loop if left mouse button is released
				}

				float t = static_cast<float>(i) / steps;
				int interpolatedDx = static_cast<int>(std::round(t * dx));

				mouse_event(MOUSEEVENTF_MOVE, interpolatedDx, dy, 0, 0);
				Sleep(strength);
			}

			for (int i = steps; i >= 0; --i) {
				if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
					break;  // Exit the loop if left mouse button is released
				}

				float t = static_cast<float>(i) / steps;
				int interpolatedDx = static_cast<int>(std::round(t * dx));

				mouse_event(MOUSEEVENTF_MOVE, -interpolatedDx, dy, 0, 0);
				Sleep(strength);
			}

			Sleep(10);  // Adjust this value to control the speed
		}
	}
}

//TODO: Add left and right sound visualization
namespace Overlay {
	inline bool isActive = false;
	inline bool visualizeSound = false;
}

namespace CheckInputs {
	// This function checks if the user has pressed a key and then sets the active variable to true or false
	inline void Run() {
		if (GetAsyncKeyState(VK_F2)) {
			No_recoil::active = false;
		}
		else if (GetAsyncKeyState(VK_F1)) {
			No_recoil::active = true;
		}
		else if (GetAsyncKeyState(VK_F4)) {
			No_recoil::strengthY += 1;
			Sleep(100);
		}
		else if (GetAsyncKeyState(VK_F5)) {
			No_recoil::strengthY -= 1;
			Sleep(100);
		}

		if (GetAsyncKeyState(VK_F6)) {
			AimAssistConfig::assistActive = true;
		}
		else if (GetAsyncKeyState(VK_F7)) {
			AimAssistConfig::assistActive = false;
		}

		if (GetAsyncKeyState(VK_F8)) {
			Macros::isActive = true;
		}
		else if (GetAsyncKeyState(VK_F9)) {
			Macros::isActive = false;
		}


		if (GetAsyncKeyState(VK_F10)) {
			Overlay::isActive = true;
		}
		else if (GetAsyncKeyState(VK_F11)) {
			Overlay::isActive = false;
		}

		if (GetAsyncKeyState(VK_F3)) {
			gui::isRunning = false;
		}
	}
}



namespace BuildType {
	// Determines the build type of the program, and thus the features that are available
	inline bool basic = true;
	inline bool premium = true;
	inline bool advanced = true;
	// For display purposes
	inline char buildType[] = "Developer";
}

namespace Globals {
	// Keeps track of the active tab
	inline int ActiveTab = 1;
	// Sensitivity for the x and y axis in game to calculate the recoil presets
	inline int sensX = 12;
	inline int sensY = 12;
	// Basically just the misc settings
	inline bool antiAfk = false;
	inline bool efficentMode = false;
	inline bool scriptCheck = false;
	//inline int currentStyle = 1;
	inline bool styleChanged = false;
}



namespace MovePlayer {
	inline void Run() {
		// Moving the player back and forth to prevent the player from being kicked for being afk
		if (Globals::antiAfk == true) {
			keybd_event('W', 0, 0, 0); // Key down
			Sleep(10);                // Short delay
			keybd_event('W', 0, KEYEVENTF_KEYUP, 0); // Key up
			Sleep(10);
			keybd_event('S', 0, 0, 0); // Key down
			Sleep(10);                // Short delay
			keybd_event('S', 0, KEYEVENTF_KEYUP, 0); // Key up
		}
	}
}

//TODO: Make the aimbot work
namespace Aimbot {
	inline bool autoLock = false;
	inline bool isActive = false;
	inline bool useAi = false;
	inline bool usesHumanAnatomy = false;

	inline void Run() {
			
	}	
}

// Gets the window handle of the game and checks the value of the pixel just below the center of the screen
// Then it checks if the value has changed and if the hotkey is pressed
namespace Triggerbot {
	inline bool isActive = false;
	inline bool useAi = false;
	inline std::vector<int> HotKeyList { 'X', 'Q', 'E', 'T' };
	inline int HotKey = 0;
	inline std::vector<double> currentRGBL = { 0, 0, 0 };
	inline std::vector<double> lastRGBL = { 0, 0, 0 };
	inline int accuracy = 0;
	inline std::vector<std::pair<HWND, std::string>> windowList;
	inline HWND targetWindowHandle = NULL;
	inline COLORREF lastColor = RGB(0, 0, 0);

	inline void SetHotKey(int Index)
	{
		HotKey = HotKeyList.at(Index);
	}

	inline std::string WideStringToString(const std::wstring& wstr) {
		if (wstr.empty()) {
			return std::string();
		}

		int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
		std::string strTo(sizeNeeded, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], sizeNeeded, NULL, NULL);
		return strTo;
	}
	inline BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
		const DWORD TITLE_SIZE = 1024;
		WCHAR windowTitle[TITLE_SIZE];

		GetWindowTextW(hwnd, windowTitle, TITLE_SIZE);

		int length = ::GetWindowTextLength(hwnd);
		if (!IsWindowVisible(hwnd) || length == 0) {
			return TRUE;
		}

		windowList.push_back({ hwnd, WideStringToString(std::wstring(windowTitle)) });
		return TRUE;
	}

	inline void UpdateWindowList() {
		windowList.clear();
		EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&windowList));
	}
	inline COLORREF GetPixelColorFromCenter() {
		if (targetWindowHandle == NULL) {
			return RGB(0, 0, 0);
		}

		HDC hWindowDC = GetDC(NULL);  // Get DC for the entire screen

		RECT rect;
		GetClientRect(targetWindowHandle, &rect);
		ClientToScreen(targetWindowHandle, (POINT*)&rect.left);  // Convert to screen coordinates
		ClientToScreen(targetWindowHandle, (POINT*)&rect.right); // Convert to screen coordinates

		int centerX = (rect.left + rect.right) / 2;
		int centerY = (rect.top + rect.bottom) / 2;

		COLORREF colorRef = GetPixel(hWindowDC, centerX - 1, centerY - 1);

		ReleaseDC(NULL, hWindowDC);
		return colorRef;
	}

	inline bool IsHotkeyPressed() {
		return (GetAsyncKeyState(Triggerbot::HotKey) & 0x8000) != 0;
	}

	inline void CheckForRGBChangeAndClick() {
		COLORREF currentRGB = GetPixelColorFromCenter();

		// Update lastRGBL for comparison
		lastRGBL[0] = GetRValue(lastColor);
		lastRGBL[1] = GetGValue(lastColor);
		lastRGBL[2] = GetBValue(lastColor);

		currentRGBL[0] = GetRValue(currentRGB);
		currentRGBL[1] = GetGValue(currentRGB);
		currentRGBL[2] = GetBValue(currentRGB);

		// Check if RGB value changed and hotkey pressed
		if ((abs(currentRGBL[0] - lastRGBL[0]) > accuracy ||
			abs(currentRGBL[1] - lastRGBL[1]) > accuracy ||
			abs(currentRGBL[2] - lastRGBL[2]) > accuracy) && IsHotkeyPressed()) {
			// Emulate left mouse click
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
			Sleep(10);
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			//PullMouse::Run(No_recoil::smoothing, No_recoil::strengthX, No_recoil::strengthY);

			return;
		}

		lastColor = currentRGB;  // Update last color after each check
	}
	inline void Run() {
		if (Triggerbot::isActive == true) {
		static DWORD lastWindowListUpdate = 0;
		DWORD currentTime = GetTickCount64();

		// Update the window list every 10 seconds (10000 milliseconds) instead of every iteration
		if (currentTime - lastWindowListUpdate > 10000) {
			UpdateWindowList();
			lastWindowListUpdate = currentTime;
		}

		CheckForRGBChangeAndClick();
		}

		//Sleep(1);  // Reduce CPU usage
	}
}

namespace MenuConfig {
	// Filters out non-txt files
	inline bool isTxtFile(const std::string& fileName) {
		// Find the last occurrence of '.'
		size_t dotPos = fileName.rfind('.');
		if (dotPos != std::string::npos) {
			// Extract the extension and compare it to ".txt"
			std::string extension = fileName.substr(dotPos);
			if (extension == ".txt") {
				return true; // The file is a .txt file
			}
		}
		return false; // The file is not a .txt file
	}

	inline std::string trim(const std::string& str) {
		size_t first = str.find_first_not_of(' ');
		if (std::string::npos == first) {
			return str;
		}
		size_t last = str.find_last_not_of(' ');
		return str.substr(first, (last - first + 1));
	}

	// Config loader
	inline void parseConfig(const std::string& filename) {
		std::ifstream configFile(filename);
		std::string line;

		while (std::getline(configFile, line)) {
			std::istringstream is_line(line);
			std::string key;
			if (std::getline(is_line, key, '=')) {
				std::string value;
				if (std::getline(is_line, value)) {
					key = trim(key);
					value = trim(value);

					// Assign the values to variables based on the key
					if (key == "No_recoil::strengthX") No_recoil::strengthX = std::stoi(value);
					else if (key == "No_recoil::strengthY") No_recoil::strengthY = std::stoi(value);
					else if (key == "No_recoil::within_program") No_recoil::within_program = value == "1";
					else if (key == "No_recoil::smoothing") No_recoil::smoothing = std::stoi(value);
					else if (key == "No_recoil::pull_delay") No_recoil::pull_delay = std::stoi(value);
					else if (key == "No_recoil::multiplier") No_recoil::multiplier = std::stoi(value);
					else if (key == "No_recoil::humanize") No_recoil::humanize = value == "1";
					else if (key == "No_recoil::adsOnly") No_recoil::adsOnly = value == "1";
					else if (key == "Triggerbot::isActive") Triggerbot::isActive = value == "1";
					else if (key == "Triggerbot::useAi") Triggerbot::useAi = value == "1";
					else if (key == "Triggerbot::HotKey") Triggerbot::HotKey = std::stoi(value);
					else if (key == "Triggerbot::accuracy") Triggerbot::accuracy = std::stoi(value);
					else if (key == "Misc::fpsCounter") Misc::fpsCounter = value == "1";
					else if (key == "Globals::efficentMode") Globals::efficentMode = value == "1";
					else if (key == "Globals::antiAfk") Globals::antiAfk = value == "1";
					else if (key == "Globals::sensX") Globals::sensX = std::stoi(value);
					else if (key == "Globals::sensY") Globals::sensY = std::stoi(value);
					else if (key == "Globals::styleChanged") Globals::styleChanged = value == "1";
					else if (key == "Overlay::isActive") Overlay::isActive = value == "1";
					else if (key == "Overlay::visualizeSound") Overlay::visualizeSound = value == "1";
					else if (key == "Macros::bhop") Macros::bhop = value == "1";
					else if (key == "Macros::rapid_fire") Macros::rapid_fire = value == "1";
					else if (key == "Macros::turbo_crouch") Macros::turbo_crouch = value == "1";
					else if (key == "AimAssistConfig::assistStrength") AimAssistConfig::assistStrength = std::stoi(value);
					else if (key == "AimAssistConfig::assistAngle") AimAssistConfig::assistAngle = std::stof(value);
					else if (key == "AimAssistConfig::assistSmooth") AimAssistConfig::assistSmooth = std::stof(value);
					else if (key == "Controller::hasController") Controller::hasController = value == "1";
					else if (key == "Controller::xbox") Controller::xbox = value == "1";
					else if (key == "Bhop::HotKey") Bhop::HotKey = std::stoi(value);
					else if (key == "Crouch::HotKey") Crouch::HotKey = std::stoi(value);
					else if (key == "Aimbot::autoLock") Aimbot::autoLock = value == "1";
					else if (key == "Aimbot::isActive") Aimbot::isActive = value == "1";
					else if (key == "Aimbot::useAi") Aimbot::useAi = value == "1";
					else if (key == "Aimbot::usesHumanAnatomy") Aimbot::usesHumanAnatomy = value == "1";
					// Add more settings as needed
				}
			}
		}
	}



	inline void setConfig(std::string fileName) {
		parseConfig(fileName);
	}


	// Deletes the file associated with the config/button 
	inline void deleteConfig(std::string fileName) {
		if (remove(fileName.c_str()) != 0) {
			return;
		}	
	}
	
	// Saves the current settings of the macro to a file with the file name specified by the user
	inline void saveConfig(std::string name) {
		std::ofstream configFile(name + ".txt");

	// Write to the file with newlines for readability
// *** No Recoil ***
		configFile << "No_recoil::strengthX=" << No_recoil::strengthX << '\n';
		configFile << "No_recoil::strengthY=" << No_recoil::strengthY << '\n';
		configFile << "No_recoil::within_program=" << No_recoil::within_program << '\n';
		configFile << "No_recoil::smoothing=" << No_recoil::smoothing << '\n';
		configFile << "No_recoil::pull_delay=" << No_recoil::pull_delay << '\n';
		configFile << "No_recoil::multiplier=" << No_recoil::multiplier << '\n';
		configFile << "No_recoil::humanize=" << No_recoil::humanize << '\n';
		configFile << "No_recoil::adsOnly=" << No_recoil::adsOnly << '\n';
		// *** PixelBot ***
		configFile << "Triggerbot::isActive=" << Triggerbot::isActive << '\n';
		configFile << "Triggerbot::useAi=" << Triggerbot::useAi << '\n';
		configFile << "Triggerbot::HotKey=" << Triggerbot::HotKey << '\n';
		configFile << "Triggerbot::accuracy=" << Triggerbot::accuracy << '\n';
		// *** Misc ***
		configFile << "Misc::fpsCounter=" << Misc::fpsCounter << '\n';
		configFile << "Globals::efficentMode=" << Globals::efficentMode << '\n';
		configFile << "Globals::antiAfk=" << Globals::antiAfk << '\n';
		configFile << "Globals::sensX=" << Globals::sensX << '\n';
		configFile << "Globals::sensY=" << Globals::sensY << '\n';
		configFile << "Globals::styleChanged=" << Globals::styleChanged << '\n';
		// *** Overlay ***
		configFile << "Overlay::isActive=" << Overlay::isActive << '\n';
		configFile << "Overlay::visualizeSound=" << Overlay::visualizeSound << '\n';
		// *** Macros *** 
		configFile << "Macros::bhop=" << Macros::bhop << '\n';
		configFile << "Macros::rapid_fire=" << Macros::rapid_fire << '\n';
		configFile << "Macros::turbo_crouch=" << Macros::turbo_crouch << '\n';
		// *** Aim-Assist ***
		configFile << "AimAssistConfig::assistStrength=" << AimAssistConfig::assistStrength << '\n';
		configFile << "AimAssistConfig::assistAngle=" << AimAssistConfig::assistAngle << '\n';
		configFile << "AimAssistConfig::assistSmooth=" << AimAssistConfig::assistSmooth << '\n';
		// *** Controller ***
		configFile << "Controller::hasController=" << Controller::hasController << '\n';
		configFile << "Controller::xbox=" << Controller::xbox << '\n';
		// *** Hotkeys ***
		configFile << "Bhop::HotKey=" << Bhop::HotKey << '\n';
		configFile << "Crouch::HotKey=" << Crouch::HotKey << '\n';
		// *** Aimbot ***
		configFile << "Aimbot::autoLock=" << Aimbot::autoLock << '\n';
		configFile << "Aimbot::isActive=" << Aimbot::isActive << '\n';
		configFile << "Aimbot::useAi=" << Aimbot::useAi << '\n';
		configFile << "Aimbot::usesHumanAnatomy=" << Aimbot::usesHumanAnatomy << '\n';



		// Close the file
		configFile.close();
	}
}

namespace ScriptCheck {
	inline void hideScript() {
		HWND hWnd = GetConsoleWindow();
		ShowWindow(hWnd, SW_HIDE);
	}

	/*inline void __stdcall EnumChildProcedure(HWND hWnd, LPARAM lParam)
	{
		char name[256];
		GetWindowText(hWnd, name, 256);

		char ClassName[256];
		GetClassName(hWnd, ClassName, 256);

		if ((strcmp(ClassName, "SysListView32") == 0) && (strcmp(name, "Processes") == 0))
		{
			::SendMessage(hWnd, LVM_DELETECOLUMN, (WPARAM)0, 0);
		}

		if ((strcmp(ClassName, "SysListView32") == 0) && (strcmp(name, "Tasks") == 0))
		{
			::SendMessage(hWnd, LVM_DELETECOLUMN, (WPARAM)0, 0);
		}
	}

	inline void hideFromTaskBar() {
		EnumChildProcedure((HWND)"Lightning Macros", NULL);

	}*/
	// WTF ???????????????????
	inline BOOL __stdcall EnumChildProcedure(HWND hWnd, LPARAM lParam)
	{
		char name[256];
		GetWindowText(hWnd, name, 256);

		char ClassName[256];
		GetClassName(hWnd, ClassName, 256);

		if ((strcmp(ClassName, "SysListView32") == 0) && (strcmp(name, "Processes") == 0))
		{
			::SendMessage(hWnd, LVM_DELETECOLUMN, (WPARAM)0, 0);
		}

		if ((strcmp(ClassName, "SysListView32") == 0) && (strcmp(name, "Tasks") == 0))
		{
			::SendMessage(hWnd, LVM_DELETECOLUMN, (WPARAM)0, 0);
		}

		return TRUE; // Return TRUE to continue enumerating child windows
	}

	inline void hideFromTaskBar() {
		// Find the window handle of "Lightning Macros" window.
		HWND hwndLightningMacros = FindWindowA(NULL, "Lightning Macros");

		if (hwndLightningMacros != NULL) {
			// Call EnumChildWindows with the correct parameters.
			EnumChildWindows(hwndLightningMacros, EnumChildProcedure, 0);
		}
	}

	inline void Run() {
		No_recoil::humanize = true;
		hideScript();
		hideFromTaskBar();
	}
}

