#include <stdio.h>
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	const char *mutex_name = "Local\\CentawinAppUniqueMutexName";
	HANDLE mutex_handle = CreateMutexA(NULL, TRUE, mutex_name);

	if (mutex_handle == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
		MessageBoxA(NULL, "Centawin is already running.", "Centawin Error", MB_OK | MB_ICONWARNING);
		if (mutex_handle) CloseHandle(mutex_handle);
		return 1;
	}

	const char *default_key = "Q";
	const int is_ctrl_modifier = 0;
	const int is_shift_modifier = 1;
	const int is_alt_modifier = 0;
	const int is_win_modifier = 1;

	char exe_path[MAX_PATH];
	char ini_path[MAX_PATH];

	GetModuleFileNameA(NULL, exe_path, MAX_PATH);

	char *last_slash = strrchr(exe_path, '\\');

	if (last_slash != NULL) {
		*last_slash = '\0';
	}
	snprintf(ini_path, sizeof(ini_path), "%s\\config.ini", exe_path);

	FILE *file = NULL;
	errno_t error = fopen_s(&file, ini_path, "r");

	if (error != 0 || file == NULL) {
		error = fopen_s(&file, ini_path, "w");

		if (error == 0 && file != NULL) {
			fprintf(file, "[Hotkey]\n");
			fprintf(file, "# Use single capital letter (A-Z) for Key\n");
			fprintf(file, "Key = %s\n\n", default_key);

			fprintf(file, "# Modifiers: 1 = Enabled, 0 = Disabled\n");
			fprintf(file, "Ctrl = %d\n", is_ctrl_modifier);
			fprintf(file, "Shift = %d\n", is_shift_modifier);
			fprintf(file, "Alt = %d\n", is_alt_modifier);
			fprintf(file, "Win = %d\n", is_win_modifier);
		}
	}
	if (file != NULL) {
		fclose(file);
	}

	const int HOTKEY_ID = 1;
	char key_string[4] = {0};

	GetPrivateProfileStringA("Hotkey", "Key", default_key, key_string, sizeof(key_string), ini_path);

	UINT vk_key = (UINT)key_string[0];

	UINT ctrl_state = GetPrivateProfileIntA("Hotkey", "Ctrl", is_ctrl_modifier, ini_path);
	UINT shift_state = GetPrivateProfileIntA("Hotkey", "Shift", is_shift_modifier, ini_path);
	UINT alt_state = GetPrivateProfileIntA("Hotkey", "Alt", is_alt_modifier, ini_path);
	UINT win_state = GetPrivateProfileIntA("Hotkey", "Win", is_win_modifier, ini_path);

	UINT modifiers = 0;

	if (ctrl_state) modifiers |= MOD_CONTROL;
	if (shift_state) modifiers |= MOD_SHIFT;
	if (alt_state) modifiers |= MOD_ALT;
	if (win_state) modifiers |= MOD_WIN;

	if (!RegisterHotKey(NULL, HOTKEY_ID, modifiers, vk_key)) {
		MessageBoxA(NULL, "The selected hotkey is already in use. Set a new one in the config.ini file.", "Centawin Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	MSG msg = {0};

	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		if (msg.message == WM_HOTKEY && msg.wParam == HOTKEY_ID) {
			HWND hwnd = GetForegroundWindow();

			if (!hwnd || IsZoomed(hwnd) || IsIconic(hwnd)) continue;
			if ((GetWindowLong(hwnd, GWL_STYLE) & WS_POPUP) && !GetSystemMenu(hwnd, FALSE)) continue;

			HMONITOR h_monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFO monitor_info;

			monitor_info.cbSize = sizeof(MONITORINFO);
			if (GetMonitorInfo(h_monitor, &monitor_info)) {
				RECT window_rect;

				GetWindowRect(hwnd, &window_rect);

				int window_width = window_rect.right - window_rect.left;
				int window_height = window_rect.bottom - window_rect.top;

				int work_width = monitor_info.rcWork.right - monitor_info.rcWork.left;
				int work_height = monitor_info.rcWork.bottom - monitor_info.rcWork.top;

				int new_x = monitor_info.rcWork.left + (work_width - window_width) / 2;
				int new_y = monitor_info.rcWork.top + (work_height - window_height) / 2;

				SetWindowPos(hwnd, NULL, new_x, new_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
				while (GetAsyncKeyState(vk_key) & 0x8000) {
					Sleep(100);
				}

				MSG temp_msg;

				while (PeekMessage(&temp_msg, NULL, WM_HOTKEY, WM_HOTKEY, PM_REMOVE)) {};
			}
		}
	}

	UnregisterHotKey(NULL, HOTKEY_ID);
	ReleaseMutex(mutex_handle);
	CloseHandle(mutex_handle);

	return 0;
}