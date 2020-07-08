#include "Includes.h"

INT main(
	_In_ UINT argc,
	_In_ CHAR** argv
)
{
	if (!LoadNTDriver((PWCHAR)L"SW", (PWCHAR)L".\\StopWorking.sys"))
		wcout << L"Æô¶¯Çý¶¯Ê§°Ü! Errorcode:" << GetLastError() << endl;

	HANDLE hDriver = CreateFileW(R3_LINK_NAME, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (hDriver != INVALID_HANDLE_VALUE)
	{
		HANDLE hEvent = OpenEventW(SYNCHRONIZE, FALSE, R3_EVENT_NAME);
		if (hEvent != INVALID_HANDLE_VALUE)
		{
			while (TRUE)
			{
				BOOL bRet = TRUE;
				R3_BUFFER Buffer = { 0 };
				DWORD ReturnedLength = 0;
				WaitForSingleObject(hEvent, INFINITE);
				bRet = DeviceIoControl(hDriver, READ_DATA, NULL, 0, &Buffer, sizeof(Buffer), &ReturnedLength, NULL);
				if (bRet)
				{
					cout << (UINT64)Buffer.hProcessID << endl;
					wcout << Buffer.wProcessPath << endl;
					cout << (Buffer.bIsCreate ? "CREATE" : "TERMINAL") << endl << endl;
				}
			}
		}
	}

	return 0;
}