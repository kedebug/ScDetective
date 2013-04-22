
#ifndef _IOCONTROLCMD_H
#define _IOCONTROLCMD_H

//////////////////////////////////////////////////////////////////////////

#define CDO_FLAG_FILE			1L
#define CDO_FLAG_DIRECTORY		2L

//
// Values defined for "Method"
// METHOD_BUFFERED
// METHOD_IN_DIRECT
// METHOD_OUT_DIRECT
// METHOD_NEITHER
// 
// Values defined for "Access"
// FILE_ANY_ACCESS
// FILE_READ_ACCESS
// FILE_WRITE_ACCESS

#define FILE_DEVICE_SEDECTIVE   0x8008
#define IOCTL_BASE              0x800
#define TEMPLATE_CTL_CODE(i)    (ULONG) \
    CTL_CODE(FILE_DEVICE_SEDECTIVE, IOCTL_BASE + i, METHOD_BUFFERED, FILE_ANY_ACCESS)


// 获取 ssdt 结构
#define IOCTL_GET_SSDT                  TEMPLATE_CTL_CODE(0x01)
// 恢复 ssdt
#define IOCTL_UNHOOK_SSDT               TEMPLATE_CTL_CODE(0x02)
// 获取 Shadow ssdt 结构
#define IOCTL_GET_SSDTSHADOW            TEMPLATE_CTL_CODE(0x03)
// 恢复 Shadow ssdt
#define IOCTL_UNHOOK_SSDTSHADOW         TEMPLATE_CTL_CODE(0x04)
// 搜索 PspCidTable 枚举进程 && 暴力搜索内存获取进程 
#define IOCTL_GET_PROCESSES             TEMPLATE_CTL_CODE(0x06)
// 根据EPROCESS获取进程dos路径
#define IOCTL_GET_PROCESS_IMAGE_PATH    TEMPLATE_CTL_CODE(0x07)
// 查看进程加载模块信息
#define IOCTL_GET_PROCESS_MODULES       TEMPLATE_CTL_CODE(0x08)
// 查看进程线程信息, 需要两次连续调用
#define IOCTL_GET_PROCESS_THREADS       TEMPLATE_CTL_CODE(0x09)
// 获取进程句柄
#define IOCTL_GET_PROCESS_HANDLES       TEMPLATE_CTL_CODE(0x0A)
// 获取内核模块信息
#define IOCTL_GET_DRIVER_OBJECT         TEMPLATE_CTL_CODE(0x11)
// 读取内核地址数据
#define IOCTL_DUMP_KERNEL_MEMORY        TEMPLATE_CTL_CODE(0x12)
// 自我保护模块
#define IOCTL_PROTECT_MYSELF            TEMPLATE_CTL_CODE(0x13)
// 退出时清理模块
#define IOCTL_EXIT_PROCESS              TEMPLATE_CTL_CODE(0x14)
// 获取指定目录下文件信息包括子目录
#define IOCTL_LIST_DIRECTORY            TEMPLATE_CTL_CODE(0x15)

//////////////////////////////////////////////////////////////////////////

#endif

