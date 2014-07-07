#define __DEAL_C__
#include "deal.h"
#include "common.h"
#include "msg.h"
#include "utils.h"
#include "comm.h"
#include "about.h"
#include "debug.h"
#include "draw.h"
#include "struct/memory.h"
#include "../res/resource.h"

#pragma comment(lib,"WinMM")	//timeSetEvent,timeKillEvent

struct deal_s deal;
static char* __THIS_FILE__ = __FILE__;

void init_deal(void)
{
	memset(&deal,0,sizeof(deal));
	deal.do_check_recv_buf     = do_check_recv_buf;
	deal.do_buf_recv           = do_buf_recv;
	deal.do_buf_send           = do_buf_send;
	deal.update_savebtn_status = update_savebtn_status;
	deal.update_status         = update_status;
	deal.thread_read           = thread_read;
	deal.thread_write          = thread_write;
	deal.cancel_auto_send      = cancel_auto_send;
	deal.check_auto_send       = check_auto_send;
	deal.do_send               = do_send;
	deal.send_char_data		   = send_char_data;
	deal.make_send_data        = make_send_data;
	deal.add_send_packet       = add_send_packet;
	deal.start_timer           = start_timer;
	deal.add_text			   = add_text;
	deal.add_text_critical     = add_text_critical;
	deal.last_show             = 1;

	InitializeCriticalSection(&deal.g_add_text_cs);
}

//���� ���浽�ļ� ��ť��״̬
//Ӧlin0119��Ҫ��, ͬʱ����"���ƽ�������"��״̬
void update_savebtn_status(void)
{
	HWND hSave = GetDlgItem(msg.hWndMain, IDC_BTN_SAVEFILE);
	HWND hCopy = GetDlgItem(msg.hWndMain, IDC_BTN_COPY_RECV);
	//����û�򿪻���ʾ����
	//PS:���Լ����ڶ��ֲ����������IHV����NULL������Ч, ����, ����, �����Ժ�϶�֪������ô����
	BOOL bEnable = msg.hComPort==INVALID_HANDLE_VALUE || !msg.hComPort || !comm.fShowDataReceived;
	EnableWindow(hSave,bEnable);
	EnableWindow(hCopy,bEnable);
}

//NULL:���¼���״̬
void update_status(char* str)
{
#define STATUS_LENGTH	6
	static char status[128] = {" ״̬:"};
	static HWND hStatus;
	//TODO:
	if(!hStatus)
		hStatus = GetDlgItem(msg.hWndMain, IDC_STATIC_STATUS);
	if(str == NULL)//���¼���״̬
		sprintf(status+STATUS_LENGTH, "���ռ���:%u,���ͼ���:%u,�ȴ�����:%u", comm.cchReceived, comm.cchSent,comm.cchNotSend);
	else//���״̬���
		_snprintf(status+STATUS_LENGTH, sizeof(status)-STATUS_LENGTH, "%s", str);
	SetWindowText(hStatus, status);
#undef STATUS_LENGTH
}
/*
��  ��:do_buf_recv
��  ��:����ֹͣ��ʾ�������
��  ��:
	chs:�ַ�ָ��,���������
	cb:�ֽ���
	action:����, ������:
		0 - ��ӻ����ڴ�, �޷���ֵ
		1 - ȡ�û���������, ����ֵΪunsigned char*
		2 - ȡ�û������ĳ���, ����int
		3 - �ͷŻ������ڴ�
����ֵ:
	int - �ο�action��������
*/

// #pragma pack(push,1)
// struct Frame{
// 	unsigned char h1;
// 	unsigned char currentFrame;
// 	unsigned char data[240];
// 	unsigned char h2[2];
// };
// #pragma pack(pop)

int do_buf_recv(unsigned char* chs, int cb, int action)
{
	static unsigned char* str = NULL;
	static unsigned char* pstr = NULL;
	//unsigned int length = 0;
	if(str == NULL && action==0){
		//TODO:
		str = (unsigned char*)GET_MEM(COMMON_INTERNAL_RECV_BUF_SIZE);
		if(str==NULL) return 0;
		pstr = str;
	}
	switch(action)
	{
	case 0://��ӻ����ַ�
		if(pstr-str + cb > COMMON_INTERNAL_RECV_BUF_SIZE){
			int ret;
			//TODO:ѯ���Ƿ���µ���ʾ���ټ���,����ɾ��
			ret = utils.msgbox(msg.hWndMain,MB_ICONERROR|MB_YESNO, "����",
				"ֹͣ��ʾ��, ��ʾ�����ݱ����浽�˳����ڲ��Ļ�����, ��:\n"
				"�ڲ�������Ĭ�ϵ�1M�ռ�����, ���ݿ����Ѳ��ֶ�ʧ!\n\n"
				"�Ƿ�Ҫ����ڲ�������?"
			);
			if(ret == IDYES){
				memory.free_mem((void**)&str,NULL);
			}
			return 0;
		}
		memcpy(pstr,chs,cb);
		pstr += cb;
		//length = sprintf(pstr, chs);
		//pstr += length;
		break;
	case 1://ȡ�û���������
		return (int)str;
	case 2://ȡ�û���������
		return (int)(pstr-(unsigned int)str/*+1*/);
	case 3://�ͷŻ�����
		if(str) memory.free_mem((void**)&str,"<do_buf_recv>");
		return 0;
	}
	return 0;
}

/**************************************************
��  ��:do_check_recv_buf@-
��  ��:����ֹͣ��ʾ�ָ���������ʾʱ��⻺�����Ƿ��б��������
��  ��:(none)
����ֵ:(none)
˵  ��:(none)
**************************************************/
void do_check_recv_buf(void)
{
	unsigned char* saved = (unsigned char*)do_buf_recv(NULL, 0, 1);
	if(saved != NULL){//�ڲ�������������
		int ret;
		ret = utils.msgbox(msg.hWndMain,MB_ICONINFORMATION|MB_YESNO, "��ʾ",
			"��~\n\n"
			"����ͣ��ʾ���ݺ�, δ����ʾ�����ݱ����浽���ڲ���������,\n\n"
			"��Ҫ���±���������ݵ���������?\n\n"
			"�����ѡ���˷�, �ⲿ�����ݽ������ٱ�����!\n\n"
			"������Ŀǰ���� %d �ֽڵ�����~",
			do_buf_recv(NULL,0,2)); //��Ϊ�Ѿ��������¼�, ����������, ������������������ı�
		if(ret == IDYES){//ϣ����ʾ���������˵�����
			/*int len1 = Edit_GetTextLength(msg.hEditRecv);
			int len2 = do_buf_recv(NULL,0, 2);
			if(len1+len2 > COMMON_RECV_BUF_SIZE){//��������װ������
				int ret;
				ret = utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION|MB_YESNO, COMMON_NAME,
					"�~~~\n\n"
					"�������ĳ����Ѵﵽ�������, ���ݿ��ܲ��ᱻ��������ʾ����������, Ҫ��ʾ�ضϺ��������?\n\n"
					"���ѡ���˷�, �ڲ�����Ļ��������ݻᱻ�����Ŷ~");
				if(ret == IDNO){//ȡ����ʾ�����
					do_buf_recv(NULL,0, 3);
					return;
				}else{//ѡ���˽ض���ʾ����
					
				}
			}
			Edit_SetSel(msg.hEditRecv, len1, len1);
			Edit_ReplaceSel(msg.hEditRecv, saved);*/
			add_text(saved,do_buf_recv(NULL,0,2));
			do_buf_recv(NULL, 0, 3);
		}else{//����Ҫ��ʾ���������,�����
			do_buf_recv(NULL, 0, 3);
		}
	}
}

/***********************************************************************
����:do_buf_send
����:����δ�����͵�����
����:action-����,pv-��������
����:
˵��:������������actionΪSEND_DATA_ACTION_RETURNʱ��Ч,ΪSEND_DATAָ��
***********************************************************************/
#define SEND_DATA_SIZE 101
int do_buf_send(int action,void* pv)
{
	static SEND_DATA* send_data[SEND_DATA_SIZE];
	//static 
	int retval;
	if(action==SEND_DATA_ACTION_GET || action==SEND_DATA_ACTION_RETURN || action==SEND_DATA_ACTION_RESET){
		EnterCriticalSection(&deal.critical_section);
	}
	switch(action)
	{
	case SEND_DATA_ACTION_INIT://��ʼ��
		{
			int it;
			int len = sizeof(SEND_DATA)*SEND_DATA_SIZE;
			void* pv=GET_MEM(len);
			if(!pv){
				utils.msgbox(msg.hWndMain,MB_ICONERROR,NULL,"��ʼ�����ͻ�����ʧ��,���������г���!");
				return 0;
			}
			for(it=0; it<SEND_DATA_SIZE; it++){
				send_data[it] = (SEND_DATA*)((unsigned char*)pv+it*sizeof(SEND_DATA));
				send_data[it]->flag = SEND_DATA_TYPE_NOTUSED;
			}
			InitializeCriticalSection(&deal.critical_section);
			return 1;
		}
	case SEND_DATA_ACTION_GET://ȡ�û�����
		{
			int i;
			for(i=0;i<SEND_DATA_SIZE;i++){
				if(send_data[i]->flag == SEND_DATA_TYPE_NOTUSED){
					send_data[i]->flag = SEND_DATA_TYPE_USED;
					retval = (int)send_data[i];
					goto _exit_dbs;
				}
			}
			retval = 0;
			goto _exit_dbs;
		}
	case SEND_DATA_ACTION_RETURN://�黹������
		{
			int i;
			for(i=0;i<SEND_DATA_SIZE;i++){
				if((SEND_DATA*)pv == send_data[i]){
					send_data[i]->flag = SEND_DATA_TYPE_NOTUSED;
					retval = 0;
					break;
				}
			}
			goto _exit_dbs;
		}
	case SEND_DATA_ACTION_FREE://�ͷ����л�����
		memory.free_mem((void**)&send_data[0],NULL);
		DeleteCriticalSection(&deal.critical_section);
		return 1;
	case SEND_DATA_ACTION_RESET://��λ����
		{
			int i;
			for(i=0;i<SEND_DATA_SIZE;i++){
				send_data[i]->flag = SEND_DATA_TYPE_NOTUSED;
			}
			retval = 0;
			goto _exit_dbs;
		}
	}
_exit_dbs:
	LeaveCriticalSection(&deal.critical_section);
	return retval;
}

/**************************************************
��  ��:add_text@8
��  ��:���baָ������ݵ���ʾ��(16���ƺ��ַ�ģʽ)
��  ��:ba - 16��������,cb - �ֽ���
����ֵ:(none)
˵  ��:
	2013-03-10:���˴����޸�, ò�����ڲ��ٶ���?�ҵ����
BUG�ɻ����Ҳ���ʱ��!!!
	2014-07-06:���ӿ����ַ�����
**************************************************/

static void add_text_helper(char* str)
{
	int len;
	unsigned int cntb = utils.eliminate_control_char(str);
	char* p = str;
	do{
		if(*p != '\b'){ // --- ׷��
			len = Edit_GetTextLength(msg.hEditRecv2);
			Edit_SetSel(msg.hEditRecv2,len,len);
			Edit_ReplaceSel(msg.hEditRecv2,p);
		}
		else{ // --- ��ǰɾ��
			len = Edit_GetTextLength(msg.hEditRecv2);
			if(len){
				Edit_SetSel(msg.hEditRecv2, len-1,len);
				Edit_ReplaceSel(msg.hEditRecv2,"");
			}
			p++;
		}
	}while(cntb--);
}

void add_text_critical(unsigned char* ba, int cb)
{
	//2012-03-19:���ӵ�10KB�ռ�
	static char inner_str[10240];
	if(cb==0) return;
	//	draw_line(ba,cb);
	if(comm.fShowDataReceived){
		if(comm.data_fmt_recv){//16����
			char* str=NULL;
			DWORD len,cur_pos;
			len = comm.data_count;//Edit_GetTextLength(msg.hEditRecv);
			cur_pos = len % (COMMON_LINE_CCH_RECV*3+2);
			cur_pos = cur_pos/3;
			str = utils.hex2str(ba,&cb,COMMON_LINE_CCH_RECV,cur_pos,inner_str,__ARRAY_SIZE(inner_str));
			__try{
				// ��Ӧ���ڷ����߳��������UI, ���ܻ����
				Edit_SetSel(msg.hEditRecv, len, len);
				Edit_ReplaceSel(msg.hEditRecv, str);
				if(str!=inner_str) memory.free_mem((void**)&str,NULL);
			}
			__except(EXCEPTION_EXECUTE_HANDLER){
				utils.msgbox(msg.hWndMain,MB_ICONERROR,COMMON_NAME,"add_text:Access Violation!");
			}
			InterlockedExchangeAdd((long volatile*)&comm.data_count,cb);
		}else{//�ַ�
			char* str=NULL;
			if(comm.fDisableChinese){//��������ʾ���ĵĻ�,������>0x7F���ַ��ĳ�'?',ͬ��Ҳ���������ַ�
				int it;
				unsigned char uch;
				for(it=0; it<cb; it++){
					uch = ba[it];

					if(uch>0 && uch<32 && (uch!='\n' && (uch=='\b' && !comm.fEnableControlChar)) || uch>0x7F){ //���ö���? ^_^
						ba[it] = (unsigned char)'?';
					}

				}
			}

			str=utils.hex2chs(ba,cb,inner_str,__ARRAY_SIZE(inner_str));
			add_text_helper(str);
			if(str!=inner_str) memory.free_mem((void**)&str,NULL);
		}
	}else{
		do_buf_recv(ba,cb,0);
	}
}

void add_text(unsigned char* ba, int cb)
{
	EnterCriticalSection(&deal.g_add_text_cs);
	add_text_critical(ba, cb);
	LeaveCriticalSection(&deal.g_add_text_cs);
}

/***********************************************************************
����:thread_read
����:������ȡ�������ݵĹ����߳�
����:pv - δʹ��
����:δʹ��
˵��:�����ĳ��첽IO��,�����֪����ô����,WaitForMultipleObjects����ʧ��,д�úú�
��һ�δ���,����ʧ����~��ȥ,�Ժ���WaitForCommEvent����,��!
***********************************************************************/
unsigned int __stdcall thread_read(void* pv)
{
	DWORD nRead,nTotalRead=0,nBytesToRead;
	unsigned char* block_data=NULL;
	BOOL retval;

	block_data = (unsigned char*)GET_MEM(COMMON_READ_BUFFER_SIZE);
	if(block_data == NULL){
		utils.msgbox(msg.hWndMain,MB_ICONERROR,COMMON_NAME,"���߳̽���!");
		return 1;
	}

	for(;;){
		COMSTAT sta;
		DWORD comerr;
		int flag=0;//���ļ��ʱ�õ�

		if(comm.fCommOpened==FALSE || msg.hComPort==INVALID_HANDLE_VALUE){
			debug_out(("��Ϊ�豸�����Ч,���߳��˳�!\n"));
			//utils.msgbox(msg.hWndMain,MB_ICONERROR,"�豸�����Ч,���߳���ֹ!");
			//return 0;
			goto _exit;
		}

		ClearCommError(msg.hComPort,&comerr,&sta);
		if(sta.cbInQue == 0){
			sta.cbInQue++;
		}
			
		nBytesToRead = sta.cbInQue;
		//�������һ���ֽ��������������������ַ�
		if(nBytesToRead>=COMMON_READ_BUFFER_SIZE){
			nBytesToRead = COMMON_READ_BUFFER_SIZE-1;
		}

		nTotalRead = 0;
		for(;;){
			for(;nTotalRead<nBytesToRead;){
				//debug_out(("����ReadFile\n"));
				retval = ReadFile(msg.hComPort, &block_data[0]+nTotalRead, nBytesToRead-nTotalRead, &nRead, NULL);
				if(comm.fCommOpened == FALSE || msg.hComPort==INVALID_HANDLE_VALUE){
					debug_out(("ReadFile��Ϊcomm.fCommOpened==FALSE���˳�!\n"));
					goto _exit;
				}
				if(retval == FALSE){
					InterlockedExchange((long volatile*)&comm.cchNotSend,0);
					//comm.close();
					//comm.update((int*)-1);
					if(comm.fAutoSend){
						deal.cancel_auto_send(0);
					}
					utils.msgerr(msg.hWndMain,"������ʱ��������!\n"
						"�Ƿ��ڰε�����֮ǰ�����˹رմ�����?\n\n"
						"����ԭ��");
					SetTimer(msg.hWndMain,TIMER_ID_THREAD,100,NULL);
					goto _exit;
				}
				
				if(nRead == 0) continue;
				nTotalRead += nRead;
				InterlockedExchangeAdd((long volatile*)&comm.cchReceived, nRead);
				update_status(NULL);
			}//for::��nBytesRead����
			//debug_out(("���ļ��...\n"));
			//������Ҫ������ֽ���,�������������Լ��
			if(!flag && !comm.fDisableChinese && utils.check_chs(&block_data[0],nTotalRead)){
				int it;
				debug_out(("��Ϊ����,���εȴ�ReadFile\n"));
				for(it=0;it<20;it++){
					ClearCommError(msg.hComPort,&comerr,&sta);
					if(sta.cbInQue){
						nBytesToRead += 1;
						flag = 1;
						goto _continue_read;
					}else{
						Sleep(50);
					}
				}
				block_data[nBytesToRead-1]='.';
				//nBytesToRead--;
				break;
			}

			// ���һ��'\r\n'�������ζ�ȡ, ��ô�����ܱ�����������س�����, ����Ӧ��һ���԰�'\r','\n'����(ָ��ʱ����)
			if(block_data[nBytesToRead-1]=='\r' || block_data[nBytesToRead-1]=='\n'){
				int it;
				for(it=0; it<40; it++){
					ClearCommError(msg.hComPort, &comerr, &sta);
					if(sta.cbInQue){
						nBytesToRead += 1;
						goto _continue_read;
					}
					else{
						Sleep(50);
					}
				}
				break;
			}

			break;
_continue_read:
			;
		}
		WaitForSingleObject(deal.hEventContinueToRead,INFINITE);
		add_text(&block_data[0],nBytesToRead);
	}
_exit:
	if(block_data){
		memory.free_mem((void**)&block_data,"���߳�");
	}
	debug_out(("���߳���Ȼ�˳�!\n"));
	UNREFERENCED_PARAMETER(pv);
	return 0;
}

/***********************************************************************
����:thread_write
����:д�����豸�߳�
����:pv-δʹ��
����:δʹ��
˵��:2013-04-13:��Ϊ�첽д��ʽ
2013-04-23:�첽ģʽʧ����,������֧��! �ٸĻ�ͬ��ģʽ!, �Ҳ�, Ҫ��Ҫ����~~~
***********************************************************************/
unsigned int __stdcall thread_write(void* pv)
{
	DWORD nWritten,nRead,nWrittenData;
	SEND_DATA* psd = NULL;
	//OVERLAPPED overlap={0};
	//HANDLE hWriteEvent = CreateEvent(NULL,TRUE, FALSE,NULL);
	BOOL bRet;

	for(;;){
		if(comm.fCommOpened==FALSE || msg.hComPort==INVALID_HANDLE_VALUE){
			return 0;
		}

		bRet = ReadFile(deal.thread.hPipeRead,(void*)&psd,4,&nRead,NULL);
		if(bRet == FALSE){
			if(!comm.fCommOpened||!deal.thread.hPipeRead)
				return 0;
			utils.msgerr(msg.hWndMain,"��ȡ�ܵ�ʱ����:");
		}
		if(nRead!=4)
			continue;
		//Լ��ָ��ֵΪ0x00000001ʱΪ�˳�(�Ƿ����ڴ�)
		if((unsigned long)psd == 0x00000001){
			debug_out(("��Ϊ�յ�������Ϊ1��ָ��,д�߳������˳�!\n"));
			return 0;
		}

		nWrittenData = 0;
		while(nWrittenData < psd->data_size){
			bRet = WriteFile(msg.hComPort, &psd->data[0]+nWrittenData,psd->data_size-nWrittenData, &nWritten, NULL);
			if(comm.fCommOpened==FALSE || msg.hComPort==INVALID_HANDLE_VALUE){
				debug_out(("��Ϊcomm.fCommOpened==FALSE��msg.hComPort,д�߳��˳�!\n"));
				return 0;
			}
			if(bRet == FALSE){
				if(comm.fAutoSend){
					deal.cancel_auto_send(0);
				}
				utils.msgerr(msg.hWndMain,"д�����豸ʱ��������");
				InterlockedExchange((long volatile*)&comm.cchNotSend,0);
				update_status(NULL);
				//comm.close();
				//comm.update((int*)-1);
				SetTimer(msg.hWndMain,TIMER_ID_THREAD,100,NULL);
				return 0;
			}
			if(nWritten==0) continue;
			//debug_out(("in WriteFile,nWritten==%u\n",nWritten));
			nWrittenData += nWritten;
			InterlockedExchangeAdd((volatile long*)&comm.cchSent,nWritten);				//���ͼ���   - ����
			InterlockedExchangeAdd((volatile long*)&comm.cchNotSend,-(LONG)nWritten);	//δ���ͼ��� - ����
			update_status(NULL);
		}
		if(psd->flag==SEND_DATA_TYPE_USED || psd->flag==SEND_DATA_TYPE_AUTO_USED)
			do_buf_send(SEND_DATA_ACTION_RETURN,(void*)psd);
		else if(psd->flag==SEND_DATA_TYPE_MUSTFREE || psd->flag==SEND_DATA_TYPE_AUTO_MUSTFREE)
			memory.free_mem((void**)&psd,"��д�������");
			
	}
	UNREFERENCED_PARAMETER(pv);
	debug_out(("д�߳���Ȼ�˳�!\n"));
	return 0;
}


/**************************************************
��  ��:cancel_auto_send@4
��  ��:ȡ���Զ����Ͳ���
��  ��:reason-ȡ������:0-check,1-�رմ���
����ֵ:(none)
˵  ��:���۴����Ƿ��
	2013-03-04����:���ڹرղ����Զ�ȡ���Զ�����(��)
**************************************************/
void cancel_auto_send(int reason)
{
	//if(!comm.fAutoSend&&msg.hComPort!=INVALID_HANDLE_VALUE) return;
	EnableWindow(GetDlgItem(msg.hWndMain,IDC_BTN_SEND),TRUE);
	EnableWindow(GetDlgItem(msg.hWndMain,IDC_EDIT_DELAY),TRUE);

	if(reason==1){

	}else if(reason == 0){
		CheckDlgButton(msg.hWndMain,IDC_CHK_AUTO_SEND,FALSE);
	}
	if(comm.fAutoSend){
		if(deal.timer_id){
			timeKillEvent(deal.timer_id);
			deal.timer_id = 0;
		}
		comm.fAutoSend=0;
	}
}

/**************************************************
��  ��:check_auto_send@-
��  ��:ʹ���Զ�����ѡ��
��  ��:(none)
����ֵ:(none)
˵  ��:���۴����Ƿ��
**************************************************/
static void __stdcall AutoSendTimerProc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	if(!comm.fAutoSend || msg.hComPort==INVALID_HANDLE_VALUE){
		deal.cancel_auto_send(0);
		debug_out(("�Զ�ָ�����ͷ�\n"));
		memory.free_mem((void**)deal.autoptr,"AutoSendTimerProc");
		return;
	}

	do_send(2);
	
	UNREFERENCED_PARAMETER(uID);
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(dwUser);
	UNREFERENCED_PARAMETER(dw1);
	UNREFERENCED_PARAMETER(dw2);
	return;
}
void check_auto_send(void)
{
	int flag;
	int elapse;
	BOOL fTranslated;


	flag = IsDlgButtonChecked(msg.hWndMain, IDC_CHK_AUTO_SEND);
	if(!flag){
		deal.cancel_auto_send(0);
		return;
	}else{
		int len = GetWindowTextLength(GetDlgItem(msg.hWndMain,IDC_EDIT_SEND));
		if(len == 0){
			utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION,COMMON_NAME,
				"����������͵����ݺ���ѡ���Զ�����!");
			CheckDlgButton(msg.hWndMain,IDC_CHK_AUTO_SEND,FALSE);
			return;
		}
	}
	elapse = GetDlgItemInt(msg.hWndMain,IDC_EDIT_DELAY,&fTranslated,FALSE);
	if(!fTranslated || (elapse>60000||elapse<10)){
		utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION,COMMON_NAME,
			"�Զ�����ʱ�����ò���ȷ, �Զ����ͱ����!\nʱ�䷶ΧΪ10ms~60000ms");
		CheckDlgButton(msg.hWndMain,IDC_CHK_AUTO_SEND,FALSE);
		return;
	}
	
	EnableWindow(GetDlgItem(msg.hWndMain, IDC_EDIT_DELAY),FALSE);
	EnableWindow(GetDlgItem(msg.hWndMain, IDC_BTN_SEND),FALSE);

	if(msg.hComPort!=INVALID_HANDLE_VALUE){
		deal.autoptr = do_send(1);
		if(deal.autoptr==NULL){
			deal.cancel_auto_send(0);
			return;
		}
		deal.timer_id=timeSetEvent(elapse,0,AutoSendTimerProc,0,TIME_PERIODIC|TIME_CALLBACK_FUNCTION);
		comm.fAutoSend = 1;
	}
}

/**************************************************
��  ��:make_send_data
��  ��:���ڴ����ݹ���SEND_DATA���ݰ�
��  ��:	fmt,˵��data���ݵĸ�ʽ:
			0 - 16���ƻ��ַ�,dataָ����ͨ�ڴ�
			1 - dataָ��SEND_DATA*
		data:����
		size:���ݴ�С
��  ��:SEND_DATA* �� NULL(ʧ��)
˵  ��:
**************************************************/
SEND_DATA* make_send_data(int fmt,void* data,size_t size)
{
	SEND_DATA* psd = NULL;
	int is_buffer_enough = 0;

	//2013��11��2�� 16:33:13 ���������Բ�����
	if(msg.hComPort==NULL||msg.hComPort==INVALID_HANDLE_VALUE){
		return NULL;
	}

	if(fmt == 0){
		is_buffer_enough = size<=sizeof(((SEND_DATA*)NULL)->data);
		if(is_buffer_enough){
			psd = (SEND_DATA*)deal.do_buf_send(SEND_DATA_ACTION_GET,NULL);
			if(psd) psd->cb = sizeof(SEND_DATA)-sizeof(((SEND_DATA*)NULL)->data)+size;
		}else{
			int total = sizeof(SEND_DATA)+size-sizeof(((SEND_DATA*)NULL)->data);
			psd = (SEND_DATA*)GET_MEM(total);
			if(psd) psd->cb = total;
		}
	}else if(fmt == 1){ //dataΪSEND_DATA*,ֱ�Ӹ�������psd
		is_buffer_enough = size<=sizeof(SEND_DATA);
		if(is_buffer_enough){
			psd = (SEND_DATA*)deal.do_buf_send(SEND_DATA_ACTION_GET,NULL);
			if(psd) psd->cb = sizeof(SEND_DATA)-sizeof(((SEND_DATA*)NULL)->data)+size;
		}else{
			psd = (SEND_DATA*)GET_MEM(size);
			if(psd) psd->cb = size;
		}
	}
	if(!psd){
		deal.cancel_auto_send(0);
		utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION,"��ȵ�...","�����ٶȹ���,�ڲ����ͻ�����û�п���!\n\n"
			"����ѿ����Զ�����,���Զ����ͱ�ȡ��!");
		return NULL;
	}

	if(fmt == 0){
		memcpy(psd->data,data,size);
		psd->data_size = size;
		//psd->flag = ��Ҫ��������������
		psd->flag = is_buffer_enough?SEND_DATA_TYPE_USED:SEND_DATA_TYPE_MUSTFREE;
	}else if(fmt==1){
		memcpy(psd,deal.autoptr,((SEND_DATA*)deal.autoptr)->cb);
		psd->flag = is_buffer_enough?SEND_DATA_TYPE_AUTO_USED:SEND_DATA_TYPE_AUTO_MUSTFREE;
	}
	
	return psd;
}
//���Ժ�save_to_file���ϵ�һ��
int get_edit_data(int fmt,void** ppv,size_t* size)
{
	HWND hSend;			//���������ݵ�EditBox
	char* buff = NULL;	//�������������
	size_t len;			//buff len
	unsigned char* bytearray=NULL;

	hSend = GetDlgItem(msg.hWndMain,IDC_EDIT_SEND);
	len = GetWindowTextLength(hSend);
	if(len == 0){
		return 0;
	}
	buff = (char*)GET_MEM(len+1);
	if(buff==NULL) return 0;
	GetWindowText(hSend,buff,len+1);

	if(fmt){		//16���Ʒ�ʽ����
		int ret;
		int length;
		bytearray = NULL;
		ret = utils.str2hex(buff,&bytearray,0);
		if(!(ret&0x80000000)){
			if(comm.fAutoSend){
				deal.cancel_auto_send(0);
			}
			length = ret & 0x7FFFFFFF;
			utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION, NULL, "�����������ݽ�������, ����!\n\n�ǲ���ѡ���˷������ݵĸ�ʽ\?\n\n"
				"�ڵ� %d ���ַ����������﷨��������!",length);
			memory.free_mem((void**)&buff,NULL);
			return 0;
		}
		//������ȷ��������
		length = ret & 0x7FFFFFFF;
		len = length;
	}else{//�ַ���ʽ
		len = utils.wstr2lstr(buff);//���ذ���'\0'
		--len;
		//�������������2�� ���ݴ���ʽ, ��ԭlen�����Ч
		if(comm.data_fmt_ignore_return){
			len = utils.remove_string_return(buff);
		}
		if(comm.data_fmt_use_escape_char){
			unsigned int ret = utils.parse_string_escape_char(buff);
			if(ret & 0x80000000){
				len = ret & 0x7FFFFFFF;
			}else{
				if(comm.fAutoSend){
					deal.cancel_auto_send(0);
				}
				len = ret & 0x7FFFFFFF;
				utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION, NULL, "����ת���ַ���ʱ��������!\n\n"
					"�ڵ� %d ���ַ����������﷨��������!",len);
				memory.free_mem((void**)&buff,NULL);
				return 0;
			}
		}
		
	}
	//lenΪ16����/�ַ���������ʵ�ʳ���
	if(fmt){
		*ppv = bytearray;
		memory.free_mem((void**)&buff,"");
	}else{
		*ppv = buff;
	}
	*size = len;
	
	return 1;
}


void add_send_packet(SEND_DATA* psd)
{
	DWORD nWritten=0;
	if(WriteFile(deal.thread.hPipeWrite,&psd,4,&nWritten,NULL) && nWritten==sizeof(psd)){
		InterlockedExchangeAdd((volatile long*)&comm.cchNotSend,psd->data_size);
	}else{
		utils.msgerr(NULL,"��ӷ������ݰ�ʱ����!");
	}
	update_status(NULL);
}

/**************************************************
��  ��:do_send@4
��  ��:���͵����� && �Զ��������
��  ��:action-0:�ֶ�����,1-���Զ����͵���do_send����(��1��,����ȡ������),2-�Զ����͵ĺ�������
����ֵ:����ָ��
˵  ��:
	2013-05-11:
		�ظ�����ʱ�Ĳ����ĵ�������
		�ظ�����ʱ��do_send(1)�õ�����
		Ȼ��do_send(2)ȡ�����ݲ����Ƶ��µĻ�����������
	2013-10-15:
		����һʱ���,�Ķ�̫��,���Թ���,���ǼӼ���Ƿ�򿪴�����, ���Ѿ�������
**************************************************/
void* do_send(int action)
{
	void* pv = NULL;
	size_t size = 0;
	SEND_DATA* psd = NULL;

	if(msg.hComPort==NULL || msg.hComPort==INVALID_HANDLE_VALUE){
		utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION,NULL,"���ȴ򿪴����豸~");
		return NULL;
	}

	if(action==0 || action==1){
		if(get_edit_data(comm.data_fmt_send,&pv,&size)){

			psd = make_send_data(0,pv,size);
			memory.free_mem((void**)&pv,"do_send_0");
			if(psd == NULL) return NULL;
			if(action == 0){
				add_send_packet(psd);
			}else if(action == 1){
				return psd;
			}
		}
		return NULL;
	}else if(action == 2){
		psd = make_send_data(1,deal.autoptr,((SEND_DATA*)deal.autoptr)->cb);
		if(psd) add_send_packet(psd);
		return NULL;
	}
	return NULL;
}

/**************************************************
��  ��:send_char_data
��  ��:����һ��char����(���Խ������ݿ�)
��  ��:ch - �ַ�
��  ��:1-�ɹ�,0-ʧ��,-1-����ĳЩԭ��ȡ��(�Ƚϴ��ڲ�δ��)(�ɹ�)
˵  ��:
**************************************************/
int send_char_data(char ch)
{
	SEND_DATA* psd = NULL;
	if(msg.hComPort==NULL || msg.hComPort==INVALID_HANDLE_VALUE){
		return -1;
	}
	psd = make_send_data(0, &ch, 1);
	if(!psd) return 0;
	add_send_packet(psd);
	return 1;
}

/**************************************************
��  ��:start_timer@4
��  ��:������ʱ��
��  ��:start:!0-����,0-�ر�
����ֵ:(none)
˵  ��:
**************************************************/
static void __stdcall TimeProc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	unsigned char *second,*minute,*hour;
	char str[9];
	second = (unsigned char *)((unsigned long)&deal.conuter + 0);
	minute = (unsigned char *)((unsigned long)&deal.conuter + 1);
	hour   = (unsigned char *)((unsigned long)&deal.conuter + 2);
	if(++*second == 60){
		*second = 0;
		if(++*minute == 60){
			*minute = 0;
			if(++*hour == 24){
				*hour = 0;
			}
		}
	}
	sprintf(&str[0],"%02d:%02d:%02d",*hour,*minute,*second);
	SetDlgItemText(msg.hWndMain,IDC_STATIC_TIMER,str);
	UNREFERENCED_PARAMETER(uID);
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(dwUser);
	UNREFERENCED_PARAMETER(dw1);
	UNREFERENCED_PARAMETER(dw2);
}

void start_timer(int start)
{
	static UINT timer_id;
	if(start){
		InterlockedExchange((volatile long*)&deal.conuter,0);
		SetDlgItemText(msg.hWndMain,IDC_STATIC_TIMER,"00:00:00");
		timer_id=timeSetEvent(1000,0,TimeProc,0,TIME_PERIODIC|TIME_CALLBACK_FUNCTION);
		if(timer_id == 0){
			//...
		}
	}else{
		if(timer_id){
			timeKillEvent(timer_id);
			timer_id = 0;
		}
	}
}
