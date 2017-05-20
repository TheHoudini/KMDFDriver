/*++

Module Name:

queue.c

Abstract:

This file contains the queue entry points and callbacks.

Environment:

Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "queue.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, JDriverQueueInitialize)
#pragma alloc_text (PAGE, JDriverEvtIoDeviceControl)
#endif




#include <hidport.h>

CONST  UCHAR           G_DefaultReportDescriptor[] = {
	// Consumer control collection
	0x05,0x01,                      // USAGE_PAGE (GENERAL DESKTOP)
	0x09,0x04,                      // USAGE (Joystick)
	0xa1, 0x01,                     // COLLECTION (Application)
	0xa1, 0x00,                     //   COLLECTION (Physical)

	0x05, 0x09,                     //     USAGE_PAGE (Button)
	0x19, 0x01,                     //     USAGE_MINIMUM (Button 1)
	0x29, 0x08,                     //     USAGE_MAXIMUM (Button 8)
	0x15, 0x00,                     //     LOGICAL_MINIMUM (0)
	0x25, 0x01,                     //     LOGICAL_MAXIMUM (1)
	0x95, 0x08,                     //     REPORT_COUNT (8)
	0x75, 0x01,                     //     REPORT_SIZE (1)
	0x81, 0x02,                     //     INPUT (Data,Var,Abs)

	0xc0,                           //   END_COLLECTION
	0xc0
};

CONST HID_DESCRIPTOR G_DefaultHidDescriptor = {
	0x09,   // length of HID descriptor
	0x21,   // descriptor type == HID  0x21
	0x0100, // hid spec release
	0x00,   // country code == Not Specified
	0x04,   // number of HID class descriptors
	{ 0x22,   // descriptor type 
	sizeof(G_DefaultReportDescriptor) }  // total length of report descriptor
};


NTSTATUS
JDriverQueueInitialize(
	_In_ WDFDEVICE Device
)
/*++

Routine Description:


The I/O dispatch callbacks for the frameworks device object
are configured in this function.

A single default I/O Queue is configured for parallel request
processing, and a driver context memory allocation is created
to hold our structure QUEUE_CONTEXT.

Arguments:

Device - Handle to a framework device object.

Return Value:

VOID

--*/
{
	WDFQUEUE queue;
	NTSTATUS status;
	WDF_IO_QUEUE_CONFIG    queueConfig;

	PAGED_CODE();

	//
	// Configure a default queue so that requests that are not
	// configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
	// other queues get dispatched here.
	//
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
		&queueConfig,
		WdfIoQueueDispatchParallel
	);

	queueConfig.EvtIoInternalDeviceControl = JDriverEvtIoDeviceControl;
	queueConfig.EvtIoStop = JDriverEvtIoStop;

	status = WdfIoQueueCreate(
		Device,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&queue
	);

	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfIoQueueCreate failed %!STATUS!", status);
		return status;
	}


	WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);

	PDEVICE_CONTEXT context = DeviceGetContext(Device);

	status = WdfIoQueueCreate(Device,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&context->interruptMsgQueue
	);

	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "Second WdfIoQueueCreate failed %!STATUS!", status);
		return status;
	}


	return status;
}

VOID
JDriverEvtIoDeviceControl(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t OutputBufferLength,
	_In_ size_t InputBufferLength,
	_In_ ULONG IoControlCode
)
{

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(Queue);

	PAGED_CODE();
	TraceEvents(TRACE_LEVEL_INFORMATION,
		TRACE_QUEUE,
		"reading");

	NTSTATUS status = STATUS_SUCCESS;


	WDFDEVICE device = WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request));
	PDEVICE_CONTEXT context = DeviceGetContext(device);

	switch (IoControlCode)
	{

	case  IOCTL_HID_GET_DEVICE_ATTRIBUTES:
		status = GetDeviceAttributes(Request);
		break;

	case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
		status = GetHidDescriptor(device, Request);
		break;

	case IOCTL_HID_GET_REPORT_DESCRIPTOR:
		status = GetReportDescriptor(Request);
		break;

	case IOCTL_HID_READ_REPORT:
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "read report");
		status = WdfRequestForwardToIoQueue(Request, context->interruptMsgQueue);
		if (!NT_SUCCESS(status)) {
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
				"WdfRequestForwardToIoQueue failed with status: 0x%x\n", status);

			WdfRequestComplete(Request, status);
		}
		return;
	}


	case IOCTL_HID_WRITE_REPORT:
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "write report");
		break;

	case IOCTL_HID_GET_FEATURE:
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "Get feature");
		break;

	case IOCTL_HID_SET_FEATURE:
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "set feature");
		break;

	case IOCTL_HID_GET_INPUT_REPORT:
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "Get input report");
		break;

	case IOCTL_HID_SET_OUTPUT_REPORT:
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "set output report");
		break;

	case IOCTL_HID_GET_STRING:
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "get string");
		break;

	case IOCTL_HID_ACTIVATE_DEVICE:
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "activate device");
		break;

	case IOCTL_HID_DEACTIVATE_DEVICE:
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "deactivate device");
		break;

	case IOCTL_GET_PHYSICAL_DESCRIPTOR:
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "get phys descriptor");
		break;

	default:
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "something else");
		break;
	}
	WdfRequestComplete(Request, status);

	return;
}



NTSTATUS GetHidDescriptor(IN WDFDEVICE Device, IN WDFREQUEST Request)
{
	NTSTATUS            status = STATUS_SUCCESS;
	size_t              bytesToCopy = 0;
	WDFMEMORY           memory;

	UNREFERENCED_PARAMETER(Device);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "Get HID DESCRIPTOR Entry");

	status = WdfRequestRetrieveOutputMemory(Request, &memory);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfRequestRetrieveOutputMemory failed 0x%x", status);
		return status;
	}

	//
	// Use hardcoded "HID Descriptor" 
	//
	bytesToCopy = G_DefaultHidDescriptor.bLength;

	if (bytesToCopy == 0) {
		status = STATUS_INVALID_DEVICE_STATE;
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "G_DefaultHidDescriptor is zero, 0x%x", status);
		return status;
	}

	status = WdfMemoryCopyFromBuffer(memory,
		0, // Offset
		(PVOID)&G_DefaultHidDescriptor,
		bytesToCopy);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfMemoryCopyFromBuffer failed 0x%x", status);
		return status;
	}

	//
	// Report how many bytes were copied
	//
	WdfRequestSetInformation(Request, bytesToCopy);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "GetHidDescriptor Exit = 0x%x", status);
	return status;
}


NTSTATUS GetDeviceAttributes(IN WDFREQUEST Request)
{
	NTSTATUS                 status = STATUS_SUCCESS;
	PHID_DEVICE_ATTRIBUTES   deviceAttributes = NULL;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "GetDeviceAttributes Entry\n");

	status = WdfRequestRetrieveOutputBuffer(Request, sizeof(HID_DEVICE_ATTRIBUTES), &deviceAttributes, NULL);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfRequestRetrieveOutputBuffer failed 0x%x", status);
		return status;
	}



	deviceAttributes->Size = sizeof(HID_DEVICE_ATTRIBUTES);
	deviceAttributes->VendorID = 458;
	deviceAttributes->ProductID = 1004;
	deviceAttributes->VersionNumber = 1;

	WdfRequestSetInformation(Request, sizeof(HID_DEVICE_ATTRIBUTES));

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "GetDeviceAttributes Exit = 0x%x", status);
	return status;
}





NTSTATUS GetReportDescriptor(IN WDFREQUEST Request)
{
	NTSTATUS            status = STATUS_SUCCESS;
	ULONG_PTR           bytesToCopy;
	WDFMEMORY           memory;


	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "GetReportDescriptor Entry");


	status = WdfRequestRetrieveOutputMemory(Request, &memory);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfRequestRetrieveOutputMemory failed 0x%x", status);
		return status;
	}


	bytesToCopy = G_DefaultHidDescriptor.DescriptorList[0].wReportLength;

	if (bytesToCopy == 0) {
		status = STATUS_INVALID_DEVICE_STATE;
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "G_DefaultHidDescriptor's reportLenght is zero, 0x%x", status);
		return status;
	}

	status = WdfMemoryCopyFromBuffer(memory, 0, (PVOID)G_DefaultReportDescriptor, bytesToCopy);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfMemoryCopyFromBuffer failed 0x%x", status);
		return status;
	}

	WdfRequestSetInformation(Request, bytesToCopy);

	TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_DRIVER, "HidFx2GetReportDescriptor Exit = 0x%x\n", status);
	return status;
}


VOID
JDriverEvtIoStop(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ ULONG ActionFlags
)
{
	TraceEvents(TRACE_LEVEL_INFORMATION,
		TRACE_QUEUE,
		"%!FUNC! Queue 0x%p, Request 0x%p ActionFlags %d",
		Queue, Request, ActionFlags);

	return;
}


VOID
InterruptPipeReadComplete(
	WDFUSBPIPE  Pipe,
	WDFMEMORY   Buffer,
	size_t      NumBytesTransferred,
	WDFCONTEXT  Context
)
{

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! entry");

	UNREFERENCED_PARAMETER(Pipe);
	UNREFERENCED_PARAMETER(Context);

	PUCHAR buf = WdfMemoryGetBuffer(Buffer, NULL);


	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "data: %x %x %x %x %x %x %x", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], (unsigned)NumBytesTransferred);

	WDFREQUEST              request;
	NTSTATUS                status;
	PDEVICE_CONTEXT context = Context;

	status = WdfIoQueueRetrieveNextRequest(
		context->interruptMsgQueue,
		&request);


	if (NT_SUCCESS(status)) {

		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "complete");

		PUCHAR reqbuf;
		size_t bytesReturned;

		status = WdfRequestRetrieveOutputBuffer(request,
			1,
			&reqbuf,
			&bytesReturned);// BufferLength

		*reqbuf = buf[4];

		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "returned : %x",(unsigned)bytesReturned);

		WdfRequestCompleteWithInformation(request, status, bytesReturned);
	}

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! exit");

}



BOOLEAN
EvtReadFailed(
	WDFUSBPIPE      Pipe,
	NTSTATUS        Status,
	USBD_STATUS     UsbdStatus
)
{
	UNREFERENCED_PARAMETER(Pipe);

	TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
		"%!FUNC! ReadersFailedCallback failed NTSTATUS 0x%x, UsbdStatus 0x%x\n",
		Status,
		UsbdStatus);


	return TRUE;
}

