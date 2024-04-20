#include "gui.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_internal.h"
#include <thread>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <sys/stat.h>
#include <shobjidl.h>
#include <winrt/base.h>
#include <shlobj.h>
#include <cstdio>
#include <tlhelp32.h>
#include <tchar.h>
#include <vector>


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter
);

long __stdcall WindowProcess(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter)
{
	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
		return true;

	switch (message)
	{
	case WM_SIZE: {
		if (gui::device && wideParameter != SIZE_MINIMIZED)
		{
			gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
			gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
			gui::ResetDevice();
		}
	}return 0;

	case WM_SYSCOMMAND: {
		if ((wideParameter & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
	}break;

	case WM_DESTROY: {
		PostQuitMessage(0);
	}return 0;

	case WM_LBUTTONDOWN: {
		gui::position = MAKEPOINTS(longParameter); // set click points
	}return 0;

	case WM_MOUSEMOVE: {
		if (wideParameter == MK_LBUTTON)
		{
			const auto points = MAKEPOINTS(longParameter);
			auto rect = ::RECT{ };

			GetWindowRect(gui::window, &rect);

			rect.left += points.x - gui::position.x;
			rect.top += points.y - gui::position.y;

			if (gui::position.x >= 0 &&
				gui::position.x <= gui::WIDTH &&
				gui::position.y >= 0 && gui::position.y <= 19)
				SetWindowPos(
					gui::window,
					HWND_TOPMOST,
					rect.left,
					rect.top,
					0, 0,
					SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
				);
		}

	}return 0;

	}

	return DefWindowProc(window, message, wideParameter, longParameter);
}

void gui::CreateHWindow(const char* windowName) noexcept
{
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = "class001";
	windowClass.hIconSm = 0;

	RegisterClassEx(&windowClass);

	window = CreateWindowEx(
		0,
		"class001",
		windowName,
		WS_POPUP,
		100,
		100,
		WIDTH,
		HEIGHT,
		0,
		0,
		windowClass.hInstance,
		0
	);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);
}

void gui::DestroyHWindow() noexcept
{
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

bool gui::CreateDevice() noexcept
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d)
		return false;

	ZeroMemory(&presentParameters, sizeof(presentParameters));

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&presentParameters,
		&device) < 0)
		return false;

	return true;
}

void gui::ResetDevice() noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = device->Reset(&presentParameters);

	if (result == D3DERR_INVALIDCALL)
		IM_ASSERT(0);

	ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept
{
	if (device)
	{
		device->Release();
		device = nullptr;
	}

	if (d3d)
	{
		d3d->Release();
		d3d = nullptr;
	}
}

void gui::CreateImGui() noexcept
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO();

	io.IniFilename = NULL;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);
}

void gui::DestroyImGui() noexcept
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void gui::BeginRender() noexcept
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);

		if (message.message == WM_QUIT)
		{
			isRunning = !isRunning;
			return;
		}
	}

	// Start the Dear ImGui frame
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui::EndRender() noexcept
{
	ImGui::EndFrame();

	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	if (device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	const auto result = device->Present(0, 0, 0, 0);

	// Handle loss of D3D9 device
	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();
}

bool programRunning(std::string pName) {
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry) == TRUE) {
		while (Process32Next(snapshot, &entry) == TRUE) {
			if (std::string(entry.szExeFile) == pName) {
				CloseHandle(snapshot);
				return true;
			}
		}
	}
	CloseHandle(snapshot);
	return false;
}


void writeCfg(std::string program, bool awaitStart) {
	// Check to see if "autoStart" file exists
	std::ifstream file(".\\autoStart.txt");
	// If it exists, wipe the file contents and re-write the new data
	if (file.good()) {
		std::ofstream file(".\\autoStart.txt");
		file << program << std::endl;
		file << awaitStart << std::endl;
		file.close();
	}
	else {
		// If not, create the file and write the data
		std::ofstream file(".\\autoStart.txt");
		file << program << std::endl;
		file << awaitStart << std::endl;
		file.close();
	}
}

 std::string loadCfgOnStart() {
	 std::string actualContents;
	 std::ifstream file(".\\autoStart.txt");
	 std::string program;
	 bool awaitStart;
	if (file.good()) {
		file >> program;
		file >> awaitStart;
		file.close();
	}
	actualContents += program;
	actualContents += " ";
	actualContents += std::to_string(awaitStart);
	return actualContents;
}


 namespace Globals {
	 inline bool editing = false;
 }


void gui::Render() noexcept
{

	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });
	ImGui::Begin(
		"Lightning Launcher",
		&isRunning
	);



	auto& colors = ImGui::GetStyle().Colors;

	// Main background
	colors[ImGuiCol_WindowBg] = ImVec4{ 0.05f, 0.05f, 0.05f, 1.00f }; // Near-black for the main background

	// Menu Bar
	colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.10f, 0.10f, 0.20f, 1.00f }; // Dark blue for menu bar

	// Borders
	colors[ImGuiCol_Border] = ImVec4{ 0.20f, 0.20f, 0.25f, 1.00f }; // Dark blue tint for borders
	colors[ImGuiCol_BorderShadow] = ImVec4{ 0.00f, 0.00f, 0.00f, 0.00f }; // Clear shadow for a cleaner look

	// Text
	colors[ImGuiCol_Text] = ImVec4{ 0.90f, 0.90f, 0.90f, 1.00f };
	colors[ImGuiCol_TextDisabled] = ImVec4{ 0.60f, 0.60f, 0.60f, 1.00f };

	// Headers
	colors[ImGuiCol_Header] = ImVec4{ 0.15f, 0.15f, 0.25f, 1.00f }; // Dark blue for headers
	colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.20f, 0.20f, 0.30f, 1.00f }; // Slightly lighter blue when hovered
	colors[ImGuiCol_HeaderActive] = ImVec4{ 0.25f, 0.25f, 0.35f, 1.00f }; // Even lighter blue when active

	// Buttons
	colors[ImGuiCol_Button] = ImVec4{ 0.15f, 0.15f, 0.25f, 1.00f }; // Dark blue for buttons
	colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.20f, 0.20f, 0.30f, 1.00f };
	colors[ImGuiCol_ButtonActive] = ImVec4{ 0.25f, 0.25f, 0.35f, 1.00f };

	// CheckMark
	colors[ImGuiCol_CheckMark] = ImVec4{ 0.90f, 0.90f, 0.00f, 1.00f }; // Yellow for visibility

	// Popups
	colors[ImGuiCol_PopupBg] = ImVec4{ 0.08f, 0.08f, 0.10f, 0.92f }; // Darker blue for popups

	// Slider
	colors[ImGuiCol_SliderGrab] = ImVec4{ 0.20f, 0.20f, 0.25f, 1.00f }; // Dark blue for slider grab
	colors[ImGuiCol_SliderGrabActive] = ImVec4{ 0.25f, 0.25f, 0.35f, 1.00f }; // Lighter blue for active slider grab

	// Frame BG
	colors[ImGuiCol_FrameBg] = ImVec4{ 0.10f, 0.10f, 0.15f, 1.00f }; // Dark blue for frame background
	colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.15f, 0.15f, 0.20f, 1.00f };
	colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.20f, 0.20f, 0.25f, 1.00f };

	// Tabs
	colors[ImGuiCol_Tab] = ImVec4{ 0.10f, 0.10f, 0.15f, 1.00f };
	colors[ImGuiCol_TabHovered] = ImVec4{ 0.25f, 0.25f, 0.30f, 1.00f };
	colors[ImGuiCol_TabActive] = ImVec4{ 0.15f, 0.15f, 0.20f, 1.00f };
	colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.10f, 0.10f, 0.15f, 1.00f };
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.15f, 0.15f, 0.20f, 1.00f };

	// Title
	colors[ImGuiCol_TitleBg] = ImVec4{ 0.08f, 0.08f, 0.10f, 1.0f }; // Dark blue for title background
	colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.10f, 0.10f, 0.12f, 1.0f }; // Slightly lighter blue for active title
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.08f, 0.08f, 0.10f, 1.0f }; // Same as title background for collapsed title

	// Scrollbar
	colors[ImGuiCol_ScrollbarBg] = ImVec4{ 0.02f, 0.02f, 0.03f, 1.0f }; // Very dark blue for scrollbar background
	colors[ImGuiCol_ScrollbarGrab] = ImVec4{ 0.10f, 0.10f, 0.15f, 1.0f }; // Dark blue for scrollbar grab
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4{ 0.12f, 0.12f, 0.17f, 1.0f }; // Slightly lighter blue for hovered scrollbar grab
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4{ 0.15f, 0.15f, 0.20f, 1.0f }; // Even lighter blue for active scrollbar grab

	// Separator
	colors[ImGuiCol_Separator] = ImVec4{ 0.10f, 0.10f, 0.15f, 1.0f }; // Dark blue for separator
	colors[ImGuiCol_SeparatorHovered] = ImVec4{ 0.12f, 0.12f, 0.17f, 1.0f };
	colors[ImGuiCol_SeparatorActive] = ImVec4{ 0.90f, 0.90f, 0.00f, 1.0f }; // Yellow for active separator

	// Resize Grip
	colors[ImGuiCol_ResizeGrip] = ImVec4{ 0.10f, 0.10f, 0.15f, 0.25f }; // Semi-transparent dark blue for resize grip
	colors[ImGuiCol_ResizeGripHovered] = ImVec4{ 0.12f, 0.12f, 0.17f, 0.67f };
	colors[ImGuiCol_ResizeGripActive] = ImVec4{ 0.90f, 0.90f, 0.00f, 1.0f }; // Yellow for active resize grip


	auto& style = ImGui::GetStyle();

	// Increase the rounding for a more bubbly look
	style.TabRounding = 5;        // More rounded tabs
	style.ScrollbarRounding = 12; // Very rounded scrollbar
	style.WindowRounding = 6;     // Gently rounded corners for the windows
	style.GrabRounding = 6;       // Rounded grab handles on sliders
	style.FrameRounding = 6;      // Rounded frames
	style.PopupRounding = 6;      // Rounded pop-up windows
	style.ChildRounding = 6;      // Rounded child windows

	// You might also want to increase padding and spacing to enhance the bubbly effect
	style.FramePadding = ImVec2(8, 8);       // Padding within the frames
	style.ItemSpacing = ImVec2(1, 6);        // Spacing between items/widgets
	style.ItemInnerSpacing = ImVec2(1, 1);   // Spacing within a complex item (e.g., within a combo box)
	style.ScrollbarSize = 10;
	

	ImGui::Text("Version:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0, 1, 0, 1), "v1.2.2");


	static bool isChecking = false;
	static bool attemptedToLoadMacro = false;

	// Create a combo box with avaliable programs
	static const char* items[] = { "Valorant", "Apex Legends", "CS:GO", "Rainbow Six Siege", "Warzone", "Rust" };
	static int item_current = 0;
	
	static bool awaitProgramStart = false;

	static bool triedToLoadCfg = false;

	static std::string programT;
	if (triedToLoadCfg == false) {
		std::string settings = loadCfgOnStart();
		// Split the string into two parts
		std::string program = settings.substr(0, settings.find(" "));
		std::string awaitStart = settings.substr(settings.find(" ") + 1, settings.length());
		// Convert the string to a bool
		if (Globals::editing == false) {

			bool awaitStartBool = awaitStart == "1" ? true : false;
			if (awaitStartBool == true) {
				awaitProgramStart = true;
			}
			// Set the combo box to the program
			if (program == "Valorant") {
				item_current = 0;
			}
			else if (program == "Apex Legends") {
				item_current = 1;
			}
			else if (program == "CS:GO") {
				item_current = 2;
			}
			else if (program == "Rainbow Six Siege") {
				item_current = 3;
			}
			else if (program == "Warzone") {
				item_current = 4;
			}
			else if (program == "Rust") {
				item_current = 5;
			}
		}
	}

	

		
	ImGui::BeginTabBar("Tabs", ImGuiTabBarFlags_None);
	if (ImGui::BeginTabItem("Main")) {
		if (isChecking == true) {
			std::ifstream file(".\\ExternalCrossHairOverlay.exe");
			if (file.good()) {
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "Crosshair: Found");
			}
			else {
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "Crosshair: Not Found");
			}
			file.close();
			std::ifstream file2(".\\Lightning Macros.exe");
			if (file2.good()) {
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "Macro: Found");
			}
			else {
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "Macro: Not Found");
			}
			file2.close();
		}



		ImGui::SetCursorPos(ImVec2((gui::WIDTH / 2) - 34, gui::HEIGHT - 190));
		if (ImGui::Button("Full Macro")) {
			const char* programPath = ".\\Lightning Macros.exe";
			ShellExecuteA(nullptr, "open", programPath, nullptr, nullptr, SW_SHOWNORMAL) <= (HINSTANCE)32;
		}

		ImGui::SetCursorPos(ImVec2((gui::WIDTH / 2) - 36, (gui::HEIGHT - 128)));
		if (ImGui::Button("Check Files")) {
			isChecking = true;
		}

		ImGui::SetCursorPos(ImVec2((gui::WIDTH / 2) - 30, (gui::HEIGHT - 65)));
		if (ImGui::Button("Uninstall")) {
			const char* programPath = ".\\uninstaller.exe";
			ShellExecuteA(nullptr, "open", programPath, nullptr, nullptr, SW_SHOWNORMAL) <= (HINSTANCE)32;

		}

		// Give the user info based on the program its looking for 
		if (item_current == 0) {
			ImGui::Text("Valorant: ");
			// Check to see if the program is running
			ImGui::SameLine();
			if (programRunning("VALORANT-Win64-Shipping.exe")) {
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "Running");
			}
			else {
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "Not Running");
			}
		}
		else if (item_current == 1) {
			ImGui::Text("Apex Legends: ");
			ImGui::SameLine();

			if (programRunning("r5apex.exe")) {
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "Running");
				if (attemptedToLoadMacro == false) {
					const char* programPath = ".\\Lightning Macros.exe";
					ShellExecuteA(nullptr, "open", programPath, nullptr, nullptr, SW_SHOWNORMAL) <= (HINSTANCE)32;
					attemptedToLoadMacro = true;
				}

			}
			else {
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "Not Running");
			}
		}
		else if (item_current == 2) {
			ImGui::Text("CS:GO: ");
			ImGui::SameLine();

			if (programRunning("csgo.exe")) {
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "Running");
				if (attemptedToLoadMacro == false) {
					const char* programPath = ".\\Lightning Macros.exe";
					ShellExecuteA(nullptr, "open", programPath, nullptr, nullptr, SW_SHOWNORMAL) <= (HINSTANCE)32;
					attemptedToLoadMacro = true;
				}
			}
			else {
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "Not Running");
			}
		}
		else if (item_current == 3) {
			ImGui::Text("Rainbow Six Siege: ");
			ImGui::SameLine();

			if (programRunning("RainbowSix.exe")) {
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "Running");
				if (attemptedToLoadMacro == false) {
					const char* programPath = ".\\Lightning Macros.exe";
					ShellExecuteA(nullptr, "open", programPath, nullptr, nullptr, SW_SHOWNORMAL) <= (HINSTANCE)32;
					attemptedToLoadMacro = true;
				}
			}
			else {
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "Not Running");
			}
		}
		else if (item_current == 4) {
			ImGui::Text("Warzone: ");
			ImGui::SameLine();

			if (programRunning("ModernWarfare.exe")) {
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "Running");
				if (attemptedToLoadMacro == false) {
					const char* programPath = ".\\Lightning Macros.exe";
					ShellExecuteA(nullptr, "open", programPath, nullptr, nullptr, SW_SHOWNORMAL) <= (HINSTANCE)32;
					attemptedToLoadMacro = true;
				}
			}
			else {
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "Not Running");
			}
		}
		else if (item_current == 5) {
			ImGui::Text("Rust: ");
			ImGui::SameLine();

			if (programRunning("RustClient.exe")) {
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "Running");
				if (attemptedToLoadMacro == false) {
					const char* programPath = ".\\Lightning Macros.exe";
					ShellExecuteA(nullptr, "open", programPath, nullptr, nullptr, SW_SHOWNORMAL) <= (HINSTANCE)32;
					attemptedToLoadMacro = true;
				}
			}
			else {
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "Not Running");
			}
		}
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Auto Exec")) {
		if (ImGui::Combo("##combo", &item_current, items, IM_ARRAYSIZE(items))) {

		}
		ImGui::Checkbox("Await Program Start", &awaitProgramStart);
		ImGui::Checkbox("Edit Auto Start", &Globals::editing);
		if (ImGui::Button("Write To File")) {
			writeCfg(items[item_current], awaitProgramStart);
		}
		ImGui::EndTabItem();
	}
	ImGui::EndTabBar();

	ImGui::End();
}




