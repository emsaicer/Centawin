#include <windows.h>

#define HOTKEY_ID 1
#define VK_Q 0x51

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	if (!RegisterHotKey(NULL, HOTKEY_ID, MOD_WIN | MOD_SHIFT, VK_Q)) return 1;

	MSG msg = {0};

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (msg.message == WM_HOTKEY && msg.wParam == HOTKEY_ID) {
			HWND hwnd = GetForegroundWindow();

			if (!hwnd || IsZoomed(hwnd) || IsIconic(hwnd)) continue;
			if ((GetWindowLong(hwnd, GWL_STYLE) & WS_POPUP) && !GetSystemMenu(hwnd, FALSE)) continue;

			HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi;

			mi.cbSize = sizeof(MONITORINFO);
			if (GetMonitorInfo(hMonitor, &mi)) {
				RECT wr;

				GetWindowRect(hwnd, &wr);

				int winW = wr.right - wr.left;
				int winH = wr.bottom - wr.top;

				int workW = mi.rcWork.right - mi.rcWork.left;
				int workH = mi.rcWork.bottom - mi.rcWork.top;

				int newX = mi.rcWork.left + (workW - winW) / 2;
				int newY = mi.rcWork.top + (workH - winH) / 2;

				SetWindowPos(hwnd, NULL, newX, newY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
				while (GetAsyncKeyState(VK_Q) & 0x8000) {
					Sleep(100);
				}
				MSG temp;
				while (PeekMessage(&temp, NULL, WM_HOTKEY, WM_HOTKEY, PM_REMOVE));
			}
		}
	}

	return 0;
}