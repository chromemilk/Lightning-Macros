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
#include <algorithm>
#include <conio.h>





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
	if (Globals::saveOnClose == true) {
		MenuConfig::saveConfig((std::string)"Last Closed Config");
	}
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

bool CustomCheckbox(const char* label, bool* v)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

	const float square_sz = ImGui::GetFrameHeight();  // Height and width of the CustomCheckbox are the same
	const ImVec2 pos = window->DC.CursorPos;

	// CustomCheckbox square definition
	const ImVec2 CustomCheckbox_pos = pos;
	const ImRect CustomCheckbox_bb(CustomCheckbox_pos, ImVec2(CustomCheckbox_pos.x + square_sz, CustomCheckbox_pos.y + square_sz));

	ImGui::ItemSize(CustomCheckbox_bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(CustomCheckbox_bb, id))
		return false;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(CustomCheckbox_bb, id, &hovered, &held);
	if (pressed)
		*v = !(*v);

	// Render the CustomCheckbox
	const ImU32 col = ImGui::GetColorU32((*v) ? ImGuiCol_ButtonActive : (hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg));
	window->DrawList->AddRectFilled(CustomCheckbox_bb.Min, CustomCheckbox_bb.Max, col, style.FrameRounding);

	// Render a smaller box inside if checked
	if (*v)
	{
		const float pad = square_sz * 0.2f;  // Internal padding for the inner box
		ImVec2 inner_min(CustomCheckbox_bb.Min.x + pad, CustomCheckbox_bb.Min.y + pad);
		ImVec2 inner_max(CustomCheckbox_bb.Max.x - pad, CustomCheckbox_bb.Max.y - pad);
		window->DrawList->AddRectFilled(inner_min, inner_max, ImGui::GetColorU32(ImGuiCol_CheckMark), style.FrameRounding);
	}

	// Render the label next to the CustomCheckbox
	if (label_size.x > 0.0f)
	{
		ImGui::RenderText(ImVec2(CustomCheckbox_bb.Max.x + style.ItemInnerSpacing.x, CustomCheckbox_bb.Min.y + style.FramePadding.y), label);
	}

	return pressed;
}

bool IsKeyPressed(char key) {
	return (GetAsyncKeyState(static_cast<int>(VkKeyScan(key))) & 0x8000) != 0;
}


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


	auto& style = ImGui::GetStyle();

	if (Colors::customColors == false) {
		if (Globals::styleChanged == false) {
			auto& colors = style.Colors;
			// Title
			colors[ImGuiCol_TitleBg] = ImVec4{ 0.4f, 0.85f, 0.4f, 1.00f };
			colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
			colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
			colors[ImGuiCol_ChildBg] = ImVec4{ 0.09f, 0.09f, 0.09f, 1.00f };

			// Use a modern, flat, and slightly darker background
			colors[ImGuiCol_WindowBg] = ImVec4{ 0.09f, 0.09f, 0.09f, 1.00f };

			// Subtle differences for the menu bar to blend with the background
			colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.1f, 0.1f, 0.1f, 1.00f };

			// Simplify borders to blend in, reducing visual noise
			colors[ImGuiCol_Border] = ImVec4{ 0.1f, 0.1f, 0.1f, 1.00f };
			colors[ImGuiCol_BorderShadow] = ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f };

			// Soften text colors for better readability and less strain
			colors[ImGuiCol_Text] = ImVec4{ 0.8f, 0.8f, 0.8f, 1.00f };
			colors[ImGuiCol_TextDisabled] = ImVec4{ 0.5f, 0.5f, 0.5f, 1.00f };

			// Use more nuanced colors for interactive elements to provide a gentle feedback loop
			colors[ImGuiCol_Header] = ImVec4{ 0.3f, 0.3f, 0.3f, 1.00f };
			colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.35f, 0.35f, 0.35f, 1.00f };
			colors[ImGuiCol_HeaderActive] = ImVec4{ 0.4f, 0.4f, 0.4f, 1.00f };

			// Soften button colors for a flatter and more modern look
			colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.2f, 0.2f, 1.00f };
			colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.25f, 0.25f, 0.25f, 1.00f };
			colors[ImGuiCol_ButtonActive] = ImVec4{ 0.3f, 0.3f, 0.3f, 1.00f };

			// Modernize CheckMark and Slider with a vibrant, yet soft color for visibility
			colors[ImGuiCol_CheckMark] = ImVec4{ 0.4f, 0.85f, 0.4f, 1.00f }; // Soft green
			colors[ImGuiCol_SliderGrab] = ImVec4{ 0.4f, 0.85f, 0.4f, 1.00f };
			colors[ImGuiCol_SliderGrabActive] = ImVec4{ 0.5f, 0.9f, 0.5f, 1.00f };

			// Update frames to be flatter and integrate better with the background
			colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.2f, 0.2f, 1.00f };
			colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.25f, 0.25f, 0.25f, 1.00f };
			colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.3f, 0.3f, 0.3f, 1.00f };

			// Tabs and titles to have a minimalistic touch with subtle contrasts
			colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.00f };
			colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.38f, 0.38f, 1.00f };
			colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.28f, 0.28f, 1.00f };
			colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.15f, 0.15f, 1.00f };
			colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.2f, 0.2f, 1.00f };

			// Adjust scrollbar to be less intrusive
			colors[ImGuiCol_ScrollbarBg] = ImVec4{ 0.02f, 0.02f, 0.02f, 1.0f };
			colors[ImGuiCol_ScrollbarGrab] = ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f };
			colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4{ 0.25f, 0.25f, 0.25f, 1.0f };
			colors[ImGuiCol_ScrollbarGrabActive] = ImVec4{ 0.3f, 0.3f, 0.3f, 1.0f };

			// Maintain the modern look with soft, yet visible separators and resize grips
			colors[ImGuiCol_Separator] = ImVec4{ 0.2f, 0.2f, 0.2f, 1.0f };
			colors[ImGuiCol_SeparatorHovered] = ImVec4{ 0.3f, 0.3f, 0.3f, 1.0f };
			colors[ImGuiCol_SeparatorActive] = ImVec4{ 0.4f, 0.4f, 0.4f, 1.0f };
			colors[ImGuiCol_ResizeGrip] = ImVec4{ 0.2f, 0.2f, 0.2f, 0.3f };
			colors[ImGuiCol_ResizeGripHovered] = ImVec4{ 0.28f, 0.28f, 0.28f, 0.6f };
			colors[ImGuiCol_ResizeGripActive] = ImVec4{ 0.4f, 0.4f, 0.4f, 0.9f };
			

			if (PhysicalAttributes::custom_attributes == true) {
				style.TabRounding = PhysicalAttributes::rounding_tab;        // Less pronounced rounded tabs for a subtler look
				style.FrameRounding = PhysicalAttributes::rounding_frame;
				style.GrabRounding = PhysicalAttributes::rounding_slider;       // Rounded grab handles for a cohesive look
			}

			else {
				style.TabRounding = 3;        // Less pronounced rounded tabs for a subtler look
				style.ScrollbarRounding = 9;  // Soft, rounded scrollbar for smoother scrolling
				style.WindowRounding = 0;     // Softly rounded corners for a modern touch
				style.GrabRounding = 4;       // Rounded grab handles for a cohesive look
				style.FrameRounding = 3;      // Rounded frames for a softer interface
				style.PopupRounding = 5;      // Consistently rounded pop-up windows for uniformity
				style.ChildRounding = 5;      // Unified rounded look for child windows

				// Adjust padding and spacing for a cleaner layout and better usability
				style.FramePadding = ImVec2(6, 6);     // Slightly larger padding within frames for a roomier feel
				style.ItemSpacing = ImVec2(6, 6);      // Increased spacing between items for clarity
				style.ItemInnerSpacing = ImVec2(8, 8); // More internal spacing for a less cramped look
				style.ScrollbarSize = 18;
			}
		}
		else if (Globals::styleChanged == true) {


			style.Alpha = 1.0f;
			style.DisabledAlpha = 1.0f;
			style.WindowPadding = ImVec2(12.0f, 12.0f);
			style.WindowRounding = 0.0f;
			style.WindowBorderSize = 0.0f;
			style.WindowMinSize = ImVec2(20.0f, 20.0f);
			style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
			style.WindowMenuButtonPosition = ImGuiDir_None;
			style.ChildRounding = 0.0f;
			style.ChildBorderSize = 1.0f;
			style.PopupRounding = 6.0f;
			style.PopupBorderSize = 1.0f;
			style.FramePadding = ImVec2(6.0f, 6.0f);
			style.FrameRounding = 6.0f;
			style.FrameBorderSize = 0.0f;
			style.ItemSpacing = ImVec2(6.f, 6.f);
			style.ItemInnerSpacing = ImVec2(6.f, 6.f);
			style.CellPadding = ImVec2(12.0f, 6.0f);
			style.IndentSpacing = 15.0f;
			style.ColumnsMinSpacing = 6.0f;
			style.ScrollbarSize = 12.0f;
			style.ScrollbarRounding = 0.0f;
			style.GrabMinSize = 12.0f;
			style.GrabRounding = 0.0f;
			style.TabRounding = 0.0f;
			style.TabBorderSize = 0.0f;
			style.TabMinWidthForCloseButton = 0.0f;
			style.ColorButtonPosition = ImGuiDir_Right;
			style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
			style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

			style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.2745098173618317f, 0.3176470696926117f, 0.4509803950786591f, 1.0f);
			style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
			style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
			style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
			style.Colors[ImGuiCol_Border] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
			style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
			style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
			style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
			style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
			style.Colors[ImGuiCol_TitleBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
			style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
			style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
			style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
			style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
			style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
			style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
			style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
			style.Colors[ImGuiCol_CheckMark] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
			style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
			style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5372549295425415f, 0.5529412031173706f, 1.0f, 1.0f);
			style.Colors[ImGuiCol_Button] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
			style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f);
			style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
			style.Colors[ImGuiCol_Header] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
			style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f);
			style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
			style.Colors[ImGuiCol_Separator] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
			style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
			style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
			style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
			style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f);
			style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
			style.Colors[ImGuiCol_Tab] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
			style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
			style.Colors[ImGuiCol_TabActive] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
			style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
			style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
			style.Colors[ImGuiCol_PlotLines] = ImVec4(0.5215686559677124f, 0.6000000238418579f, 0.7019608020782471f, 1.0f);
			style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.03921568766236305f, 0.9803921580314636f, 0.9803921580314636f, 1.0f);
			style.Colors[ImGuiCol_PlotHistogram] = ImVec4(1.0f, 0.2901960909366608f, 0.5960784554481506f, 1.0f);
			style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.9960784316062927f, 0.4745098054409027f, 0.6980392336845398f, 1.0f);
			style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
			style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
			style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
			style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
			style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
			style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
			style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
			style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
			style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
			style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
			style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
		}
	}
	else {
		Colors::accentColorImVec4 = ImVec4(Colors::accentColor[0], Colors::accentColor[1], Colors::accentColor[2], Colors::accentColor[3]);

		auto& colors = style.Colors;
		// Title
		colors[ImGuiCol_CheckMark] = Colors::accentColorImVec4;
		if (Colors::includeText == true) {
			colors[ImGuiCol_Text] = Colors::accentColorImVec4;
		}
		else {
			colors[ImGuiCol_Text] = ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f };
		}
		colors[ImGuiCol_TitleBg] = Colors::accentColorImVec4;
		colors[ImGuiCol_SliderGrab] = Colors::accentColorImVec4;
		// Making title background a little weaker to allow text to be more visible
		colors[ImGuiCol_TitleBgActive] = ImVec4{ Colors::accentColor[0], Colors::accentColor[1], Colors::accentColor[2] , Colors::accentColor[3] - 0.8f };
		colors[ImGuiCol_SliderGrabActive] = Colors::accentColorImVec4;
		//colors[] = Colors::accentColorImVec4;



		if (PhysicalAttributes::custom_attributes == true) {
			style.TabRounding = PhysicalAttributes::rounding_tab;        // Less pronounced rounded tabs for a subtler look
			style.FrameRounding = PhysicalAttributes::rounding_frame;
			style.GrabRounding = PhysicalAttributes::rounding_slider;       // Rounded grab handles for a cohesive look
		}

		else {
			style.ScrollbarRounding = 9;  // Soft, rounded scrollbar for smoother scrolling
			style.WindowRounding = 0;     // Softly rounded corners for a modern touch
			style.PopupRounding = 5;      // Consistently rounded pop-up windows for uniformity
			style.ChildRounding = 5;      // Unified rounded look for child windows

			// Adjust padding and spacing for a cleaner layout and better usability
			style.FramePadding = ImVec2(6, 6);     // Slightly larger padding within frames for a roomier feel
			style.ItemSpacing = ImVec2(6, 6);      // Increased spacing between items for clarity
			style.ItemInnerSpacing = ImVec2(8, 8); // More internal spacing for a less cramped look
			style.ScrollbarSize = 18;
		}
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

		if (AutomaticRecoil::isTraining == true) {
			AutomaticRecoil::Run();
		}


		if (!CustomMacros::runKeybind.empty()) {
			if (CustomMacros::runKeybind == "Shift") {
				if (GetAsyncKeyState(VK_SHIFT)) {
					CustomMacros::Run();
				}
			}
			else if (CustomMacros::runKeybind == "Alt") {
				if (GetAsyncKeyState(VK_LMENU) || GetAsyncKeyState(VK_RMENU)) {
					CustomMacros::Run();
				}
			}
			else if (CustomMacros::runKeybind == "Mouse2") {
				if (GetAsyncKeyState(VK_RBUTTON)) {
					CustomMacros::Run();
				}
			}
			else {
				if (GetAsyncKeyState(CustomMacros::runKeybind[0])) {
					CustomMacros::Run();
				}
			}
		}
		Misc::Run();
		Macros::Run();
		Triggerbot::Run();
		MovePlayer::Run();
		if (Overlay::isActive == true) {
			Overlay::Run();
		}
		if (AntiVirus::isChecking == true) {
			AntiVirus::Run();
		}
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
	ImGui::TextColored(ImVec4(0, 1, 0, 1), "v2.1.1 (Beta)");
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
		if (ImGui::Button("Recoil Assist", ImVec2(100, 40))) {
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
		if (ImGui::Button("Gun Profiles", ImVec2(100, 40))) {
			Globals::ActiveTab = 7;
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
	if (BuildType::advanced == true) {
		if (ImGui::Button("Misc", ImVec2(100, 40))) {
			Globals::ActiveTab = 6;
		}
		ImGui::Spacing();
		ImGui::Spacing();
	}

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
			if (No_recoil::strengthY > No_recoil::maxValueY) {
				No_recoil::strengthY = No_recoil::maxValueY;
			}
			else if (No_recoil::strengthY < 0) {
				No_recoil::strengthY = 0;
			}
			if (No_recoil::strengthX < -1 * No_recoil::maxValueX) {
				No_recoil::strengthX = -1 * No_recoil::maxValueX;
			}
			else if (No_recoil::strengthX > No_recoil::maxValueX) {
				No_recoil::strengthX = No_recoil::maxValueX;
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
			ImGui::SetCursorPos(ImVec2(280, 190));
			//if (ImGui::Combo("Profile", &Preset::CurrentPreset, (char*)Profile::profileListChar)) {
			//	Preset::SetPreset(Preset::CurrentPreset);
			//}
			ImGui::SetCursorPos(ImVec2(125, 190));
			ImGui::BeginTabBar("Recoil");
			if (ImGui::BeginTabItem("Primary")) {
				ImGui::SliderInt("Vertical Strength", &No_recoil::strengthY, 0, No_recoil::maxValueY);
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();

				//ImGui::InputInt("Vertical Manual", &No_recoil::strengthY);
				ImGui::SetCursorPos(ImVec2(252, 255));
				if (ImGui::ArrowButton("##left", ImGuiDir_Left)) {
					No_recoil::strengthY--;
				}
				ImGui::SameLine();
				if (ImGui::ArrowButton("##right", ImGuiDir_Right)) {
					No_recoil::strengthY++;
				}
				ImGui::Spacing();
				ImGui::SliderInt("Horizontal Strength", &No_recoil::strengthX, -1 * No_recoil::maxValueX, No_recoil::maxValueX);
				//ImGui::InputInt("Horizontal Manual", &No_recoil::strengthX);
				ImGui::SetCursorPos(ImVec2(252, 325));
				if (ImGui::ArrowButton("##leftX", ImGuiDir_Left)) {
					No_recoil::strengthX--;
				}
				ImGui::SameLine();
				if (ImGui::ArrowButton("##rightX", ImGuiDir_Right)) {
					No_recoil::strengthX++;
				}
				ImGui::Spacing();
				ImGui::SliderFloat("Smoothing", &No_recoil::smoothing, 1.f, 0.f);
				ImGui::Spacing();
				ImGui::SliderInt("Delay (milliseconds)", &No_recoil::pull_delay, 0.f, 100.f);
				ImGui::Spacing();
				ImGui::SliderFloat("Multiplier", &No_recoil::multiplier, 1.f, 2.0f, "%0.1f", ImGuiSliderFlags_NoInput);
				ImGui::Spacing();
				if (ImGui::Combo("Preset", &Preset::CurrentPreset, "DEFAULT\0PISTOL\0DMR\0RIFLE")) {
					Preset::SetPreset(Preset::CurrentPreset);
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Secondary")) {
				ImGui::SliderInt("Vertical Strength", &No_recoil::pistolY, 0, 15);
				ImGui::SliderInt("Horizontal Strength", &No_recoil::pistolX, -5, 5);
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();	
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Automatic")) {
				// The automatic tab will automatically determine the best settings based on the guns movement
				// So, it will find out if the gun is moving left, or right, and it will determine how fast it is moving up
				// We will ask the user to help it train
				// Create a button to initiate the training
				// Create a button to stop the training
				// Create a button to save the training
				// Create a button to load the training
				// Create a button to delete the training
				if (ImGui::Button("Start Training")) {
					AutomaticRecoil::isTraining = true;
				}
				if (ImGui::Button("Stop Training")) {
					AutomaticRecoil::isTraining = false;
				}
				if (ImGui::Button("Save Training")) {
					AutomaticRecoil::saveTraining();
				}

				if (ImGui::Button("Set Starting Coords (return to game, will record in 5 seconds)")) {
					// Sleep for 5 seconds 
					Sleep(5000);
					AutomaticRecoil::StartTraining();
				}
				// To the right of the buttons, display the current training status, and the current X and Y values
				ImGui::Text("Training Status:");
				ImGui::SameLine();
				if (AutomaticRecoil::isTraining) {
					ImGui::TextColored(ImVec4(0, 1, 0, 1), "Training");
				}
				else {
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "Not Training");
				}
				ImGui::Text("Current X:");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "%d", AutomaticRecoil::recoilX);
				ImGui::Text("Current Y:");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "%d", AutomaticRecoil::recoilY);
				ImGui::Text("Starting X:");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "%d", AutomaticRecoil::startingX);
				ImGui::Text("Starting Y:");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "%d", AutomaticRecoil::startingY);
			}
			ImGui::EndTabBar();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			CustomCheckbox("Active", &No_recoil::active);
			CustomCheckbox("Anti-Cheat bypass", &No_recoil::within_program);
			CustomCheckbox("Humanize", &No_recoil::humanize);
			if (ImGui::IsItemHovered()) {
				// Begin the tooltip block
				ImGui::BeginTooltip();

				// Add text to the tooltip
				ImGui::Text("Makes small jitters to make the recoil look more realistic/human");

				// End the tooltip block
				ImGui::EndTooltip();
			}
			ImGui::SetCursorPos(ImVec2(300, 510));
			CustomCheckbox("ADS Only", &No_recoil::adsOnly);
			ImGui::SetCursorPos(ImVec2(300, 540));
			CustomCheckbox("Use F12 to switch", &No_recoil::zToSwitch);

			ImGui::Spacing();
			ImGui::SetCursorPos(ImVec2(300, 575));	
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
			ImGui::SameLine();
			if (No_recoil::usingSecondary == true) {
				ImGui::Text("Using secondary:");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "Yes");
			}
			else {
				ImGui::Text("Using secondary:");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "No");
			}
		}
		if (Globals::ActiveTab == 2)
		{
			ImGui::SetCursorPos(ImVec2(150, 60));
			ImGui::Text("           === Macros (Working) ===");
			//ImGui::TextColored(ImVec4(0, 1, 0, 1), "Macros are game dependent");
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "F8");
			ImGui::SameLine();
			ImGui::Text("to enable the macro");
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "F9");
			ImGui::SameLine();
			ImGui::Text("to disable the macro");
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::BeginTabBar("Macros");
			if (ImGui::BeginTabItem("Default")) {
				CustomCheckbox("Bhop", &Macros::bhop);
				if (Macros::bhop == true) {
					if (ImGui::Combo("Bhop Key", &Bhop::HotKey, "Menu\0Left Click\0Right Click\0XBUTTON 1\0XBUTTON 2\0Capital\0Shift\0Control")) {
						Bhop::SetHotKey(Bhop::HotKey);
					}
				}
				ImGui::Spacing();
				CustomCheckbox("Turbo Crouch", &Macros::turbo_crouch);
				if (Macros::turbo_crouch == true) {
					if (ImGui::Combo("Crouch Key", &Crouch::HotKey, "Menu\0Left Click\0Right Click\0XBUTTON 1\0XBUTTON 2\0Capital\0Shift\0Control")) {
						Crouch::SetHotKey(Crouch::HotKey);
					}
				}
				ImGui::Spacing();
				CustomCheckbox("Tap Fire (happens on fire)", &Macros::rapid_fire);
				ImGui::Spacing();
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
				CustomCheckbox("Master Switch", &Macros::isActive);
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
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Custom")) {
				/*ImGui::SetCursorPos(ImVec2(210, 180));

				ImGui::Text("Custom macros are still in development");
				// Add a button that will add a keybind to the custom macros
				// Delete this later
				static int currentCount = 0;
				// Add the button here
				ImGui::SetCursorPos(ImVec2(150, 200));
				if (ImGui::Button("Add Keybind", ImVec2(400, 40))) {
					currentCount += 1;
					CustomMacros::keyCommands.push_back("Keybind " + std::to_string(currentCount));
					// Add the delay to the vector
					CustomMacros::timeBetweenCommands.push_back(CustomMacros::currentDelay);
				}
				// Add a slide for the delay
				ImGui::SetCursorPos(ImVec2(185, 250));

				ImGui::SliderInt("Delay ms", &CustomMacros::currentDelay, 1, 100);
				// Add a separator for a cleaner look
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);


				ImGui::Spacing();
				ImGuiStyle& oldStyle = ImGui::GetStyle();
				float oldWindowRounding = oldStyle.WindowRounding;
				float oldChildRounding = oldStyle.ChildRounding;
				if (Globals::styleChanged == true) {
					ImVec4 oldChildBg = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
				}
				else {

				}
				ImVec4 oldBorderColor = oldStyle.Colors[ImGuiCol_Border];
				float oldBorderSize = oldStyle.WindowBorderSize;

				// Adjust the style for the child window
				oldStyle.WindowRounding = 5.0f; // Adjust rounding of the corners
				oldStyle.ChildRounding = 5.0f;
				oldStyle.Colors[ImGuiCol_Border] = ImVec4(0.85f, 0.85f, 0.78f, 0.7f); // Bone white border color
				oldStyle.WindowBorderSize = 1.0f; // Set the border size
				ImGui::SetCursorPos(ImVec2(150, 290));
				ImGui::BeginChild("macroKeys", ImVec2(400, 320), true);
				// TODO: Switch this over to a traditional for loop lmao this sucks
				for (auto& entry : CustomMacros::keyCommands)
				{

						// Unique ID for each entry
						ImGui::PushID(entry.c_str());
						// Also display the delay for each entry
						if (ImGui::Button(entry.c_str())) {
						}
						ImGui::SameLine();
						// Ensure each "Delete" button has a unique ID within the loop
						if (ImGui::Button("Delete")) {
							// Remove the entry from the specific id index
							CustomMacros::keyCommands.erase(std::remove(CustomMacros::keyCommands.begin(), CustomMacros::keyCommands.end(), entry), CustomMacros::keyCommands.end());
							// Also delete the same index from the delay vector
							CustomMacros::timeBetweenCommands.erase(CustomMacros::timeBetweenCommands.begin() + ImGui::GetID(entry.c_str()));
						}
						ImGui::Text("-------------------------------------------------------");

						// Pop the ID after the buttons for this entry
						ImGui::PopID();
				}
				ImGui::EndChild();

				style.WindowRounding = oldWindowRounding;
				style.ChildRounding = oldChildRounding;
				style.Colors[ImGuiCol_Border] = oldBorderColor;
				style.WindowBorderSize = oldBorderSize;
				ImGui::EndTabItem();
				*/
	

				// Static variables for handling the keybind selection
				ImGui::SetCursorPos(ImVec2(210, 180));
				ImGui::Text("Custom macros are still in development");

				// Static variables for handling the keybind selection
				static bool showPopup = false;
				static std::string selectedKeybind = "";

				// Add the button here
				ImGui::SetCursorPos(ImVec2(150, 200));
				if (ImGui::Button("Add Keybind", ImVec2(400, 40))) {
					showPopup = true;
				}

				// Display the key selection popup
				if (showPopup) {
					ImGui::OpenPopup("Select Keybind");
				}

				if (ImGui::BeginPopupModal("Select Keybind", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
					ImGui::Text("Select a key to bind:");

					// List of key options
					const char* keys[] = { "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
										  "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z" };
					const char* mouseButtons[] = { "Mouse1", "Mouse2" };

					// Create a table with 3 columns
					if (ImGui::BeginTable("KeyTable", 10)) {
						for (int i = 0; i < IM_ARRAYSIZE(keys); i++) {
							ImGui::TableNextColumn();
							if (ImGui::Selectable(keys[i])) {
								selectedKeybind = keys[i];
								CustomMacros::keyCommands.push_back(selectedKeybind);
								CustomMacros::timeBetweenCommands.push_back(CustomMacros::currentDelay);
								showPopup = false;
								ImGui::CloseCurrentPopup();
							}
						}
						ImGui::EndTable();
					}

					ImGui::Separator();
					ImGui::Text("Mouse Buttons:");
					for (int i = 0; i < IM_ARRAYSIZE(mouseButtons); i++) {
						if (ImGui::Selectable(mouseButtons[i])) {
							selectedKeybind = mouseButtons[i];
							CustomMacros::keyCommands.push_back(selectedKeybind);
							CustomMacros::timeBetweenCommands.push_back(CustomMacros::currentDelay);
							showPopup = false;
							ImGui::CloseCurrentPopup();
						}
					}

					if (ImGui::Button("Cancel")) {
						showPopup = false;
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

				// Add a slider for the delay
				ImGui::SetCursorPos(ImVec2(185, 250));
				ImGui::SliderInt("Delay ms", &CustomMacros::currentDelay, 1, 100);

				// Add a separator for a cleaner look
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
				ImGui::Spacing();

				// Button to set the run keybind
				ImGui::SetCursorPos(ImVec2(150, 290));
				if (ImGui::Button("Set Run Keybind", ImVec2(400, 40))) {
					CustomMacros::settingRunKeybind = true;
				}

				if (CustomMacros::settingRunKeybind) {
					ImGui::SetCursorPos(ImVec2(230, 335));

					ImGui::Text("Press a key to set as Run Keybind...");
					for (int vkCode = 0x41; vkCode <= 0x5A; ++vkCode) { // A to Z
						if (GetAsyncKeyState(vkCode) & 0x8000) {
							CustomMacros::runKeybind = std::string(1, static_cast<char>(vkCode));
							CustomMacros::settingRunKeybind = false;
							break;
						}
					}
					if (GetAsyncKeyState(VK_LSHIFT) || GetAsyncKeyState(VK_RSHIFT)) {
						CustomMacros::runKeybind = "Shift";
						CustomMacros::settingRunKeybind = false;
					}
					if (GetAsyncKeyState(VK_LMENU) || GetAsyncKeyState(VK_RMENU)) {
						CustomMacros::runKeybind = "Alt";
						CustomMacros::settingRunKeybind = false;
					}
					if (GetAsyncKeyState(VK_RBUTTON)) {
						CustomMacros::runKeybind = "Mouse2";
						CustomMacros::settingRunKeybind = false;
					}
				}
				else if (!CustomMacros::runKeybind.empty()) {
					ImGui::SetCursorPos(ImVec2(300, 335));
					ImGui::Text("Run Keybind: %s", CustomMacros::runKeybind.c_str());
				}

				ImGuiStyle& style = ImGui::GetStyle();
				float oldWindowRounding = style.WindowRounding;
				float oldChildRounding = style.ChildRounding;
				ImVec4 oldBorderColor = style.Colors[ImGuiCol_Border];
				float oldBorderSize = style.WindowBorderSize;

				// Adjust the style for the child window
				style.WindowRounding = 5.0f; // Adjust rounding of the corners
				style.ChildRounding = 5.0f;
				style.Colors[ImGuiCol_Border] = ImVec4(0.85f, 0.85f, 0.78f, 0.7f); // Bone white border color
				style.WindowBorderSize = 1.0f; // Set the border size

				ImGui::SetCursorPos(ImVec2(150, 350));
				ImGui::BeginChild("macroKeys", ImVec2(400, 240), true);

				for (size_t i = 0; i < CustomMacros::keyCommands.size(); i++) {
					std::string& entry = CustomMacros::keyCommands[i];
					int delay = CustomMacros::timeBetweenCommands[i];

					// Unique ID for each entry
					ImGui::PushID(static_cast<int>(i));

					if (ImGui::Button(entry.c_str())) {
						// Button action (if any)
					}

					ImGui::SameLine();
					ImGui::Text("Delay: %d ms", delay);

					ImGui::SameLine();

					// Ensure each "Delete" button has a unique ID within the loop
					if (ImGui::Button("Delete")) {
						// Remove the entry and its delay
						CustomMacros::keyCommands.erase(CustomMacros::keyCommands.begin() + i);
						CustomMacros::timeBetweenCommands.erase(CustomMacros::timeBetweenCommands.begin() + i);
					}

					ImGui::Separator();

					// Pop the ID after the buttons for this entry
					ImGui::PopID();
				}

				ImGui::EndChild();

				style.WindowRounding = oldWindowRounding;
				style.ChildRounding = oldChildRounding;
				style.Colors[ImGuiCol_Border] = oldBorderColor;
				style.WindowBorderSize = oldBorderSize;

				ImGui::EndTabItem();
			}
	
			ImGui::EndTabBar();

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
			CustomCheckbox("Active", &Overlay::isActive);
			if (Overlay::isActive == true) {
				CustomCheckbox("Show anti-recoil status", &Overlay::recoilStatus);
				CustomCheckbox("Show version", &Overlay::currentVersion);
				CustomCheckbox("Show FPS", &Overlay::fps);
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
			CustomCheckbox("Aim Assist", &AimAssistConfig::assistActive);
		}
		if (Globals::ActiveTab == 6) {
			ImGui::SetCursorPos(ImVec2(gui::WIDTH / 2.8, 60));
			ImGui::Text("           === Misc ===");
			CustomCheckbox("FPS Counter", &Misc::fpsCounter);
			CustomCheckbox("Controller Support", &Controller::hasController);	
			CustomCheckbox("Anti-Afk kick", &Globals::antiAfk);
			ImGui::SetCursorPos(ImVec2(300, 77));
			CustomCheckbox("Low Usage", &Globals::efficentMode);
			if (ImGui::IsItemHovered()) {
				// Begin the tooltip block
				ImGui::BeginTooltip();

				// Add text to the tooltip
				ImGui::Text("Turns off other features except anti-recoil");

				// End the tooltip block
				ImGui::EndTooltip();
			}
			ImGui::SetCursorPos(ImVec2(300, 108));
			CustomCheckbox("Anti-script check", &Globals::scriptCheck);
			if (ImGui::IsItemHovered()) {
				// Begin the tooltip block
				ImGui::BeginTooltip();

				// Add text to the tooltip
				ImGui::Text("Hides program from task manager and taskbar");

				// End the tooltip block
				ImGui::EndTooltip();
			}
			ImGui::SetCursorPos(ImVec2(300, 140));
			CustomCheckbox("Purple Style", &Globals::styleChanged);
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
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
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			CustomCheckbox("Anti-Recoil Limit", &Globals::hasMaxTime);
			if (Globals::hasMaxTime == true) {
				ImGui::SliderInt("Max Time (seconds)", &Globals::maxTime, 0, 100);
				if (ImGui::IsItemHovered()) {
					// Begin the tooltip block
					ImGui::BeginTooltip();

					// Add text to the tooltip
					ImGui::Text("The mouse pulldown for the anti-recoil will only last for this long");

					// End the tooltip block
					ImGui::EndTooltip();
				}
			}
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
            ImGui::Text("Set the max values for the anti-recoil");
			ImGui::SliderInt("Max Vertical", &No_recoil::maxValueY, 0, 50);
			ImGui::SliderInt("Max/min Horizontal", &No_recoil::maxValueX, 0, 30);
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			CustomCheckbox("Custom accents", &Colors::customColors);
			if (Colors::customColors == true) {
				Globals::styleChanged = false;
				CustomCheckbox("Include text", &Colors::includeText);
				ImGui::ColorEdit4("Accent color", (float*)&Colors::accentColor, ImGuiColorEditFlags_NoInputs);
			}
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			CustomCheckbox("Custom menu", &PhysicalAttributes::custom_attributes);
			if (PhysicalAttributes::custom_attributes == true) {
				Globals::styleChanged = false;
				ImGui::SliderFloat("Frame rounding", &PhysicalAttributes::rounding_frame, 0.0f, 20.0f, "%.1f");
				ImGui::SliderFloat("Slider rounding", &PhysicalAttributes::rounding_slider, 0.0f, 20.0f, "%.1f");;
				ImGui::SliderFloat("Tab rounding", &PhysicalAttributes::rounding_tab, 0.0f, 20.0f, "%.1f");
			}
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			if (ImGui::Button("Reset")) {
				Misc::fpsCounter = false;
				Controller::hasController = false;
				Globals::antiAfk = false;
				Globals::efficentMode = false;
				Globals::scriptCheck = false;
				Globals::styleChanged = false;
				Globals::sensY = 12;
				Globals::sensX = 12;
			}
			ImGui::SameLine();
			CustomCheckbox("Save on close", &Globals::saveOnClose);
		}
		if (Globals::ActiveTab == 7) {
			ImGui::SetCursorPos(ImVec2(gui::WIDTH / 2.8, 60));
			ImGui::Text("        === Recoil Profiles ===");
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "                Add shared configs here");
				static char configNameBuffer[128] = "Shared Config 1";
				ImGui::SetCursorPos(ImVec2(200, 80));
				ImGui::InputText("", configNameBuffer, sizeof(configNameBuffer));
				// Add a plus button next to the text box
				ImGui::SameLine();
				if (ImGui::Button("+")) {
					// Add the config to the list
					Profile::AddProfile(configNameBuffer);
				}
				std::filesystem::path sharedFolderPath = std::filesystem::current_path() / "shared";
				std::filesystem::directory_iterator dirIter(sharedFolderPath);
				if (std::filesystem::exists(sharedFolderPath) && std::filesystem::is_directory(sharedFolderPath)) {
					ImGui::Spacing();
					ImGuiStyle& oldStyle = ImGui::GetStyle();
					float oldWindowRounding = oldStyle.WindowRounding;
					float oldChildRounding = oldStyle.ChildRounding;
					if (Globals::styleChanged == true) {
						ImVec4 oldChildBg = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
					}
					else {

					}
					ImVec4 oldBorderColor = oldStyle.Colors[ImGuiCol_Border];
					float oldBorderSize = oldStyle.WindowBorderSize;

					// Adjust the style for the child window
					oldStyle.WindowRounding = 5.0f; // Adjust rounding of the corners
					oldStyle.ChildRounding = 5.0f;
					oldStyle.Colors[ImGuiCol_Border] = ImVec4(0.85f, 0.85f, 0.78f, 0.7f); // Bone white border color
					oldStyle.WindowBorderSize = 1.0f; // Set the border size
					int fileCount = 0;
					ImGui::SetCursorPos(ImVec2(150, 150));
					ImGui::BeginChild("ConfigsS", ImVec2(400, 400), true);
					for (auto& entry : dirIter)
					{
						if (entry.is_regular_file() && MenuConfig::isTxtFile(entry.path().filename().string().c_str())) {
							fileCount++;
							std::string name = entry.path().filename().string();
							std::string path = entry.path().string();
							name = name.substr(0, name.size() - 4);

							// Unique ID for each entry
							ImGui::PushID(name.c_str());
							if (ImGui::Button(name.c_str())) {
								MenuConfig::setConfig(path);
							}
							ImGui::SameLine();
							if (ImGui::Button("Load")) {
								MenuConfig::setConfig(path);
							}
							ImGui::SameLine();

							// Ensure each "Delete" button has a unique ID within the loop
							if (ImGui::Button("Delete")) {
								MenuConfig::deleteConfig(path);
							}

							ImGui::SameLine();
							if (ImGui::Button("Open Folder")) {
								ShellExecute(NULL, "open", "explorer", ".", NULL, SW_SHOWDEFAULT);
							}
							ImGui::SameLine();
							ImGui::Text("Id:");
							ImGui::SameLine();
							ImGui::TextColored(ImVec4(0, 1, 0, 1), "%d", fileCount);
							ImGui::Text("-------------------------------------------------------");

							// Pop the ID after the buttons for this entry
							ImGui::PopID();
						}
					}
					ImGui::EndChild();

					style.WindowRounding = oldWindowRounding;
					style.ChildRounding = oldChildRounding;
					style.Colors[ImGuiCol_Border] = oldBorderColor;
					style.WindowBorderSize = oldBorderSize;
				}
				
				
		}
		if (Globals::ActiveTab == 8) {
			ImGui::SetCursorPos(ImVec2(gui::WIDTH / 2.8, 60));
			ImGui::Text("           === PixelBot ===");
			CustomCheckbox("Enable", &Triggerbot::isActive);
			if (Triggerbot::isActive == true) {
				CustomCheckbox("Smart Pixel Detection", &Triggerbot::useAi);
				ImGui::SliderInt("Accuracy", &Triggerbot::accuracy, 1, 25);
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
				if (Triggerbot::isActive == true) {
				ImGui::Text("Triggerbot:");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "Active");
				}
				else {
					ImGui::Text("Triggerbot:");
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "Not Active");
				}
			}
		}
		if (Globals::ActiveTab == 9) {
			ImGui::SetCursorPos(ImVec2(gui::WIDTH / 2.8, 60));
			ImGui::Text("           === Help ===");
			ImGui::Text("--- Macro Updates v2.1.1 (Beta) ---");
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
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			ImGui::Text("--- Macro Help v2.1.1 (Beta) ---");
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
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			ImGui::Text("--- Anti-Cheat status v2.1.1 (Beta) ---");
			ImGui::Text("VAC:");
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "[Undetected]");
			ImGui::Spacing();
			ImGui::Text("Battleye:");
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "[Undetected] (Macros & Anti-afk will not work)");
			ImGui::Spacing();
			ImGui::Text("EAC:");
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "[Unknown]");
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			ImGui::Text("--- Gun-Profiles v2.1.1 (Beta) ---");
			ImGui::Text("1. Download the 'config' into your downloads folder");
			ImGui::Text("2. Open the 'Gun profile' tab");
			ImGui::Text("3. Input the name of the file");
			ImGui::Text("4. Click '+'");
		}
		if (Globals::ActiveTab == 10) {
			ImGui::SetCursorPos(ImVec2(gui::WIDTH / 2.8, 60));
			ImGui::Text("             === Config ===");
			auto dirIter = std::filesystem::directory_iterator(std::filesystem::current_path());
			static char configNameBuffer[128] = "Config 1";
			ImGui::SetCursorPos(ImVec2(200, 80));
			ImGui::InputText("", configNameBuffer, sizeof(configNameBuffer));
			ImGui::SetCursorPos(ImVec2(305, 120));
			if (ImGui::Button("Create Config")) {
				MenuConfig::saveConfig((std::string)configNameBuffer);
			}
			ImGui::Spacing();
			ImGuiStyle& oldStyle = ImGui::GetStyle();
			float oldWindowRounding = oldStyle.WindowRounding;
			float oldChildRounding = oldStyle.ChildRounding;
			if (Globals::styleChanged == true) {
				ImVec4 oldChildBg = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
			}
			else {

			}
			ImVec4 oldBorderColor = oldStyle.Colors[ImGuiCol_Border];
			float oldBorderSize = oldStyle.WindowBorderSize;

			// Adjust the style for the child window
			oldStyle.WindowRounding = 5.0f; // Adjust rounding of the corners
			oldStyle.ChildRounding = 5.0f;
			oldStyle.Colors[ImGuiCol_Border] = ImVec4(0.85f, 0.85f, 0.78f, 0.7f); // Bone white border color
			oldStyle.WindowBorderSize = 1.0f; // Set the border size
			int fileCount = 0;
			ImGui::SetCursorPos(ImVec2(150, 150));
			ImGui::BeginChild("Configs", ImVec2(400, 400), true);
			for (auto& entry : dirIter)
			{
				if (entry.is_regular_file() && MenuConfig::isTxtFile(entry.path().filename().string().c_str())) {
					++fileCount;
					std::string name = entry.path().filename().string();
					std::string path = entry.path().string();

					// Clense name of .txt
					name = name.substr(0, name.size() - 4);

					// Unique ID for each entry
					ImGui::PushID(name.c_str());

					if (ImGui::Button(name.c_str())) {
						MenuConfig::setConfig(path);
					}
					ImGui::SameLine();
					if (ImGui::Button("Load")) {
						MenuConfig::setConfig(path);
					}
					ImGui::SameLine();

					// Ensure each "Delete" button has a unique ID within the loop
					if (ImGui::Button("Delete")) {
						MenuConfig::deleteConfig(path);
					}

					ImGui::SameLine();
					if (ImGui::Button("Open Folder")) {
						ShellExecute(NULL, "open", "explorer", ".", NULL, SW_SHOWDEFAULT);
					}
					ImGui::SameLine();
					ImGui::Text("Id:");
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(0, 1, 0, 1), "%d", fileCount);
					ImGui::Text("-------------------------------------------------------");

					// Pop the ID after the buttons for this entry
					ImGui::PopID();
				}
			}
			ImGui::EndChild();

			style.WindowRounding = oldWindowRounding;
			style.ChildRounding = oldChildRounding;
			style.Colors[ImGuiCol_Border] = oldBorderColor;
			style.WindowBorderSize = oldBorderSize;

			// Center the text 

			ImGui::SetCursorPos(ImVec2((gui::WIDTH / 2) + 80, 570));

			ImGui::Text("Config Files:");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "%d", fileCount);

			ImGui::SetCursorPos(ImVec2((gui::WIDTH / 2) - 150 , 564));
			//CustomCheckbox("Detect antivirus problems", &AntiVirus::isChecking);
			CustomCheckbox("Detect antivirus problems", &AntiVirus::isChecking);
			if (AntiVirus::isChecking == true) {
				if (AntiVirus::problem == true) {
					ImGui::SetCursorPos(ImVec2((gui::WIDTH / 2) - 90, 620));

					ImGui::Text("Antivirus Status:");
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(0, 1, 0, 1), "No issues detected");
				}
				else {
					ImGui::SetCursorPos(ImVec2((gui::WIDTH / 2) - 90, 620));

					ImGui::Text("Antivirus Status:");
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "Issues detected");
				}
			}
		}
	}
	ImGui::End();
}




