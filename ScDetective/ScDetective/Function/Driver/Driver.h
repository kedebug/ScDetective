///////////////////////////////////////////////////
// Driver.h文件

#pragma once

#include <WinSvc.h>

class CDriver 
{
public:
    VOID LoadDriver(LPCTSTR pszDriverName, LPCTSTR pszLinkName);
    VOID UnloadDriver();

    // 构造函数，pszDriverPath为驱动所在目录，pszLinkName为符号连接名字
    // 在类的构造函数中，将试图创建或打开服务，
    BOOL DoCDriver(LPCTSTR pszDriverPath, LPCTSTR pszLinkName);
    // 析构函数。在这里，将停止服务，
    VOID UnDoCDriver();

    // 属性
    // 此驱动是否可用
    BOOL IsValid() { return (m_hSCM != NULL && m_hService != NULL); }

    // 操作
    // 开启服务。也就是说驱动的DriverEntry函数将被调用
    BOOL StartDriver();
    // 结束服务。即驱动程序的DriverUnload例程将被调用
    BOOL StopDriver();

    // 打开设备，即取得到此驱动的一个句柄
    BOOL OpenDevice();

    // 向设备发送控制代码
    DWORD IoControl(DWORD nCode, PVOID pInBuffer, 
                    DWORD nInCount, PVOID pOutBuffer, DWORD nOutCount);
    // 实现
    WCHAR m_szLinkName[56];	// 符号连接名称

    BOOL  m_bStarted;	        // 指定服务是否启动
    BOOL  m_bCreateService;	// 指定是否创建了服务

    SC_HANDLE  m_hSCM;		// SCM数据库句柄
    SC_HANDLE  m_hService;	// 服务句柄

    HANDLE m_hDriver;	    // 设备句柄
};

extern CDriver theDriver;