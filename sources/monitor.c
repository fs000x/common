#define IM_MONITOR
#include "monitor.h"
#include "utils.h"
#include "about.h"
#include "utils.h"
#include "msg.h"
#include "debug.h"
#include "../res/resource.h"
#include "load_driver/load_driver.h"
#include "load_driver/com_drv.h"
#include "struct/memory.h"
#include <process.h>

static char* __THIS_FILE__ = __FILE__;

/**********************************************************
�ļ�����:monitor.c
�ļ�·��:../common/monitor.c
����ʱ��:2013-04-04,13:26:01,��������
�ļ�����:Ů������
�ļ�˵��:���ڹ�������
**********************************************************/

static HFONT hFont;
static HANDLE hXorMutex;
static HANDLE hComDevice;
static MHANDLE hComHandle;
static HANDLE hEventThread;
static HANDLE hThread;
static HWND hEdit;
static HWND hDlgMain;

#define MONITOR_WINDOW_FONT_WIDTH		8
#define MONITOR_WINDOW_FONT_HEIGHT		16
#define MONITOR_MUTEX					"COM_MONITOR_MUTEX"
#define MONITOR_SERVICE_NAME			"ComMonitor"
#define MONITOR_DEVICE_NAME				"\\\\.\\SerMon"

static void add_mon_text(unsigned char* ba, int length, char* str)
{
	int len;
	char* rstr;
	static char inner_str[10240];
	debug_out(("����add_mon_text\r\n"));
	if(ba){
		rstr=utils.hex2str(ba,&length,0,0,&inner_str[0],__ARRAY_SIZE(inner_str));
		if(!rstr){
			utils.msgerr(hDlgMain,"<add_mon_text>");
			return;
		}
	}
	len=GetWindowTextLength(hEdit);
	Edit_SetSel(hEdit,len,len);
	Edit_ReplaceSel(hEdit,str);
	if(ba){
		len=GetWindowTextLength(hEdit);
		Edit_SetSel(hEdit,len,len);
		Edit_ReplaceSel(hEdit,rstr);
		len=GetWindowTextLength(hEdit);
		Edit_SetSel(hEdit,len,len);
		Edit_ReplaceSel(hEdit,"\r\n");
	}
	if(ba && rstr!=inner_str){
		memory.free_mem((void**)&rstr,"<add_mon_text>");
	}
	return;
}

static void ProcessDataPacket(IOReq* pIrp)
{
	static char inner_str[10240];
	switch(pIrp->type)
	{
	case REQ_OPEN:add_mon_text(NULL,0,"�����Ѵ�!\r\n");break;
	case REQ_CLOSE:add_mon_text(NULL,0,"�����ѹر�!\r\n");break;
	case REQ_READ:
		{
			unsigned char* ba = (unsigned char*)pIrp+sizeof(IOReq);
			add_mon_text(ba,pIrp->SizeCopied,"��ȡ:");
			//char str[64];
			//sprintf(str,"��ȡ�ֽ���:%d\r\n",pIrp->SizeCopied);
			//add_mon_text(NULL,0,str);
			//break;
			break;
		}
	case REQ_WRITE:
		{
			unsigned char* ba = (unsigned char*)pIrp+sizeof(IOReq);
			add_mon_text(ba,pIrp->SizeCopied,"д��:");
			//char str[64];
			//sprintf(str,"д���ֽ���:%d\r\n",pIrp->SizeCopied);
			//add_mon_text(NULL,0,str);
			break;
		}
	case REQ_SETBAUDRATE:
		{
			char ts[128];
			_snprintf(ts,__ARRAY_SIZE(ts),"���ò�����Ϊ:%u\r\n",*(unsigned long*)((unsigned char*)pIrp+sizeof(IOReq)));
			add_mon_text(NULL,0,ts);
			break;
		}
	case REQ_SETLINECONTROL:
		{
			char ts[128];
			SERIAL_LINE_CONTROL* pslc = (SERIAL_LINE_CONTROL*)((unsigned char*)pIrp+sizeof(IOReq));
			char* stopbits = pslc->StopBits==ONESTOPBIT?"1λ":(pslc->StopBits==ONE5STOPBITS?"1.5λ":"2λ");
			char* parity;
			switch(pslc->Parity){
				case EVENPARITY:parity="ż";break;
				case ODDPARITY:parity="��";break;
				case NOPARITY:parity="��";break;
				case MARKPARITY:parity="���";break;
				case SPACEPARITY:parity="�ո�";break;
				default:parity="<δ֪>";break;
			}
			_snprintf(ts,__ARRAY_SIZE(ts),"ֹͣλ:%s,У�鷽ʽ:%sУ��,�ֽڳ�:%dλ\r\n",stopbits,parity,pslc->WordLength);
			add_mon_text(NULL,0,ts);
			break;
		}
	}
	memory.free_mem((void**)&pIrp,"<ProcessDataPacket>");
}

static unsigned __stdcall MonitorThreadProc(void* pv)
{
	DWORD size,dw;
	OVERLAPPED o={0};
	HANDLE hEventsArray[2];
	o.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	utils.assert_expr((void*)o.hEvent,"MonitorThreadProc");
	hEventsArray[1] = hEventThread;
	hEventsArray[0] = o.hEvent;
	debug_out(("����MonitorThreadProc\r\n"));
	for(;;){
		BOOL WaitRet,IoRet;
		BOOL flag;
		IoRet = DeviceIoControl(hComDevice,IOCTL_COMMON_GETINFOSIZE,&hComHandle,sizeof(MHANDLE),&size,sizeof(size),&dw,&o);
		if(IoRet == TRUE){//----------�������

		}else{//----------------------����δ���
			if(GetLastError()!=ERROR_IO_PENDING){
				utils.msgerr(hDlgMain,"MonitorThreadProc");
				return 0;
			}
			debug_out(("IOCTL_SERMON_GETINFOSIZE ����\r\n"));
			WaitRet = WaitForMultipleObjects(2,hEventsArray,FALSE,INFINITE);
			debug_out(("����֮һ������!\r\n"));
			flag = GetOverlappedResult(hComDevice,&o,&dw,FALSE);
			if(WaitRet==WAIT_OBJECT_0+1 || !flag){//---�ر�
				debug_out(("�߳��˳�!\r\n"));
				return 0;
			}
		}
		do{
			unsigned char* pb = (unsigned char*)GET_MEM(size);
			IoRet = DeviceIoControl(hComDevice,IOCTL_COMMON_GETINFO,&hComHandle,sizeof(hComHandle),pb,size,&dw,&o);
			if(IoRet == TRUE){//----�ɹ���ȡ����
				ProcessDataPacket((IOReq*)pb);
				continue;
			}else{
				if(GetLastError()!=ERROR_IO_PENDING){
					utils.msgerr(hDlgMain,"IOCTL_SERMON_GETINFO");
					memory.free_mem((void**)&pb,"MonitorThreadProc");
					return 0;
				}
				WaitRet = WaitForMultipleObjects(2,hEventsArray,FALSE,INFINITE);
				flag = GetOverlappedResult(hComDevice,&o,&dw,FALSE);
				if(WaitRet == WAIT_OBJECT_0+1 || !flag){
					memory.free_mem((void**)&pb,"<>");
					debug_out(("�߳��˳�!\r\n"));
					return 0;
				}
				ProcessDataPacket((IOReq*)pb);
			}
		}while(0);
	}
}


#define SET_RESULT(result) SetDlgMsgResult(hWndDlg,uMsg,result)

static INT_PTR __stdcall MonitorWindowProc(HWND hWndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_SIZE:
		if(wParam==SIZE_MAXIMIZED || wParam==SIZE_RESTORED){
			int w=LOWORD(lParam);
			int h=HIWORD(lParam);
			MoveWindow(hEdit,0,0,w,h,TRUE);
			return 0;
		}
		break;
	case WM_INITDIALOG:
		{
			hDlgMain = hWndDlg;
			hEdit = GetDlgItem(hWndDlg,IDC_EDIT_MONITOR_RECV);
			SendMessage(hEdit,EM_SETLIMITTEXT,(WPARAM)(1<<22),0);
			hEventThread=CreateEvent(NULL,TRUE,FALSE,NULL);
			utils.assert_expr((void*)hEventThread,"WM_INITDIALOG");
			hThread = (HANDLE)_beginthreadex(NULL,0,MonitorThreadProc,NULL,0,NULL);
			utils.assert_expr((void*)hThread,"_beginthreadex");
			return 0;
		}
	case WM_CLOSE:
		{
			do{
				//HANDLE h = CreateFile
			}while(0);
			debug_out(("����WM_CLOSE\r\n"));
			SetEvent(hEventThread);
			debug_out(("����WM_CLOSE�ȴ�\r\n"));
			WaitForSingleObject(hThread,1000);
			debug_out(("�̵߳ȴ�����\r\n"));
			StartMonitor(NULL,0);
			debug_out(("ֹͣ����\r\n"));
			CloseHandle(hEventThread);
			debug_out(("�ر�hEventThread\r\n"));
			CloseHandle(hComDevice);
			debug_out(("�ر�hComDevice\r\n"));
			CloseHandle(hThread);
			debug_out(("�ر�hThread\r\n"));
			DoLoadDriver(0);
			debug_out(("ж������\r\n"));
			ReleaseMutex(hXorMutex);
			CloseHandle(hXorMutex);
			EndDialog(hWndDlg,0);
			SET_RESULT(1);
			return 1;
		}
	}
	return 0;
}

static int DoLoadDriver(int load)
{
	if(load){
		char sys[MAX_PATH];
		GetModuleFileName(NULL,sys,__ARRAY_SIZE(sys));
		strcpy(strrchr(sys,'\\'),"\\common.sys");
		if(!driver.load(sys,MONITOR_SERVICE_NAME,"Com Monitor",1)){
			utils.msgbox(hDlgMain,MB_ICONERROR,COMMON_NAME,"�޷���������!");
			return 0;
		}
		hComDevice = CreateFile(MONITOR_DEVICE_NAME,GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE,FILE_SHARE_WRITE|FILE_SHARE_READ,
			NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,NULL);
		if(hComDevice == INVALID_HANDLE_VALUE){
			utils.msgerr(hDlgMain,"CreateFile");
			DoLoadDriver(0);
			return 0;
		}
		return 1;
	}else{
		if(!driver.unload(MONITOR_SERVICE_NAME)){
			utils.msgbox(hDlgMain,MB_ICONERROR,COMMON_NAME,"δ�ܳɹ�ж����������!");
			return 0;
		}
	}
	return 1;
}


// "\\??\\COMx"
int StartMonitor(wchar_t* com_str, int start)
{
	BOOL ret;
	DWORD dw;
	if(start){
		ret=DeviceIoControl(hComDevice,IOCTL_COMMON_STARTMONITOR,com_str,(wcslen(com_str)+1)*sizeof(wchar_t),&hComHandle,sizeof(hComHandle),&dw,NULL);
		if(ret==FALSE){
			utils.msgerr(hDlgMain,"�޷���������!");
			return 0;
		}
	}else{
		debug_out(("����STOPMONITOR\r\n"));
		ret=DeviceIoControl(hComDevice,IOCTL_COMMON_STOPMONITOR,&hComHandle,sizeof(hComHandle),NULL,0,&dw,NULL);
		debug_out(("�뿪STOPMONITOR\r\n"));
		if(ret==FALSE){
			utils.msgbox(hDlgMain,MB_ICONERROR,COMMON_NAME,"δ�ܳɹ�ֹͣ!");
			return 0;
		}
	}
	return 1;
}

int ShowMonitorWindow(void)
{
	hXorMutex = CreateMutex(NULL,FALSE,MONITOR_MUTEX);
	if(GetLastError()==ERROR_ALREADY_EXISTS){
		CloseHandle(hXorMutex);
		utils.msgbox(msg.hWndMain,MB_ICONERROR,COMMON_NAME,"��ǰ��֧�ֹ��˶���豸!");
		return 0;
	}
	if(!DoLoadDriver(1)){
		ReleaseMutex(hXorMutex);
		CloseHandle(hXorMutex);
		return 0;
	}
	if(!StartMonitor(L"\\??\\COM9",1)){
		utils.msgbox(msg.hWndMain,MB_ICONERROR,COMMON_NAME,"�޷���ʼ��¼!");
		ReleaseMutex(hXorMutex);
		CloseHandle(hXorMutex);
		CloseHandle(hComDevice);
		DoLoadDriver(0);
		return 0;
	}
	DialogBoxParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DLG_MONITOR),NULL,MonitorWindowProc,0);
	return 1;
}

int CloseMonitorWindow(void)
{
	if(IsWindow(hDlgMain)){
		SendMessage(hDlgMain,WM_CLOSE,0,0);
		return 1;
	}
	return 1;
}
