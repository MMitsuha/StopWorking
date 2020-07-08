#pragma once
#pragma warning(disable:4201)
#pragma warning(disable:4100)

#include <ntifs.h>
#include <ntdef.h>
#include <windef.h>
#include <ntimage.h>
#include <intrin.h>
#include <ntddkbd.h>
#include "ErrorCode.h"
#include "IoCode.h"
#include "Undisclosed.h"
#include "Public.h"

#define TRY_START __try \
						{
#define TRY_END(RetStatus) } \
							__except (1) \
							{ \
								KdPrint(("[-] SuperDriver:[%S] 发生未知错误! 异常编号:%X", __FUNCTIONW__, GetExceptionCode())); \
							} \
							return RetStatus;
#define TRY_END_NOSTATUS } \
							__except (1) \
							{ \
								KdPrint(("[-] SuperDriver:[%S] 发生未知错误! 异常编号:%X", __FUNCTIONW__, GetExceptionCode())); \
							} \
							return;

#ifdef DBG
#define PrintSuc(Format, ...) DbgPrint("[+] SuperDriver: " Format, __VA_ARGS__)
#define PrintIfm(Format, ...) DbgPrint("[*] SuperDriver: " Format, __VA_ARGS__)
#define PrintErr(Format, ...) DbgPrint("[-] SuperDriver: " Format, __VA_ARGS__)
/*
#define PrintSuc(Format, ...) DbgPrint("[+] SuperDriver:[%S] %s", __FUNCTIONW__, Format, __VA_ARGS__, "\n")
#define PrintIfm(Format, ...) DbgPrint("[*] SuperDriver:[%S] %s", __FUNCTIONW__, Format, __VA_ARGS__, "\n")
#define PrintErr(Format, ...) DbgPrint("[-] SuperDriver:[%S] %s", __FUNCTIONW__, Format, __VA_ARGS__, "\n")
*/

#define BugCheck KeBugCheckEx
#else
#define PrintSuc
#define PrintIfm
#define PrintErr

#define BugCheck
#endif