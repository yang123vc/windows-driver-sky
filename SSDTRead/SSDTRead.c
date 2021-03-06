#include "SSDTRead.h"

int DriverEntry(PDRIVER_OBJECT pDriverObject,PUNICODE_STRING RegistryPath)
{
    KdPrint(("Hello Driver World 驱动加载成功"));
	
	asmReadSSDT();
	cReadSSDT();
	
	getSystemCallAddr();
	
	 //注册派遣函数
	pDriverObject->MajorFunction[IRP_MJ_CREATE]=ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE相关IRP处理函数
	pDriverObject->MajorFunction[IRP_MJ_CLOSE]=ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE相关IRP处理函数
	pDriverObject->MajorFunction[IRP_MJ_READ]=ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE相关IRP处理函数
	pDriverObject->MajorFunction[IRP_MJ_CLOSE]=ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE相关IRP处理函数
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]=ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE相关IRP处理函数
	
	CreateTheDevice(pDriverObject);
    pDriverObject->DriverUnload = Driver_Unload;
    return (1);
}

VOID Driver_Unload(IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT pDevObj;
	UNICODE_STRING symLinkName;
	
	pDevObj = pDriverObject->DeviceObject;
	IoDeleteDevice(pDevObj);
	
	RtlInitUnicodeString(&symLinkName, L"\\??\\firstSymDevice");
	IoDeleteSymbolicLink(&symLinkName);
	
    KdPrint(("Hello Driver World 驱动卸载成功"));
}

NTSTATUS CreateTheDevice(IN PDRIVER_OBJECT pDeviceObject)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	
	UNICODE_STRING devName;
	UNICODE_STRING symLinkName;
	
	RtlInitUnicodeString(&devName, L"\\Device\\first_Device");
	
	//create device
	status = IoCreateDevice(pDeviceObject,\
		0,\
		&devName,\
		FILE_DEVICE_UNKNOWN,\
		0,\
		TRUE,\
		&pDevObj\
	);
	if (!NT_SUCCESS(status))
	{
		if (STATUS_INSUFFICIENT_RESOURCES == status)
		{
			KdPrint(("资源不足"));
		}
		
		if (STATUS_OBJECT_NAME_EXISTS == status)
		{
			KdPrint(("指定对象名存在"));
		}
		
		if (STATUS_OBJECT_NAME_COLLISION == status)
		{
			KdPrint(("//对象名有冲突"));
		}
        
		KdPrint(("设备创建失败"));
		return status;
	}
	
	KdPrint(("创建设备成功"));
	
	pDevObj->Flags |= DO_BUFFERED_IO;
	
	//创建符号链接
	RtlInitUnicodeString(&symLinkName,L"\\??\\firstSymDevice");
	status = IoCreateSymbolicLink( &symLinkName,&devName );
	if (!NT_SUCCESS(status)) /*status等于0*/
	{
		IoDeleteDevice( pDevObj );
		return status;
	}
	return STATUS_SUCCESS;
}

//派遣函数
NTSTATUS ddk_DispatchRoutine_CONTROL(IN PDEVICE_OBJECT pDevobj,IN PIRP pIrp	)
{
	//对相应的IPR进行处理
	pIrp->IoStatus.Information=0;//设置操作的字节数为0，这里无实际意义
	pIrp->IoStatus.Status=STATUS_SUCCESS;//返回成功
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);//指示完成此IRP
	KdPrint(("离开派遣函数\n"));//调试信息
	return STATUS_SUCCESS; //返回成功
}

//读取SSDT
#pragma PAGECODE
ULONG asmReadSSDT()
{

	ULONG SSDT_NtOpenProcess_Cur_Addr;
__asm
{
	push eax
	push ebx
	mov eax,KeServiceDescriptorTable
	mov eax,[eax] //表的基地址
	//[[KeServiceDescriptorTable]+0x7A*4]
	mov ebx, 0x7A
	shl ebx, 2
	add eax, ebx
	mov eax, [eax]
	mov SSDT_NtOpenProcess_Cur_Addr, eax
	pop ebx
	pop eax
}
	KdPrint(("NtOpenProcess Current address is:%x", SSDT_NtOpenProcess_Cur_Addr));
	
	return SSDT_NtOpenProcess_Cur_Addr;
}

ULONG cReadSSDT()
{
	LONG *SSDT_Adr,SSDT_NtOpenProcess_Cur_Addr,t_addr; 
	//读取SSDT表中索引值为0x7A的函数
	//poi(poi(KeServiceDescriptorTable)+0x7a*4)
	t_addr=(LONG)KeServiceDescriptorTable->ServiceTableBase;
	KdPrint(("当前ServiceTableBase地址为%x \n",t_addr));
	SSDT_Adr=(PLONG)(t_addr+0x7A*4);
	KdPrint(("当前t_addr+0x7A*4=%x \n",SSDT_Adr)); 
	SSDT_NtOpenProcess_Cur_Addr=*SSDT_Adr;	
	KdPrint(("当前SSDT_NtOpenProcess_Cur_Addr地址为%x \n",SSDT_NtOpenProcess_Cur_Addr));
	
	return SSDT_NtOpenProcess_Cur_Addr;
}

ULONG getSystemCallAddr()
{
	UNICODE_STRING Old_NtOpenProcess;
    ULONG Old_Addr;
	RtlInitUnicodeString(&Old_NtOpenProcess,L"NtOpenProcess");
	Old_Addr=(ULONG)MmGetSystemRoutineAddress(&Old_NtOpenProcess);//取得NtOpenProcess的地址
	KdPrint(("取得原函数NtOpenProcess的值为 %x",Old_Addr));
	
	return Old_Addr;
}