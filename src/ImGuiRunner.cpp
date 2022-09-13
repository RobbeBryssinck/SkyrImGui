#include "ImGuiRunner.h"

#include <PCH.h>

#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <d3d11.h>

ImGuiRunner& ImGuiRunner::Get() noexcept
{
	static ImGuiRunner s_instance;
	return s_instance;
}

ImGuiRunner::ImGuiRunner()
{
	ImGui::CreateContext();
}

void ImGuiRunner::Create(IDXGISwapChain* pChain) noexcept
{
	ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dContext = nullptr;

	DXGI_SWAP_CHAIN_DESC desc{};
    desc.OutputWindow = nullptr;
	pChain->GetDesc(&desc);

    // init platform
	if (!ImGui_ImplWin32_Init(desc.OutputWindow))
		logger::error("Failed to init ImGui Win32");

    pChain->GetDevice(__uuidof(d3dDevice), reinterpret_cast<void**>(&d3dDevice));
    d3dDevice->GetImmediateContext(&d3dContext);

    ImGui_ImplDX11_Init(d3dDevice, d3dContext);
}

void ImGuiRunner::Present() noexcept
{
    ImGui_ImplDX11_NewFrame();

    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

	// TODO: draw stuff
	ImGui::Begin("Test");
	ImGui::Text("Hello world");
	ImGui::End();

    ImGui::Render();

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiRunner::Lost() noexcept
{
}

// Using XInput for gamepad (will load DLL dynamically)
#ifndef IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
#include <xinput.h>
typedef DWORD (WINAPI *PFN_XInputGetCapabilities)(DWORD, DWORD, XINPUT_CAPABILITIES*);
typedef DWORD (WINAPI *PFN_XInputGetState)(DWORD, XINPUT_STATE*);
#endif

struct ImGui_ImplWin32_Data
{
    HWND                        hWnd;
    HWND                        MouseHwnd;
    bool                        MouseTracked;
    INT64                       Time;
    INT64                       TicksPerSecond;
    ImGuiMouseCursor            LastMouseCursor;
    bool                        HasGamepad;
    bool                        WantUpdateHasGamepad;

#ifndef IMGUI_IMPL_WIN32_DISABLE_GAMEPAD
    HMODULE                     XInputDLL;
    PFN_XInputGetCapabilities   XInputGetCapabilities;
    PFN_XInputGetState          XInputGetState;
#endif

    ImGui_ImplWin32_Data()      { memset(this, 0, sizeof(*this)); }
};


bool ImGui_ImplWin32_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return false;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        ::SetCursor(NULL);
    }
    else
    {
        // Show OS mouse cursor
        LPTSTR win32_cursor = IDC_ARROW;
        switch (imgui_cursor)
        {
        case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
        case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
        case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
        case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
        case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
        case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
        case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
        case ImGuiMouseCursor_Hand:         win32_cursor = IDC_HAND; break;
        case ImGuiMouseCursor_NotAllowed:   win32_cursor = IDC_NO; break;
        }
        ::SetCursor(::LoadCursor(NULL, win32_cursor));
    }
    return true;
}

// Allow compilation with old Windows SDK. MinGW doesn't have default _WIN32_WINNT/WINVER versions.
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif
#ifndef DBT_DEVNODES_CHANGED
#define DBT_DEVNODES_CHANGED 0x0007
#endif

LRESULT ImGuiRunner::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui::GetCurrentContext() == NULL)
        return 0;

    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Data* bd = ImGui::GetCurrentContext() ? (ImGui_ImplWin32_Data*)ImGui::GetIO().BackendPlatformUserData : NULL;

    switch (msg)
    {
    case WM_MOUSEMOVE:
        // We need to call TrackMouseEvent in order to receive WM_MOUSELEAVE events
        bd->MouseHwnd = hwnd;
        if (!bd->MouseTracked)
        {
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
            ::TrackMouseEvent(&tme);
            bd->MouseTracked = true;
        }
        break;
    case WM_MOUSELEAVE:
        if (bd->MouseHwnd == hwnd)
            bd->MouseHwnd = NULL;
        bd->MouseTracked = false;
        break;
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
    {
        int button = 0;
        if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) { button = 0; }
        if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) { button = 1; }
        if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) { button = 2; }
        if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
        if (!ImGui::IsAnyMouseDown() && ::GetCapture() == NULL)
            ::SetCapture(hwnd);
        io.MouseDown[button] = true;
        return 0;
    }
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP:
    {
        int button = 0;
        if (msg == WM_LBUTTONUP) { button = 0; }
        if (msg == WM_RBUTTONUP) { button = 1; }
        if (msg == WM_MBUTTONUP) { button = 2; }
        if (msg == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
        io.MouseDown[button] = false;
        if (!ImGui::IsAnyMouseDown() && ::GetCapture() == hwnd)
            ::ReleaseCapture();
        return 0;
    }
    case WM_MOUSEWHEEL:
        io.MouseWheel += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
        return 0;
    case WM_MOUSEHWHEEL:
        io.MouseWheelH += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
        return 0;
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    {
        bool down = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
        if (wParam < 256)
            io.KeysDown[wParam] = down;
        if (wParam == VK_CONTROL)
        {
            io.KeysDown[VK_LCONTROL] = ((::GetKeyState(VK_LCONTROL) & 0x8000) != 0);
            io.KeysDown[VK_RCONTROL] = ((::GetKeyState(VK_RCONTROL) & 0x8000) != 0);
            io.KeyCtrl = io.KeysDown[VK_LCONTROL] || io.KeysDown[VK_RCONTROL];
        }
        if (wParam == VK_SHIFT)
        {
            io.KeysDown[VK_LSHIFT] = ((::GetKeyState(VK_LSHIFT) & 0x8000) != 0);
            io.KeysDown[VK_RSHIFT] = ((::GetKeyState(VK_RSHIFT) & 0x8000) != 0);
            io.KeyShift            = io.KeysDown[VK_LSHIFT] || io.KeysDown[VK_RSHIFT];
        }
        if (wParam == VK_MENU)
        {
            io.KeysDown[VK_LMENU] = ((::GetKeyState(VK_LMENU) & 0x8000) != 0);
            io.KeysDown[VK_RMENU] = ((::GetKeyState(VK_RMENU) & 0x8000) != 0);
            io.KeyAlt             = io.KeysDown[VK_LMENU] || io.KeysDown[VK_RMENU];
        }
        return 0;
    }
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        io.AddFocusEvent(msg == WM_SETFOCUS);
        return 0;
    case WM_CHAR:
        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
        if (wParam > 0 && wParam < 0x10000)
            io.AddInputCharacterUTF16((unsigned short)wParam);
        return 0;
    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT && ImGui_ImplWin32_UpdateMouseCursor())
            return 1;
        return 0;
    case WM_DEVICECHANGE:
        if ((UINT)wParam == DBT_DEVNODES_CHANGED)
            bd->WantUpdateHasGamepad = true;
        return 0;
    }
    return 0;
}

void ImGuiRunner::RawInputHandler(RAWINPUT& aRawinput)
{
    if (ImGui::GetCurrentContext() == NULL)
        return;

    ImGuiIO& io = ImGui::GetIO();
    if (aRawinput.header.dwType == RIM_TYPEMOUSE)
    {
        const auto mouse = aRawinput.data.mouse;

        if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
        {
            io.MouseDown[0] = true;
        }

        if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
        {
            io.MouseDown[0] = false;
        }

        if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
        {
            io.MouseDown[1] = true;
        }

        if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
        {
            io.MouseDown[1] = false;
        }

        if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
        {
            io.MouseDown[2] = true;
        }

        if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
        {
            io.MouseDown[2] = false;
        }
    }
}

