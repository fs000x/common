#ifndef __LOADDRIVER_H__
#define __LOADDRIVER_H__

/**********************************************************
�ļ�����:LoadDriver.h/LoadDriver.c
�ļ�·��:./LoadDriver/LoadDriver.h,LoadDriver.c
����ʱ��:2013-2-1,22:24:09
�ļ�����:Ů������
�ļ�˵��:��C���Գ���ʵ�ֶ��ں���������ļ�����ж��
	LoadDriver - ��������
	UnloadDriver - ж������
**********************************************************/

void init_driver(void);
struct driver_s{
	int (*load)(char* DriverAbsPath, char* ServiceName, char* DisplayName,int PromptIfExists);
	int (*unload)(char* ServiceName);
};



#ifndef LOAD_DRIVER
extern struct driver_s driver;
#else
struct driver_s driver;

int LoadDriver(char* DriverAbsPath, char* ServiceName, char* DisplayName,int PromptIfExists);
int UnloadDriver(char* ServiceName);

#endif

#endif//!__LOADDRIVER_H__
