#pragma once

#include <windef.h>

#define R0_DEVICE_NAME L"\\Device\\SWDevice"
#define R0_LINK_NAME L"\\??\\SWLink"
#define R0_EVENT_NAME L"\\BaseNamedObjects\\SWEvent"

#define R3_LINK_NAME L"\\\\.\\SWLink"
#define R3_EVENT_NAME L"Global\\SWEvent"

typedef struct _R3_BUFFER
{
	HANDLE hProcessID;       //进程的PID
	WCHAR wProcessPath[MAX_PATH];
	BOOLEAN bIsCreate;    //表示是创建进程还是销毁.创建进程回调可以看到
}R3_BUFFER, * PR3_BUFFER;