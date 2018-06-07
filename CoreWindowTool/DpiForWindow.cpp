
#include <iostream>
#include <sstream>
#include <string> 

#include "CoreWindowTool.h"
#include <Shtypes.h>
#include <ShellScalingAPI.h>

using namespace std;
// typedef ULONG HMONITOR;
typedef DEVICE_SCALE_FACTOR MODERN_SCALE_FACTOR;
typedef DEVICE_SCALE_FACTOR DESKTOP_SCALE_FACTOR;
typedef struct _DPI_OVERRIDE_RANGE
{
	_Field_range_(<= , 0) LONG nMinimum; // The smallest override that has an actual effect on a currently used monitor
	LONG nCurrent; // The currently set override
	_Field_range_(>= , 0) LONG nMaximum; // The largest override that has an actual effect on a currently used monitor
} DPI_OVERRIDE_RANGE, *PDPI_OVERRIDE_RANGE;



typedef DWORD(__stdcall *GetCurrentDpi) (
	HMONITOR monitorHandle,
	PVOID lpThreadParameter
	);

typedef struct _DPI_INFORMATION
{
	MODERN_SCALE_FACTOR DeviceScaleFactor;
	MODERN_SCALE_FACTOR BaselineDeviceScaleFactor; // The value of above without user overrides taken into account
	DESKTOP_SCALE_FACTOR DesktopScaleFactor;
	DESKTOP_SCALE_FACTOR BaselineDesktopScaleFactor; // The value of above without user overrides taken into account
	UINT BucketedScaleFactor;
	UINT BaselineBucketedScaleFactor; // The value of above without user overrides taken into account
	SIZE PhysDimInMM; // Can be 0. MM == millimeters
	SIZE CurrentResolution;
	SIZE PhysicalDpi; // Can be 0.
	SIZE PhysicalDpiOverride; // Can be 0.
	SIZE OptimalZoomPercentage; // Can be 0.
	UINT ViewDistTenthsOfInch; // Can be 0.
	DPI_OVERRIDE_RANGE DeviceOverride; // How many plateaus up/down this monitor supports plus the global current
	DPI_OVERRIDE_RANGE DesktopOverride; // How many plateaus up/down this monitor supports plus the global current

	union
	{
		struct
		{
			// Do NOT move these flags around, as they get SQMed as a whole, and the ordering must match the variable created.
			// If removing a flag, leave a reserved bit for it. Additions can be made up to 16 bits (this is how much the SQM
			// variable can hold).
			UINT InClone : 1; // ( 1) If we're in clone mode, data is not strictly per monitor here
			UINT InSpanMode : 1; // ( 2) If we detected we're in span mode
			UINT InternalMonitor : 1; // ( 3) If this source is connected to an internal monitor
			UINT NoEdid : 1; // ( 4) At least one monitor (for this source) had no EDID
			UINT OEMSpecifiedViewDist : 1; // ( 5) Whether the OEM overrode the viewing distance heuristic or not
			UINT PrimaryMonitor : 1; // ( 6) If this source is connected to the primary monitor on the system
			UINT IncorrectValuesDetected : 1; // ( 7) If we ignored the data in the EDID because we believe them to be wrong (e.g. 16cm x 9cm external monitor)
			UINT RotatedMode : 1; // ( 8) CurrentResolution/PhysDimInMM/PhysicalDpi/OptimalZoomPercentage are always given unrotated. True for 90 or 270 modes, implying x/y values should be swapped
			UINT CustomPathScaling : 1; // ( 9) D3DKMDT_VPPS_CUSTOM was used for D3DKMDT_VIDPN_PRESENT_PATH_SCALING. Values may be incorrect.
			UINT ScaleFactorsFromRemote : 1; // (10) Indicates whether the desktop and device scale factors are from a remote client.
			UINT HeuristicScaleFactors : 1; // (11) Currently done in EDID-less, internal panel BDD situations
			UINT OEMScaleFactorFromMonitor : 1; // (12) OEM preferred scale factors come from monitor registry path
			UINT OEMScaleFactorFromDevice : 1; // (13) OEM preferred scale factors come from device registry path
			UINT Unused : 19;
		} Flags;
		UINT AllFlags;
	} FlagsUnion;
}DPI_INFORMATION, *PDPI_INFORMATION;

GetCurrentDpi GetDpiApi()
{
	HMODULE pGdiModule = LoadLibraryW(L"gdi32.dll");
	if (pGdiModule != nullptr)
	{
		return reinterpret_cast<GetCurrentDpi>(GetProcAddress(pGdiModule, "GetCurrentDpiInfo"));
	}

	return nullptr;
}

void WriteIfError(HRESULT hr)
{
	if (FAILED(hr))
	{
		cout << "error " + hr;
	}
}

void WriteDpiInfo(DPI_INFORMATION *pDpiInfo)
{
	cout << endl;
	cout << "DeviceScaleFactor: " + to_string(pDpiInfo->DeviceScaleFactor) << endl;
	cout << "BaselineDeviceScaleFactor: " + to_string(pDpiInfo->BaselineDeviceScaleFactor) << endl;
	cout << "DesktopScaleFactor: " + to_string(pDpiInfo->DesktopScaleFactor) << endl;
	cout << "BaselineDesktopScaleFactor: " + to_string(pDpiInfo->BaselineDesktopScaleFactor) << endl;
	cout << "BucketedScaleFactor: " + to_string(pDpiInfo->BucketedScaleFactor) << endl;
	cout << "BaselineBucketedScaleFactor: " + to_string(pDpiInfo->BaselineBucketedScaleFactor) << endl;


	cout << "PhysDimInMM.cx: " + to_string(pDpiInfo->PhysDimInMM.cx) << endl;
	cout << "CurrentResolution.cx: " + to_string(pDpiInfo->CurrentResolution.cx) << endl;
	cout << "PhysicalDpi.cx: " + to_string(pDpiInfo->PhysicalDpi.cx) << endl;
	cout << "PhysicalDpiOverride.cx: " + to_string(pDpiInfo->PhysicalDpiOverride.cx) << endl;
	cout << "OptimalZoomPercentage.cx: " + to_string(pDpiInfo->OptimalZoomPercentage.cx) << endl;
	cout << "ViewDistTenthsOfInch: " + to_string(pDpiInfo->ViewDistTenthsOfInch) << endl;
	cout << "DeviceOverride.nCurrent: " + to_string(pDpiInfo->DeviceOverride.nCurrent) << endl;
	cout << "DesktopOverride.nCurrent: " + to_string(pDpiInfo->DesktopOverride.nCurrent) << endl;

}

BOOL CALLBACK MonitorEnumProc(
	_In_ HMONITOR hMonitor,
	_In_ HDC      hdcMonitor,
	_In_ LPRECT   lprcMonitor,
	_In_ LPARAM   dwData)
{
	cout << endl;
	cout << "hMonitor: " << hex << hMonitor << endl;
	cout << "Rect.left: " + to_string(lprcMonitor->left) << endl;
	cout << "Rect.top: " + to_string(lprcMonitor->top) << endl;
	cout << "Rect.right: " + to_string(lprcMonitor->right) << endl;
	cout << "Rect.bottom: " + to_string(lprcMonitor->bottom) << endl;

	return TRUE;
}
void DisplayMonitors()
{
	cout << "Monitors" << endl;
	EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, 0);
}

int DumpDpi(HWND hWnd)
{
	HRESULT hr;
	HMONITOR hMonitor;
	UINT dpiX;
	UINT dpiY;

	cout << "CoreWindow handle: ";
	cout << hex << hWnd << endl;

	// Enumerate Monitors
	DisplayMonitors();

	DPI_INFORMATION dpiInfo;
	GetCurrentDpi pGetCurrentDpi = GetDpiApi();

	// Get Monitor for the Window hWnd
	hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
	WriteIfError(HRESULT_FROM_WIN32(GetLastError()));
	cout << endl;
	cout << "Monitor handle for window: " << hex << hMonitor << endl;

	// Get the DPI info for the window monitor
	cout << endl;
	cout << "GetCurrentDpiInfo with MONITOR_DEFAULTTOPRIMARY";
	hr = pGetCurrentDpi(hMonitor, &dpiInfo);
	WriteDpiInfo(&dpiInfo);

	// Get the DPI for the window monitor
	cout << endl;
	cout << "GetDpiForMonitor" << endl;
	GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

	cout << "dpiX: " + to_string(dpiX) << endl;
	cout << "dpiY: " + to_string(dpiY) << endl;

	/*hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	cout << endl;
	cout << endl;
	cout << "GetCurrentDpiInfo with MONITOR_DEFAULTTONEAREST";
	hr = pGetCurrentDpi(hMonitor, &dpiInfo);
	WriteDpiInfo(&dpiInfo);
	*/
	cout << endl;
	cout << "GetWindowRect " << endl;

	RECT rect;
	if (GetWindowRect(hWnd, &rect))
	{
		cout << "rect.left: " + to_string(rect.left) << endl;
		cout << "rect.top: " + to_string(rect.top) << endl;
		cout << "rect.right: " + to_string(rect.right) << endl;
		cout << "rect.bottom: " + to_string(rect.bottom) << endl;
	}
	else
	{
		WriteIfError(HRESULT_FROM_WIN32(GetLastError()));
	}

	cout << endl;
	cout << "GetClientRect " << endl;
	rect;
	if (GetClientRect(hWnd, &rect))
	{
		cout << "rect.left: " + to_string(rect.left) << endl;
		cout << "rect.top: " + to_string(rect.top) << endl;
		cout << "rect.right: " + to_string(rect.right) << endl;
		cout << "rect.bottom: " + to_string(rect.bottom) << endl;
	}
	else
	{
		WriteIfError(HRESULT_FROM_WIN32(GetLastError()));
	}
	cout << endl;

	cout << "GetDpiForMonitor " << endl;
	GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
	cout << "dpiX: " + to_string(dpiX) << endl;
	cout << "dpiY: " + to_string(dpiY) << endl;
	cout << endl;

	cout << "GetDeviceCaps(LOGPIXELSX)" << endl;
	HDC hdc = GetDC(hWnd);
	int logPixelsX = GetDeviceCaps(hdc, LOGPIXELSX);
	int logPixelsY = GetDeviceCaps(hdc, LOGPIXELSY);
	cout << "LOGPIXELSX: " + to_string(logPixelsX) << endl;
	cout << "LOGPIXELSY: " + to_string(logPixelsY) << endl;
	cout << endl;

	cout << "GetDeviceCaps(HORZRES)" << endl;
	int horzRes = GetDeviceCaps(hdc, HORZRES);
	int verRes = GetDeviceCaps(hdc, VERTRES);
	cout << "HORZRES: " + to_string(horzRes) << endl;
	cout << "VERTRES: " + to_string(verRes) << endl;
	cout << endl;

	cout << "GetSystemMetrics(SM_CXSCREEN)" << endl;
	int cxScreen = GetSystemMetrics(SM_CXSCREEN);
	int cyScreen = GetSystemMetrics(SM_CYSCREEN);
	cout << "SM_CXSCREEN: " + to_string(cxScreen) << endl;
	cout << "SM_CYSCREEN: " + to_string(cyScreen) << endl;
	cout << endl;

	cout << "GetSystemMetrics(SM_CXVIRTUALSCREEN)" << endl;
	int cxVirtualScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int cyVirtualScreen = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	cout << "SM_CXVIRTUALSCREEN: " + to_string(cxVirtualScreen) << endl;
	cout << "SM_CYVIRTUALSCREEN: " + to_string(cyVirtualScreen) << endl;
	cout << endl;

	cout << "Window parent, root, and rootowner" << endl;
	cout << "parent: " << hex << GetParent(hWnd) << endl;
	cout << "root: " << hex << GetAncestor(hWnd, GA_ROOT) << endl;
	cout << "rootowner: " << hex << GetAncestor(hWnd, GA_ROOTOWNER) << endl;

	return 0;
}
