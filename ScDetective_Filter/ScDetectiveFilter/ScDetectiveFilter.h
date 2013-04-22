///////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2010 - <company name here>
///
/// Original filename: ScDetectiveFilter.h
/// Project          : ScDetectiveFilter
/// Date of creation : <see ScDetectiveFilter.cpp>
/// Author(s)        : <see ScDetectiveFilter.cpp>
///
/// Purpose          : <see ScDetectiveFilter.cpp>
///
/// Revisions:         <see ScDetectiveFilter.cpp>
///
///////////////////////////////////////////////////////////////////////////////

// $Id$

#ifndef __SCDETECTIVEFILTER_H_VERSION__
#define __SCDETECTIVEFILTER_H_VERSION__ 100

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif


#include "drvcommon.h"
#include "drvversion.h"
#include "fastIO.h"
#include "myfs.h"

// 一些常用的结构体
#include "d:\DriverStudy\struct.h"
// 用户层，内核公用模块
#include "../../Common/IoControlCmd.h"

#define MEM_TAG             'ScTt'

#define DEVICE_NAME			"\\FileSystem\\ScFilter"
#define SYMLINK_NAME		"\\DosDevices\\ScFilter"
PRESET_UNICODE_STRING(usDeviceName, DEVICE_NAME);
PRESET_UNICODE_STRING(usSymlinkName, SYMLINK_NAME);

#ifndef FILE_DEVICE_SCDETECTIVEFILTER
#define FILE_DEVICE_SCDETECTIVEFILTER 0x8000
#endif

//////////////////////////////////////////////////////////////////////////

typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT AttachedDevice;
    PDEVICE_OBJECT PhysicDevice;
    UNICODE_STRING DeviceName;
    WCHAR NameBuffer[256];
} DEVICE_EXTENSION, * PDEVICE_EXTENSION ;

typedef struct _HIDE_OBJECT {
    LIST_ENTRY  ObjectLink;
    WCHAR       ObjectName[256];
    ULONG       HideFlag;
} HIDE_OBJECT, * PHIDE_OBJECT;


//////////////////////////////////////////////////////////////////////////

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

const ULONG IOCTL_SCDETECTIVEFILTER_OPERATION = CTL_CODE(FILE_DEVICE_SCDETECTIVEFILTER, 0x01, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA);

#endif // __SCDETECTIVEFILTER_H_VERSION__
