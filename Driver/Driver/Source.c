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
	case IRP_MJ_READ:
		KdPrint(("Read request \r\n"));
		break;
	default:
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

	//DriverObject->MajorFunction[IRP_MJ_WRITE] = DispatchWrite;
	//DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;

	KdPrint(("Driver successfully loaded \r\n"));

	return status;
}