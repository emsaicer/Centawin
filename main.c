#include <shlobj.h>
#include <stdio.h>
#include <windows.h>

#define HOTKEY_ID 1

#define ID_TRAY_ICON 2
#define WM_TRAY_MESSAGE (WM_USER + 1)

#define ID_MENU_AUTOSTART 3
#define ID_MENU_AUTOSTART_ADMIN 4
#define ID_MENU_OPEN_CONFIG 5
#define ID_MENU_EXIT 6

#define ID_CONFIG_CHANGED (WM_USER + 2)

const char *config_file_name = "config.ini";
char ini_path[MAX_PATH];

const char *default_vk_key = "Q";
const int default_ctrl_modifier = 0;
const int default_shift_modifier = 1;
const int default_alt_modifier = 0;
const int default_win_modifier = 1;

NOTIFYICONDATA notify_icon_data;
HWND hwnd = NULL;
UINT vk_key;
UINT modifiers = 0;

void center_active_window() {
	HWND hwnd = GetForegroundWindow();

	if (!hwnd || IsZoomed(hwnd) || IsIconic(hwnd)) return;
	if ((GetWindowLong(hwnd, GWL_STYLE) & WS_POPUP) && !GetSystemMenu(hwnd, FALSE)) return;

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

		while (GetAsyncKeyState(vk_key) & 0x8000) Sleep(100);

		MSG temp_msg;
		while (PeekMessage(&temp_msg, NULL, WM_HOTKEY, WM_HOTKEY, PM_REMOVE)) {};
	}
}

BOOL get_startup_folder_path(char *out_path) {
	return SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_STARTUP, NULL, 0, out_path));
}

char *get_startup_shortcut_path(char *out_path, DWORD size) {
	char startup_path[MAX_PATH];

	if (!get_startup_folder_path(startup_path)) return NULL;
	snprintf(out_path, size, "%s\\Centawin.lnk", startup_path);

	return out_path;
}

BOOL is_run_as_admin() {
	BOOL is_admin = FALSE;
	PSID administrators_sid = NULL;
	SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;

	if (AllocateAndInitializeSid(&nt_authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &administrators_sid)) {
		if (!CheckTokenMembership(NULL, administrators_sid, &is_admin)) is_admin = FALSE;
		FreeSid(administrators_sid);
	}

	return is_admin;
}

void enable_autostart() {
	HRESULT h_result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (FAILED(h_result) && h_result != RPC_E_CHANGED_MODE) return;
	wchar_t exe_path[MAX_PATH];
	wchar_t startup_dir[MAX_PATH];
	wchar_t shortcut_path[MAX_PATH];
	IShellLinkW *p_shell_link = NULL;

	GetModuleFileNameW(NULL, exe_path, MAX_PATH);
	SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL, 0, startup_dir);
	swprintf_s(shortcut_path, MAX_PATH, L"%s\\Centawin.lnk", startup_dir);
	h_result = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (LPVOID *)&p_shell_link);

	if (SUCCEEDED(h_result)) {
		IPersistFile *p_persist_file = NULL;

		p_shell_link->lpVtbl->SetPath(p_shell_link, exe_path);
		h_result = p_shell_link->lpVtbl->QueryInterface(p_shell_link, &IID_IPersistFile, (LPVOID *)&p_persist_file);
		if (SUCCEEDED(h_result)) {
			p_persist_file->lpVtbl->Save(p_persist_file, shortcut_path, TRUE);
			p_persist_file->lpVtbl->Release(p_persist_file);
		}
		p_shell_link->lpVtbl->Release(p_shell_link);
	}
	CoUninitialize();
}

void disable_autostart() {
	char buf[MAX_PATH];

	DeleteFileA(get_startup_shortcut_path(buf, sizeof(buf)));
}

BOOL is_autostart_enabled() {
	char buf[MAX_PATH];
	DWORD attributes = GetFileAttributesA(get_startup_shortcut_path(buf, sizeof(buf)));

	return (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}

void enable_autostart_admin() {
	char exe_path[MAX_PATH];
	char command[MAX_PATH * 4];
	STARTUPINFOA startup_info = {sizeof(STARTUPINFOA)};
	PROCESS_INFORMATION process_information;

	GetModuleFileNameA(NULL, exe_path, MAX_PATH);
	snprintf(command, sizeof(command),
		"cmd.exe /c \"schtasks /Create /TN \"Centawin Autostart\" /TR \"\\\"%s\\\"\" /SC ONLOGON /RL HIGHEST /F && "
		"powershell -Command \"\"$task = Get-ScheduledTask -TaskName 'Centawin Autostart'; "
		"$task.Settings.ExecutionTimeLimit = 'PT0S'; "
		"$task.Settings.DisallowStartIfOnBatteries = $false; "
		"Set-ScheduledTask $task\"\"\"",
		exe_path);

	startup_info.dwFlags = STARTF_USESHOWWINDOW;
	startup_info.wShowWindow = SW_HIDE;

	if (!CreateProcessA(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &startup_info, &process_information)) return;

	WaitForSingleObject(process_information.hProcess, INFINITE);
	CloseHandle(process_information.hProcess);
	CloseHandle(process_information.hThread);
}

void disable_autostart_admin() {
	const char *command = "schtasks /Delete /TN \"Centawin Autostart\" /F";
	STARTUPINFOA startup_info = {sizeof(STARTUPINFOA)};
	PROCESS_INFORMATION process_information;

	startup_info.dwFlags = STARTF_USESHOWWINDOW;
	startup_info.wShowWindow = SW_HIDE;

	if (!CreateProcessA(NULL, (char *)command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &startup_info, &process_information)) return;

	WaitForSingleObject(process_information.hProcess, INFINITE);
	CloseHandle(process_information.hProcess);
	CloseHandle(process_information.hThread);
}

void open_config() {
	if ((INT_PTR)ShellExecuteA(NULL, "open", ini_path, NULL, NULL, SW_SHOWNORMAL) <= 32) {
		MessageBoxA(NULL, "Config config_file can't be opened.", "Centawin Error", MB_OK | MB_ICONERROR);
	}
}

ULONGLONG get_file_write_time(const char *full_path) {
	WIN32_FILE_ATTRIBUTE_DATA attribute_data;

	if (!GetFileAttributesExA(full_path, GetFileExInfoStandard, &attribute_data)) return 0;

	return (ULONGLONG)attribute_data.ftLastWriteTime.dwHighDateTime << 32 | attribute_data.ftLastWriteTime.dwLowDateTime;
}

DWORD WINAPI watch_config_changes(LPVOID lpParam) {
	char dir_path[MAX_PATH];
	GetModuleFileNameA(NULL, dir_path, MAX_PATH);
	char *last_slash = strrchr(dir_path, '\\');

	if (last_slash) *last_slash = '\0';
	printf("Dir Path: %s", dir_path);

	char full_config_path[MAX_PATH];

	snprintf(full_config_path, sizeof(full_config_path), "%s\\%s", dir_path, config_file_name);

	ULONGLONG local_last_time = get_file_write_time(full_config_path);
	HANDLE h_notify = FindFirstChangeNotificationA(dir_path, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);

	if (h_notify == INVALID_HANDLE_VALUE) return 0;

	while (TRUE) {
		DWORD wait_status = WaitForSingleObject(h_notify, INFINITE);

		if (wait_status != WAIT_OBJECT_0) continue;

		ULONGLONG current = get_file_write_time(full_config_path);

		if (current > local_last_time) {
			local_last_time = current;

			HWND hwndMain = (HWND)lpParam;
			PostMessage(hwndMain, ID_CONFIG_CHANGED, 0, 0);
		}

		FindNextChangeNotification(h_notify);
	}

	FindCloseChangeNotification(h_notify);

	return 0;
}

BOOL is_autostart_admin_enabled(void) {
	const char *command = "schtasks /Query /TN \"Centawin Autostart\"";
	STARTUPINFOA startup_info = {sizeof(STARTUPINFOA)};
	PROCESS_INFORMATION process_information;
	BOOL is_enabled = FALSE;

	startup_info.dwFlags = STARTF_USESHOWWINDOW;
	startup_info.wShowWindow = SW_HIDE;

	if (!CreateProcessA(NULL, (char *)command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &startup_info, &process_information)) return FALSE;

	DWORD exit_code;

	WaitForSingleObject(process_information.hProcess, INFINITE);
	GetExitCodeProcess(process_information.hProcess, &exit_code);
	if (exit_code == 0) is_enabled = TRUE;
	CloseHandle(process_information.hProcess);
	CloseHandle(process_information.hThread);

	return is_enabled;
}

void get_keybinding(const char *ini_path, UINT *out_vk, UINT *out_modifiers) {
	char key_string[4] = {0};

	GetPrivateProfileStringA("Hotkey", "Key", default_vk_key, key_string, sizeof(key_string), ini_path);
	*out_vk = (UINT)key_string[0];

	UINT ctrl_modifier = GetPrivateProfileIntA("Hotkey", "Ctrl", default_ctrl_modifier, ini_path);
	UINT shift_modifier = GetPrivateProfileIntA("Hotkey", "Shift", default_shift_modifier, ini_path);
	UINT alt_modifier = GetPrivateProfileIntA("Hotkey", "Alt", default_alt_modifier, ini_path);
	UINT win_modifier = GetPrivateProfileIntA("Hotkey", "Win", default_win_modifier, ini_path);

	UINT modifiers = 0;
	if (ctrl_modifier) modifiers |= MOD_CONTROL;
	if (shift_modifier) modifiers |= MOD_SHIFT;
	if (alt_modifier) modifiers |= MOD_ALT;
	if (win_modifier) modifiers |= MOD_WIN;

	*out_modifiers = modifiers;
}

void show_tray_menu(HWND hwnd) {
	HMENU h_menu = CreatePopupMenu();
	UINT autostart_admin_flags = MF_STRING;

	autostart_admin_flags |= is_autostart_admin_enabled() ? MF_CHECKED : MF_UNCHECKED;
	if (!is_run_as_admin()) autostart_admin_flags |= MF_GRAYED;

	AppendMenuA(h_menu, MF_STRING, ID_MENU_OPEN_CONFIG, "Change Keybinding");
	AppendMenuA(h_menu, MF_SEPARATOR, 0, NULL);
	AppendMenuA(h_menu, MF_STRING | is_autostart_enabled() ? MF_CHECKED : MF_UNCHECKED, ID_MENU_AUTOSTART, "Run at Startup");
	AppendMenuA(h_menu, autostart_admin_flags, ID_MENU_AUTOSTART_ADMIN, "Run at Startup (Admin)");
	AppendMenuA(h_menu, MF_SEPARATOR, 0, NULL);
	AppendMenuA(h_menu, MF_STRING, ID_MENU_EXIT, "Exit");

	POINT point;

	GetCursorPos(&point);
	SetForegroundWindow(hwnd);
	TrackPopupMenu(h_menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, point.x, point.y, 0, hwnd, NULL);
	DestroyMenu(h_menu);
}

void handle_menu_command(HWND hwnd, WORD menu_id) {
	switch (menu_id) {

	case ID_MENU_AUTOSTART:
		if (is_autostart_enabled()) disable_autostart();
		else {
			enable_autostart();
			disable_autostart_admin();
		}
		break;

	case ID_MENU_AUTOSTART_ADMIN:
		if (is_autostart_admin_enabled()) disable_autostart_admin();
		else {
			enable_autostart_admin();
			disable_autostart();
		}
		break;

	case ID_MENU_OPEN_CONFIG:
		open_config();
		break;

	case ID_MENU_EXIT:
		DestroyWindow(hwnd);
		break;
	}
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {

	case WM_HOTKEY:
		if (wParam == HOTKEY_ID) center_active_window();
		break;

	case WM_TRAY_MESSAGE:
		if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) show_tray_menu(hwnd);
		break;

	case WM_COMMAND:
		handle_menu_command(hwnd, LOWORD(wParam));
		break;

	case WM_DESTROY:
		Shell_NotifyIcon(NIM_DELETE, &notify_icon_data);
		PostQuitMessage(0);
		break;

	case ID_CONFIG_CHANGED:
		UnregisterHotKey(hwnd, HOTKEY_ID);
		get_keybinding(ini_path, &vk_key, &modifiers);
		if (!RegisterHotKey(hwnd, HOTKEY_ID, modifiers, vk_key)) {
			MessageBoxA(NULL, "The selected hotkey is already in use. Set a new one in the config.ini file.", "Centawin Error", MB_OK | MB_ICONERROR);
		}
		break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	(void)hPrevInstance;
	(void)lpCmdLine;
	(void)nCmdShow;

	// Set high DPI
	HMODULE h_user_32 = GetModuleHandle("user32.dll");

	if (h_user_32) {
		typedef BOOL(WINAPI * SetProcessDpiAwarenessContextProc)(HANDLE);
		SetProcessDpiAwarenessContextProc set_dpi_context = (SetProcessDpiAwarenessContextProc)GetProcAddress(h_user_32, "SetProcessDpiAwarenessContext");
		if (set_dpi_context) set_dpi_context(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	}

	// Don't run the program if it is already running
	HANDLE mutex_handle = CreateMutexA(NULL, TRUE, "Local\\CentawinUniqueMutexName");

	if (mutex_handle == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
		if (mutex_handle) CloseHandle(mutex_handle);
		return 0;
	}

	// Rewrite path in autostart if it is changed
	if (is_autostart_enabled()) enable_autostart();
	if (is_autostart_admin_enabled()) enable_autostart_admin();

	// Create a hidden window
	const char CLASS_NAME[] = "CentawinWindowClass";
	WNDCLASS window_class = {0};

	window_class.lpfnWndProc = WindowProc;
	window_class.hInstance = hInstance;
	window_class.lpszClassName = CLASS_NAME;
	RegisterClass(&window_class);

	hwnd = CreateWindowEx(0, CLASS_NAME, "Centawin", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
	if (hwnd == NULL) {
		ReleaseMutex(mutex_handle);
		CloseHandle(mutex_handle);
		return 0;
	}

	// Get and register a hotkey
	char exe_path[MAX_PATH];

	GetModuleFileNameA(NULL, exe_path, MAX_PATH);

	char *last_slash = strrchr(exe_path, '\\');
	if (last_slash != NULL) *last_slash = '\0';

	snprintf(ini_path, sizeof(ini_path), "%s\\%s", exe_path, config_file_name);

	FILE *config_file = NULL;
	errno_t error = fopen_s(&config_file, ini_path, "r");

	if (error != 0 || config_file == NULL) {
		error = fopen_s(&config_file, ini_path, "w");

		if (error == 0 && config_file != NULL) {
			fprintf(config_file, "[Hotkey]\n");
			fprintf(config_file, "# Use single capital letter (A-Z) for Key\n");
			fprintf(config_file, "Key = %s\n\n", default_vk_key);

			fprintf(config_file, "# Modifiers: 1 = Enabled, 0 = Disabled\n");
			fprintf(config_file, "Ctrl = %d\n", default_ctrl_modifier);
			fprintf(config_file, "Shift = %d\n", default_shift_modifier);
			fprintf(config_file, "Alt = %d\n", default_alt_modifier);
			fprintf(config_file, "Win = %d\n", default_win_modifier);
		}
	}
	if (config_file != NULL) fclose(config_file);

	char key_string[4] = {0};

	GetPrivateProfileStringA("Hotkey", "Key", default_vk_key, key_string, sizeof(key_string), ini_path);

	get_keybinding(ini_path, &vk_key, &modifiers);

	if (!RegisterHotKey(hwnd, HOTKEY_ID, modifiers, vk_key)) {
		int response = MessageBoxA(
			NULL,
			"The selected hotkey is already in use.\n\n"
			"Do you want to open config.ini to change it? (Yes - Open, No - Exit Program)",
			"Centawin Error",
			MB_YESNO | MB_ICONERROR);

		if (response == IDYES) open_config();
		else {
			DestroyWindow(hwnd);
			ReleaseMutex(mutex_handle);
			CloseHandle(mutex_handle);
			return 1;
		}
	}

	// Create a thread to watch config changes
	HANDLE hThread = CreateThread(NULL, 0, watch_config_changes, (LPVOID)hwnd, 0, NULL);
	if (hThread != NULL) CloseHandle(hThread);

	// Create a tray icon
	notify_icon_data.cbSize = sizeof(NOTIFYICONDATA);
	notify_icon_data.hWnd = hwnd;
	notify_icon_data.uID = ID_TRAY_ICON;
	notify_icon_data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	notify_icon_data.uCallbackMessage = WM_TRAY_MESSAGE;
	notify_icon_data.hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	if (notify_icon_data.hIcon == NULL) notify_icon_data.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	lstrcpyA(notify_icon_data.szTip, "Centawin");

	if (!Shell_NotifyIcon(NIM_ADD, &notify_icon_data)) {
		DestroyIcon(notify_icon_data.hIcon);
		UnregisterHotKey(hwnd, HOTKEY_ID);
		DestroyWindow(hwnd);
		ReleaseMutex(mutex_handle);
		CloseHandle(mutex_handle);
		return 1;
	}

	MSG message;

	while (GetMessage(&message, NULL, 0, 0)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	// Clear
	Shell_NotifyIconA(NIM_DELETE, &notify_icon_data);
	DestroyIcon(notify_icon_data.hIcon);
	UnregisterHotKey(hwnd, HOTKEY_ID);
	DestroyWindow(hwnd);
	ReleaseMutex(mutex_handle);
	CloseHandle(mutex_handle);

	return 0;
}