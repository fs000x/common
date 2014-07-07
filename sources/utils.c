#define __UTILS_C__
#include "utils.h"
#include "msg.h"
#include "expr.h"
#include "about.h"
#include "debug.h"
#include "struct/memory.h"
#include "comm.h"
#include "../res/resource.h"

struct utils_s utils;

static char* __THIS_FILE__ = __FILE__;

void init_utils(void)
{
	memset(&utils, 0, sizeof(utils));
	utils.msgbox                   = msgbox;
	utils.msgerr                   = msgerr;
	utils.get_file_name            = get_file_name;
	utils.set_clip_data            = set_clip_data;
	utils.str2hex                  = str2hex;
	utils.hex2str                  = hex2str;
	utils.center_window            = center_window;
	utils.show_expr                = ShowExpr;
	utils.hex2chs                  = hex2chs;
	utils.assert_expr              = myassert;
	utils.wstr2lstr                = wstr2lstr;
	utils.check_chs                = check_chs;
	utils.remove_string_return     = remove_string_return;
	utils.remove_string_linefeed   = remove_string_linefeed;
	utils.parse_string_escape_char = parse_string_escape_char;
	utils.eliminate_control_char   = eliminate_control_char;
	return;
}

/***************************************************
��  ��:msgbox
��  ��:��ʾ��Ϣ��
��  ��:
	msgicon:��Ϣ���
	caption:�Ի������
	fmt:��ʽ�ַ���
	...:���
����ֵ:
	�û�����İ�ť��Ӧ��ֵ(MessageBox)
˵  ��:
***************************************************/
int msgbox(HWND hOwner,UINT msgicon, char* caption, char* fmt, ...)
{
	va_list va;
	char smsg[1024]={0};
	va_start(va, fmt);
	_vsnprintf(smsg, sizeof(smsg), fmt, va);
	va_end(va);
	return MessageBox(hOwner, smsg, caption, msgicon);
}
/***************************************************
��  ��:msgerr
��  ��:��ʾ��prefixǰ׺��ϵͳ������Ϣ
��  ��:prefix-ǰ׺�ַ���
����ֵ:(��)
˵  ��:
***************************************************/
void msgerr(HWND hOwner,char* prefix)
{
	char* buffer = NULL;
	if(!prefix) prefix = "";
	if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,GetLastError(),MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),(LPSTR)&buffer,1,NULL)) 
	{
		utils.msgbox(hOwner,MB_ICONHAND, NULL, "%s:%s", prefix, buffer);
		LocalFree(buffer);
	}
}

/***************************************************
��  ��:get_file_name
��  ��:��ʾ��/�����ļ��Ի���
��  ��:
	title:�Ի������
	filter:�Ի������ѡ��
	action:�򿪵Ķ���: 0-��,1-����
	opentype:0-16����,1-�ı�,2-����(����ʱ),��������
����ֵ:
	NULL:û����ȷ��ѡ��
	����:ѡ����ļ����ַ���
˵  ��:��ȥ,��һ��ʼ������(2013-02-06),���ҵ��Ǹ��޷���ʾ
	OFN_EXPLORER����ԭ��,��һ��_WIN32_WINNT��,������~
***************************************************/
UINT_PTR __stdcall OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
char* get_file_name(char* title, char* filter, int action, int* opentype)
{
	OPENFILENAME ofn = {0};
	int ret;
	static char buffer[MAX_PATH];
	*buffer = 0;
	ofn.lStructSize = sizeof(ofn);
	ofn.hInstance = msg.hInstance;
	if(action == 0) ofn.Flags = OFN_ENABLESIZING|OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_ENABLEHOOK|OFN_ENABLETEMPLATE;
	else ofn.Flags = OFN_PATHMUSTEXIST|OFN_ENABLESIZING|OFN_EXPLORER|OFN_NOREADONLYRETURN|OFN_ENABLETEMPLATE|OFN_ENABLEHOOK|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
	ofn.hwndOwner = msg.hWndMain;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = &buffer[0];
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = title;
	ofn.lpfnHook = OFNHookProc;
	ofn.lCustData = (LPARAM)opentype;
	ofn.lpTemplateName = MAKEINTRESOURCE(IDD_DLG_TEMPLATE);
	ret = action?GetSaveFileName(&ofn):GetOpenFileName(&ofn);
	return ret?&buffer[0]:NULL;
}

UINT_PTR CALLBACK OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND hParent = NULL;
	static int* popentype = NULL;
	if(uiMsg == WM_NOTIFY){
		LPOFNOTIFY pofn = (LPOFNOTIFY)lParam;
		if(pofn->hdr.code == CDN_FILEOK){
			HWND hCboOpenType = GetDlgItem(hdlg, IDC_HOOK_COMBO);
			int index = ComboBox_GetCurSel(hCboOpenType);
			if(index == -1){
				int id;
				id=MessageBox(GetParent(hdlg), 
					"����������б���ѡ���ļ���/����ķ�ʽ!\n\n"
					"����㲻��������ַ�ʽ������,����\"ȷ��(OK)\"�鿴��Ҫ�İ���!", 
					"δѡ���ļ�������ʽ", MB_ICONEXCLAMATION|MB_OKCANCEL);
				if(id==IDOK){
					//todo:��������˵��....
					char* helpmsg = 
						"(1)���ڼ����ļ�\n"
						"    (a)ѡ��ʮ������\n"
						"        ���ļ�Ϊ�����ʽ���ļ�,����Ὣ���ļ������ݽ���Ϊ16�������е��ı��ַ���\n"
						"    (b)ѡ���ı��ַ�\n"
						"        ���ļ�������Ч��16���������ı�,��Ч��16���������ı�Ϊÿ���ֽ�������16�����ַ�ָʾ���ı�,����ֱ�Ӱ����ŵ����ͻ�������,ֻ�����﷨���\n"
						"\n"
						"(2)���ڱ����ļ�:\n"
						"    (a)ѡ��ʮ������\n"
						"        <1>�����16���Ʒ�ʽ��ʾ:�����ѽ����������е��ı���ÿ�����ַ�Ϊ1�鱣�浽16�����ļ�,������ֱ�ӱ���16������������,��������յ�������ASCII��/��Ч����,�򱣴��,�Ϳ���ֱ�Ӵ򿪲���ʾ������\n"
						"        <2>������ַ���ʽ��ʾ:��������Ϊ16��������,��Ϊ��ʽ������Ҫ��\n"
						"    (b)ѡ���ı��ַ�\n"
						"        <1>�����16���Ʒ�ʽ��ʾ:�����ֱ�Ӱ�16���������ı����浽ָ�����ļ�,��ͽ�������������ʾ��һ��\n"
						"        <2>������ı��ַ���ʽ��ʾ:�����ֱ�ӱ����ı���ָ���ļ�,ͬ�����16����һ��";
					MessageBox(GetParent(hdlg),helpmsg,"���ڱ���/�򿪷�ʽ",MB_OK);
				}
				//2013-01-17
				SetFocus(hCboOpenType);
				SetWindowLong(hdlg, DWL_MSGRESULT, 1);
				return 1;
			}
			*popentype = index;
			return 0;
		}
	}else if(uiMsg == WM_SIZE){
		HWND hCboCurFlt = GetDlgItem(GetParent(hdlg), cmb1);
		HWND hCboOpenType = GetDlgItem(hdlg, IDC_HOOK_COMBO);
		RECT rcCboFlt;
		GetWindowRect(hCboCurFlt, &rcCboFlt);
		SetWindowPos(hCboOpenType,HWND_NOTOPMOST,0,0,rcCboFlt.right-rcCboFlt.left,rcCboFlt.bottom-rcCboFlt.top, SWP_NOMOVE|SWP_NOZORDER);
		return 0;
	}else if(uiMsg == WM_INITDIALOG){
		HWND hCboOpenType = GetDlgItem(hdlg, IDC_HOOK_COMBO);
		ComboBox_AddString(hCboOpenType, "�����ļ�, ��������(������ͨ�ı��ļ�)");
		ComboBox_AddString(hCboOpenType, "ʮ������, ����16�������е��ı��ļ�");
		//todo:����ʱ�Ŵ���
		ComboBox_AddString(hCboOpenType, "�����ļ�, ���������б���ı��ļ�");
		
		popentype = (int*)((OPENFILENAME*)lParam)->lCustData;
		return 0;
	}
	UNREFERENCED_PARAMETER(wParam);
	return 0;
}

/***************************************************
��  ��:set_clip_data
��  ��:����strָ����ַ�����������
��  ��:
	str:�ַ���,��0��β
����ֵ:
	�ɹ�:����
	ʧ��:��
˵  ��:
***************************************************/
int set_clip_data(char* str)
{
	HGLOBAL hGlobalMem = NULL;
	char* pMem = NULL;
	int lenstr;

	if(str == NULL) return 1;
	if(!OpenClipboard(NULL)) return 0;

	lenstr = strlen(str)+1;//Makes it null-terminated
	hGlobalMem = GlobalAlloc(GHND, lenstr);
	if(!hGlobalMem) return 0;
	pMem = (char*)GlobalLock(hGlobalMem);
	EmptyClipboard();
	memcpy(pMem, str, lenstr);
	SetClipboardData(CF_TEXT, hGlobalMem);
	CloseClipboard();
	GlobalFree(hGlobalMem);
	return 1;
}

/**************************************************
��  ��:str2hex
��  ��:ת��16�����ַ�����16����ֵ����
��  ��:
	str:ָ�����16���Ƶ��ַ���
	ppBuffer:unsigned char**,��������ת����Ľ���Ļ�����,��ָ��Ĭ�ϻ�����
	buf_size:������Ĭ�ϻ�����, ָ��Ĭ�ϻ������Ĵ�С(�ֽ�)
����ֵ:
	�ɹ�:���λΪ1,��31λ��ʾ�õ���16�������ĸ���
	ʧ��:���λΪ0,��31λ��ʾ�Ѿ��������ַ����ĳ���
˵  ��:�������sscanfӦ��Ҫ��Щ,�����տ�ʼд��ʱ��û���ǵ�
	2013-04-07����:����Ϊ�ʷ�������,�������16�������ݵ������
	2013-07-27����:����Ĭ�ϻ�����,ע��:
			ppBufferӦ��ָ��һ��ָ������ĵ�ַ,��ָ�������ֵΪĬ�ϻ�������NULL
			��ָ��Ĭ�ϻ�����,��ôbuf_sizeΪ����������
***************************************************/
enum{S2H_NULL,S2H_SPACE,S2H_HEX,S2H_END};

unsigned int str2hex(char* str, unsigned char** ppBuffer,unsigned int buf_size)
{
	unsigned char hex=0;			//������������ĵ���16����ֵ
	unsigned int count=0;			//���������16���Ƶĸ���
	unsigned char* hexarray;		//����ת����Ľ��
	unsigned char* pba;				//������hexarray��д����
	unsigned char* pp = (unsigned char*)str;	//���������ַ���

	int flag_last=S2H_NULL,flag;				//�ʷ������õ��ı��λ

	if(str==NULL) return 0;
	//������2���ַ�+���ɿհ����һ��16����, ������಻���ܳ���(strlen(str)/2)
	if(*ppBuffer && buf_size>=strlen(str)/2){
		hexarray = *ppBuffer;
	}else{
		hexarray = (unsigned char*)GET_MEM(strlen(str)/2);
		if(hexarray == NULL){
			*ppBuffer = NULL;
			return 0;
		}else{
			//�ŵ����,�ж��Ƿ���Ҫ�ͷ�
			//*ppBuffer = hexarray;
		}
	}
	pba = hexarray;

	for(;;){
		if(*pp == 0) 
			flag = S2H_END;
		else if(isxdigit(*pp)) 
			flag = S2H_HEX;
		else if(*pp==0x20||*pp==0x09||*pp=='\r'||*pp=='\n')
			flag = S2H_SPACE;
		else{
			//printf("�Ƿ��ַ�!\n");
			goto _parse_error;
		}

		switch(flag_last)
		{
		case S2H_HEX:
			{
				if(flag==S2H_HEX){
					hex <<= 4;
					if(isdigit(*pp)) hex += *pp-'0';
					else hex += (*pp|0x20)-87;
					*pba++ = hex;
					count++;
					flag_last = S2H_NULL;
					pp++;
					continue;
				}else{
					//printf("������!\n");
					goto _parse_error;
				}
			}
		case S2H_SPACE:
			{
				if(flag == S2H_SPACE){
					pp++;
					continue;
				}else if(flag == S2H_HEX){
					if(isdigit(*pp)) hex = *pp-'0';
					else hex = (*pp|0x20)-87;  //'a'(97)-->10
					pp++;
					flag_last = S2H_HEX;
					continue;
				}else if(flag == S2H_END){
					goto _exit_for;
				}
			}
		case S2H_NULL:
			{
				if(flag==S2H_HEX){
					if(isdigit(*pp)) hex = *pp-'0';
					else hex = (*pp|0x20)-87;
					pp++;
					flag_last = S2H_HEX;
					continue;
				}else if(flag == S2H_SPACE){
					flag_last = S2H_SPACE;
					pp++;
					continue;;
				}else if(flag==S2H_END){
					goto _exit_for;
				}
			}
		}
	}
_parse_error:
	if(hexarray != *ppBuffer){
		memory.free_mem((void**)&hexarray,"<utils.str2hex>");
	}
	return 0|((unsigned int)pp-(unsigned int)str);
_exit_for:
	//printf("������:%d\n",pba-(unsigned int)ba);
	*ppBuffer = hexarray;
	return count|0x80000000;
}

/**************************************************
��  ��:hex2chs
��  ��:ת��16�������鵽�ַ��ַ���
��  ��:	hexarray - 16��������
		length - ����
		buf - Ĭ�ϻ���ռ�
		buf_size - Ĭ�Ͽռ��С
����ֵ:�ַ���
˵  ��:2013-03-10:���˺ܶ��޸�,�������ٶ���
2013-03-23 ����:
	��C���Ե����Ƕ���ϰ����ʹ��'\n'��Ϊ���з�,��Ҳ����ʹ��,
��ƫƫWindows�ı༭����'\r\n'��Ϊ���з�,û�а취

2014-07-07 ����:
	��������������, ֻʹ��\r����, ��TM���ٱ�׼һ��ô!!!
		����������ͳһ����Ҫ��:
			�� \r ����û�� \n
			�� \n 
			�� \r\n
			�� \r\n\r
			������������������, ����Ϊһ�����з�����
	ͻȻ����, ��ʵ���������ȫ�����������(���һ������)��'\0'.
**************************************************/
char* hex2chs(unsigned char* hexarray,int length,char* buf,int buf_size)
{
	char* buffer=NULL;
	int total_length;
	int line_r=0;
	int line_rnr=0;
	int z_cnt=0;
	int len_addend=0;
	//��������ǰ����������ֵĸ���(��3�ֲ���Ҫ����)
	do{
		int len=0;
		for(line_r=0,line_rnr=0; len<length; len+=len_addend+1){
			len_addend=0;
			if(hexarray[len]=='\r'){
				if(len<length-1){
					if(hexarray[len+1]!='\n'){
						line_r++;
					}
					else{
						len_addend++; // skip next '\n'
						if(len<length-2){
							if(hexarray[len+2]=='\r'){
								if(len<length-3){
									if(hexarray[len+3]!='\n'){
										line_rnr++;
										len_addend ++; //skip next '\r'
									}
								}
								else{
									line_rnr++;
									len_addend ++;
								}
							}
						}
					}
				}
				else{
					line_r++;
				}
			}
			else if(hexarray[len]=='\n'){
				line_r++;
			}
			else if(hexarray[len]==0){
				z_cnt++;
			}
		}
	}while((0));

	total_length = 0
		+ length*1 - line_r*1 - line_rnr*3	// ���� ǰ������� �Ѿ��ڼ���ת��Ϊ'\r\n'ʱ�����
		+ line_r * 2						// ÿ�� ǰ�������֮һ �ᱻת���� '\r\n'
		+ line_rnr * 2						// ÿ�� '\r\n\r' ���� '\r\n'
		- z_cnt								// 0 ����Ҫ��������
		+ 1									// ��'\0'��β
		;
	if(total_length<=buf_size && buf){
		buffer = buf;
		//memset(buffer,0,buf_size);
	}else{
		buffer = (char*)GET_MEM(total_length);
		if(!buffer) return NULL;
	}

	// ת��ǰ������� + ����'\0'
	do{
		unsigned char* pch=(unsigned char*)buffer;
		int itx,itx_addend;
		for(itx=0; itx<length; itx+=itx_addend+1){
			itx_addend=0;
			if(hexarray[itx]=='\r'){
				*pch++ = '\r';
				*pch++ = '\n';

				if(itx<length-1){
					if(hexarray[itx+1]=='\n'){
						itx_addend++;
						if(itx<length-2){
							if(hexarray[itx+2]=='\r'){
								if(itx<length-3){
									if(hexarray[itx+3]!='\n'){
										itx_addend++;
									}
								}
							}
						}
					}
				}
			}
			else if(hexarray[itx]=='\n'){
				*pch++ = '\r';
				*pch++ = '\n';
			}
			else if(hexarray[itx]==0){

			}
			else{
				*pch++ = hexarray[itx];
			}
		}
// 		if((((unsigned int)pch-(unsigned int)buffer) != total_length-1))
// 		{
// 			utils.assert_expr(NULL,"");
// 		}
	}while((0));
	buffer[total_length-1] = '\0';
	return buffer;
}

/*************************************************
��  ��:hex2str
��  ��:ת��16����ֵ���鵽16�����ַ���
��  ��:
	hexarray:16��������
	*length:16�������鳤��
	linecch:ÿ�е�16���Ƶĸ���,Ϊ0��ʾ������
	start:��ʼ�ڵڼ���16��������
	buf:Ĭ�Ͽռ�,����ռ��С����,����ô˿ռ�
	buf_size:�ռ��С
����ֵ:
	�ɹ�:�ַ���ָ��(�������Ĭ�ϻ�����,��Ҫ�ֶ��ͷ�)
	ʧ��:NULL
	*length ���ط����ַ����ĳ���
˵  ��:
	2013-03-05:����, ���ڿ�����ӽ�Ƶ��,��ÿ�ε������ֺ���,
�������ڿ��Դ����û�����Ļ���������������,������ֵ==buf,
˵���û��ռ䱻ʹ��
	2013-03-10:��ǰ�ټ���һ��:*pb='\0'; 
		���½�����������ʾ����(����������ȷ),���˺þòŷ���....
**************************************************/
char* hex2str(unsigned char* hexarray, int* length, int linecch, int start, char* buf, int buf_size)
{
	char* buffer = NULL;
	char* pb = NULL;
	int count = start;
	int total_length;
	//int ret_length=0;
	int k;

	//2013-01-17���¼������:
	//	ÿ�ֽ�ռ��2��ASCII+1���ո�:length*3
	//  �����ַ�ռ��:length/linecch*2
	if(linecch){
		total_length = *length*3 + *length/linecch*2+1+2;//+1:���1��'\0';+2:�����ǵ�1��\r\n
	}else{
		total_length = *length*3+1+2;//+1:���1��'\0';+2:�����ǵ�1��\r\n
	}
	if(buf_size>=total_length && buf){
		buffer = buf;
		//memset(buffer,0,buf_size);
	}else{
		buffer=(char*)GET_MEM(total_length);
		if(buffer == NULL) return NULL;
	}
	//memset(buffer,0,total_length);
	for(k=0,pb=buffer; k<*length; k++){
		sprintf(pb, "%02X ", hexarray[k]);
		pb += 3;
		//���д���
		if(linecch && ++count == linecch){
			pb[0] = '\r';
			pb[1] = '\n';
			pb += 2;
			count = 0;
		}
	}
	//2013-03-10:��ǰ�ټ���һ��:*pb='\0'; 
	//���½�����������ʾ����(����������ȷ),���˺þòŷ���....
	*pb = '\0';
	*length = pb-buffer;
	return buffer;
}

/**************************************************
��  ��:center_window@8
��  ��:��ָ�����ھ�����ָ������һ����
��  ��:	hWnd - �����еĴ��ھ��
		hWndOwner - �ο����ھ��
����ֵ:
˵  ��:����������Ļ,��hWndOwnerΪNULL
**************************************************/
void center_window(HWND hWnd, HWND hWndOwner)
{
	RECT rchWnd,rchWndOwner;
	int width,height;
	int x,y;

	if(!IsWindow(hWnd)) return;
	GetWindowRect(hWnd,&rchWnd);

	if(!hWndOwner||!IsWindow(hWndOwner)){
		int scrWidth,scrHeight;
		scrWidth = GetSystemMetrics(SM_CXSCREEN);
		scrHeight = GetSystemMetrics(SM_CYSCREEN);
		SetRect(&rchWndOwner,0,0,scrWidth,scrHeight);
	}else{
		GetWindowRect(hWndOwner,&rchWndOwner);
	}
	width = rchWnd.right-rchWnd.left;
	height = rchWnd.bottom-rchWnd.top;
	
	x = (rchWndOwner.right-rchWndOwner.left-width)/2+rchWndOwner.left;
	y = (rchWndOwner.bottom-rchWndOwner.top-height)/2+rchWndOwner.top;

	MoveWindow(hWnd,x,y,width,height,TRUE);
}

/***********************************************************************
����:Assert
����:Debug
����:pv-�κα��ʽ,str-��ʾ
����:
˵��:
***********************************************************************/
void myassert(void* pv,char* str)
{
	if(!pv){
		utils.msgbox(msg.hWndMain,MB_ICONERROR,COMMON_NAME,"Debug Error:%s\n\n"
			"Ӧ�ó��������ڲ�����,�뱨�����!\n\n"
			"����������Ӧ�ó��������!",str);
	}
}

/***********************************************************************
����:wstr2lstr (Windows -> Linux (Standard new line format))
����:ת��'\r\n'�س����и�ʽ���ַ�����'\n'��ʽ
����:src-��ת�����ַ���
����:0-ʧ��,����:ת����ĳ���,����'\0'
˵��:ԭ�ַ������޸�,src����ָ����޸��ڴ�
***********************************************************************/
int wstr2lstr(char* src)
{
	char* pdst = src;
	char* psrc = src;
	if(pdst == NULL){
		debug_out(("utils.wstr2lstr:src == NULL!(file:%s,line:%d)\n",__FILE__,__LINE__));
		return 0;
	}
	for(;;){
		if(*psrc=='\r' && *(psrc+1) && *(psrc+1)=='\n'){
			*pdst++ = '\n';
			psrc += 2;
		}else{
			*pdst++ = *psrc;
			if(!*psrc){
				break;
			}else{
				psrc++;
			}
		}
	}
	return (unsigned int)(pdst-(unsigned int)src);
}

/***********************************************************************
����:check_chs
����:���ba�����е������Ƿ�Ϊ�Ϸ�����Ӣ������,����Ϊ����>=0x80��ֵ,Ӣ��С��0x80
����:ba-����,cb�ֽ���
����:!0-�����Ҫһ�������ַ�,0-�Ϸ�
˵��:�����⵽һ��>0x7F+һ��С�ڵ���0x7F,����ڵĻᱻ�ĳ�'?'
***********************************************************************/
int check_chs(unsigned char* ba, int cb)
{
	int it;
	enum{
		CHARFMT_NULL,
		CHARFMT_ASCII,
		CHARFMT_OEMCP
	};
	int flag=CHARFMT_NULL;
	int flag_current=CHARFMT_NULL;
	for(it=0; it<cb;it++){
		flag_current = ba[it]<=0x7F?CHARFMT_ASCII:CHARFMT_OEMCP;
		switch(flag)
		{
		case CHARFMT_NULL:
			flag = flag_current;
			break;
		case CHARFMT_ASCII:
			if(flag_current == CHARFMT_ASCII){
				continue;
			}else if(flag_current == CHARFMT_OEMCP){
				flag = CHARFMT_OEMCP;
			}
			break;
		case CHARFMT_OEMCP:
			if(flag_current == CHARFMT_ASCII){
				ba[it-1] = '?';
				flag = CHARFMT_ASCII;
			}else if(flag_current == CHARFMT_OEMCP){
				flag = CHARFMT_NULL;
			}
			break;
		}
	}
	return flag_current==CHARFMT_OEMCP && flag==CHARFMT_OEMCP;
}

/**************************************************
��  ��:remove_string_return
��  ��:�Ƴ��ַ����е� �س�����
��  ��:str - ���޳��س����е��ַ���
��  ��:����ַ����ĳ���
˵  ��: '\r','\n' �����޳�
**************************************************/
unsigned int remove_string_return(char* str)
{
	char* p1 = str;
	char* p2 = str;

	while(*p2){
		if(*p2=='\r' || *p2=='\n'){
			p2++;
		}else{
			*p1++ = *p2++;
		}
	}
	*p1 = '\0';
	return (unsigned int)p1-(unsigned int)str;
}

/**************************************************
��  ��:remove_string_linefeed
��  ��:�Ƴ��ַ����е� '\r'
��  ��:str - ���޳��س����е��ַ���
��  ��:����ַ����ĳ���
˵  ��: 
**************************************************/
unsigned int remove_string_linefeed(char* str)
{
	char* p1 = str;
	char* p2 = str;

	while(*p2){
		if(*p2=='\r'){
			p2++;
		}else{
			*p1++ = *p2++;
		}
	}
	*p1 = '\0';
	return (unsigned int)p1-(unsigned int)str;
}

/**************************************************
��  ��:parse_string_escape_char
��  ��:�������ַ����е�ת���ַ�
��  ��:str - ���������ַ���
��  ��:
	1.������ȫ���ɹ�:
		���λΪ1,����λΪ��������ַ�������
	2.������ʱ��������:
		���λΪ0,����λΪ����ֱ������ʱ�ĳ���
˵  ��:
	1.֧�ֵ��ַ���ת���ַ�:
		\r,\n,\t,\v,\a,\b,\\
	2.֧�ֵ�16����ת���ַ���ʽ:
		\x?? - ����һ���ʺŴ���һ��16�����ַ�, ����ʡ����һ,
		���豣֤4���ַ��ĸ�ʽ
	3.'?',''','"', ��print-able�ַ�����Ҫת��
	4.Դ�ַ����ᱻ�޸� - һֱ��ϰ����const����, ��ע������
**************************************************/
unsigned int parse_string_escape_char(char* str)
{
	char* p1 = str;
	char* p2 = str;

	while(*p2){
		if(*p2 == '\\'){
			p2++;
			switch(*p2)
			{
			case '\\':*p1++ = '\\';p2++;break;
			case 'b':*p1++  = '\b';p2++;break;
			case 'a':*p1++  = '\a';p2++;break;
			case 'v':*p1++  = '\v';p2++;break;
			case 't':*p1++  = '\t';p2++;break;
			case 'n':*p1++  = '\n';p2++;break;
			case 'r':*p1++  = '\r';p2++;break;
			case 'x'://����Ƿ�Ϊ2��16�����ַ�
				{
					p2++;
					if(*p2 && *(p2+1)){
						if(isxdigit(*p2) && isxdigit(*(p2+1))){
							unsigned char hex =*p2-'0';
							hex = (hex<<4) + (*(p2+1)-'0');
							*(unsigned char*)p1 = hex;
							p1++;
							p2 += 2;
						}else{
							goto _error;
						}
					}else{
						goto _error;
					}
					break;
				}
			case '\0':
				goto _error;
				break;
			default:
				goto _error;
			}
		}else{
			*p1++ = *p2++;
		}
	}
	*p1 = '\0';
	return 0x80000000|(unsigned int)p1-(unsigned int)str;

_error:
	return (unsigned int)p2-(unsigned int)str & 0x7FFFFFFF;
}

/**************************************************
��  ��: eliminate_control_char
��  ��: �����ַ���str�еĿ����ַ�, ��ǰֻ����\bɾ���ַ�
��  ��: str - ��������ַ���
��  ��: ������Ҫ����ɾ�����ַ��ĸ���
˵  ��: ���ڿ��ܶ��\b����һ��, ����Ӧ�ø��ݷ���ֵ��ȷ����Ҫ��ǰɾ��
	���ٸ��ַ�; \bֻ���ܳ��������еķ�\b֮ǰ;
	����: "\b\bABC\b" ��������2
	��������������ʹ�ô˺���:
	char str[] = "xxx";
	unsigned int i = eliminate_control_char(str);
	char* p = str;
	do{
		if(*p != '\b'){
			׷�����;
			//һ�����������˳�ѭ��,�����޸�p
		}
		else{
			��ǰɾ��һ���Ѿ����ڵ��ַ�
			p++;
		}
	while(i--);
**************************************************/
unsigned int eliminate_control_char(char* str)
{
	unsigned int unhandled = 0;
	char* p = str;
	char* q = str+1;

	if(!p || !*p)
		return unhandled;

	while(*p){
		if(*p=='\b'){
			unhandled++;
			break;
		}
		p++;
	}
	if(!unhandled)
		return 0;

	unhandled = 0;
	p = str;

	if(*p=='\b') 
		unhandled++;

	while(*q){
		if(*q != '\b'){
			*++p = *q++;
		}
		else{
			while(*q && *q=='\b'){
				if(p>=str && *p!='\b'){
					--p;
					++q;
				}
				else{
					unhandled++;
					*++p = *q++;
				}
			}
		}
	}
	*++p = '\0';
	return unhandled;
}
