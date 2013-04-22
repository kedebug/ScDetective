
#include "Initialize.h"

typedef NTSTATUS 
(NTAPI * pFnRtlGetVersion)(
    PRTL_OSVERSIONINFOW lpVersionInformation
    );

NTSTATUS GetWindowsVersion()
{
    UNICODE_STRING ustrFuncName = { 0 }; 
    RTL_OSVERSIONINFOEXW osverinfo = { sizeof(osverinfo) }; 
    pFnRtlGetVersion pfnRtlGetVersion = NULL; 
    g_WindowsVersion= WINDOWS_VERSION_NONE;

    //获取 RtlGetVersion 函数的地址
    RtlInitUnicodeString(&ustrFuncName, L"RtlGetVersion"); 
    pfnRtlGetVersion = MmGetSystemRoutineAddress(&ustrFuncName); 

    //如果获取到 RtlGetVersion 函数的地址 则直接调用该函数
    if (pfnRtlGetVersion)
    { 
        DbgPrint("[GetWindowsVersion] Using \"RtlGetVersion\"\n"); 
        if (!NT_SUCCESS(pfnRtlGetVersion((PRTL_OSVERSIONINFOW)&osverinfo))) 
            return STATUS_UNSUCCESSFUL;  
    } 
    //否则 调用 PsGetVersion 函数来获取操作系统版本信息
    else 
    {
        DbgPrint("[GetWindowsVersion] Using \"PsGetVersion\"\n");
        if(!PsGetVersion(&osverinfo.dwMajorVersion, &osverinfo.dwMinorVersion, &osverinfo.dwBuildNumber, NULL)) 
            return STATUS_UNSUCCESSFUL; 
    }

    //打印操作系统版本信息
    DbgPrint("[GetWindowsVersion] OSVersion NT %d.%d:%d sp%d.%d\n", 
        osverinfo.dwMajorVersion, osverinfo.dwMinorVersion, osverinfo.dwBuildNumber, 
        osverinfo.wServicePackMajor, osverinfo.wServicePackMinor); 

    //保存操作系统版本到 全局变量 g_WindowsVersion
    //5.0 = 2k
    if (osverinfo.dwMajorVersion == 5 && osverinfo.dwMinorVersion == 0) 
    {
        g_WindowsVersion= WINDOWS_VERSION_2K; 
    } 
    //5.1 = xp
    else if (osverinfo.dwMajorVersion == 5 && osverinfo.dwMinorVersion == 1) 
    {
        g_WindowsVersion= WINDOWS_VERSION_XP;
    } 
    //5.2 = 2k3
    else if (osverinfo.dwMajorVersion == 5 && osverinfo.dwMinorVersion == 2) 
    {
        if (osverinfo.wServicePackMajor==0) 
        { 
            g_WindowsVersion= WINDOWS_VERSION_2K3;
        } 
        else 
        {
            g_WindowsVersion= WINDOWS_VERSION_2K3_SP1_SP2;
        }
    } 
    //6.0 = vista
    else if (osverinfo.dwMajorVersion == 6 && osverinfo.dwMinorVersion == 0) 
    {
        g_WindowsVersion= WINDOWS_VERSION_VISTA;
    }
    //6.1 = win7
    else if (osverinfo.dwMajorVersion == 6 && osverinfo.dwMinorVersion == 1) 
    {
        g_WindowsVersion= WINDOWS_VERSION_7;
    }

    //保存操作系统的Build Number到全局变量g_WindowsBuildNumber
    g_WindowsBuildNumber=osverinfo.dwBuildNumber;

    return STATUS_SUCCESS;
}

NTSTATUS InitializeOffsetByOsVersion()
{

    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    switch(g_WindowsVersion)
    {
    case WINDOWS_VERSION_XP:
        offset_Process_CreateTime = 0x70;
        offset_Process_ExitTime = 0x78;
        offset_Process_ActiveProcessLinks = 0x88 ;
        offset_Process_UniqueProcessId = 0x84;
        offset_Process_ImageFileName = 0x174;
        offset_Process_ThreadListHead = 0x190;
        offset_Process_ObjectTable = 0xc4;
        offset_Process_Peb = 0x1b0;
        offset_Process_InheritedProcessId = 0x14c;

        offset_Process_SectionObject = 0x138;
        EPROCESS_SIZE = 0x25C;

        offset_Thread_Cid = 0x1ec;
        offset_Thread_StartAddress=0x224;
        offset_Thread_ThreadListEntry=0x22c;
        offset_Thread_Teb = 0x20;
        offset_Thread_ThreadsProcess = 0x220;
        offset_Thread_ApcState = 0x34;
        offset_Thread_ApcStateIndex = 0x165;
        offset_Thread_SavedApcState = 0x14c;
        offset_Thread_State = 0x02d;
        offset_Thread_Priority = 0x033;
        offset_Thread_ContextSwitches=0x04c;
        offset_Thread_Win32StartAddress = 0x228;
        
        offset_LdrData_DLLBase = 0x18;
        offset_LdrData_EntryPoint = 0x1c;
        offset_LdrData_SizeOfImage = 0x20; 
        offset_LdrData_FullDllName = 0x24;
        offset_LdrData_BaseDllName = 0x2c;
  
        offset_kpcr_IdleThread = 0x12c;

        offset_ObjectType_Key = 0xAC;
        offset_ObjectType_TotalNumberOfObjects = 0x50;

        offset_HandleTable_HandleTableList = 0x1c;
        offset_HandleTable_QuotaProcess = 0x04;

        ntStatus=STATUS_SUCCESS;
        break;

    case WINDOWS_VERSION_7:
        offset_Process_CreateTime = 0xA0;
        offset_Process_ExitTime = 0xA8;
        offset_Process_ActiveProcessLinks = 0xB8 ;
        offset_Process_UniqueProcessId = 0xB4;
        offset_Process_ImageFileName = 0x16C;
        offset_Process_ThreadListHead = 0x188;
        offset_Process_ObjectTable = 0xf4;
        offset_Process_Peb = 0x1A8;
        offset_Process_InheritedProcessId = 0x140;

        offset_Process_SectionObject = 0x128;
        EPROCESS_SIZE = 0x2c0;

        offset_Thread_Cid = 0x22c;
        offset_Thread_StartAddress = 0x218;
        offset_Thread_ThreadListEntry = 0x268;
        offset_Thread_Teb = 0x88;
        offset_Thread_ThreadsProcess = 0x150;
        offset_Thread_ApcState = 0x40; 
        offset_Thread_ApcStateIndex = 0x134;
        offset_Thread_SavedApcState = 0x170;
        offset_Thread_State = 0x68;
        offset_Thread_Priority = 0x057;
        offset_Thread_ContextSwitches = 0x064;
        offset_Thread_Win32StartAddress = 0x260;

        offset_kpcr_IdleThread = 0x12c;

        offset_ObjectType_Key = 0x7c;
        offset_ObjectType_TotalNumberOfObjects = 0x18;

        offset_HandleTable_HandleTableList = 0x10;
        offset_HandleTable_QuotaProcess = 0x04;

        offset_LdrData_DLLBase = 0x18;
        offset_LdrData_EntryPoint = 0x1c;
        offset_LdrData_SizeOfImage = 0x20; 
        offset_LdrData_FullDllName = 0x24;
        offset_LdrData_BaseDllName = 0x2c;
        break;
    }
    return ntStatus;
}

//////////////////////////////////////////////////////////////////////////

NTSTATUS InitializeScDetective()
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG Number = 100;
    
    ntStatus = GetWindowsVersion();
    if (!NT_SUCCESS(ntStatus)) {
        return STATUS_UNSUCCESSFUL;
    }

    InitializeOffsetByOsVersion();
    InitializeMemoryValue();

    g_SystemProcess = PsGetCurrentProcess();
    g_SystemProcessId = (ULONG)PsGetCurrentProcessId();

    g_CsrssProcessId = -1;
    g_CsrssProcess = NULL;
    ScPsLookupProcessByName(&g_CsrssProcess, L"csrss.exe");

    g_OwnerProcessId = -1;
    ScPsLookupProcessByName(&g_OwnerProcess, L"ScDetective.exe");
    
    g_IdleProcess = ScPsGetSystemIdleProcess();
    if (MmIsAddressValid(g_IdleProcess)) {
        g_IdleProcessId = *(PULONG)((PUCHAR)g_IdleProcess + offset_Process_UniqueProcessId);
    }
    
    g_PspCidTable = ScPsGetPspCidTable();
    g_PsActiveProcessHead = ScPsGetPsActiveProcessHead();

    //
    // Get undocumented routine id
    //
    ServiceId_NtQueryVirtualMemory  = GetServiceIdByName("NtQueryVirtualMemory");
    ServiceId_NtCreateThread        = GetServiceIdByName("NtCreateThread");
    ServiceId_NtTerminateThread     = GetServiceIdByName("NtTerminateThread");

    InitilizeHook();

    return ntStatus;
}