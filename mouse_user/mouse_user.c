/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
PURPOSE.

Module Name:

mouse_user.C

Abstract:


Environment:

usermode console application

--*/


#include <basetyps.h>
#include <stdlib.h>
#include <wtypes.h>
#include <initguid.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <ntddmou.h>
#include <Subauth.h>

#pragma warning(disable:4201)

#include <setupapi.h>
#include <winioctl.h>

#pragma warning(default:4201)

#include"mouse_user.h"
//-----------------------------------------------------------------------------
// 4127 -- Conditional Expression is Constant warning
//-----------------------------------------------------------------------------
#define WHILE(constant) \
__pragma(warning(disable: 4127)) while(constant); __pragma(warning(default: 4127))

DEFINE_GUID(GUID_DEVINTERFACE_MOUFILTER,
	0x3fb7299d, 0x6847, 0x4490, 0xb0, 0xc9, 0x99, 0xe0, 0x98, 0x6a, 0xb8, 0x86);
// {3FB7299D-6847-4490-B0C9-99E0986AB886}

HANDLE hEvent;
HANDLE                              file;
ULONG                 moudattrib = 0;
SP_DEVICE_INTERFACE_DATA            deviceInterfaceData;
PSP_DEVICE_INTERFACE_DETAIL_DATA    deviceInterfaceDetailData = NULL;
ULONG bytes = 0;

void info_thread()
{
	while (1)
	{
		auto ret = WaitForSingleObject(hEvent, INFINITE);
		switch (ret)
		{
		case WAIT_ABANDONED:
			printf("WAIT_ABANDONED\n");
			break;
		case WAIT_TIMEOUT:
			printf("WAIT_TIMEOUT\n");
			break;
		case WAIT_FAILED:
			printf("WAIT_FAILED\n");
			break;
		default:
			break;
		}

		if (!DeviceIoControl(file,
			IOCTL_MOUFILTR_GET_MOUSE_ATTRIBUTES,
			NULL, 0,
			&moudattrib, sizeof(ULONG),
			&bytes, NULL)) {
			printf("Retrieve Mouse Attributes request failed:0x%x\n", GetLastError());
			free(deviceInterfaceDetailData);
			CloseHandle(file);
			CloseHandle(hEvent);
			return;
		}

		printf("\nMouse Attributes:\n"
			" MouseIdentifier:          0x%x\n",
			moudattrib);
	}
}

int
_cdecl
main(
	_In_ int argc,
	_In_ char *argv[]
)
{
	HDEVINFO                            hardwareDeviceInfo;
	ULONG                               predictedLength = 0;
	ULONG                               requiredLength = 0;
	ULONG                               i = 0;


	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	//
	// Open a handle to the device interface information set of all
	// present toaster class interfaces.
	//

	hardwareDeviceInfo = SetupDiGetClassDevs(
		(LPGUID)&GUID_DEVINTERFACE_MOUFILTER,
		NULL, // Define no enumerator (global)
		NULL, // Define no
		(DIGCF_PRESENT | // Only Devices present
			DIGCF_DEVICEINTERFACE)); // Function class devices.
	if (INVALID_HANDLE_VALUE == hardwareDeviceInfo)
	{
		printf("SetupDiGetClassDevs failed: %x\n", GetLastError());
		return 0;
	}

	deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	printf("\nList of MOUSEFILTER Device Interfaces\n");
	printf("---------------------------------\n");

	i = 0;

	//
	// Enumerate devices of toaster class
	//

	do {
		if (SetupDiEnumDeviceInterfaces(hardwareDeviceInfo,
			0, // No care about specific PDOs
			(LPGUID)&GUID_DEVINTERFACE_MOUFILTER,
			i, //
			&deviceInterfaceData)) {

			if (deviceInterfaceDetailData) {
				free(deviceInterfaceDetailData);
				deviceInterfaceDetailData = NULL;
			}

			//
			// Allocate a function class device data structure to
			// receive the information about this particular device.
			//

			//
			// First find out required length of the buffer
			//

			if (!SetupDiGetDeviceInterfaceDetail(
				hardwareDeviceInfo,
				&deviceInterfaceData,
				NULL, // probing so no output buffer yet
				0, // probing so output buffer length of zero
				&requiredLength,
				NULL)) { // not interested in the specific dev-node
				if (ERROR_INSUFFICIENT_BUFFER != GetLastError()) {
					printf("SetupDiGetDeviceInterfaceDetail failed %d\n", GetLastError());
					SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
					return FALSE;
				}

			}

			predictedLength = requiredLength;

			deviceInterfaceDetailData = malloc(predictedLength);

			if (deviceInterfaceDetailData) {
				deviceInterfaceDetailData->cbSize =
					sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			}
			else {
				printf("Couldn't allocate %d bytes for device interface details.\n", predictedLength);
				SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
				return FALSE;
			}


			if (!SetupDiGetDeviceInterfaceDetail(
				hardwareDeviceInfo,
				&deviceInterfaceData,
				deviceInterfaceDetailData,
				predictedLength,
				&requiredLength,
				NULL)) {
				printf("Error in SetupDiGetDeviceInterfaceDetail\n");
				SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
				free(deviceInterfaceDetailData);
				return FALSE;
			}
			printf("%d) %s\n", ++i,
				deviceInterfaceDetailData->DevicePath);
		}
		else if (ERROR_NO_MORE_ITEMS != GetLastError()) {
			free(deviceInterfaceDetailData);
			deviceInterfaceDetailData = NULL;
			continue;
		}
		else
			break;

	} WHILE(TRUE);


	SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);

	if (!deviceInterfaceDetailData)
	{
		printf("No device interfaces present\n");
		return 0;
	}

	//
	// Open the last toaster device interface
	//

	printf("\nOpening the last interface:\n %s\n",
		deviceInterfaceDetailData->DevicePath);

	file = CreateFile(deviceInterfaceDetailData->DevicePath,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL, // no SECURITY_ATTRIBUTES structure
		OPEN_EXISTING, // No special create flags
		0, // No special attributes
		NULL);

	if (INVALID_HANDLE_VALUE == file) {
		printf("Error in CreateFile: %x", GetLastError());
		free(deviceInterfaceDetailData);
		return 0;
	}
	
	hEvent = OpenEventW (SYNCHRONIZE, FALSE, L"Global\\MouseFilter");
	if (NULL == hEvent)
	{
		printf("Error in OpenEvent: %x", GetLastError());
		free(deviceInterfaceDetailData);
		return 0;
	}


	//
	// Send an IOCTL to retrive the Mouse attributes
	// These are cached in the kbfiltr
	//
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)info_thread, NULL, 0, 0);
	
	for (;;)
	{
		printf("If you want to exit, press 'q'\n");
		char c;
		c = (char )getchar();
		if (c == 'q')
			break;
	}

	if (deviceInterfaceDetailData)
		free(deviceInterfaceDetailData);
	CloseHandle(file);
	CloseHandle(hEvent);
	return 0;
}



