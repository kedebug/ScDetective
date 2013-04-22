#ifndef	__OS_H
#define __OS_H

#define VER_GET_ERROR		0
#define VER_UNSUPPORT		1
#define VER_WINXP			2
#define VER_WXPSP1			3
#define VER_WXPSP2			4
#define VER_WXPSP3			5
#define	VER_W2K3			6
#define VER_W2K3SP1			7
#define VER_W2K3SP2			8
#define VER_VISTA11			9
#define VER_VISTASP1		10
#define VER_VISTAULT		11
#define VER_WINDOWS7        12

//////////////////////////////////////////////////////////////////////////

WORD GetCurrentOSVersion(VOID);

#endif