
#include "stdafx.h"

BOOL GetFileCorporation(PWCHAR pszFileName, PWCHAR pszFileCorporation)
{
    ULONG uLen;
    PVOID buf;
    PLANGUAGE_CODE_PAGE lpTranslate;
    UINT uTranslate;
    WCHAR szSubBlock[260];
    PVOID szInfo;
    UINT uLen1;

    if (pszFileName == NULL)   return FALSE;

    uLen = GetFileVersionInfoSize(pszFileName, 0);   
    if (uLen <= 0)  return FALSE;

    buf = GlobalAlloc(GPTR, uLen);  
    if (!GetFileVersionInfo(pszFileName, NULL, uLen, buf))   
    {   
        GlobalFree(buf);
        return FALSE;  
    }  
    VerQueryValue(buf, L"\\VarFileInfo\\Translation", 
                  (LPVOID*)&lpTranslate, &uTranslate); 
    wsprintf(szSubBlock, L"\\StringFileInfo\\%04x%04x\\CompanyName", 
                        lpTranslate[0].wLanguage, lpTranslate[0].wCodePage); 
    if(VerQueryValue(buf,szSubBlock, &szInfo, &uLen1) == FALSE)   
    {   
        GlobalFree(buf);
        return FALSE;
    }  
    wcscpy(pszFileCorporation, (PWCHAR)szInfo);
    GlobalFree(buf);
    return TRUE;
}

VOID BrowseFolder(LPCTSTR szImagePath)
{
    TCHAR   szCmdLine[MAX_PATH+24] = {0};
    TCHAR   szDir[MAX_PATH] = {0};

    if (szImagePath != NULL)
    {
        GetWindowsDirectory(szDir, _countof(szDir) );
        wsprintf(szCmdLine, TEXT("%s\\Explorer /select,%s"), szDir, szImagePath);

        PROCESS_INFORMATION pi = {0};

        STARTUPINFO StartUp = {0};
        StartUp.cb          = sizeof(StartUp);
        StartUp.dwFlags     = STARTF_USESHOWWINDOW;
        StartUp.wShowWindow = SW_SHOW;

        CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, 
                      NORMAL_PRIORITY_CLASS, NULL, NULL, &StartUp, &pi);
    }
}

///////////////////////////////////////////////////////////////////////////////////
//
//	功能实现： 把获取的文件路径转换为统一的Win32路径
//	输入参数： pszFileName为要原始路径名；
//			  pszWin32Name为转换好的路径名；
//	返回参数： 无
//
///////////////////////////////////////////////////////////////////////////////////
VOID ModifyFileImagePath(PSTR pszFilePath, PSTR pszWin32Name, ULONG cbszWin32Name)
{
    char* szInfo;

    if (pszFilePath == NULL || pszFilePath[0] == '\0')
    {
        pszWin32Name[0] = '\0';
        return ;
    }

    if (_strnicmp(pszFilePath, "\\windows\\", strlen("\\windows\\")) == 0)
    {
        UINT nLen = GetWindowsDirectoryA(pszWin32Name, MAX_PATH);
        if (nLen != 0)
        {
            strcat_s(pszWin32Name, cbszWin32Name, "\\");
            strcat_s(pszWin32Name, cbszWin32Name, &pszFilePath[strlen("\\windows\\")]);
            return ;
        }
    }

    if (_strnicmp(pszFilePath, "\\Program Files\\", strlen("\\Program Files\\")) == 0)
    {
        UINT nLen = GetWindowsDirectoryA(pszWin32Name, MAX_PATH);
        if (nLen != 0)
        {
            szInfo = strstr(pszWin32Name, "\\");
            if(szInfo != NULL)
            {
                strcpy_s(szInfo, cbszWin32Name - (ULONG)(pszWin32Name - szInfo), pszFilePath); 
                return;	
            } 
        }
    }

    szInfo = strstr(pszFilePath, "\\SystemRoot\\");
    if (szInfo == pszFilePath)
    {
        //       \SystemRoot\System32\smss.exe
        // -->   c:\winnt\System32\smss.exe  using GetWindowsDirectory()
        UINT Len = ::GetWindowsDirectoryA(pszWin32Name, MAX_PATH);
        if (Len != 0)
        {
            strcat_s(pszWin32Name, cbszWin32Name, "\\");
            strcat_s(pszWin32Name, cbszWin32Name, &pszFilePath[lstrlenA("\\SystemRoot\\")]); 
            return;
        }
    }
    else
    {
        //       \??\C:\WINNT\system32\winlogon.exe
        // -->   C:\WINNT\system32\winlogon.exe  
        szInfo = strstr(pszFilePath, "\\??\\");
        if (szInfo == pszFilePath)
            strcpy_s(pszWin32Name, cbszWin32Name, &pszFilePath[strlen("\\??\\")]);
        else
            strcpy_s(pszWin32Name, cbszWin32Name, pszFilePath);
        return;
    }

    strcpy_s(pszWin32Name, cbszWin32Name, pszFilePath);
}

//////////////////////////////////////////////////////////////////////////

VOID GetFileNameByImagePath(LPTSTR pszImagePath, LPTSTR pszFileName)
{
    if (pszImagePath == NULL || pszFileName == NULL)   return;

    PWCHAR pos = pszImagePath;
    PWCHAR lastpos = pos;
    UINT Length = wcslen(pszImagePath);
    // wcsrchr instead
    for (UINT i = 0; i < wcslen(pszImagePath); i++)
    {
        pos = StrStr(pos, L"\\");
        if (pos != NULL) {
            pos ++;
            lastpos = pos;
        } else {
            wcscpy(pszFileName, lastpos);
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////