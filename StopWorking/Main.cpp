#include "Includes.h"

PDEVICE_OBJECT g_pCDODevice = NULL;

BOOLEAN g_bCreate = FALSE;

UNICODE_STRING g_uniDeviceName = RTL_CONSTANT_STRING(R0_DEVICE_NAME);
UNICODE_STRING g_uniLinkName = RTL_CONSTANT_STRING(R0_LINK_NAME);
UNICODE_STRING g_uniEventName = RTL_CONSTANT_STRING(R0_EVENT_NAME);

typedef struct _DEVICE_EXTENSION
{
	R3_BUFFER R3Buffer;

	HANDLE R3EventHandle;
	PKEVENT R3Event;   //全局事件对象,ring3使用

	KEVENT pkKernelEvent;
	KSPIN_LOCK SpinLock;
}DEVICE_EXTENSION, * PDEVICE_EXTENSION;

#ifdef __cplusplus
EXTERN_C_START
#endif // __cplusplus

NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT pDriverObject,
	_In_ PUNICODE_STRING puniRegistryPath
);

NTSTATUS
InitWorkFunction(
	_In_ PDRIVER_OBJECT pDriverObject,
	_In_ PUNICODE_STRING uniDeviceName,
	_In_ PUNICODE_STRING uniLinkName,
	_In_ PUNICODE_STRING uniEventName
);

#ifdef __cplusplus
EXTERN_C_END
#endif // __cplusplus

VOID
DriverUnload(
	_In_ PDRIVER_OBJECT pDriverObject
);

NTSTATUS
DefaultDispatch(
	_In_ PDEVICE_OBJECT pDeviceObject,
	_In_ PIRP pIrp
);

NTSTATUS
IoControlDispatch(
	_In_ PDEVICE_OBJECT pDeviceObject,
	_In_ PIRP pIrp
);

VOID ProcessCreateMonEx					//进程事件 回调处理函数
(
	__inout PEPROCESS EProcess,
	_In_ HANDLE ProcessID,
	__in_opt PPS_CREATE_NOTIFY_INFO CreateInfo
);

NTSTATUS Mon_CreateProcMonEx(
	_In_ BOOLEAN bCreate
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (INIT, InitWorkFunction)
#endif

NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT pDriverObject,
	_In_ PUNICODE_STRING puniRegistryPath
)
{
	PrintIfm("驱动已加载,作者:caizhe666.\n");

	NTSTATUS ntStatus = STATUS_SUCCESS;
	pDriverObject->DriverUnload = DriverUnload;
	do
	{
		if (!pDriverObject)
		{
			PrintErr("无效的参数!\n");
			ntStatus = STATUS_INVALID_PARAMETER;
			break;
		}

		PLDR_DATA_TABLE_ENTRY ldrDataTable = { 0 };
		ldrDataTable = (PLDR_DATA_TABLE_ENTRY)pDriverObject->DriverSection;
		ldrDataTable->Flags |= 0x20;

		ntStatus = InitWorkFunction(pDriverObject, &g_uniDeviceName, &g_uniLinkName, &g_uniEventName);
		if (!NT_SUCCESS(ntStatus))
		{
			PrintErr("InitWorkFunction 错误! Errorcode:%X\n", ntStatus);
			break;
		}
		KeInitializeEvent(&((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->pkKernelEvent, SynchronizationEvent, TRUE);
		KeInitializeSpinLock(&((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->SpinLock);

		for (SIZE_T i = 0; i < IRP_MJ_MAXIMUM_FUNCTION + 1; i++)
			pDriverObject->MajorFunction[i] = DefaultDispatch;
		pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoControlDispatch;
	} while (FALSE);

	return ntStatus;
}

VOID
DriverUnload(
	_In_ PDRIVER_OBJECT pDriverObject
)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	if (g_bCreate)
	{
		ntStatus = Mon_CreateProcMonEx(FALSE);
		if (!NT_SUCCESS(ntStatus))
			BugCheck(UNABLE_TO_STOP_CALLBACKS, ntStatus, 0, 0, 0);
	}

	if (!((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->R3Event)
		ZwClose(((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->R3Event);
	if (g_pCDODevice)
		IoDeleteDevice(g_pCDODevice);
	ntStatus = IoDeleteSymbolicLink(&g_uniLinkName);
	if (!NT_SUCCESS(ntStatus))
		BugCheck(UNABLE_TO_DELETE_SYMBOLIC_LINK, ntStatus, 0, 0, 0);
}

NTSTATUS
InitWorkFunction(
	_In_ PDRIVER_OBJECT pDriverObject,
	_In_ PUNICODE_STRING uniDeviceName,
	_In_ PUNICODE_STRING uniLinkName,
	_In_ PUNICODE_STRING uniEventName
)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	do
	{
		ntStatus = IoCreateDevice(pDriverObject, sizeof(DEVICE_EXTENSION), uniDeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, TRUE, &g_pCDODevice);
		if (!NT_SUCCESS(ntStatus))
		{
			PrintErr("创建设备对象失败! Errorcode:%X\n", ntStatus);
			break;
		}

		pDriverObject->Flags |= DO_BUFFERED_IO;
		PrintSuc("创建设备对象成功!\n");

		//创建事件.ring3->ring0的事件
		RtlZeroMemory((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension, sizeof(DEVICE_EXTENSION));
		((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->R3Event = IoCreateNotificationEvent(uniEventName, &((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->R3EventHandle);
		if (!((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->R3Event)
		{
			PrintErr("创建全局事件失败!\n");
			ntStatus = STATUS_UNSUCCESSFUL;
			break;
		}
		PrintSuc("创建全局事件成功!");
		KeClearEvent(((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->R3Event);

		//创建符号链接

		ntStatus = IoCreateSymbolicLink(uniLinkName, uniDeviceName);
		if (!NT_SUCCESS(ntStatus))
		{
			PrintErr("创建符号链接失败! Errorcode:%X\n", ntStatus);
			break;
		}
		PrintSuc("创建符号链接成功!");

		ntStatus = Mon_CreateProcMonEx(TRUE);
		if (!NT_SUCCESS(ntStatus))
		{
			PrintErr("Mon_CreateProcMonEx 错误! Errorcode:%X\n", ntStatus);
			break;
		}
		g_bCreate = TRUE;
		PrintSuc("Mon_CreateProcMonEx 成功!");
	} while (FALSE);

	if (!NT_SUCCESS(ntStatus))
	{
		if (g_bCreate)
		{
			ntStatus = Mon_CreateProcMonEx(FALSE);
			if (!NT_SUCCESS(ntStatus))
				BugCheck(UNABLE_TO_STOP_CALLBACKS, ntStatus, 0, 0, 0);
		}

		if (!((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->R3Event)
			ZwClose(((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->R3Event);
		if (g_pCDODevice)
			IoDeleteDevice(g_pCDODevice);
		ntStatus = IoDeleteSymbolicLink(uniLinkName);
		if (!NT_SUCCESS(ntStatus))
			BugCheck(UNABLE_TO_DELETE_SYMBOLIC_LINK, ntStatus, 0, 0, 0);
	}

	return ntStatus;
}

NTSTATUS
DefaultDispatch(
	_In_ PDEVICE_OBJECT pDeviceObject,
	_In_ PIRP pIrp
)
{
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return pIrp->IoStatus.Status;
}

NTSTATUS
IoControlDispatch(
	_In_ PDEVICE_OBJECT pDeviceObject,
	_In_ PIRP pIrp
)
{
	ULONG_PTR Size = 0;
	NTSTATUS ntStatus = STATUS_SUCCESS;

	PIO_STACK_LOCATION StackLocation = IoGetCurrentIrpStackLocation(pIrp);
	PVOID SystemBuffer = pIrp->AssociatedIrp.SystemBuffer;
	ULONG InBufferLength = StackLocation->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutBufferLength = StackLocation->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG ControlCode = StackLocation->Parameters.DeviceIoControl.IoControlCode;

	switch (ControlCode)
	{
		case READ_DATA:
			if (OutBufferLength < sizeof(PR3_BUFFER))
			{
				PrintErr("缓冲区过小!");
				ntStatus = STATUS_BUFFER_TOO_SMALL;
				break;
			}
			//((PR3_BUFFER)SystemBuffer)->bIsCreateMark = ((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->bIsCreateMark;
			//((PR3_BUFFER)SystemBuffer)->hParProcessId = ((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->hParProcessId;
			RtlCopyMemory((PR3_BUFFER)SystemBuffer, &((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->R3Buffer, sizeof(R3_BUFFER));
			Size = sizeof(R3_BUFFER);

			if (!KeResetEvent(((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->R3Event))
				BugCheck(EVENT_FIRED, 2, 0, 0, 0);

			if (KeSetEvent(&((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->pkKernelEvent, 0, FALSE))
				BugCheck(EVENT_FIRED, 0, 0, 0, 0);
			break;

		default:
			PrintErr("未知控制代码:%X\n", ControlCode);
			ntStatus = STATUS_INVALID_PARAMETER;
			break;
	}

	pIrp->IoStatus.Information = Size;
	pIrp->IoStatus.Status = ntStatus;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return pIrp->IoStatus.Status;
}

VOID ProcessCreateMonEx					//进程事件 回调处理函数
(
	__inout PEPROCESS EProcess,
	_In_ HANDLE ProcessID,
	__in_opt PPS_CREATE_NOTIFY_INFO CreateInfo
) {
	__try {
		LARGE_INTEGER LI;
		LI.QuadPart = 1000;
		KIRQL OldIrql = { 0 };
		KeAcquireSpinLock(&((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->SpinLock, &OldIrql);
		KeWaitForSingleObject(&((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->pkKernelEvent, UserRequest, KernelMode, FALSE, &LI);
		KeClearEvent(&((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->pkKernelEvent);
		KeReleaseSpinLock(&((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->SpinLock, OldIrql);

		do
		{
			NTSTATUS ntStatus = STATUS_SUCCESS;
			PUNICODE_STRING puniProcImageName = NULL;
			ntStatus = SeLocateProcessImageName(EProcess, &puniProcImageName);
			if (!NT_SUCCESS(ntStatus))
			{
				PrintErr("SeLocateProcessImageName 错误! Errorcode:%X\n", ntStatus);
				break;
			}

			((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->R3Buffer.hProcessID = ProcessID;
			RtlZeroMemory((((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->R3Buffer.wProcessPath), sizeof((((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->R3Buffer.wProcessPath)));
			if (puniProcImageName->Length > MAX_PATH - 1)
			{
				PrintErr("路径过长!");
				break;
			}
			RtlCopyMemory((((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->R3Buffer.wProcessPath), puniProcImageName->Buffer, puniProcImageName->Length);
			((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->R3Buffer.bIsCreate = CreateInfo ? TRUE : FALSE;

			if (KeSetEvent(((PDEVICE_EXTENSION)g_pCDODevice->DeviceExtension)->R3Event, 0, FALSE))
				BugCheck(EVENT_FIRED, 3, 0, 0, 0);

			if (CreateInfo)			//进程创建事件
			{
				if (CreateInfo->IsSubsystemProcess == TRUE || CreateInfo->FileOpenNameAvailable == FALSE)
				{
					PrintErr("不满足过滤 ,回调退出!\n");
					break;
				}

				PrintIfm("[CREATE] ProcPath:%wZ,%wZ ,ProcID:%I64u ,ParentProcID:%I64u ,CmdLine:%wZ ,ProcAddress:%p\n",
					CreateInfo->ImageFileName,
					puniProcImageName,
					(UINT64)ProcessID,
					(UINT64)CreateInfo->ParentProcessId,
					CreateInfo->CommandLine,
					EProcess
				);

				CreateInfo->CreationStatus = STATUS_ACCESS_DENIED;
			}
			else
			{
				PrintIfm("[TERMINAL] ProcPath:%wZ ,ProcID:%I64u ,ProcAddress:%p\n",
					puniProcImageName,
					(UINT64)ProcessID,
					EProcess
				);
			}
		} while (FALSE);
	}
	__except (1)
	{
		PrintErr("[-] SuperDriver: 发生未知错误! 异常编号:%X", __FUNCTIONW__, GetExceptionCode());
	}

	return;
}

NTSTATUS Mon_CreateProcMonEx(
	_In_ BOOLEAN bCreate
) {
	if (bCreate)
		return PsSetCreateProcessNotifyRoutineEx(ProcessCreateMonEx, FALSE);
	else
		return PsSetCreateProcessNotifyRoutineEx(ProcessCreateMonEx, TRUE);
}