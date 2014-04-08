#include <windows.h>
#include <stdio.h>
#define LOAD_DRIVER
#include "load_driver.h"

static SC_HANDLE hScManager;

static void drvShowError(char* msg);
static int drvOpenScManager(int open);
static int drvCreateService(char* DriverAbsolutePath,char* ServiceName,char* ServiceDispalyName,SC_HANDLE* phService);
static int drvAddService(char* DriverAbsPath, char* ServiceName, char* DisplayName, int PromptIfExists);
static int drvDeleteService(char* ServiceName);

void init_driver(void)
{
	memset(&driver,0,sizeof(driver));
	driver.load = LoadDriver;
	driver.unload = UnloadDriver;
}

/**************************************************
��  ��:drvShowError@4
��  ��:��ʾ���һ��ϵͳ�������õĴ�����Ϣ
��  ��:msg - ��Ϣǰ׺˵��
����ֵ:(none)
˵  ��:�ڲ�����,����ʹ��MessageBox,���Ըĳ��Լ���
**************************************************/
void drvShowError(char* msg)
{
	void* pBuffer = NULL;
	DWORD dwLastError;
	dwLastError = GetLastError();
	if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPSTR)&pBuffer, 1, NULL))
	{
		char buf[1024];
		_snprintf(buf,sizeof(buf),"%s\n\n%s", msg,pBuffer);
		MessageBox(NULL,buf,NULL,MB_OK);
		LocalFree((HLOCAL)pBuffer);
	}
}

/**************************************************
��  ��:drvOpenScManager@4
��  ��:�򿪷�����ƹ�����
��  ��:open - !0:��,0:�ر�
����ֵ:�ɹ�:!0;ʧ��:0
˵  ��:�ڲ�����
	2013-02-17:
		������ ���ü�����֧�ּ��ض������,δ����
**************************************************/
int drvOpenScManager(int open)
{
	static DWORD refcount=0;
	if(open){
		if(hScManager){
			InterlockedIncrement(&refcount);
			return 1;
		}
		hScManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if(hScManager == NULL){
			drvShowError("�򿪷�����ƹ�����ʧ��!");
			return 0;
		}
		InterlockedIncrement(&refcount);
		return 1;
	}else{
		if(hScManager&&!InterlockedDecrement(&refcount)){
			CloseServiceHandle(hScManager);
			hScManager=NULL;
		}
		return 1;
	}
}

/**************************************************
��  ��:drvCreateService@16
��  ��:�����·���,�����ط�����(if succeeded)
��  ��:	DriverAbsolutePath - �����ļ��ľ���·��
		ServiceName - ������
		ServiceDisplayName - �������ʾ��
		*phService - ���صķ�����
����ֵ:�ɹ�:!0;ʧ��:0
˵  ��:�ڲ�����
**************************************************/
int drvCreateService(char* DriverAbsolutePath,char* ServiceName,char* ServiceDispalyName,SC_HANDLE* phService)
{
	SC_HANDLE hService;
	hService = CreateService(
		hScManager,				//������������������
		ServiceName,			//���������
		ServiceDispalyName,		//�������ʾ����
		SERVICE_ALL_ACCESS,		//�Ը÷���ķ���Ȩ��
		SERVICE_KERNEL_DRIVER,	//���������:�ں�����
		SERVICE_DEMAND_START,	//��������:�ֶ�����
		SERVICE_ERROR_NORMAL,	//����������:����
		DriverAbsolutePath,		//�����ļ��ľ���·��
		NULL,					//û��������
		NULL,					//������Ĭ�ϵı�ǩID
		NULL,					//û�з���������
		NULL,					//ʹ��Ĭ�϶�������
		NULL					//û������
		);
	*phService = hService;
	return hService!=NULL;
}

/**************************************************
��  ��:drvDeleteService@4
��  ��:ɾ��ָ���������ĵķ���
��  ��:ServiceName - ������
����ֵ:�ɹ�:!0;ʧ��:0
˵  ��:	�ڲ�����
		�Բ����ڵķ��񷵻�-1(�ɹ�)
**************************************************/
int drvDeleteService(char* ServiceName)
{
	int sehcode=0;
	SERVICE_STATUS ServiceStatus;
	SC_HANDLE hService=NULL;
	DWORD dwLastError;
	__try{
		hService=OpenService(hScManager,ServiceName,SERVICE_ALL_ACCESS);
		if(hService==NULL){
			dwLastError=GetLastError();
			if(dwLastError==ERROR_SERVICE_DOES_NOT_EXIST){
				sehcode=-1;
				__leave;
			}else{
				drvShowError("�ڴ��Ѿ����ڵķ���ʱ�������´���:");
				__leave;
			}
		}
		if(!ControlService(hService,SERVICE_CONTROL_STOP,&ServiceStatus)){//ֹͣ����ʧ��
			dwLastError = GetLastError();
			if(dwLastError != ERROR_SERVICE_NOT_ACTIVE){//��������Ϊû������������
				drvShowError("��ֹͣ����ʱ�������´���:");
				__leave;
			}
		}
		if(!DeleteService(hService)){
			drvShowError("��ɾ���Ѵ��ڵķ���ʱ�������´���:");
			__leave;
		}
		sehcode=1;
	}
	__finally{
		if(hService){
			CloseServiceHandle(hService);
			hService=NULL;
		}
	}
	return sehcode;
}

/**************************************************
��  ��:drvAddService@12
��  ��:���ָ���ķ���
��  ��:	DriverAbsPath - �����������·��
		ServiceName - ������
		DisplayName - ������ʾ��
		PromptIfExists - ���ڵ�ʱ���Ƿ����:
			1:ɾ�������´���
			0:���ټ���,����-1(�ɹ�)
			-1:��ʾ�Ƿ����
����ֵ:�ɹ�:!0;ʧ��:0
˵  ��:	�ڲ�����
		��ѡ���˲��ټ���,����-1(�ɹ�)
**************************************************/
int drvAddService(char* DriverAbsPath, char* ServiceName, char* DisplayName, int PromptIfExists)
{
	int sehcode=0;
	SC_HANDLE hService = NULL;		//����/�򿪵ķ�����
	DWORD dwErrCode = 0;
	__try{
		//�ٶ����񲻴��ڲ�����
		if(!drvCreateService(DriverAbsPath,ServiceName,DisplayName,&hService)){
			//���񴴽�ʧ��,�����Ѿ�����
			DWORD dwLastError = GetLastError();
			//����Ƿ����Ѿ�����,ɾ��,���°�װ
			if(dwLastError == ERROR_SERVICE_EXISTS){
				switch(PromptIfExists)
				{
				case 1:break;//���´���
				case 0:{sehcode=-1;__leave;break;}
				case -1:
					{
						char* yesmsg = "ָ���ķ����Ѿ�����,Ҫ����������?";
						if(MessageBox(NULL,yesmsg,DisplayName,MB_ICONQUESTION|MB_YESNO)!=IDYES){
							sehcode=-1;
							__leave;
						}
						break;
					}
				}
				if(!drvDeleteService(ServiceName)){
					__leave;
				}
				if(!drvCreateService(DriverAbsPath,ServiceName,DisplayName,&hService)){
					drvShowError("���´�������ʱ�������´���:");
					__leave;
				}
			}else{//����ԭ����ɷ��񴴽�ʧ��
				drvShowError("��������ʱ�������´���:");
				__leave;
			}
		}
		//����ɹ�������������
		if(!StartService(hService,0,NULL)){
			drvShowError("����������ʱ�������´���:");
			__leave;
		}
		sehcode=1;
	}
	__finally{
		if(hService){
			CloseServiceHandle(hService);
			hService=NULL;
		}
	}
	return sehcode;
}

/**************************************************
��  ��:LoadDriver@12
��  ��:����ָ������
��  ��:	DriverAbsPath - �����������·��
		ServiceName - ������
		DisplayName - ������ʾ��
		PromptIfExists - ���ڵ�ʱ���Ƿ����:
			1:ɾ�������´���
			0:���ټ���,����-1(�ɹ�)
			-1:��ʾ�Ƿ����
����ֵ:�ɹ�:!0;ʧ��:0
˵  ��:	�ⲿ����
		����ʧ�ܲ���һ������ȫʧ��,������ʼ�ճ�
�Եص���LoadDriver/UnloadDriver�����ע��������Ϣ
**************************************************/
int LoadDriver(char* DriverAbsPath, char* ServiceName, char* DisplayName,int PromptIfExists)
{
	if(!drvOpenScManager(1))
		return 0;
	return drvAddService(DriverAbsPath,ServiceName,DisplayName, PromptIfExists);
}

/**************************************************
��  ��:UnloadDriver@4
��  ��:ж��ָ�����Ƶ���������
��  ��:ServiceName - ���������
����ֵ:�ɹ�:!0;ʧ��:0
˵  ��:	�ⲿ����
		�Բ����ڵķ��񷵻�-1(�ɹ�)
**************************************************/
int UnloadDriver(char* ServiceName)
{
	int ret;
	ret=drvDeleteService(ServiceName);
	drvOpenScManager(0);
	return ret;
}

//~~~ʾ��~~~
#if 0
int main(void){
	int err;
	char sys[MAX_PATH];
	GetModuleFileName(NULL,sys,sizeof(sys));
	strcpy(strrchr(sys,'\\'),"\\drv.sys");
	printf("sys:%s\n",sys);
	err=LoadDriver(sys, "drv","drv Test Service",-1);
	MessageBox(NULL,err==-1?"�����Ѵ���!":err?"�����Ѽ���!":"����δ�ܳɹ�����!","",MB_OK);
	if(err!=-1){
		err=UnloadDriver("drv");
		MessageBox(NULL,err==-1?"����������!":err?"������ж��!":"����δ�ܳɹ�ж��!","",MB_OK);
	}
	return 0;
}
#endif
