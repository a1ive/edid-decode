#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <devguid.h>
#include <stdio.h>

#include "winedid.h"

#pragma comment(lib, "setupapi.lib")

// from ntddvdeo.h
DEFINE_GUID(GUID_DEVINTERFACE_MONITOR, 0xe6f07b5f, 0xee97, 0x4a90, 0xb0, 0x76, 0x33, 0xf5, 0x7b, 0xf4, 0xea, 0xa7);

/**
 * @brief Get the EDID of a monitor using the SetupAPI to read from the registry.
 * This is the standard, user-mode way to get EDID.
 *
 * @param index The 0-based index of the monitor.
 * @param buffer The buffer to store the EDID data.
 * @param size The size of the buffer.
 * @return int Returns the size of the EDID data read, or -1 on error.
 */
int get_monitor_edid(int index, void* buffer, size_t size)
{
	// Get the device information set for all monitor devices.
	HDEVINFO h_dev_info = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_MONITOR, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (h_dev_info == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Error: SetupDiGetClassDevsW failed with error %lu\n", GetLastError());
		return -1;
	}

	SP_DEVINFO_DATA dev_info_data;
	dev_info_data.cbSize = sizeof(dev_info_data);

	// Enumerate through the monitor devices.
	if (!SetupDiEnumDeviceInfo(h_dev_info, index, &dev_info_data))
	{
		if (GetLastError() != ERROR_NO_MORE_ITEMS)
		{
			fprintf(stderr, "Error: SetupDiEnumDeviceInfo failed for index %d with error %lu\n", index, GetLastError());
		}
		// If we're here, it means the index was out of bounds (no such monitor).
		SetupDiDestroyDeviceInfoList(h_dev_info);
		return -1;
	}

	// Open the device's hardware registry key.
	HKEY h_dev_reg_key = SetupDiOpenDevRegKey(h_dev_info, &dev_info_data, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
	if (h_dev_reg_key == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Error: SetupDiOpenDevRegKey failed with error %lu\n", GetLastError());
		SetupDiDestroyDeviceInfoList(h_dev_info);
		return -1;
	}

	// Read the "EDID" value from the registry.
	DWORD dw_type;
	DWORD dw_size = size;
	LONG lResult = RegQueryValueExW(h_dev_reg_key, L"EDID", NULL, &dw_type, buffer, &dw_size);

	RegCloseKey(h_dev_reg_key);
	SetupDiDestroyDeviceInfoList(h_dev_info);

	if (lResult != ERROR_SUCCESS)
	{
		fprintf(stderr, "Error: RegQueryValueExW failed with error %ld\n", lResult);
		return -1;
	}

	if (dw_type != REG_BINARY || dw_size < 128)
	{
		fprintf(stderr, "Error: Invalid EDID data found in registry (type=%lu, size=%lu).\n", dw_type, dw_size);
		return -1;
	}

	return (int)dw_size;
}
