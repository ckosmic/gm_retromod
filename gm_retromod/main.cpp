#define GMMODULE
#define _CRT_SECURE_NO_WARNINGS

#include <string>
#include <fstream>
#include <sstream>
#include <Interface.h>
#include <Types.h>
#include <stdio.h>
#include <Windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>
#include <gdiplus.h>
#include <chrono>
#include <thread>
#include <Urlmon.h>
#include <atlbase.h>
#pragma comment(lib,"gdiplus.lib")
#pragma comment(lib, "urlmon.lib")

#include "base64.h"
#include "dirent.h"

using namespace std;
using namespace GarrysMod::Lua;
using namespace Gdiplus;

HWND g_retroarch;
GdiplusStartupInput g_gdiplusStartupInput;
ULONG_PTR g_gdiplusToken;
vector<BYTE> g_frameData;
ULONG g_streamQuality = 50L;
bool g_usePng = false;
bool g_downloading = false;

bool IsRetroArchOpen() {
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			if (_stricmp(entry.szExeFile, "retroarch.exe") == 0)
			{
				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);

				return true;

				CloseHandle(hProcess);
			}
		}
	}

	CloseHandle(snapshot);

	return false;
}

DWORD GetPID(const char* processName) {
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			if (_stricmp(entry.szExeFile, processName) == 0)
			{
				return entry.th32ProcessID;
			}
		}
	}

	CloseHandle(snapshot);
}

BOOL IsMainWindow(HWND hwnd) {
	return GetWindow(hwnd, GW_OWNER) == (HWND)0 && IsWindowVisible(hwnd);
}

BOOL CALLBACK EnumWindowsProcCallback(HWND hwnd, LPARAM lparam) {
	DWORD lpdwProcessId;
	GetWindowThreadProcessId(hwnd, &lpdwProcessId);

	if (lpdwProcessId != lparam || !IsMainWindow(hwnd)) {
		return TRUE;
	}
	g_retroarch = hwnd;
	return FALSE;
}

LUA_FUNCTION(SetRetroArchHWND) {
	if (g_retroarch == nullptr || !IsWindow(g_retroarch)) {
		const char* processName = "retroarch.exe";
		EnumWindows(EnumWindowsProcCallback, GetPID(processName));
	}

	return 0;
}

LUA_FUNCTION(TestFunction)
{
	if (LUA->IsType(1, Type::NUMBER))
	{
		char strOut[512];
		float fNumber = LUA->GetNumber(1);
		sprintf(strOut, "Thanks for the number - I love %f!!", fNumber);
		LUA->PushString(strOut);
		return 1;
	}

	LUA->PushString("This string is returned");
	return 1;
}

LUA_FUNCTION(SetQuality)
{
	if (LUA->IsType(1, Type::STRING))
	{
		const char* qualityString = LUA->GetString(1);
		g_usePng = false;
		if (!strcmp(qualityString, "Awful Quality")) {
			g_streamQuality = 20L;
		} else if (!strcmp(qualityString, "Low Quality")) {
			g_streamQuality = 50L;
		} else if (!strcmp(qualityString, "Medium Quality")) {
			g_streamQuality = 80L;
		} else if (!strcmp(qualityString, "High Quality")) {
			g_streamQuality = 100L;
		} else if (!strcmp(qualityString, "Best Quality")) {
			g_streamQuality = 100L;
			g_usePng = true;
		}
	}
	return 0;
}

LUA_FUNCTION(OpenRetroArch) {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOWNOACTIVATE;
	ZeroMemory(&pi, sizeof(pi));

		
	if (CreateProcess("bin\\RetroArch\\retroarch.exe", LPSTR(""), NULL, NULL, false, 0, NULL, NULL, &si, &pi)) {
		LUA->PushString("Created process");
	} else {
		LUA->PushString("Failed to create process");
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return 1;
}

LUA_FUNCTION(CloseRetroArch) {
	const char* processName = "retroarch.exe";
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			if (_stricmp(entry.szExeFile, processName) == 0)
			{
				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
				TerminateProcess(hProcess, 1);
				CloseHandle(hProcess);
			}
		}
	}

	CloseHandle(snapshot);

	return 0;
}

LUA_FUNCTION(ToggleMenu) {
	PostMessage(g_retroarch, WM_COMMAND, 0x00009C44, 0);
	return 0; 
}

LUA_FUNCTION(ToggleAudio) {
	PostMessage(g_retroarch, WM_COMMAND, 0x00009C5A, 0);
	return 0;
}

LUA_FUNCTION(SaveState) {
	PostMessage(g_retroarch, WM_COMMAND, 0x00009C48, 0);
	return 0;
}

LUA_FUNCTION(LoadState) {
	PostMessage(g_retroarch, WM_COMMAND, 0x00009C47, 0);
	return 0;
}

LUA_FUNCTION(OpenContentFileDialog) {
	PostMessage(g_retroarch, WM_COMMAND, 0x00009C41, 0);
	HWND fileDialog = GetWindow(g_retroarch, GW_HWNDPREV);
	BringWindowToTop(fileDialog);
	return 0;
}

LUA_FUNCTION(OpenCoreFileDialog) {
	PostMessage(g_retroarch, WM_COMMAND, 0x00009C46, 0);
	HWND fileDialog = GetWindow(g_retroarch, GW_HWNDPREV);
	BringWindowToTop(fileDialog);
	return 0;
}

LUA_FUNCTION(GetWindowTitle) {
	char title[256];
	GetWindowText(g_retroarch, title, 256);
	LUA->PushString(title);

	return 1;
}

BITMAPINFOHEADER createBitmapHeader(int width, int height)
{
	BITMAPINFOHEADER  bi;

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = width;
	bi.biHeight = height;
	bi.biPlanes = 1;
	bi.biBitCount = 24;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	return bi;
}

HBITMAP GdiPlusWindowCapture(HWND hwnd) {
	HDC hwindowDC = GetDC(hwnd);
	HDC hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
	if (!hwindowCompatibleDC) {
		DeleteDC(hwindowCompatibleDC);
		ReleaseDC(hwnd, hwindowDC);
		return NULL;
	}

	RECT rect;
	GetClientRect(g_retroarch, &rect);
	int width = rect.right;
	int height = rect.bottom;

	HBITMAP hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
	BITMAPINFOHEADER bi = createBitmapHeader(width, height);

	SelectObject(hwindowCompatibleDC, hbwindow);

	DWORD dwBmpSize = ((width * bi.biBitCount + 31) & ~31) * 4 * height;
	HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
	char* lpbitmap = (char*)GlobalLock(hDIB);

	if (!BitBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, 0, 0, SRCCOPY)) {
		DeleteDC(hwindowCompatibleDC);
		ReleaseDC(hwnd, hwindowDC);

		GlobalUnlock(hDIB);
		GlobalFree(hDIB);
		return NULL;
	}
	GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, lpbitmap, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

	DeleteDC(hwindowCompatibleDC);
	ReleaseDC(hwnd, hwindowDC);

	GlobalUnlock(hDIB);
	GlobalFree(hDIB);

	return hbwindow;
}

bool saveToMemory(HBITMAP* hbitmap, std::vector<BYTE>& data, std::string dataFormat = "png", ULONG quality = 50L)
{
	Gdiplus::Bitmap bmp(*hbitmap, nullptr);
	IStream* istream = nullptr;
	CreateStreamOnHGlobal(NULL, TRUE, &istream);

	CLSID clsid;
	EncoderParameters encoderParameters;
	if (dataFormat.compare("bmp") == 0) { CLSIDFromString(L"{557cf400-1a04-11d3-9a73-0000f81ef32e}", &clsid); }
	else if (dataFormat.compare("jpg") == 0) { 
		CLSIDFromString(L"{557cf401-1a04-11d3-9a73-0000f81ef32e}", &clsid);
		encoderParameters.Count = 1;
		encoderParameters.Parameter[0].Guid = EncoderQuality;
		encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
		encoderParameters.Parameter[0].NumberOfValues = 1;
		encoderParameters.Parameter[0].Value = &quality;
	}
	else if (dataFormat.compare("gif") == 0) { CLSIDFromString(L"{557cf402-1a04-11d3-9a73-0000f81ef32e}", &clsid); }
	else if (dataFormat.compare("tif") == 0) { CLSIDFromString(L"{557cf405-1a04-11d3-9a73-0000f81ef32e}", &clsid); }
	else if (dataFormat.compare("png") == 0) { CLSIDFromString(L"{557cf406-1a04-11d3-9a73-0000f81ef32e}", &clsid); }

	Gdiplus::Status status;
	if (dataFormat.compare("jpg") == 0) {
		status = bmp.Save(istream, &clsid, &encoderParameters);
	}
	else {
		status = bmp.Save(istream, &clsid, NULL);
	}
	if (status != Gdiplus::Status::Ok)
		return false;

	HGLOBAL hg = NULL;
	GetHGlobalFromStream(istream, &hg);

	int bufsize = GlobalSize(hg);
	data.resize(bufsize);

	LPVOID pimage = GlobalLock(hg);
	memcpy(&data[0], pimage, bufsize);
	GlobalUnlock(hg);
	GlobalFree(hg);
	istream->Release();
	DeleteObject(hbitmap);
	return true;
}

LUA_FUNCTION(GrabWindowFrame) {
	try {
		HBITMAP hBitmap = GdiPlusWindowCapture(g_retroarch);
		if (hBitmap != NULL) {
			string format = "jpg";
			if (g_usePng) format = "png";
			g_frameData.clear();
			if (saveToMemory(&hBitmap, g_frameData, format, g_streamQuality)) {
				LUA->PushBool(true);
			}
			else {
				LUA->PushBool(false);
			}
		}
		else {
			LUA->PushBool(false);
		}
		DeleteObject(hBitmap);
	} catch (int e) {
		g_retroarch = nullptr;
		LUA->PushBool(false);
	}

	return 1;
}

LUA_FUNCTION(GetFrameBase64) {
	string encoded = "data:image/bmp;base64," + base64_encode(g_frameData.data(), g_frameData.size(), false);
	LUA->PushString(encoded.c_str());

	return 1;
}

LUA_FUNCTION(GetCores) {
	stringstream stream;
	const char* path = "bin\\RetroArch\\cores";
	DIR* dir;
	struct dirent* ent;
	if ((dir = opendir(path)) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			stream << ent->d_name << "|";
		}
		closedir(dir);
		LUA->PushString(stream.str().c_str());
		return 1;
	}
	else {
		return 0;
	}
}

LUA_FUNCTION(GetCoreInfo) {
	if (LUA->IsType(1, Type::STRING)) {
		string coreName = LUA->GetString(1);
		const char* fileName = string("bin\\RetroArch\\info\\" + coreName + ".info").c_str();
		ifstream infoFile(fileName);
		if (infoFile.is_open()) {
			LUA->CreateTable();
			string line;
			while (getline(infoFile, line)) {
				if (line[0] != '#') {
					size_t pos = line.find(" = ");
					string key = line.substr(0, pos);
					line.erase(0, pos + 3);

					LUA->PushString(key.c_str());
					LUA->PushString(line.c_str());
					LUA->SetTable(-3);
				}
			}
			return 1;
		}
		infoFile.close();
		return 0;
	}
	return 0;
}

LUA_FUNCTION(GetInfos) {
	stringstream stream;
	const char* path = "bin\\RetroArch\\info";
	DIR* dir;
	struct dirent* ent;
	if ((dir = opendir(path)) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			stream << ent->d_name << "|";
		}
		closedir(dir);
		LUA->PushString(stream.str().c_str());
		return 1;
	}
	else {
		return 0;
	}
}

LUA_FUNCTION(RetroArchExists) {
	string exe = "bin\\RetroArch\\retroarch_debug.exe";
	
	if (FILE* file = fopen(exe.c_str(), "r")) {
		fclose(file);
		LUA->PushBool(true);
	}
	else {
		LUA->PushBool(false);
	}

	return 1;
}

LUA_FUNCTION(DownloadCore) {
	if (LUA->IsType(1, Type::STRING)) {
		const char* coreName = LUA->GetString(1);
		string coreZip = string(coreName) + ".zip";
		string url = "https://buildbot.libretro.com/nightly/windows/x86_64/latest/" + coreZip;
		string zipDest = "bin\\RetroArch\\downloads\\" + coreZip;
		string coreDest = "bin\\RetroArch\\downloads\\" + string(coreName);
		HRESULT result = URLDownloadToFile(NULL, url.c_str(), zipDest.c_str(), 0, NULL);

		if (SUCCEEDED(result)) {
			string command = "powershell.exe Expand-Archive -Path \"" + zipDest + "\" -DestinationPath \"bin\\RetroArch\\downloads\\downloaded_core\"; Move-Item -Path \"bin\\RetroArch\\downloads\\downloaded_core\\" + coreName + "\" -Destination \"bin\\RetroArch\\cores\\" + coreName + "\"; Remove-Item \"" + zipDest + "\"; Remove-Item \"bin\\RetroArch\\downloads\\downloaded_core\" -Recurse;";
			STARTUPINFO si;
			PROCESS_INFORMATION pi;

			ZeroMemory(&si, sizeof(si));
			ZeroMemory(&pi, sizeof(pi));

			CreateProcess(NULL, LPSTR(command.c_str()), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
			WaitForSingleObject(pi.hProcess, 10000);

			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			LUA->PushBool(true);
		}
		else {
			LUA->PushBool(false);
		}
	}
	return 1;
}

struct ComInit {
	HRESULT hr;
	ComInit() : hr(::CoInitialize(nullptr)) {}
	~ComInit() { if (SUCCEEDED(hr)) ::CoUninitialize(); }
};

LUA_FUNCTION(GetAvailableCores) {
	ComInit init;
	CComPtr<IStream> pStream;

	HRESULT hr = URLOpenBlockingStreamW(nullptr, L"http://buildbot.libretro.com/nightly/windows/x86_64/latest/.index", &pStream, 0, nullptr);
	if (FAILED(hr)) {
		LUA->PushString("Could not connect to buildbot.");
		return 1;
	}

	char buffer[8192];
	do {
		DWORD bytesRead = 0;
		hr = pStream->Read(buffer, sizeof(buffer), &bytesRead);
		if (bytesRead > 0) {
			LUA->PushString(buffer);
			return 1;
		}
	} while (SUCCEEDED(hr) && hr != S_FALSE);

	if (FAILED(hr)) {
		LUA->PushString("Could not download index.");
		return 1;
	}
}

LUA_FUNCTION(OpenDownloadLink) {
	ShellExecute(0, 0, "https://www.retroarch.com/?page=platforms", 0, 0, SW_SHOW);
	return 0;
}

void SetRetroArchConfigValues(vector<string> values) {
	string cfgPath = "bin\\RetroArch\\retroarch.cfg";
	ifstream cfgFileIn(cfgPath);

	if (cfgFileIn.is_open()) {
		string line;
		bool changed = false;
		vector<string> cfgOut;
		while (getline(cfgFileIn, line)) {
			if (line.find("video_window_opacity = ") != string::npos) {
				string replaceString = "video_window_opacity = " + values[0];
				if (strcmp(line.c_str(), replaceString.c_str()) != 0) {
					line = replaceString;
					changed = true;
				}
			}
			if (line.find("video_scale = ") != string::npos) {
				string replaceString = "video_scale = " + values[1];
				if (strcmp(line.c_str(), replaceString.c_str()) != 0) {
					line = replaceString;
					changed = true;
				}
			}
			cfgOut.push_back(line);
		}

		cfgFileIn.close();

		if (changed) {
			ofstream cfgFileOut(cfgPath);
			if (cfgFileOut.is_open()) {
				for (auto it = cfgOut.begin(); it != cfgOut.end(); ++it) {
					cfgFileOut << *it << endl;
				}
			}
		}
	}
}

LUA_FUNCTION(SetRetroArchCfg) {
	int valueCount = LUA->GetNumber(1);
	vector<string> values;
	for (int i = 0; i < valueCount; i++) {
		values.push_back(LUA->GetString(i + 2));
	}
	SetRetroArchConfigValues(values);

	return 0;
}


//
// Called when your module is opened
//
GMOD_MODULE_OPEN()
{
	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->PushCFunction(OpenRetroArch);
	LUA->SetField(-2, "OpenRetroArch");
	LUA->PushCFunction(CloseRetroArch);
	LUA->SetField(-2, "CloseRetroArch");
	LUA->PushCFunction(GrabWindowFrame);
	LUA->SetField(-2, "GrabWindowFrame");
	LUA->PushCFunction(GetFrameBase64);
	LUA->SetField(-2, "GetFrameBase64");
	LUA->PushCFunction(SetRetroArchHWND);
	LUA->SetField(-2, "SetRetroArchHWND");
	LUA->PushCFunction(SetQuality);
	LUA->SetField(-2, "SetQuality");
	LUA->PushCFunction(GetCores);
	LUA->SetField(-2, "GetCores");
	LUA->PushCFunction(ToggleMenu);
	LUA->SetField(-2, "ToggleMenu");
	LUA->PushCFunction(ToggleAudio);
	LUA->SetField(-2, "ToggleAudio");
	LUA->PushCFunction(SaveState);
	LUA->SetField(-2, "SaveState");
	LUA->PushCFunction(LoadState);
	LUA->SetField(-2, "LoadState");
	LUA->PushCFunction(GetWindowTitle);
	LUA->SetField(-2, "GetWindowTitle");
	LUA->PushCFunction(GetCoreInfo);
	LUA->SetField(-2, "GetCoreInfo");
	LUA->PushCFunction(GetInfos);
	LUA->SetField(-2, "GetInfos");
	LUA->PushCFunction(OpenContentFileDialog);
	LUA->SetField(-2, "OpenContentFileDialog");
	LUA->PushCFunction(OpenCoreFileDialog);
	LUA->SetField(-2, "OpenCoreFileDialog");
	LUA->PushCFunction(DownloadCore);
	LUA->SetField(-2, "DownloadCore");
	LUA->PushCFunction(GetAvailableCores);
	LUA->SetField(-2, "GetAvailableCores");
	LUA->PushCFunction(RetroArchExists);
	LUA->SetField(-2, "RetroArchExists");
	LUA->PushCFunction(OpenDownloadLink);
	LUA->SetField(-2, "OpenDownloadLink");
	LUA->PushCFunction(SetRetroArchCfg);
	LUA->SetField(-2, "SetRetroArchCfg");

	GdiplusStartup(&g_gdiplusToken, &g_gdiplusStartupInput, NULL);

	return 0;
}

//
// Called when your module is closed
//
GMOD_MODULE_CLOSE()
{
	GdiplusShutdown(g_gdiplusToken);

	return 0;
}