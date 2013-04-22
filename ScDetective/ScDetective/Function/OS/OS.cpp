
#include "stdafx.h"

///////////////////////////////////////////////////////////////////////////////////
//
//	功能实现：判断当前操作系统版本
//	输入参数：无
//	输出参数：输出参数返回函数声明处定义的宏
//
///////////////////////////////////////////////////////////////////////////////////
WORD 
GetCurrentOSVersion()
{
    OSVERSIONINFOEX	osVersion;
    BOOL	bStatus=FALSE;

    //获取操作系统版本
    osVersion.dwOSVersionInfoSize=sizeof(OSVERSIONINFOEX);
    bStatus=GetVersionEx((OSVERSIONINFO*)&osVersion);
    if(!bStatus)
    {
        TRACE0("[GetCurrentOSVersion] 获取操作系统版本失败!");
        return VER_GET_ERROR;
    }

    //确定是NT系统
    if(VER_PLATFORM_WIN32_NT!=osVersion.dwPlatformId)
    {
        TRACE0("[GetCurrentOSVersion] 不支持非NT的Windows版本");
        return VER_UNSUPPORT;
    }

    //判断是哪个操作系统类型
    switch (osVersion.dwMajorVersion)
    {
    case 5:
        {
            switch(osVersion.dwMinorVersion)
            {
            case 1:
                {
                    switch (osVersion.wServicePackMajor)
                    {
                    case 0:
                        {
                            TRACE0("[GetCurrentOSVersion] Windows XP普通版本");
                            return VER_WINXP;
                        }
                    case 1:
                        {
                            //XP sp1
                            TRACE0("[GetCurrentOSVersion] Windows XP SP1");
                            return VER_WXPSP1;
                        }
                    case 2:
                        {
                            //XP sp2
                            TRACE0("[GetCurrentOSVersion] Windows XP SP2");
                            return VER_WXPSP2;
                        }
                    case 3:
                        {
                            //XP sp3
                            TRACE0("[GetCurrentOSVersion] Windows XP SP3");
                            return VER_WXPSP3;
                        }
                    default:
                        {
                            TRACE0("[GetCurrentOSVersion]  Windows 5.1 SPXXXX 不支持");
                            return VER_UNSUPPORT;
                        }
                    }
                }
            case 2:
                {
                    switch (osVersion.wServicePackMajor)
                    {
                    case 0:
                        {
                            //2003普通版本
                            TRACE0("[GetCurrentOSVersion]  Windows 2003 普通版本");
                            return VER_W2K3;
                        }
                    case 1:
                        {
                            //2003sp1
                            TRACE0("[GetCurrentOSVersion]  Windows 2003 SP1");
                            return VER_W2K3SP1;
                        }
                    case 2:
                        {
                            //2003sp2
                            TRACE0("[GetCurrentOSVersion]  Windows 2003 SP2");
                            return VER_W2K3SP2;
                        }
                    default:
                        {
                            TRACE0("[GetCurrentOSVersion]  Windows 2003 5.2 SPXXXX 不支持");
                            break;
                        }
                    }
                }
            }

            break;
        }
    case 6:
        {
            switch (osVersion.dwMinorVersion)
            {
            case 0:
                {
                    switch (osVersion.wServicePackMajor)
                    {
                    case 0:
                        {
                            //VISTA 1.1
                            TRACE0("[GetCurrentOSVersion]  Windows Vista 1.1");
                            return VER_VISTA11;
                        }
                    case 1:
                        {
                            //VISTA SP1
                            TRACE0("[GetCurrentOSVersion]  Windows Vista SP1");
                            return VER_VISTASP1;
                        }
                    default:
                        {
                            TRACE0("[GetCurrentOSVersion]  Windows Vista 6.0 SPXXXX 不支持");
                            return VER_UNSUPPORT;
                        }
                    }
                    break;
                }
            case 1:
                {
                    return VER_WINDOWS7;
                    break;
                }
            default:
                {
                    TRACE0("[GetCurrentOSVersion]  Windows Vista 6.XXXX 不支持");
                    return VER_UNSUPPORT;
                }
            }
            break;
        }
    default:
        {
            return VER_UNSUPPORT;
        }
    }

    return VER_GET_ERROR;
}