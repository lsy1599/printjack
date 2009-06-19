//-----------------------------------------------------------------------------
// Copyright 2009 Vadim Macagon
// 
// Licensed under the Apache License, Version 2.0 (the "License"); 
// you may not use this file except in compliance with the License. 
// You may obtain a copy of the License at 
// 
// http://www.apache.org/licenses/LICENSE-2.0 
// 
// Unless required by applicable law or agreed to in writing, software 
// distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and 
// limitations under the License.
//-----------------------------------------------------------------------------
#include "precompiled.h"
#if DBG == 1
#include "trace.h"
#include "main.tmh"
#endif
#include "portmonitor.h"
#include "port.h"
#include "xcvport.h"

HINSTANCE hDllInstance;
const wchar_t* moduleName = L"PrintJackServer";

//-----------------------------------------------------------------------------
/**
	@brief DLL entry point.
*/
BOOL
WINAPI 
DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpRes)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			hDllInstance = hModule;
			DisableThreadLibraryCalls(hModule);
			WPP_INIT_TRACING(moduleName);
			return TRUE;

		case DLL_PROCESS_DETACH:
			WPP_CLEANUP();
			return TRUE;
	}

	UNREFERENCED_PARAMETER(lpRes);

	return TRUE;
}

//-----------------------------------------------------------------------------
/**
	@brief Enumerate the ports that the port monitor supports.
*/
BOOL
WINAPI
EnumPorts(HANDLE hMonitor, LPWSTR pName, DWORD dwLevel, LPBYTE pPorts,
          DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
	DoTraceMessage(MSG_INFO, "(dwLevel %lu)", dwLevel);
	printjack::PortMonitor* monitor = (printjack::PortMonitor*)hMonitor;
	if (monitor->EnumPorts(pName, dwLevel, pPorts, cbBuf, pcbNeeded, pcReturned))
		return TRUE;
	else
		return FALSE;
}

//-----------------------------------------------------------------------------
/**
	@brief Obtain a handle for a particular port.
*/
BOOL
WINAPI
OpenPort(HANDLE hMonitor, LPWSTR pName, PHANDLE pHandle)
{
	printjack::PortMonitor* monitor = (printjack::PortMonitor*)hMonitor;
	DoTraceMessage(MSG_INFO, "Opening port (%S)", pName);
	if (monitor->OpenPort(pName, pHandle))
		return TRUE;
	else
		return FALSE;
}

//-----------------------------------------------------------------------------
/**
	
*/
BOOL
WINAPI
StartDocPort(HANDLE hPort, LPWSTR pPrinterName, DWORD dwJobId, 
             DWORD dwLevel, LPBYTE pDocInfo)
{
	printjack::Port* port = (printjack::Port*)hPort;
	DoTraceMessage(
		MSG_INFO, 
		"Port: %S, Printer: %S, Job: %lu", 
		port->GetName().c_str(), pPrinterName, dwJobId
	);
	return port->StartDoc(pPrinterName, dwJobId, dwLevel, pDocInfo);
}

//-----------------------------------------------------------------------------
/**
	
*/
BOOL
WINAPI
WritePort(HANDLE hPort, LPBYTE pBuffer, DWORD cbBuf, LPDWORD pcbWritten)
{
	printjack::Port* port = (printjack::Port*)hPort;
	DoTraceMessage(MSG_INFO, "%S", port->GetName().c_str());
	return port->Write(pBuffer, cbBuf, pcbWritten);
}

//-----------------------------------------------------------------------------
/**
	
*/
BOOL
WINAPI
ReadPort(HANDLE hPort, LPBYTE pBuffer, DWORD cbBuffer, LPDWORD pcbRead)
{
	printjack::Port* port = (printjack::Port*)hPort;
	DoTraceMessage(MSG_INFO, "%S", port->GetName().c_str());
	return port->Read(pBuffer, cbBuffer, pcbRead);
}

//-----------------------------------------------------------------------------
/**
	
*/
BOOL
WINAPI
EndDocPort(HANDLE hPort)
{
	printjack::Port* port = (printjack::Port*)hPort;
	DoTraceMessage(MSG_INFO, "%S", port->GetName().c_str());
	return port->EndDoc();
}

//-----------------------------------------------------------------------------
/**
	
*/
BOOL
WINAPI
ClosePort(HANDLE hPort)
{
	printjack::Port* port = (printjack::Port*)hPort;
	DoTraceMessage(MSG_INFO, "Closing port (%S)", port->GetName().c_str());
	// nothing to do here... yet
	return TRUE;
}

//-----------------------------------------------------------------------------
/**
	@brief Open a port for configuration operations.
	@param pszObject The name of the port, can be NULL.
	@param dwGrantedAccess The access granted to the user during a print 
	                       monitor UI DLL's call to the spooler's OpenPrinter().
	@param phXcv Location where the handle for the opened port should be stored.
	@return TRUE if the port is opened successfully, FALSE otherwise.
*/
BOOL 
WINAPI 
XcvOpenPort(HANDLE hMonitor, LPCWSTR pszObject, 
            ACCESS_MASK dwGrantedAccess, PHANDLE phXcv)
{
	printjack::PortMonitor* monitor = (printjack::PortMonitor*)hMonitor;
	printjack::XcvPort* port = new printjack::XcvPort(pszObject, dwGrantedAccess, monitor);
	DoTraceMessage(MSG_INFO, "Opening XcvPort (%S) (Handle %p) (Instance %d)", pszObject, port, port->instanceId);
	if (port)
	{
		*phXcv = port;
		return TRUE;
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
/**
	
*/
DWORD 
WINAPI 
XcvDataPort(HANDLE hXcv, LPCWSTR pszDataName, PBYTE pInputData,
            DWORD cbInputData, PBYTE pOutputData, DWORD cbOutputData,
            PDWORD pcbOutputNeeded)
{
	printjack::XcvPort* port = (printjack::XcvPort*)hXcv;
	DoTraceMessage(
		MSG_INFO, 
		"Calling %S on XcvPort (%S) (Handle %p) (Instance %d)", 
		pszDataName, port->GetName().c_str(), hXcv, port->instanceId
	);
	return printjack::XcvPort::CallMethod(
		port, pszDataName, pInputData, cbInputData,
		pOutputData, cbOutputData, pcbOutputNeeded
	);
}

//-----------------------------------------------------------------------------
/**
	@brief Close a port previously opened by XcvOpenPort().
*/
BOOL
WINAPI 
XcvClosePort(HANDLE hXcv)
{
	printjack::XcvPort* port = (printjack::XcvPort*)hXcv;
	DoTraceMessage(
		MSG_INFO, 
		"Closing XcvPort (%S) (Handle %p) (Instance %d)", 
		port->GetName().c_str(), hXcv, port->instanceId
	);
	if (port)
		delete port;
	return TRUE;
}

//-----------------------------------------------------------------------------
/**
	
*/
void
WINAPI
Shutdown(HANDLE hMonitor)
{
	printjack::PortMonitor* monitor = (printjack::PortMonitor*)hMonitor;
	DoTraceMessage(MSG_INFO, "Shutdown");
	delete monitor;
}

MONITOR2 Monitor2 = {
	sizeof(MONITOR2),
	EnumPorts,
	OpenPort,
	NULL,           // OpenPortEx is not supported
	StartDocPort,
	WritePort,
	ReadPort,
	EndDocPort,
	ClosePort,
	NULL,           // AddPort is not supported
	NULL,			// AddPortEx is not supported
	NULL,           // ConfigurePort is not supported
	NULL,           // DeletePort is not supported
	NULL,           // GetPrinterDataFromPort not supported
	NULL,           // SetPortTimeOuts not supported
	XcvOpenPort,
	XcvDataPort,
	XcvClosePort,
	Shutdown
};

//-----------------------------------------------------------------------------
/**
	
*/
BOOL 
OperatingSystemIsWin2k()
{
	OSVERSIONINFOEX osvi;
	DWORDLONG dwlConditionMask = 0;

	// Initialize the OSVERSIONINFOEX structure.
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwMajorVersion = 5;
	osvi.dwMinorVersion = 0;

	// Initialize the condition mask.
	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_EQUAL);

	// Perform the test.
	return VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);
}

//-----------------------------------------------------------------------------
/**
	
*/
extern "C"
LPMONITOR2
WINAPI
InitializePrintMonitor2(PMONITORINIT pMonitorInit, PHANDLE phMonitor)
{
	printjack::PortMonitor* monitor = new printjack::PortMonitor(hDllInstance, pMonitorInit);
	if (!monitor)
		return NULL;
	
	monitor->LoadPorts();

	*phMonitor = static_cast<HANDLE>(monitor);
	
	// this should allow the monitor to run on Windows 2000 even if it was 
	// built for Windows XP
	if (OperatingSystemIsWin2k())
		Monitor2.cbSize = MONITOR2_SIZE_WIN2K;
	
	return &Monitor2;
}
