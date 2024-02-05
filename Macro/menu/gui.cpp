#include "gui.h"
#include "macro.h"
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


/*void screenOverlayManager()
{
	float ScreenX = GetSystemMetrics(SM_CXSCREEN);
	float ScreenY = GetSystemMetrics(SM_CYSCREEN);
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(1920, 1080));
	ImGui::Begin("#overlay", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMouseInputs);

	auto& colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_WindowBg] = ImVec4{1.f,1.f,0.f,1.f};
	auto draw = ImGui::GetBackgroundDrawList();
	if (true)
	{
		draw->AddCircle(ImVec2(ScreenX / 2, ScreenY / 2), AimAssistConfig::assistAngle, IM_COL32(255, 0, 0, 255), 100, 1.0f);
	}
	ImGui::End();
}*/


void gui::Render() noexcept
{

	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });
	ImGui::Begin(
		"Lightning Macros",
		&isRunning//,
		//ImGuiWindowFlags_MenuBar
	);
	/*
	auto& colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.1f, 0.13f, 1.0f };
	colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

	// Border
	colors[ImGuiCol_Border] = ImVec4{ 0.44f, 0.37f, 0.61f, 0.29f };
	colors[ImGuiCol_BorderShadow] = ImVec4{ 0.0f, 0.0f, 0.0f, 0.24f };

	// Text
	colors[ImGuiCol_Text] = ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f };
	colors[ImGuiCol_TextDisabled] = ImVec4{ 0.5f, 0.5f, 0.5f, 1.0f };

	// Headers
	colors[ImGuiCol_Header] = ImVec4{ 0.13f, 0.13f, 0.17, 1.0f };
	colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
	colors[ImGuiCol_HeaderActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

	// Buttons
	colors[ImGuiCol_Button] = ImVec4{ 0.13f, 0.13f, 0.17, 1.0f };
	colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
	colors[ImGuiCol_ButtonActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
	colors[ImGuiCol_CheckMark] = ImVec4{ 0.74f, 0.58f, 0.98f, 1.0f };

	// Popups
	colors[ImGuiCol_PopupBg] = ImVec4{ 0.1f, 0.1f, 0.13f, 0.92f };

	// Slider
	colors[ImGuiCol_SliderGrab] = ImVec4{ 0.44f, 0.37f, 0.61f, 0.54f };
	colors[ImGuiCol_SliderGrabActive] = ImVec4{ 0.74f, 0.58f, 0.98f, 0.54f };

	// Frame BG
	colors[ImGuiCol_FrameBg] = ImVec4{ 0.13f, 0.13, 0.17, 1.0f };
	colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
	colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

	// Tabs
	colors[ImGuiCol_Tab] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
	colors[ImGuiCol_TabHovered] = ImVec4{ 0.24, 0.24f, 0.32f, 1.0f };
	colors[ImGuiCol_TabActive] = ImVec4{ 0.2f, 0.22f, 0.27f, 1.0f };
	colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

	// Title
	colors[ImGuiCol_TitleBg] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
	colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

	// Scrollbar
	colors[ImGuiCol_ScrollbarBg] = ImVec4{ 0.1f, 0.1f, 0.13f, 1.0f };
	colors[ImGuiCol_ScrollbarGrab] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4{ 0.24f, 0.24f, 0.32f, 1.0f };

	// Seperator
	colors[ImGuiCol_Separator] = ImVec4{ 0.44f, 0.37f, 0.61f, 1.0f };
	colors[ImGuiCol_SeparatorHovered] = ImVec4{ 0.74f, 0.58f, 0.98f, 1.0f };
	colors[ImGuiCol_SeparatorActive] = ImVec4{ 0.84f, 0.58f, 1.0f, 1.0f };

	// Resize Grip
	colors[ImGuiCol_ResizeGrip] = ImVec4{ 0.44f, 0.37f, 0.61f, 0.29f };
	colors[ImGuiCol_ResizeGripHovered] = ImVec4{ 0.74f, 0.58f, 0.98f, 0.29f };
	colors[ImGuiCol_ResizeGripActive] = ImVec4{ 0.84f, 0.58f, 1.0f, 0.29f };
	


	auto& style = ImGui::GetStyle();
	style.TabRounding = 4;
	style.ScrollbarRounding = 9;
	style.WindowRounding = 3;
	style.GrabRounding = 3;
	style.FrameRounding = 3;
	style.PopupRounding = 3;
	style.ChildRounding = 3;
	*/

	if (Globals::styleChanged == false) {

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
		style.FramePadding = ImVec2(6, 6);       // Padding within the frames
		style.ItemSpacing = ImVec2(6, 6);        // Spacing between items/widgets
		style.ItemInnerSpacing = ImVec2(6, 6);   // Spacing within a complex item (e.g., within a combo box)
		style.ScrollbarSize = 20;
	}
	else if (Globals::styleChanged == true) {
		auto& colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.1f, 0.13f, 1.0f };
		colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

		// Border
		colors[ImGuiCol_Border] = ImVec4{ 0.44f, 0.37f, 0.61f, 0.29f };
		colors[ImGuiCol_BorderShadow] = ImVec4{ 0.0f, 0.0f, 0.0f, 0.24f };

		// Text
		colors[ImGuiCol_Text] = ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f };
		colors[ImGuiCol_TextDisabled] = ImVec4{ 0.5f, 0.5f, 0.5f, 1.0f };

		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 0.13f, 0.13f, 0.17, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

		// Buttons
		colors[ImGuiCol_Button] = ImVec4{ 0.13f, 0.13f, 0.17, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
		colors[ImGuiCol_CheckMark] = ImVec4{ 0.74f, 0.58f, 0.98f, 1.0f };

		// Popups
		colors[ImGuiCol_PopupBg] = ImVec4{ 0.1f, 0.1f, 0.13f, 0.92f };

		// Slider
		colors[ImGuiCol_SliderGrab] = ImVec4{ 0.44f, 0.37f, 0.61f, 0.54f };
		colors[ImGuiCol_SliderGrabActive] = ImVec4{ 0.74f, 0.58f, 0.98f, 0.54f };

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.13f, 0.13, 0.17, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.24, 0.24f, 0.32f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.2f, 0.22f, 0.27f, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

		// Title
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

		// Scrollbar
		colors[ImGuiCol_ScrollbarBg] = ImVec4{ 0.1f, 0.1f, 0.13f, 1.0f };
		colors[ImGuiCol_ScrollbarGrab] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4{ 0.24f, 0.24f, 0.32f, 1.0f };

		// Seperator
		colors[ImGuiCol_Separator] = ImVec4{ 0.44f, 0.37f, 0.61f, 1.0f };
		colors[ImGuiCol_SeparatorHovered] = ImVec4{ 0.74f, 0.58f, 0.98f, 1.0f };
		colors[ImGuiCol_SeparatorActive] = ImVec4{ 0.84f, 0.58f, 1.0f, 1.0f };

		// Resize Grip
		colors[ImGuiCol_ResizeGrip] = ImVec4{ 0.44f, 0.37f, 0.61f, 0.29f };
		colors[ImGuiCol_ResizeGripHovered] = ImVec4{ 0.74f, 0.58f, 0.98f, 0.29f };
		colors[ImGuiCol_ResizeGripActive] = ImVec4{ 0.84f, 0.58f, 1.0f, 0.29f };



		auto& style = ImGui::GetStyle();
		style.TabRounding = 4;
		style.ScrollbarRounding = 9;
		style.WindowRounding = 3;
		style.GrabRounding = 3;
		style.FrameRounding = 3;
		style.PopupRounding = 3;
		style.ChildRounding = 3;
	}
	/*if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Options"))
		{
			if (ImGui::MenuItem("Restart", "Ctrl+Alt+Del")) {
				ShellExecute(NULL, "close", "Lightning Macros.exe", NULL, NULL, SW_SHOWDEFAULT);

				ShellExecute(NULL, "open", "Lightning Macros.exe", NULL, NULL, SW_SHOWDEFAULT);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}*/
	CheckInputs::Run();
	if (Globals::efficentMode == false) {
		Misc::Run();
		Macros::Run();
		Triggerbot::Run();
		MovePlayer::Run();
		//Aimbot::Run();

		if (Globals::scriptCheck == true) {
			ScriptCheck::Run();
		}
	}
	if (Controller::hasController == false) {
		if (No_recoil::active) {
			PullMouse::Run(No_recoil::smoothing, No_recoil::strengthX, No_recoil::strengthY);
		}

		if (AimAssistConfig::assistActive) {
			Assist::Run(AimAssistConfig::assistStrength, AimAssistConfig::assistAngle);
		}
	}
	else if (Controller::hasController == true) {
		Controller::Run();
	}
	
	if (Globals::sensY > 12) {
		if (Preset::CurrentPreset == PISTOL) {
			No_recoil::strengthX = 0;
			No_recoil::strengthY = 6 - (0.15 * Globals::sensY);
			No_recoil::smoothing = 1.f;
			No_recoil::pull_delay = 0.f;
			No_recoil::multiplier = 1.f;
		}
		else if (Preset::CurrentPreset == DMR) {
			No_recoil::strengthX = -1;
			No_recoil::strengthY = 16 - (0.15 * Globals::sensY);
			No_recoil::smoothing = 1.f;
			No_recoil::pull_delay = 0.f;
			No_recoil::multiplier = 1.f;
		}
		else if (Preset::CurrentPreset == RIFLE) {
			No_recoil::strengthX = -1;
			No_recoil::strengthY = 11 - (0.15 * Globals::sensY);
			No_recoil::smoothing = 1.f;
			No_recoil::pull_delay = 0.f;
			No_recoil::multiplier = 1.f;
		}
	}
	else if (Globals::sensY < 12) {
		if (Preset::CurrentPreset == PISTOL) {
			No_recoil::strengthX = 0;
			No_recoil::strengthY = 6 + (0.15 * Globals::sensY);
			No_recoil::smoothing = 1.f;
			No_recoil::pull_delay = 0.f;
			No_recoil::multiplier = 1.f;
		}
		else if (Preset::CurrentPreset == DMR) {
			No_recoil::strengthX = -1;
			No_recoil::strengthY = 16 + (0.15 * Globals::sensY);
			No_recoil::smoothing = 1.f;
			No_recoil::pull_delay = 0.f;
			No_recoil::multiplier = 1.f;
		}
		else if (Preset::CurrentPreset == RIFLE) {
			No_recoil::strengthX = -1;
			No_recoil::strengthY = 11 + (0.15 * Globals::sensY);
			No_recoil::smoothing = 1.f;
			No_recoil::pull_delay = 0.f;
			No_recoil::multiplier = 1.f;
		}
	}
	else if (Globals::sensY == 12) {
		if (Preset::CurrentPreset == PISTOL) {
			No_recoil::strengthX = 0;
			No_recoil::strengthY = 6;
			No_recoil::smoothing = 1.f;
			No_recoil::pull_delay = 0.f;
			No_recoil::multiplier = 1.f;
		}
		else if (Preset::CurrentPreset == DMR) {
			No_recoil::strengthX = -1;
			No_recoil::strengthY = 16;
			No_recoil::smoothing = 1.f;
			No_recoil::pull_delay = 0.f;
			No_recoil::multiplier = 1.f;
		}
		else if (Preset::CurrentPreset == RIFLE) {
			No_recoil::strengthX = -1;
			No_recoil::strengthY = 11;
			No_recoil::smoothing = 1.f;
			No_recoil::pull_delay = 0.f;
			No_recoil::multiplier = 1.f;
		}
	}



	ImGui::Text("Version:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0, 1, 0, 1), "v1.4.4 (Beta)");
	ImGui::SameLine();
	ImGui::Text("FPS:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0, 1, 0, 1), "%.1f", Misc::currentFps);
	ImGui::SameLine();
	ImGui::Text("Build:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(1, 0, 0, 1), BuildType::buildType);


	ImGui::Columns(2);
	ImGui::SetColumnOffset(1, 120);

	ImGui::Spacing();
	if (BuildType::basic == true) {
		if (ImGui::Button("Anti-Recoil", ImVec2(100, 40))) {
			Globals::ActiveTab = 1;
		}
		ImGui::Spacing();
		ImGui::Spacing();
	}

	if (BuildType::basic == true) {
		if (ImGui::Button("Crosshair", ImVec2(100, 40))) {
			Globals::ActiveTab = 3;
		}
		ImGui::Spacing();
		ImGui::Spacing();
	}

	if (BuildType::advanced == true) {
		if (ImGui::Button("Macros (WIP)", ImVec2(100, 40))) {
			Globals::ActiveTab = 2;
		}
		ImGui::Spacing();
		ImGui::Spacing();
	}



	if (BuildType::premium == true) {
		if (ImGui::Button("Overlay", ImVec2(100, 40))) {
			Globals::ActiveTab = 4;
		}
		if (ImGui::IsItemHovered()) {
			// Begin the tooltip block
			ImGui::BeginTooltip();

			// Add text to the tooltip
			ImGui::Text("Not Working");

			// End the tooltip block
			ImGui::EndTooltip();
		}
		ImGui::Spacing();
		ImGui::Spacing();
	}

	if (BuildType::advanced == true) {
		if (ImGui::Button("Aim-Assist", ImVec2(100, 40))) {
			Globals::ActiveTab = 5;
		}
		ImGui::Spacing();
		ImGui::Spacing();
	}

	if (BuildType::premium == true) {
		if (ImGui::Button("Ai Aimbot", ImVec2(100, 40))) {
			Globals::ActiveTab = 7;
		}
		if (ImGui::IsItemHovered()) {
			// Begin the tooltip block
			ImGui::BeginTooltip();

			// Add text to the tooltip
			ImGui::Text("Not Working");

			// End the tooltip block
			ImGui::EndTooltip();
		}
		ImGui::Spacing();
		ImGui::Spacing();
		if (ImGui::Button("PixelBot", ImVec2(100, 40))) {
			Globals::ActiveTab = 8;
		}
		if (ImGui::IsItemHovered()) {
			// Begin the tooltip block
			ImGui::BeginTooltip();

			// Add text to the tooltip
			ImGui::Text("Some games might need to be in windowed/borderless windowed");

			// End the tooltip block
			ImGui::EndTooltip();
		}
		ImGui::Spacing();
		ImGui::Spacing();
		if (ImGui::Button("Help", ImVec2(100, 40))) {
			Globals::ActiveTab = 9;
		}
		ImGui::Spacing();
		ImGui::Spacing();
	}

	if (ImGui::Button("Misc", ImVec2(100, 40))) {
		Globals::ActiveTab = 6;
	}
	ImGui::Spacing();
	ImGui::Spacing();

	if (ImGui::Button("Config", ImVec2(100, 40))) {
		Globals::ActiveTab = 10;
	}

	ImGui::Spacing();
	ImGui::NextColumn();
	{
		if (Globals::ActiveTab == 1)
		{
			ImGui::SetCursorPos(ImVec2(gui::WIDTH / 3, 60));
			ImGui::Text("           === Anti-Recoil ===");
			ImGui::Spacing();
			int min_valueY = 0;
			int min_valueX = -20;
			if (No_recoil::strengthY > 25) {
				No_recoil::strengthY = 25;
			}
			else if (No_recoil::strengthY < 0) {
				No_recoil::strengthY = 0;
			}
			if (No_recoil::strengthX < -20) {
				No_recoil::strengthX = -20;
			}
			else if (No_recoil::strengthX > 20) {
				No_recoil::strengthX = 20;
			};
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "F1");
			ImGui::SameLine();
			ImGui::Text("to enable the macro");
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "F2");
			ImGui::SameLine();
			ImGui::Text("to disable the macro");
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "F3");
			ImGui::SameLine();
			ImGui::Text("to close the window");
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "F4");
			ImGui::SameLine();
			ImGui::Text("to increase the vertical strength");
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "F5");
			ImGui::SameLine();
			ImGui::Text("to decrease the vertical strength");
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::SliderInt("Vertical Strength", &No_recoil::strengthY, min_valueY, 25);
			ImGui::InputInt("Vertical Manual", &No_recoil::strengthY);
			ImGui::Spacing();
			ImGui::SliderInt("Horizontal Strength", &No_recoil::strengthX, min_valueX, 20);
			ImGui::InputInt("Horizontal Manual", &No_recoil::strengthX);
			ImGui::Spacing();
			ImGui::SliderFloat("Smoothing", &No_recoil::smoothing, 0.f, 1.f);
			ImGui::Spacing();
			ImGui::SliderInt("Delay (milliseconds)", &No_recoil::pull_delay, 0.f, 100.f);
			ImGui::Spacing();
			ImGui::SliderFloat("Multiplier", &No_recoil::multiplier, 1.f, 2.0f, "%0.1f", ImGuiSliderFlags_NoInput);
			ImGui::Spacing();
			if (ImGui::Combo("Preset", &Preset::CurrentPreset, "DEFAULT\0PISTOL\0DMR\0RIFLE")) {
				Preset::SetPreset(Preset::CurrentPreset);
			}
			ImGui::Separator();
			ImGui::Checkbox("Active", &No_recoil::active);
			ImGui::Checkbox("Attach to game", &No_recoil::within_program);
			ImGui::Checkbox("Humanize", &No_recoil::humanize);
			if (ImGui::IsItemHovered()) {
				// Begin the tooltip block
				ImGui::BeginTooltip();

				// Add text to the tooltip
				ImGui::Text("Makes small jitters to make the recoil look more realistic/human");

				// End the tooltip block
				ImGui::EndTooltip();
			}
			ImGui::Spacing();
			if (ImGui::Button("Reset")) {
				No_recoil::active = false;
				No_recoil::strengthX = 0;
				No_recoil::strengthY = 0;
				No_recoil::smoothing = 1.f;
				No_recoil::pull_delay = 0.f;
				No_recoil::multiplier = 1.f;
			}
			ImGui::Spacing();
			if (No_recoil::active) {
				ImGui::Text("Anti-Recoil Status:");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "Active");
			}
			else {
				ImGui::Text("Anti-Recoil Status:");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "Not Active");
			}
		}
		if (Globals::ActiveTab == 2)
		{
			ImGui::SetCursorPos(ImVec2(150, 60));
			ImGui::Text("           === Macros (Unavaliable) ===");
			//ImGui::TextColored(ImVec4(0, 1, 0, 1), "Macros are game dependent");
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "F8");
			ImGui::SameLine();
			ImGui::Text("to enable the macro");
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "F9");
			ImGui::SameLine();
			ImGui::Text("to disable the macro");
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Checkbox("Bhop", &Macros::bhop);
			if (Macros::bhop == true) {
				if (ImGui::Combo("Bhop Key", &Bhop::HotKey, "LALT\0LBUTTON\0RBUTTON\0XBUTTON1\0XBUTTON2\0CAPITAL\0SHIFT\0CONTROL")) {
					Bhop::SetHotKey(Bhop::HotKey);
				}
			}
			ImGui::Spacing();
			ImGui::Checkbox("Turbo Crouch", &Macros::turbo_crouch);
			if (Macros::turbo_crouch == true) {
				if (ImGui::Combo("Crouch Key", &Crouch::HotKey, "LALT\0LBUTTON\0RBUTTON\0XBUTTON1\0XBUTTON2\0CAPITAL\0SHIFT\0CONTROL")) {
					Crouch::SetHotKey(Crouch::HotKey);
				}
			}
			ImGui::Spacing();
			ImGui::Checkbox("Tap Fire (happens on fire)", &Macros::rapid_fire);
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Checkbox("Master Switch", &Macros::isActive);
			ImGui::Spacing();
			if (ImGui::Button("Reset")) {
				Macros::bhop = false;
				Macros::turbo_crouch = false;
				Macros::rapid_fire = false;
			}
			ImGui::Spacing();
			if (Macros::isActive) {
				ImGui::Text("Macro Status:");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "Active");
			}
			else {
				ImGui::Text("Macro Status:");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "Not Active");
			}
			ImGui::Spacing();

		}
		if (Globals::ActiveTab == 3)
		{
			ImGui::SetCursorPos(ImVec2(gui::WIDTH / 3, 60));
			ImGui::Text("           === Crosshair ===");
			ImGui::SetCursorPos(ImVec2(gui::WIDTH / 4, 80));
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "Some games might have to be in fullscreen windowed to work");
			ImGui::SetCursorPos(ImVec2(gui::WIDTH / 2.3, 100));
			if (ImGui::Button("Open crosshair menu")) {
				const char* programPath = ".\\ExternalCrossHairOverlay.exe";
				if (ShellExecuteA(nullptr, "open", programPath, nullptr, nullptr, SW_SHOWNORMAL) <= (HINSTANCE)32) {
					CrosshairConfig::executed = false;
					CrosshairConfig::triedToExecute = true;
				}
				else {
					CrosshairConfig::executed = true;
					CrosshairConfig::triedToExecute = true;
				}
			}
			if (CrosshairConfig::triedToExecute == true && CrosshairConfig::executed == false) {
				ImGui::SetCursorPos(ImVec2(gui::WIDTH / 4, 130));
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "There was an error opening the external crosshair program");
			}
			else if (CrosshairConfig::triedToExecute == true && CrosshairConfig::executed == true) {
				ImGui::SetCursorPos(ImVec2(gui::WIDTH / 4, 130));
				ImGui::TextColored(ImVec4(0, 0, 1, 1), "The external crosshair program was opened successfully");
			}
			ImGui::Spacing();
		}
		if (Globals::ActiveTab == 4) {
			ImGui::SetCursorPos(ImVec2(gui::WIDTH / 3.8, 60));
			ImGui::Text("                   === Overlay ===");
			ImGui::Text("This feature is still in development");
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "F10");
			ImGui::SameLine();
			ImGui::Text("to enable the overlay");
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "F11");
			ImGui::SameLine();
			ImGui::Text("to disable the overlay");
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Checkbox("Active", &Overlay::isActive);
			if (Overlay::isActive == true) {
				ImGui::Checkbox("Visualize Sound", &Overlay::visualizeSound);
			}
		}
		if (Globals::ActiveTab == 5)
		{
			ImGui::SetCursorPos(ImVec2(gui::WIDTH / 3, 60));
			ImGui::Text("           === Aim-Assist ==");
			ImGui::Spacing();
			ImGui::Text("Moves the cursor from left to right");
			ImGui::Text("Press");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "F6");
			ImGui::SameLine();
			ImGui::Text("to enable the macro");
			ImGui::Text("Press");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "F7");
			ImGui::SameLine();
			ImGui::Text("to disable the macro");
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::SliderInt("Aim Assist Strength", &AimAssistConfig::assistStrength, 0, 35);
			ImGui::SliderFloat("Aim Assist FOV", &AimAssistConfig::assistAngle, 1.f, 50.f, "%0.1f");
			ImGui::SliderFloat("Aim Assist Smoothing", &AimAssistConfig::assistSmooth, 0.1f, 1.f, "%0.01f");
			ImGui::Checkbox("Aim Assist", &AimAssistConfig::assistActive);
		}
		if (Globals::ActiveTab == 6) {
			ImGui::SetCursorPos(ImVec2(gui::WIDTH / 2.8, 60));
			ImGui::Text("           === Misc ===");
			ImGui::Checkbox("FPS Counter", &Misc::fpsCounter);
			ImGui::Checkbox("Controller Support", &Controller::hasController);
			if (Controller::hasController) {
				ImGui::Separator();
				ImGui::Checkbox("Xbox", &Controller::xbox);
				if (ImGui::IsItemHovered()) {
					// Begin the tooltip block
					ImGui::BeginTooltip();

					// Add text to the tooltip
					ImGui::Text("Not Working");

					// End the tooltip block
					ImGui::EndTooltip();
				}
			//	ImGui::Checkbox("PS4", &Controller::ps4);
			//	ImGui::Checkbox("PS5", &Controller::ps5);
			}
			ImGui::SliderInt("Vertical Sens", &Globals::sensY, 0, 100);
			if (ImGui::IsItemHovered()) {
				// Begin the tooltip block
				ImGui::BeginTooltip();

				// Add text to the tooltip
				ImGui::Text("Used to calculate preset anti-recoil values");

				// End the tooltip block
				ImGui::EndTooltip();
			}
			ImGui::SliderInt("Horizontal Sens", &Globals::sensX, 0, 100);
			if (ImGui::IsItemHovered()) {
				// Begin the tooltip block
				ImGui::BeginTooltip();

				// Add text to the tooltip
				ImGui::Text("Used to calculate preset anti-recoil values");

				// End the tooltip block
				ImGui::EndTooltip();
			}
			ImGui::Checkbox("Anti-Afk kick", &Globals::antiAfk);
			ImGui::Checkbox("Low Usage", &Globals::efficentMode);
			if (ImGui::IsItemHovered()) {
				// Begin the tooltip block
				ImGui::BeginTooltip();

				// Add text to the tooltip
				ImGui::Text("Turns off other features except anti-recoil");

				// End the tooltip block
				ImGui::EndTooltip();
			}
			ImGui::Checkbox("Anti-script check", &Globals::scriptCheck);
			if (ImGui::IsItemHovered()) {
				// Begin the tooltip block
				ImGui::BeginTooltip();

				// Add text to the tooltip
				ImGui::Text("Hides program from task manager and taskbar");

				// End the tooltip block
				ImGui::EndTooltip();
			}
			ImGui::Checkbox("Purple Style", &Globals::styleChanged);
		}
		if (Globals::ActiveTab == 7) {
			ImGui::SetCursorPos(ImVec2(gui::WIDTH / 2.8, 60));
			ImGui::Text("           === Ai Aimbot ===");
			ImGui::Checkbox("Enable", &Aimbot::isActive);
			if (Aimbot::isActive == true) {
				ImGui::Separator();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Checkbox("Human Anatomy", &Aimbot::usesHumanAnatomy);
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Checkbox("Ai Detection", &Aimbot::useAi);
				ImGui::Checkbox("Auto Lock", &Aimbot::autoLock);
			}

		}
		if (Globals::ActiveTab == 8) {
			ImGui::SetCursorPos(ImVec2(gui::WIDTH / 2.8, 60));
			ImGui::Text("           === PixelBot ===");
			ImGui::Checkbox("Enable", &Triggerbot::isActive);
			if (Triggerbot::isActive == true) {
				ImGui::Checkbox("Smart Pixel Detection", &Triggerbot::useAi);
				ImGui::SliderInt("Accuracy", &Triggerbot::accuracy, 1, 10);
				if (ImGui::Combo("Hold Key", &Triggerbot::HotKey, "X\0Q\0E\0T\0")) {
					Triggerbot::SetHotKey(Triggerbot::HotKey);
				}
				for (const auto& window : Triggerbot::windowList) {
					if (ImGui::Button(window.second.c_str())) {
						Triggerbot::targetWindowHandle = window.first;
					}
				}
				ImGui::Text("Current RGB:");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "%d, %d, %d", Triggerbot::currentRGBL[0], Triggerbot::currentRGBL[1], Triggerbot::currentRGBL[2]);
				ImGui::Text("Last RGB:");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "%d, %d, %d", Triggerbot::lastRGBL[0], Triggerbot::lastRGBL[1], Triggerbot::lastRGBL[2]);
				if (Triggerbot::currentActive == true) {
				ImGui::Text("Debug:");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "Active");
				}
				else {
					ImGui::Text("Debug:");
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "Not Active");
				}
			}
		}
		if (Globals::ActiveTab == 9) {
			ImGui::SetCursorPos(ImVec2(gui::WIDTH / 2.8, 60));
			ImGui::Text("           === Help ===");
			ImGui::Text("---Macro Updates v1.4.4 (Beta) ---");
			ImGui::Text("-");
			ImGui::SameLine();
			ImGui::Text("Complete redesign of the macro interface");
			ImGui::Text("-");
			ImGui::SameLine();
			ImGui::Text("Working pixelbot");
			ImGui::Text("-");
			ImGui::SameLine();
			ImGui::Text("Cleaned up code (dev)");
			ImGui::Text("-");
			ImGui::SameLine();
			ImGui::Text("Fixed random macro failures");
			ImGui::Text("-");
			ImGui::SameLine();
			ImGui::Text("Working config system");
			ImGui::Text("-");
			ImGui::SameLine();
			ImGui::Text("Implemented structures for new features (dev)");
			ImGui::Separator();
			ImGui::Text("---Macro Help v1.4.4 (Beta) ---");
			ImGui::Spacing();
			ImGui::Text("Macro stopped working:");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "Restart the macro");
			ImGui::Text("Features not working after update:");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "Create a ticket in the discord");
			ImGui::Text("Macro not working in game:");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "Confirm macro is enabled & strength set");
			ImGui::Text("Macro not working in game (continued):");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "Restart macro and/or game");
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Text("--- Anti-Cheat status v1.4.4 (Beta) ---");
			ImGui::Text("VAC:");
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "[Undetected]");
			ImGui::Spacing();
			ImGui::Text("Battleye:");
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "[Undetected]");
			ImGui::Spacing();
			ImGui::Text("EAC:");
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "[Unknown]");
		}
		if (Globals::ActiveTab == 10) {
			ImGui::SetCursorPos(ImVec2(gui::WIDTH / 2.8, 60));
			ImGui::Text("           === Config ===");
			auto dirIter = std::filesystem::directory_iterator(std::filesystem::current_path());
			static char configNameBuffer[128] = "Config 1";
			ImGui::InputText("Config Name", configNameBuffer, sizeof(configNameBuffer));
			if (ImGui::Button("Create Config")) {
				MenuConfig::saveConfig((std::string)configNameBuffer);
			}
			ImGui::Spacing();
			int fileCount = 0;

			for (auto& entry : dirIter)
			{

				if (entry.is_regular_file() && MenuConfig::isTxtFile(entry.path().filename().string().c_str())) {
					++fileCount;
					std::string name = entry.path().filename().string().c_str();
					if (ImGui::Button(name.c_str())) {
						std::string path = entry.path().string().c_str();
						MenuConfig::setConfig(path);
					}
					ImGui::SameLine();
					if (ImGui::Button("Delete")) {
						std::string path = entry.path().string().c_str();
						MenuConfig::deleteConfig(path);
					}
				}

			}
			ImGui::Text("Config Files:");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "%d", fileCount);

		}
	}
	ImGui::End();
}




