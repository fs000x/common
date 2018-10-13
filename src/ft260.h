#pragma once

#include "LibFT260.h"

bool IsFt260DevConnected(void);
FT260_HANDLE OpenFt260Uart(void);
void CloseFt260Handle(FT260_HANDLE handle);
