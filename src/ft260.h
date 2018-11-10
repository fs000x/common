#pragma once

#include "LibFT260.h"

bool IsFT260Dev(WCHAR* devPath);
const char* FT260StatusToString(FT260_STATUS i);
bool IsFt260DevConnected(void);
void CloseFt260Handle(FT260_HANDLE handle);
FT260_HANDLE OpenFt260Uart(WCHAR *path);
bool ConfigFt260(FT260_HANDLE handle, DWORD baudRate, BYTE parity, BYTE stopBit, BYTE dataBit);
DWORD GetFt260QueueBytesToRead(FT260_HANDLE handle);
BOOL FT260ReadUart(FT260_HANDLE handle, LPVOID lpBuffer, DWORD dwBufferLength, DWORD dwBytesToRead, LPDWORD lpdwBytesReturned);
BOOL FT260WriteUart(FT260_HANDLE handle, LPVOID lpBuffer, DWORD dwBufferLength, DWORD dwBytesToWrite, LPDWORD lpdwBytesWritten);
