#pragma once

#include <windef.h>

#define R0_DEVICE_NAME L"\\Device\\SWDevice"
#define R0_LINK_NAME L"\\??\\SWLink"
#define R0_EVENT_NAME L"\\BaseNamedObjects\\SWEvent"

#define R3_LINK_NAME L"\\\\.\\SWLink"
#define R3_EVENT_NAME L"Global\\SWEvent"

typedef struct _R3_BUFFER
{
	HANDLE hProcessID;       //���̵�PID
	WCHAR wProcessPath[MAX_PATH];
	BOOLEAN bIsCreate;    //��ʾ�Ǵ������̻�������.�������̻ص����Կ���
}R3_BUFFER, * PR3_BUFFER;