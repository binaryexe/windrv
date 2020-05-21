#include<ntddk.h>

typedef struct {
	PDEVICE_OBJECT LowerKbddevice;
} DEVICE_EXTENTION, * PDEVICE_EXTENTION;

typedef struct _KEYBOARD_INPUT_DATA {
	USHORT UnitId;
	USHORT MakeCode;
	USHORT Flags;
	USHORT Reserved;
	ULONG  ExtraInformation;
} KEYBOARD_INPUT_DATA, * PKEYBOARD_INPUT_DATA;

PDEVICE_OBJECT kbdDevice = NULL;
ULONG pendingkeys = 0;

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	LARGE_INTEGER interval = { 0 };
	PDEVICE_OBJECT DeviceObject = DriverObject->DeviceObject;
	interval.QuadPart = -10 * 1000 * 1000;

	IoDetachDevice(((PDEVICE_EXTENTION)DeviceObject->DeviceExtension)->LowerKbddevice);

	while (pendingkeys)
	{
		KeDelayExecutionThread(KernelMode, FALSE, &interval);
	}
	IoDeleteDevice(kbdDevice);
	KdPrint(("Unload Filter Driver"));
}





NTSTATUS AttachedDevice(PDRIVER_OBJECT DriverObject)
{
	NTSTATUS status;

	UNICODE_STRING TargetDevice = RTL_CONSTANT_STRING(L"\\Device\\KeyboardClass0");
	status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENTION), NULL,
		FILE_DEVICE_KEYBOARD, 0, FALSE, &kbdDevice);

	if (!NT_SUCCESS(status))
	{
		return status;
	}
	kbdDevice->Flags |= DO_BUFFERED_IO;
	kbdDevice->Flags &= ~DO_DEVICE_INITIALIZING;

	RtlZeroMemory(kbdDevice->DeviceExtension, sizeof(DEVICE_EXTENTION));

	status = IoAttachDevice(kbdDevice, &TargetDevice,
		&((PDEVICE_EXTENTION)kbdDevice->DeviceExtension)->LowerKbddevice);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(kbdDevice);
		return status;
	}

	return status;
}
NTSTATUS DispatchPass(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	IoCopyCurrentIrpStackLocationToNext(Irp);
	
	return IoCallDriver(((PDEVICE_EXTENTION)DeviceObject->DeviceExtension)->LowerKbddevice, Irp);;

}

NTSTATUS ReadComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Context);
	PKEYBOARD_INPUT_DATA keys = (PKEYBOARD_INPUT_DATA)Irp->AssociatedIrp.SystemBuffer;
	CHAR *keyFlag[4] = { "KeyDown", "KeyUp", "KeyE0", "KeyE1" };
	int i = 0;
	int structnum = Irp->IoStatus.Information / sizeof(KEYBOARD_INPUT_DATA);
	if (Irp->IoStatus.Status == STATUS_SUCCESS)
	{
		for (i = 0;i < structnum;i++)
		{
			KdPrint(("The scan code is %x (%s)\n", keys[i].MakeCode, keyFlag[keys[i].Flags]));
		}
	}

	if (Irp->PendingReturned)
	{
		IoMarkIrpPending(Irp);
	}
	pendingkeys--;
	return Irp->IoStatus.Status;
}

NTSTATUS DispatchRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	IoCopyCurrentIrpStackLocationToNext(Irp);

	IoSetCompletionRoutine(Irp, ReadComplete, NULL, TRUE, TRUE, TRUE);
	pendingkeys++;

	return IoCallDriver(((PDEVICE_EXTENTION)DeviceObject->DeviceExtension)->LowerKbddevice, Irp);;
}


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	int i;
	NTSTATUS status;
	UNREFERENCED_PARAMETER(RegistryPath);
	DriverObject->DriverUnload = DriverUnload;

	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		DriverObject->MajorFunction[i] = DispatchPass;
	}

	DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;

	KdPrint(("Driver is loaded \r\n"));

	status = AttachedDevice(DriverObject);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("!! attachement failed!!! \r\n"));
	}
	else
		KdPrint(("** attachement successful!!! \r\n"));

	return status;
}