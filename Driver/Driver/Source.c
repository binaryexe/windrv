#include <ntddk.h>

UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\device\\mydevice123");

PDEVICE_OBJECT DeviceObject = NULL;

UNICODE_STRING SymLinkName = RTL_CONSTANT_STRING(L"\\??\\mydevice123");

VOID Unload(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	IoDeleteSymbolicLink(&SymLinkName);
	IoDeleteDevice(DeviceObject);
	KdPrint(("Driver unloaded \r\n"));
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS status;
	int i;
	UNREFERENCED_PARAMETER(DriverObject);
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
		KdPrint(("Creating sysmbolic failed!\r\n"));
		IoDeleteDevice(DeviceObject);
		return status;
	}

	for (i = 0;i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		DriverObject->MajorFunction[i] = DispatchPassThru;
	}

	KdPrint(("Driver successfully loaded \r\n"));

	return status;
}