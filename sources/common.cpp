#include "stdafx.h"

#include "msg.h"
#include "utils.h"
#include "about.h"
#include "comm.h"
#include "deal.h"
#include "debug.h"
#include "struct/memory.h"
#include "load_driver/load_driver.h"
#pragma warning(disable:4100) //unreferenced formal parameter(s)

static char* __THIS_FILE__ = __FILE__;
comconfig* g_cfg;

void com_load_config(void)
{
	char mp[MAX_PATH]={0};
	GetModuleFileName(NULL, mp, __ARRAY_SIZE(mp));
	strcpy(strrchr(mp, '\\')+1, "common.ini");
	g_cfg = config_create(mp, 1);
}

void com_unload_config(void)
{
	config_close(g_cfg);
}

#if 1
int CALLBACK WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nShowCmd)
{
	MSG message;
	init_msg();
	init_utils();
	init_about();
	init_comm();
	init_deal();
	init_memory();

	//InitCommonControls();
	LoadLibrary("RichEd20.dll");

#ifdef _DEBUG
	AllocConsole();
#endif
	debug_out(("����������\n"));

	com_load_config();

	msg.run_app();

	while(GetMessage(&message,NULL,0,0)){
		if(!TranslateAccelerator(msg.hWndMain,msg.hAccel,&message)){
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}

	com_unload_config();

	debug_out(("�����ѽ���\n"));
#ifdef _DEBUG
	Sleep(1000);
	FreeConsole();
#endif
	MessageBeep(MB_OK);
	return 0;
}

#endif
