/*++

Module Name:

queue.h

Abstract:

This file contains the queue definitions.

Environment:

Kernel-mode Driver Framework

--*/

EXTERN_C_START

//
// This is the context that can be placed per queue
// and would contain per queue information.
//
typedef struct _QUEUE_CONTEXT {

	ULONG PrivateDeviceData;  // just a placeholder

} QUEUE_CONTEXT, *PQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, QueueGetContext)

NTSTATUS
JDriverQueueInitialize(
	_In_ WDFDEVICE Device
);

//
// Events from the IoQueue object
//
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL JDriverEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_STOP JDriverEvtIoStop;



NTSTATUS GetHidDescriptor(IN WDFDEVICE Device, IN WDFREQUEST Request);
NTSTATUS GetDeviceAttributes(IN WDFREQUEST Request);
NTSTATUS GetReportDescriptor(IN WDFREQUEST Request);


EVT_WDF_USB_READER_COMPLETION_ROUTINE InterruptPipeReadComplete;
EVT_WDF_USB_READERS_FAILED EvtReadFailed;


EXTERN_C_END
