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
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.10f, 0.10f, 0.10f, 1.00f };

		// Menu Bar
		colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.14f, 0.14f, 0.14f, 1.00f };

		// Borders
		colors[ImGuiCol_Border] = ImVec4{ 0.23f, 0.23f, 0.23f, 1.00f };
		colors[ImGuiCol_BorderShadow] = ImVec4{ 0.00f, 0.00f, 0.00f, 0.00f }; // Clear shadow for a cleaner look

		// Text
		colors[ImGuiCol_Text] = ImVec4{ 0.90f, 0.90f, 0.90f, 1.00f };
		colors[ImGuiCol_TextDisabled] = ImVec4{ 0.60f, 0.60f, 0.60f, 1.00f };

		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 0.40f, 0.40f, 0.40f, 1.00f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.45f, 0.45f, 0.45f, 1.00f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.53f, 0.53f, 0.53f, 1.00f };

		// Buttons
		colors[ImGuiCol_Button] = ImVec4{ 0.25f, 0.25f, 0.25f, 1.00f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.30f, 0.30f, 0.30f, 1.00f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.35f, 0.35f, 0.35f, 1.00f };

		// CheckMark
		colors[ImGuiCol_CheckMark] = ImVec4{ 0.90f, 0.90f, 0.00f, 1.00f }; // Yellow for visibility

		// Popups
		colors[ImGuiCol_PopupBg] = ImVec4{ 0.12f, 0.12f, 0.12f, 0.92f };

		// Slider
		colors[ImGuiCol_SliderGrab] = ImVec4{ 0.90f, 0.90f, 0.00f, 1.00f }; // Yellow for visibility
		colors[ImGuiCol_SliderGrabActive] = ImVec4{ 1.00f, 1.00f, 0.14f, 1.00f }; // Bright yellow for active state

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.20f, 0.20f, 0.20f, 1.00f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.25f, 0.25f, 0.25f, 1.00f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.30f, 0.30f, 0.30f, 1.00f };

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.20f, 0.20f, 0.20f, 1.00f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.60f, 0.60f, 0.60f, 1.00f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.28f, 0.28f, 1.00f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.00f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.25f, 0.25f, 0.25f, 1.00f };

		// Title
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.12f, 0.12f, 0.12f, 1.0f }; // Dark gray for title background
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.18f, 0.18f, 0.18f, 1.0f }; // Slightly lighter gray for active title
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.12f, 0.12f, 0.12f, 1.0f }; // Dark gray for collapsed title

		// Scrollbar
		colors[ImGuiCol_ScrollbarBg] = ImVec4{ 0.05f, 0.05f, 0.05f, 1.0f }; // Darker gray for scrollbar background
		colors[ImGuiCol_ScrollbarGrab] = ImVec4{ 0.18f, 0.18f, 0.18f, 1.0f }; // Gray for scrollbar grab
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4{ 0.21f, 0.21f, 0.21f, 1.0f }; // Slightly lighter gray for hovered scrollbar grab
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4{ 0.24f, 0.24f, 0.24f, 1.0f }; // Even lighter gray for active scrollbar grab

		// Separator
		colors[ImGuiCol_Separator] = ImVec4{ 0.14f, 0.14f, 0.14f, 1.0f }; // Gray for separator
		colors[ImGuiCol_SeparatorHovered] = ImVec4{ 0.26f, 0.26f, 0.26f, 1.0f }; // Lighter gray for hovered separator
		colors[ImGuiCol_SeparatorActive] = ImVec4{ 0.90f, 0.90f, 0.00f, 1.0f }; // Yellow for active separator

		// Resize Grip
		colors[ImGuiCol_ResizeGrip] = ImVec4{ 0.18f, 0.18f, 0.18f, 0.25f }; // Semi-transparent gray for resize grip
		colors[ImGuiCol_ResizeGripHovered] = ImVec4{ 0.21f, 0.21f, 0.21f, 0.67f }; // Slightly lighter and more opaque for hovered resize grip
		colors[ImGuiCol_ResizeGripActive] = ImVec4{ 0.90f, 0.90f, 0.00f, 1.0f }; // Yellow for active resize grip


		auto& style = ImGui::GetStyle();

		// Increase the rounding for a more bubbly look
		style.TabRounding = 8;        // More rounded tabs
		style.ScrollbarRounding = 12; // Very rounded scrollbar
		style.WindowRounding = 6;     // Gently rounded corners for the windows
		style.GrabRounding = 6;       // Rounded grab handles on sliders
		style.FrameRounding = 6;      // Rounded frames
		style.PopupRounding = 6;      // Rounded pop-up windows
		style.ChildRounding = 6;      // Rounded child windows

		// You might also want to increase padding and spacing to enhance the bubbly effect
		style.FramePadding = ImVec2(15, 15);       // Padding within the frames
		style.ItemSpacing = ImVec2(6, 6);        // Spacing between items/widgets
		style.ItemInnerSpacing = ImVec2(6, 6);   // Spacing within a complex item (e.g., within a combo box)
		style.ScrollbarSize = 20;

		ImGui::Text("Version:");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "v1.0.1");


		static bool isChecking = false;

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



		ImGui::SetCursorPos(ImVec2((gui::WIDTH / 2) - 70, gui::HEIGHT - 250));
		if (ImGui::Button("Standalone Crosshair")) {
			const char* programPath = ".\\ExternalCrossHairOverlay.exe";
			ShellExecuteA(nullptr, "open", programPath, nullptr, nullptr, SW_SHOWNORMAL) <= (HINSTANCE)32;
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
			remove(".\\ExternalCrossHairOverlay.exe");
			remove(".\\Lightning Macros.exe");
			remove(".\\Lightning Launcher.exe");
		}

	ImGui::End();
}




