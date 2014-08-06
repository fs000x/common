#ifndef __DEAL_H__
#define __DEAL_H__
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#ifdef __cplusplus
extern "C" {
#endif

void init_deal(void);

/***********************************************************************
����:������������͵�����
����:
����:data_size,��ǰ���ݰ��Ĵ�С
����:
˵��:
***********************************************************************/
enum{SEND_DATA_TYPE_NOTUSED,SEND_DATA_TYPE_USED,SEND_DATA_TYPE_MUSTFREE,SEND_DATA_TYPE_AUTO_USED,SEND_DATA_TYPE_AUTO_MUSTFREE};
enum{SEND_DATA_ACTION_GET,SEND_DATA_ACTION_RETURN,SEND_DATA_ACTION_INIT,SEND_DATA_ACTION_FREE,SEND_DATA_ACTION_RESET};
typedef struct _SEND_DATA{
	DWORD cb;						//��ǰ���ݰ��Ĵ�С
	DWORD data_size;				//��ǰ�����Ĵ����͵����ݵĴ�С
	int flag;						//��Ӧ�����enum
	unsigned char data[10240];		//2013-03-23:���ӵ�10KB
}SEND_DATA;

struct deal_s{
	void (*do_check_recv_buf)(void);
	int (*do_buf_send)(int action,void* pv);
	int (*do_buf_recv)(unsigned char* chs, int cb, int action);
	void (*update_status)(char* str);
	void (*update_savebtn_status)(void);
	void (*cancel_auto_send)(int reason);
	void (*check_auto_send)(void);
	unsigned int (__stdcall* thread_read)(void* pv);
	unsigned int (__stdcall* thread_write)(void* pv);
	void* (*do_send)(int action);
	int (*send_char_data)(char ch);
	void (*add_send_packet)(SEND_DATA* psd);
	SEND_DATA* (*make_send_data)(int fmt,void* data,size_t size);
	void (*start_timer)(int start);
	void (*add_text)(unsigned char* ba, int cb);
	void (*add_text_critical)(unsigned char* ba, int cb);
	//....
	int last_show;
	//��ʱ��
	unsigned int conuter;
	struct{
		//��д�ܵ�
		HANDLE hPipeRead;
		HANDLE hPipeWrite;
		//��д�߳�
		HANDLE hThreadRead;
		HANDLE hThreadWrite;
		//ͬ���¼� - since the serial driver does not support Async. I/O, deprecated
		//HANDLE hEventRead;
		//HANDLE hEventWrite;
	}thread;
	void* autoptr;//�Զ�����ʱʹ�õ�����ָ��,��do_send����
	unsigned int timer_id;	//�Զ�����ʱ�Ķ�ʱ��ID
	CRITICAL_SECTION critical_section;
	CRITICAL_SECTION g_add_text_cs;

	//�������û���δ��ʾ������ʱ, �������read�߳�
	HANDLE hEventContinueToRead;

	// ���㵱ǰ�������ݰ���ʼ��\r\n(������), Ҳ������˵�������һ���س�����, ��Ϊ��һ���п�������\r/\n��β��!!!
	// �ǲ����һ��������ô������������ν��"�ַ��豸"�ı�׼������?
	// �ټ���Windows�ļ��±�"����"ȡ�õ�ǰ��󼸸��ַ���ʲô, ���������ﱣ��һ�������յļ����ַ�
#define DEAL_CACHE_SIZE 10240
	struct{
		int cachelen;	//ת��֮ǰ��crlf����
		int crlflen;	//ת��֮���crlf����, ��������
		unsigned char cache[DEAL_CACHE_SIZE];
		unsigned char* ptr; // Ŀǰ��δʹ��
	}cache;
};

#ifndef __DEAL_C__
extern struct deal_s deal;
#endif

#ifdef __DEAL_C__
#undef __DEAL_C__
int do_buf_send(int action,void* pv);
int do_buf_recv(unsigned char* chs, int cb, int action);
void do_check_recv_buf(void);

void update_status(char* str);
void update_savebtn_status(void);
void cancel_auto_send(int reason);
void check_auto_send(void);


unsigned int __stdcall thread_read(void* pv);
unsigned int __stdcall thread_write(void* pv);

void* do_send(int action);
int send_char_data(char ch);

void add_send_packet(SEND_DATA* psd);
SEND_DATA* make_send_data(int fmt,void* data,size_t size);

//void add_ch(unsigned char ch);
void add_text(unsigned char* ba, int cb);
void add_text_critical(unsigned char* ba, int cb);

void start_timer(int start);

#endif


#ifdef __cplusplus
}
#endif


#endif//__DEAL_H__
