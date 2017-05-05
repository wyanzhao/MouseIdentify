/*++
Copyright (c) 2008  Microsoft Corporation

Module Name:

    moufiltr.h

Abstract:

    This module contains the common private declarations for the mouse
    packet filter

Environment:

    kernel mode only

Notes:


Revision History:


--*/

#ifndef MOUFILTER_H
#define MOUFILTER_H


#include <ntddk.h>
#include <kbdmou.h>
#include <ntddmou.h>
#include <ntdd8042.h>
#include <wdf.h>
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>

#include <initguid.h>
#include <devguid.h>

#define MOUFILTER_POOL_TAG (ULONG) 'tlfK'
#if DBG

#define TRAP()                      DbgBreakPoint()

#define DebugPrint(_x_) DbgPrint _x_

#else   // DBG

#define TRAP()

#define DebugPrint(_x_)

#endif

#define IOCTL_INDEX             0x800
#define IOCEL_HANDLE            0x801
#define IOCTL_MOUFILTR_GET_MOUSE_ATTRIBUTES CTL_CODE( FILE_DEVICE_MOUSE,   \
                                                        IOCTL_INDEX,    \
                                                        METHOD_BUFFERED,    \
                                                        FILE_READ_DATA)

#define IOCTL_MOUFILTR_GET_EVENT_HANDLE CTL_CODE( FILE_DEVICE_MOUSE,   \
                                                        IOCEL_HANDLE,    \
                                                        METHOD_BUFFERED,    \
                                                        FILE_READ_DATA)
//WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_EXTENSION, FilterGetData)


	//
	// IOCTL Related defintions
	//

	//
	// Used to identify kbfilter bus. This guid is used as the enumeration string
	// for the device id.
DEFINE_GUID(GUID_BUS_MOUFILTER,0xa65c87f9, 0xbe02, 0x4ed9, 0x92, 0xec, 0x1, 0x2d, 0x41, 0x61, 0x69, 0xfa);
// {A65C87F9-BE02-4ed9-92EC-012D416169FA}

DEFINE_GUID(GUID_DEVINTERFACE_MOUFILTER,0x3fb7299d, 0x6847, 0x4490, 0xb0, 0xc9, 0x99, 0xe0, 0x98, 0x6a, 0xb8, 0x86);
// {3FB7299D-6847-4490-B0C9-99E0986AB886}


#define  MOUFILTER_DEVICE_ID L"{A65C87F9-BE02-4ed9-92EC-012D416169FA}\\MouseFilter\0"

typedef struct _DEVICE_EXTENSION
{
     //
    // Previous hook routine and context
    //                               
    PVOID UpperContext;
     
    PI8042_MOUSE_ISR UpperIsrHook;

	WDFQUEUE rawPdoQueue;

    //
    // Write to the mouse in the context of MouFilter_IsrHook
    //
    IN PI8042_ISR_WRITE_PORT IsrWritePort;

    //
    // Context for IsrWritePort, QueueMousePacket
    //
    IN PVOID CallContext;

    //
    // Queue the current packet (ie the one passed into MouFilter_IsrHook)
    // to be reported to the class driver
    //
    IN PI8042_QUEUE_PACKET QueueMousePacket;

    //
    // The real connect data that this driver reports to
    //
    CONNECT_DATA UpperConnectData;
	ULONG AppendDevice;
	PKEVENT Event;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_EXTENSION,
	FilterGetData)

	typedef struct _WORKER_ITEM_CONTEXT {

	WDFREQUEST  Request;
	WDFIOTARGET IoTarget;

} WORKER_ITEM_CONTEXT, *PWORKER_ITEM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WORKER_ITEM_CONTEXT, GetWorkItemContext)

typedef struct _RPDO_DEVICE_DATA
{

	ULONG InstanceNo;

	//
	// Queue of the parent device we will forward requests to
	//
	WDFQUEUE ParentQueue;

} RPDO_DEVICE_DATA, *PRPDO_DEVICE_DATA;


WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(RPDO_DEVICE_DATA, PdoGetData)
//
// Prototypes
//
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD MouFilter_EvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL MouFilter_EvtIoInternalDeviceControl;
 

VOID
MouFilter_DispatchPassThrough(
     _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target
    );

BOOLEAN
MouFilter_IsrHook (
    PVOID         DeviceExtension,
    PMOUSE_INPUT_DATA       CurrentInput, 
    POUTPUT_PACKET          CurrentOutput,
    UCHAR                   StatusByte,
    PUCHAR                  DataByte,
    PBOOLEAN                ContinueProcessing,
    PMOUSE_STATE            MouseState,
    PMOUSE_RESET_SUBSTATE   ResetSubState
);

VOID
MouFilter_ServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMOUSE_INPUT_DATA InputDataStart,
    IN PMOUSE_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
    );

VOID
MouFilter_EvtIoDeviceControlFromRawPdo(
	IN WDFQUEUE      Queue,
	IN WDFREQUEST    Request,
	IN size_t        OutputBufferLength,
	IN size_t        InputBufferLength,
	IN ULONG         IoControlCode
);

NTSTATUS
MouFilter_CreateRawPdo(
	WDFDEVICE       Device,
	ULONG           InstanceNo
);

VOID
MouFilter_EvtIoDeviceControlForRawPdo(
	IN WDFQUEUE      Queue,
	IN WDFREQUEST    Request,
	IN size_t        OutputBufferLength,
	IN size_t        InputBufferLength,
	IN ULONG         IoControlCode
);

#endif  // MOUFILTER_H

