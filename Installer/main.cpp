#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <fstream>
#include <urlmon.h>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:WinMainCRTStartup")

#define IDI_ICON1 101

struct AppInfo {
	std::wstring name;
	std::wstring description;
	std::wstring licenseUrl;
	std::wstring downloadUrl;
	std::wstring installArgs;
	std::wstring registryKey;
	bool accepted;
	bool installed;
};

struct AppState {
	HWND hWnd;
	HWND hProgress;
	int currentPage;
	std::vector<AppInfo> apps;
	bool silentMode;
	std::wstring downloadsPath;
};

void ShowPage1(AppState* state);
void ShowPage2(AppState* state);
void ShowPage3(AppState* state);
void ShowPage4(AppState* state);
void ShowPage5(AppState* state);
std::wstring GetDownloadsPath();

bool IsAppInstalled(const std::wstring& registryKey) {
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, registryKey.c_str(), 0, KEY_READ | KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return true;
	}
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, registryKey.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return true;
	}
	if (RegOpenKeyEx(HKEY_CURRENT_USER, registryKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return true;
	}
	return false;
}

HRESULT DownloadFile(const std::wstring& url, const std::wstring& localPath) {
	return URLDownloadToFile(NULL, url.c_str(), localPath.c_str(), 0, NULL);
}

bool InstallApp(const std::wstring& installerPath, const std::wstring& args) {
	SHELLEXECUTEINFO sei = { sizeof(sei) };
	sei.lpVerb = L"runas";
	sei.lpFile = installerPath.c_str();
	sei.lpParameters = args.c_str();
	sei.nShow = SW_HIDE;
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;

	if (!ShellExecuteEx(&sei)) {
		DWORD err = GetLastError();
		if (err == ERROR_CANCELLED) {
			return false;
		}
	}

	WaitForSingleObject(sei.hProcess, INFINITE);

	DWORD exitCode;
	GetExitCodeProcess(sei.hProcess, &exitCode);
	CloseHandle(sei.hProcess);

	return (exitCode == ERROR_SUCCESS) ||
		(exitCode == ERROR_SUCCESS_REBOOT_REQUIRED) ||
		(exitCode == 0);
}

HWND CreateButton(HWND hParent, int x, int y, int width, int height, const wchar_t* text, int id) {
	return CreateWindow(
		L"BUTTON", text,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
		x, y, width, height,
		hParent, (HMENU)id, NULL, NULL);
}

HWND CreateStaticText(HWND hParent, int x, int y, int width, int height, const wchar_t* text) {
	return CreateWindow(
		L"STATIC", text,
		WS_VISIBLE | WS_CHILD,
		x, y, width, height,
		hParent, NULL, NULL, NULL);
}

HWND CreateHyperlink(HWND hParent, int x, int y, int width, int height, const wchar_t* text, int id) {
	HWND hLink = CreateWindow(
		L"STATIC", text,
		WS_VISIBLE | WS_CHILD | SS_NOTIFY,
		x, y, width, height,
		hParent, (HMENU)id, NULL, NULL);

	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	LOGFONT lf;
	GetObject(hFont, sizeof(lf), &lf);
	lf.lfUnderline = TRUE;
	HFONT hUnderlinedFont = CreateFontIndirect(&lf);
	SendMessage(hLink, WM_SETFONT, (WPARAM)hUnderlinedFont, TRUE);

	return hLink;
}

HWND CreateProgressBar(HWND hParent, int x, int y, int width, int height) {
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(icex);
	icex.dwICC = ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&icex);

	return CreateWindowEx(
		0, PROGRESS_CLASS, NULL,
		WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
		x, y, width, height,
		hParent, NULL, NULL, NULL);
}

void ClearWindow(HWND hWnd) {
	HWND hChild = GetWindow(hWnd, GW_CHILD);
	while (hChild) {
		DestroyWindow(hChild);
		hChild = GetWindow(hWnd, GW_CHILD);
	}
}

void ShowPage1(AppState* state) {
	state->currentPage = 1;
	ClearWindow(state->hWnd);

	CreateStaticText(state->hWnd, 20, 20, 550, 100,
		L"Добро пожаловать в установщик приложений.\n\n"
		L"Этот мастер установит выбранные вами приложения на ваш компьютер.\n"
		L"Нажмите 'Далее' для продолжения.");

	CreateButton(state->hWnd, 470, 140, 100, 30, L"Далее", 101);
}

void ShowPage2(AppState* state) {
	state->currentPage = 2;
	ClearWindow(state->hWnd);

	if (state->apps[0].installed) {
		ShowPage3(state);
		return;
	}

	CreateStaticText(state->hWnd, 20, 20, 550, 100, state->apps[0].description.c_str());
	CreateButton(state->hWnd, 200, 140, 200, 30, L"Лицензионное соглашение", 201);
	CreateButton(state->hWnd, 20, 140, 100, 30, L"Принять", 102);
	CreateButton(state->hWnd, 470, 140, 100, 30, L"Отклонить", 103);
}

void ShowPage3(AppState* state) {
	state->currentPage = 3;
	ClearWindow(state->hWnd);

	if (state->apps[1].installed) {
		ShowPage4(state);
		return;
	}

	CreateStaticText(state->hWnd, 20, 20, 550, 100, state->apps[1].description.c_str());
	CreateButton(state->hWnd, 200, 140, 200, 30, L"Лицензионное соглашение", 202);
	CreateButton(state->hWnd, 20, 140, 100, 30, L"Принять", 104);
	CreateButton(state->hWnd, 470, 140, 100, 30, L"Отклонить", 105);
}

void ShowPage4(AppState* state) {
	state->currentPage = 4;
	ClearWindow(state->hWnd);

	CreateStaticText(state->hWnd, 20, 20, 550, 50, L"Идет установка выбранных приложений...");
	state->hProgress = CreateProgressBar(state->hWnd, 20, 140, 550, 30);
	SendMessage(state->hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	SendMessage(state->hProgress, PBM_SETSTEP, 10, 0);

	CreateThread(NULL, 0, [](LPVOID lpParam) -> DWORD {
		AppState* state = (AppState*)lpParam;
		int totalSteps = 0;
		int completedSteps = 0;

		for (const auto& app : state->apps) {
			if (app.accepted && !app.installed) {
				totalSteps += 2;
			}
		}

		if (totalSteps == 0) {
			PostMessage(state->hWnd, WM_USER + 1, 0, 0);
			return 0;
		}

		int stepSize = totalSteps > 0 ? 100 / totalSteps : 1;

		for (auto& app : state->apps) {
			if (app.accepted && !app.installed) {
				std::wstring tempPath = state->downloadsPath + L"\\" + app.name + L"_installer.exe";
				SendMessage(state->hProgress, PBM_SETPOS, completedSteps * stepSize, 0);

				if (DownloadFile(app.downloadUrl, tempPath) == S_OK) {
					completedSteps++;
					SendMessage(state->hProgress, PBM_SETPOS, completedSteps * stepSize, 0);

					if (InstallApp(tempPath, app.installArgs)) {
						completedSteps++;
						SendMessage(state->hProgress, PBM_SETPOS, completedSteps * stepSize, 0);
						app.installed = IsAppInstalled(app.registryKey);
						if (app.installed) {
							completedSteps++;
							SendMessage(state->hProgress, PBM_SETPOS, completedSteps * stepSize, 0);
						}
					}
				}

				DeleteFile(tempPath.c_str());
			}
		}

		PostMessage(state->hWnd, WM_USER + 1, 0, 0);
		return 0;
		}, state, 0, NULL);
}

void ShowPage5(AppState* state) {
	state->currentPage = 5;
	ClearWindow(state->hWnd);

	std::wstring resultText = L"Результаты установки:\n\n";

	for (auto& app : state->apps) {
		Sleep(800);
		bool isInstalledNow = IsAppInstalled(app.registryKey);

		resultText += app.name + L": ";
		if (app.accepted) {
			if (isInstalledNow) {
				resultText += L"Приложение успешно установлено.\n";
				app.installed = true;
			}
			else {
				resultText += L"Не удалось установить.\n";
			}
		}
		else if (app.installed) {
			resultText += L"Приложение существует. Установка не требуется.\n";
		}
		else {
			resultText += L"Установка отменена.\n";
		}
	}

	CreateStaticText(state->hWnd, 20, 20, 550, 100, resultText.c_str());
	CreateButton(state->hWnd, 20, 140, 550, 30, L"Выход", 106);
}

std::wstring GetDownloadsPath() {
	PWSTR path;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, NULL, &path))) {
		std::wstring downloadsPath(path);
		CoTaskMemFree(path);
		return downloadsPath;
	}
	return L"C:\\Users\\Public\\Downloads";
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	AppState* state = (AppState*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	switch (message) {
	case WM_CREATE: {
		CREATESTRUCT* create = (CREATESTRUCT*)lParam;
		state = (AppState*)create->lpCreateParams;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)state);
		state->hWnd = hWnd;
		break;
	}
	case WM_COMMAND: {
		int wmId = LOWORD(wParam);

		if (wmId >= 201 && wmId <= 202) {
			int appIndex = wmId - 201;
			ShellExecute(NULL, L"open", state->apps[appIndex].licenseUrl.c_str(), NULL, NULL, SW_SHOW);
		}
		else {
			switch (wmId) {
			case 101: ShowPage2(state); break;
			case 102: state->apps[0].accepted = true; ShowPage3(state); break;
			case 103: state->apps[0].accepted = false; ShowPage3(state); break;
			case 104: state->apps[1].accepted = true; ShowPage4(state); break;
			case 105: state->apps[1].accepted = false; ShowPage4(state); break;
			case 106: PostQuitMessage(0); break;
			}
		}
		break;
	}
	case WM_USER + 1:
		ShowPage5(state);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	AppState state = { 0 };
	state.silentMode = (strstr(lpCmdLine, "/S") || strstr(lpCmdLine, "/verysilent"));
	state.downloadsPath = GetDownloadsPath();

	HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

	state.apps = {
		{
			L"7-Zip",
			L"7-Zip - это архиватор файлов с высокой степенью сжатия.\n"
			L"Поддерживает множество форматов: 7z, ZIP, RAR, GZIP, TAR, ZIP, WIM.\n"
			L"Распаковка только для следующих форматов: APFS, AR, ARJ,\n"
			L"CAB, CHM, CPIO, CramFS, DMG, EXT, FAT, GPT, HFS, IHEX,\n"
			L"ISO, LZH, LZMA, MBR, MSI, NSIS, NTFS, QCOW2, RAR, RPM,\n"
			L"SquashFS, UDF, UEFI, VDI, VHD, VHDX, VMDK, XAR и Z.",
			L"https://www.7-zip.org/license.txt",
			L"https://www.7-zip.org/a/7z2501.exe",
			L"/S /D=" + state.downloadsPath + L"\\7-Zip",
			L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\7-Zip",
			false,
			IsAppInstalled(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\7-Zip")
		},
		{
			L"XnView MP",
			L"XnView MP - это мощный просмотрщик и конвертер изображений.\n"
			L"Поддерживает более 500 форматов изображений и имеет множество функций редактирования.",
			L"https://www.xnview.com/wiki/index.php?title=XnView_User_Guide",
			L"https://download.xnview.com/XnViewMP-win.exe",
			L"/verysilent /DIR=" + state.downloadsPath + L"\\XnView",
			L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\XnViewMP_is1",
			false,
			IsAppInstalled(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\XnViewMP_is1")
		}
	};

	if (state.silentMode) {
		for (auto& app : state.apps) {
			if (!app.installed) {
				std::wstring tempPath = state.downloadsPath + L"\\" + app.name + L"_installer.exe";
				if (DownloadFile(app.downloadUrl, tempPath) == S_OK) {
					InstallApp(tempPath, app.installArgs);
					DeleteFile(tempPath.c_str());
				}
			}
		}
		return 0;
	}

	WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
	wc.style = CS_CLASSDC;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hIcon = hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = L"InstallerClass";
	wc.hIconSm = hIcon;

	RegisterClassEx(&wc);

	HWND hWnd = CreateWindow(wc.lpszClassName, L"Установщик приложений",
		WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 600, 220,
		NULL, NULL, hInstance, &state);

	if (!hWnd) {
		return 0;
	}

	SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	ShowPage1(&state);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}