#include "cmd_dlg.h"
#include "msg.h"
#include "utils.h"
#include "comm.h"
#include "debug.h"
#include "deal.h"
#include "struct/memory.h"
#include "../res/resource.h"
#include "struct/parse_cmd.h"
#include "about.h"

static char* __THIS_FILE__ = __FILE__;

static HWND hList;
static HWND hSend;
static HWND hPrompt;

typedef struct{
	command_item* pci;
	size_t nitems;
}cmd_dlg_class;

void set_window_text(HWND hDlg,char* file)
{
	char* p = strrchr(file,'\\');
	char name[MAX_PATH+10];
	if(p) ++p;
	else p = file;
	
	_snprintf(name,sizeof(name),"��������:%s",p);
	SetWindowText(hDlg,name);
}

INT_PTR CALLBACK CmdProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	cmd_dlg_class* pcdc = (cmd_dlg_class*)GetWindowLong(hDlg,DWL_USER);
	switch(uMsg)
	{
	case WM_COMMAND:
		{
			if(LOWORD(wParam)==IDC_CMD_CMDLIST){
				if(HIWORD(wParam)==CBN_SELCHANGE){
					int sel = ComboBox_GetCurSel(hList);
					char prompt[1024];
					if(pcdc->pci[sel].method[0] == '0'){//16
						char hexa[CMDI_MAX_COMMAND]={0};
						int len = pcdc->pci[sel].bytes;
						if(pcdc->pci[sel].valid){
							utils.hex2str(pcdc->pci[sel].command,&len,0,0,hexa,sizeof(hexa));
						}
						_snprintf(prompt,sizeof(prompt),
							"��ʽ:ʮ������\r\n"
							"��С:%d �ֽ�\r\n"
							"����:%s%s\r\n",
							pcdc->pci[sel].bytes,
							pcdc->pci[sel].valid?"":"(��Ч)",
							pcdc->pci[sel].valid?hexa:(char*)pcdc->pci[sel].command
						);
					}else if(pcdc->pci[sel].method[0] == '1'){//char
						_snprintf(prompt,sizeof(prompt),
							"��ʽ:�ַ�\r\n"
							"��С:%d �ֽ�\r\n"
							"����:%s%s\r\n",
							pcdc->pci[sel].bytes,
							pcdc->pci[sel].valid?"":"(��Ч)",
							pcdc->pci[sel].command
						);
					}else{
						_snprintf(prompt,sizeof(prompt),
							"��ʽ:δ֪\r\n"
							"��С:%d �ֽ�\r\n"
							"����:%s\r\n",
							pcdc->pci[sel].bytes,
							pcdc->pci[sel].command
						);
					}
					SetDlgItemText(hDlg,IDC_CMD_DATA,prompt);
					SetDlgMsgResult(hDlg,uMsg,0);
					return 1;
				}
			}else if(LOWORD(wParam)==IDC_CMD_SEND){
				if(comm.fAutoSend){
					utils.msgbox(hDlg,MB_ICONEXCLAMATION,NULL,"����ֹͣ�Զ����ͷ���ʹ���ֶ�����!");
					break;
				}
				if(msg.hComPort==NULL || msg.hComPort==INVALID_HANDLE_VALUE){
					utils.msgbox(hDlg,MB_ICONEXCLAMATION,NULL,"���ȴ򿪴����ٷ�������!");
					break;
				}
				{
					int sel = ComboBox_GetCurSel(hList);
					if(sel == -1){
						utils.msgbox(hDlg,MB_ICONEXCLAMATION,NULL,"����ѡ��һ������!");
						break;
					}else{
						//���췢�����ݰ�
						if(pcdc->pci[sel].valid){
							SEND_DATA* psd = NULL;
							char* p = NULL;
							int len=0;

							if(pcdc->pci[sel].method[0] == '1'){//����Ч, ��Ϊ�ַ�, ����ת���ַ�
								len = strlen((char*)pcdc->pci[sel].command)+1;
								p=(char*)GET_MEM(len);
								memcpy(p,pcdc->pci[sel].command,len);
								pcdc->pci[sel].bytes = utils.parse_string_escape_char((char*)pcdc->pci[sel].command)&0x7FFFFFFF;
							}
							psd = deal.make_send_data(0,pcdc->pci[sel].command,pcdc->pci[sel].bytes);
							if(pcdc->pci[sel].method[0] == '1'){
								pcdc->pci[sel].bytes = len-1;
								memcpy(pcdc->pci[sel].command,p,len);
								memory.free_mem((void**)&p,NULL);
							}
							if(psd){
								deal.add_send_packet(psd);
							}
						}else{
							utils.msgbox(hDlg,MB_ICONEXCLAMATION,NULL,"��Ч������,���ܷ���!");
						}
						break;
					}
				}
			}
		}
		break;
	case WM_INITDIALOG:
		{
			cmd_dlg_class* p = (cmd_dlg_class*)GET_MEM(sizeof(cmd_dlg_class));
			if(p == NULL){
				utils.msgerr(msg.hWndMain,"�Ի����ʼ��ʧ��!");
				DestroyWindow(hDlg);
				//todo:
			}else{
				SetWindowLong(hDlg,DWL_USER,(LONG)p);
				pcdc = p;
			}
			
			hList = GetDlgItem(hDlg,IDC_CMD_CMDLIST);
			hSend = GetDlgItem(hDlg,IDC_CMD_SEND);
			hPrompt = GetDlgItem(hDlg,IDC_CMD_DATA);

			debug_out(("�����ļ�:%s\n",(char*)lParam));
			set_window_text(hDlg,(char*)lParam);

			SetWindowPos(hDlg,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);

			if(parse_cmd((char*)lParam,&pcdc->pci,&pcdc->nitems)){
				size_t it;
				if(pcdc->nitems == 0){
					utils.msgbox(hDlg,MB_ICONINFORMATION,COMMON_NAME,"��ѡ��������ļ��������κ�����!");
// 					DestroyWindow(hDlg);
// 					return FALSE;
				}
				for(it=0; it<pcdc->nitems; it++){
					ComboBox_AddString(hList,pcdc->pci[it].name);
				}
			}else{
				utils.msgbox(hDlg,MB_ICONERROR,NULL,"�������ʧ��!");
// 				DestroyWindow(hDlg);
// 				return FALSE;
			}
			return FALSE;
		}
	case WM_CLOSE:
		SendMessage(msg.hWndMain,WM_APP+0,1,(LPARAM)hDlg);
		DestroyWindow(hDlg);
		SetDlgMsgResult(hDlg,uMsg,0);
		return 1;
//�Ի���Ϊʲô�ղ��������Ϣ?
// 	case WM_NCCREATE:
// 		{
// 			cmd_dlg_class* p = (cmd_dlg_class*)GET_MEM(sizeof(cmd_dlg_class));
// 			if(p == NULL){
// 				utils.msgerr("�Ի����ʼ��ʧ��!");
// 				return FALSE;
// 			}else{
// 				SetWindowLong(hDlg,DWL_USER,(LONG)p);
// 				return TRUE;
// 			}
// 		}
	case WM_NCDESTROY:
		{
			memory.free_mem((void**)&pcdc->pci,"WM_NCDESTROY");
			memory.free_mem((void**)&pcdc,"WM_NCDESTROY");
			return 0;
		}
	}//switch
	return 0;
}

int showCmd(char* file)
{
	HWND hDlg = CreateDialogParam(msg.hInstance,MAKEINTRESOURCE(IDD_CMD),msg.hWndMain,CmdProc,(LPARAM)file);
	if(hDlg == NULL){
		utils.msgerr(msg.hWndMain,"�޷���������ģ��,ʧ��!");
		return 0;
	}
	ShowWindow(hDlg,SW_SHOW);
	SendMessage(msg.hWndMain,WM_APP+0,0,(LPARAM)hDlg);
	return 1;
}
