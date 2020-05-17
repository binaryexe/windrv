#include <ntddk.h>

UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\device\\SasDemoDev");

PDEVICE_OBJECT DeviceObject = NULL;

UNICODE_STRING SymLinkName = RTL_CONSTANT_STRING(L"\\??\\SasDemoDev");

VOID Unload(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	IoDeleteSymbolicLink(&SymLinkName);
	IoDeleteDevice(DeviceObject);
	KdPrint(("Driver unloaded \r\n"));
}

#define DEVICE_SEND CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_DATA)
#define DEVICE_RECV CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_DATA)

NTSTATUS ControlOperation(PDEVICE_OBJECT DeviceObject1, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject1);
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status = STATUS_SUCCESS;
	ULONG returnLength = 0;
	PVOID buffer = Irp->AssociatedIrp.SystemBuffer;
	ULONG inLength = irpsp->Parameters.DeviceIoControl.InputBufferLength;
	ULONG outLength = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
	WCHAR* demo = L"Sample Data returned from driver";
	
	switch (irpsp->Parameters.DeviceIoControl.IoControlCode)
	{
	case DEVICE_SEND:
		KdPrint(("output buffer length [%d]\r\n", outLength));
		KdPrint(("send data is %ws \r\n", buffer));
		returnLength = (wcsnlen(buffer, 511) + 1) * 2;
		break;
	case DEVICE_RECV:
		KdPrint(("input buffer length [%d]\r\n", inLength));
		wcsncpy(buffer, demo, 511);
		returnLength = (wcsnlen(buffer, 511) + 1) * 2;
		break;
	default:
		status = STATUS_INVALID_PARAMETER;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = returnLength;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
	
}

NTSTATUS DispatchPassThru(PDEVICE_OBJECT DeviceObject1, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject1);
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status = STATUS_SUCCESS;
	//KdPrint(("SasDemoDrv: DispatchPassThru \r\n"));
	switch (irpsp->MajorFunction)
	{
	case IRP_MJ_CREATE:
		KdPrint(("Create request \r\n"));
		break;
	case IRP_MJ_CLOSE:
		KdPrint(("Close request \r\n"));
		break;

	default:
		status = STATUS_INVALID_PARAMETER;
		break;
	}

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS status;
	int i;
	UNREFERENCED_PARAMETER(RegistryPath);
	DriverObject->DriverUnload = Unload;

	status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, 
		FILE_DEVICE_SECURE_OPEN, FALSE, &DeviceObject);

	if (!NT_SUCCESS( status))
	{
		KdPrint(("Creating device failed!\r\n"));
		return status;
	}

	status = IoCreateSymbolicLink(&SymLinkName, &DeviceName);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Creating symbolic link failed!\r\n"));
		IoDeleteDevice(DeviceObject);
		return status;
	}

	for (i = 0;i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		DriverObject->MajorFunction[i] = DispatchPassThru;
	}
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ControlOperation;
	//DriverObject->MajorFunction[IRP_MJ_WRITE] = DispatchWrite;
	//DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;

	KdPrint(("Driver successfully loaded \r\n"));

	return status;
}