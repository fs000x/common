#include "StdAfx.h"

//------------------------------------------------------------------------------
#include "debug.h"
#include "comm.h"

//------------------------------------------------------------------------------
// include FTDI libraries
//
#include "LibFT260.h"


#define MASK_1 0x0f

WORD FT260_Vid = 0x0403;
WORD FT260_Pid = 0x6030;


std::string sFT260Status[18] =
{
	"FT260_OK",
	"FT260_INVALID_HANDLE",
	"FT260_DEVICE_NOT_FOUND",
	"FT260_DEVICE_NOT_OPENED",
	"FT260_DEVICE_OPEN_FAIL",
	"FT260_DEVICE_CLOSE_FAIL",
	"FT260_INCORRECT_INTERFACE",
	"FT260_INCORRECT_CHIP_MODE",
	"FT260_DEVICE_MANAGER_ERROR",
	"FT260_IO_ERROR",
	"FT260_INVALID_PARAMETER",
	"FT260_NULL_BUFFER_POINTER",
	"FT260_BUFFER_SIZE_ERROR",
	"FT260_UART_SET_FAIL",
	"FT260_RX_NO_DATA",
	"FT260_GPIO_WRONG_DIRECTION",
	"FT260_INVALID_DEVICE",
	"FT260_OTHER_ERROR"
};


bool IsFT260Dev(WCHAR* devPath)
{
	WCHAR findStr[100];
	swprintf_s(findStr, _countof(findStr), L"vid_%04x&pid_%04x", FT260_Vid, FT260_Pid);

	wdebug_out((L"Device path:%s\nfindStr:%s\n", devPath, findStr));
	if (NULL == wcsstr(devPath, findStr))
	{
		return false;
	}
	else
	{
		return true;
	}
}


const char* FT260StatusToString(FT260_STATUS i)
{
	switch (i)
	{
	case  0:
		return sFT260Status[0].c_str();
	case  1:
		return sFT260Status[1].c_str();
	case  2:
		return sFT260Status[2].c_str();
	case  3:
		return sFT260Status[3].c_str();
	case  4:
		return sFT260Status[4].c_str();
	case  5:
		return sFT260Status[5].c_str();
	case  6:
		return sFT260Status[6].c_str();
	case  7:
		return sFT260Status[7].c_str();
	case  8:
		return sFT260Status[8].c_str();
	case  9:
		return sFT260Status[9].c_str();
	case 10:
		return sFT260Status[10].c_str();
	case 11:
		return sFT260Status[11].c_str();
	case 12:
		return sFT260Status[12].c_str();
	case 13:
		return sFT260Status[13].c_str();
	case 14:
		return sFT260Status[14].c_str();
	case 15:
		return sFT260Status[15].c_str();
	case 16:
		return sFT260Status[16].c_str();
	case 17:
		return sFT260Status[17].c_str();
	default:
		return "Not a valid FT260 status";
	}
}

FT260_Data_Bit toFt260DataBit(BYTE dataBit)
{
	FT260_Data_Bit ret = FT260_DATA_BIT_8;

	switch (dataBit)
	{
		case 7:
			ret = FT260_DATA_BIT_7;
			break;

		case 8:
			ret = FT260_DATA_BIT_8;
			break;
	}

	return ret;
}
FT260_Stop_Bit toFt260StopBit(BYTE stopBit)
{
	FT260_Stop_Bit ret = FT260_STOP_BITS_1;

	switch (stopBit)
	{
		case 1:
			ret = FT260_STOP_BITS_1;
			break;

		case 2:
			ret = FT260_STOP_BITS_2;
			break;
	}

	return ret;
}
FT260_Parity toFt260ParityBit(BYTE parity)
{
	FT260_Parity ret = FT260_PARITY_NONE;

	switch (parity)
	{
		case NOPARITY:
			ret = FT260_PARITY_NONE;
			break;

		case ODDPARITY:
			ret = FT260_PARITY_ODD;
			break;

		case EVENPARITY:
			ret = FT260_PARITY_EVEN;
			break;

		case MARKPARITY:
			ret = FT260_PARITY_MARK;
			break;

		case SPACEPARITY:
			ret = FT260_PARITY_SPACE;
			break;
	}

	return ret;
}

bool IsFt260DevConnected(void)
{
	FT260_STATUS ftStatus = FT260_OTHER_ERROR;
	DWORD devNum = 0;
	WCHAR pathBuf[128];
	bool ret = false;

	FT260_CreateDeviceList(&devNum);
	debug_out(("Number of devices : %d\n\n", devNum));

	for (DWORD i = 0; i < devNum; i++)
	{
		// Get device path and open device by device path
		ftStatus = FT260_GetDevicePath(pathBuf, 128, i);
		if (FT260_OK != ftStatus)
		{
			debug_out(("FT260 Get Device Path NG, status: %s\n", FT260StatusToString(ftStatus)));
		}
		else
		{
			wdebug_out((L"FT260 Device path:%s \n", pathBuf));
		}

		if (false == IsFT260Dev(pathBuf))
		{
			debug_out(("Not FT260 device\n"));
		}
		else
		{
			ret = true;
		}
	}

	return ret;
}

void CloseFt260Handle(FT260_HANDLE handle)
{
	FT260_Close(handle);
	debug_out(("Close FT260 device OK\n"));
}

FT260_HANDLE OpenFt260Uart(WCHAR *path)
{
	FT260_STATUS ftStatus = FT260_OTHER_ERROR;
	FT260_HANDLE handle = INVALID_HANDLE_VALUE;

	// Open device by Vid/Pid
	// mode 0 is I2C, mode 1 is UART
	//ftStatus = FT260_OpenByVidPid(FT260_Vid, FT260_Pid, 1, &handle);
	ftStatus = FT260_OpenByDevicePath(path, &handle);
	if (FT260_OK != ftStatus)
	{
		//debug_out(("FT260 Open device by vid pid NG, status: %s\n", FT260StatusToString(ftStatus)));
		debug_out(("FT260 Open device by Device Path NG, status: %s\n", FT260StatusToString(ftStatus)));
	}
	else
	{
		//debug_out(("Open FT260 device by vid pid OK %p\n", handle));
		debug_out(("Open FT260 device by Device Path OK %p\n", handle));
	}

	// Show version information
	DWORD dwChipVersion = 0;

	ftStatus = FT260_GetChipVersion(handle, &dwChipVersion);
	if (FT260_OK != ftStatus)
	{
		debug_out(("FT260 Get chip version Failed, status: %s\n", FT260StatusToString(ftStatus)));
	}
	else
	{
		debug_out(("FT260 Chip version : %d.%d.%d.%d\n",
			((dwChipVersion >> 24) & MASK_1),
			((dwChipVersion >> 16) & MASK_1),
			((dwChipVersion >> 8) & MASK_1),
			(dwChipVersion & MASK_1)));
	}

	ftStatus = FT260_UART_Init(handle);
	if (FT260_OK != ftStatus)
	{
		debug_out(("FT260 UART Init NG, status :%s\n", FT260StatusToString(ftStatus)));
		CloseFt260Handle(handle);
		return INVALID_HANDLE_VALUE;
	}

	//config TX_ACTIVE for UART 485
	ftStatus = FT260_SelectGpioAFunction(handle, FT260_GPIOA_TX_ACTIVE);
	if (FT260_OK != ftStatus)
	{
		debug_out(("FT260 UART TX_ACTIVE NG, status : %s\n", FT260StatusToString(ftStatus)));
		CloseFt260Handle(handle);
		return INVALID_HANDLE_VALUE;
	}

	return handle;
}

bool ConfigFt260(FT260_HANDLE handle, DWORD baudRate, BYTE parity, BYTE stopBit, BYTE dataBit)
{
	FT260_STATUS ftStatus = FT260_OTHER_ERROR;
	UartConfig uartConfig;

	//config UART
	FT260_UART_SetFlowControl(handle, FT260_UART_NO_FLOW_CTRL_MODE);
	ULONG ulBaudrate = baudRate;
	FT260_UART_SetBaudRate(handle, ulBaudrate);
	FT260_UART_SetDataCharacteristics(handle, FT260_DATA_BIT_8, FT260_STOP_BITS_1, FT260_PARITY_NONE);
	FT260_UART_SetBreakOff(handle);

	ftStatus = FT260_UART_GetConfig(handle, &uartConfig);
	if (FT260_OK != ftStatus)
	{
		debug_out(("FT260 UART Get config NG : %s\n", FT260StatusToString(ftStatus)));
	}
	else
	{
		debug_out(("FT260 config baud:%ld, ctrl:%d, data_bit:%d, stop_bit:%d, parity:%d, breaking:%d\n",
			uartConfig.baud_rate, uartConfig.flow_ctrl, uartConfig.data_bit, uartConfig.stop_bit, uartConfig.parity, uartConfig.breaking));
	}

	return true;
}

DWORD GetFt260QueueBytesToRead(FT260_HANDLE handle)
{
	DWORD dwAvailableData = 0;

	FT260_UART_GetQueueStatus(handle, &dwAvailableData);

	return dwAvailableData;
}

BOOL FT260ReadUart(FT260_HANDLE handle, LPVOID lpBuffer, DWORD dwBufferLength, DWORD dwBytesToRead, LPDWORD lpdwBytesReturned)
{
	FT260_STATUS ftStatus = FT260_OTHER_ERROR;
	BOOL ret = FALSE;

	ftStatus = FT260_UART_Read(handle, lpBuffer, dwBufferLength, dwBytesToRead, lpdwBytesReturned);
	if (FT260_OK != ftStatus)
	{
		debug_out(("FT260 UART Read NG : %d\n", ftStatus));
	}
	else
	{
		debug_out(("FT260 Read bytes : %d\n", *lpdwBytesReturned));
		ret = TRUE;
	}

	return ret;
}

BOOL FT260WriteUart(FT260_HANDLE handle, LPVOID lpBuffer, DWORD dwBufferLength, DWORD dwBytesToWrite, LPDWORD lpdwBytesWritten)
{
	FT260_STATUS ftStatus = FT260_OTHER_ERROR;
	BOOL ret = FALSE;

	// Write data
	ftStatus = FT260_UART_Write(handle, lpBuffer, dwBufferLength, dwBytesToWrite, lpdwBytesWritten);
	if (FT260_OK != ftStatus)
	{
		debug_out(("FT260 UART Write NG : %d\n", ftStatus));
	}
	else
	{
		debug_out(("FT260 Write bytes : %d\n", *lpdwBytesWritten));
		ret = TRUE;
	}

	return ret;
}
