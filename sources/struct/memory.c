#include <Windows.h>
#define __MEMORY_C__
#include "memory.h"
#include "list.h"
#include "../utils.h"
#include "../debug.h"
#include "../about.h"
#include "../msg.h"

/**********************************************************
�ļ�����:memory.c/memory.h
�ļ�·��:./common/struct/memory.c/.h
����ʱ��:2013-07-14,09:55
�ļ�����:Ů������
�ļ�˵��:�ڴ����
**********************************************************/

static char* __THIS_FILE__ = __FILE__;

void init_memory(void)
{
	memory.manage_mem = manage_mem;
//#ifdef _DEBUG
	memory.get_mem_debug = get_mem_debug;
//#else
//	memory.get_mem = get_mem;
//#endif
	memory.free_mem = free_mem;
}

/**************************************************
��  ��:get_mem@4
��  ��:�����ڴ�,������
��  ��:size-������Ĵ�С
����ֵ:�ڴ���ָ��
˵  ��:
	2013-07-13:�޸ķ��䷽ʽ,������Գ���
**************************************************/
#pragma pack(push,1)
typedef struct {
	unsigned char sign_head[2];
//#ifdef _DEBUG
	char* file;
	int line;
//#else

//#endif
	size_t size;
	list_s entry;
	//unsigned char buffer[1];
}common_mem_context;
typedef struct {
	unsigned char sign_tail[2];
}common_mem_context_tail;
#pragma pack(pop)

/**************************************************
��  ��:manage_mem
��  ��:�ҵ��ڴ���������,���������ڴ�,�ͷŵ��ڴ�
��  ��:	pv - &list_entry
��  ��:
˵  ��:
**************************************************/
void manage_mem(int what,void* pv)
{
	static list_s list_head;
	static CRITICAL_SECTION critical_section;

	if(what==MANMEM_INTSERT || what==MANMEM_REMOVE){
		__try{
			EnterCriticalSection(&critical_section);
		}
		__except(EXCEPTION_EXECUTE_HANDLER){
			MessageBox(NULL,"�ڴ�δ��ȷ��ʼ��!",NULL,MB_ICONERROR);
		}
	}

	switch(what)
	{
	case MANMEM_INITIALIZE:
		list->init(&list_head);
		InitializeCriticalSection(&critical_section);
		return;
	case MANMEM_INTSERT:
		list->insert_tail(&list_head,(list_s*)pv);
		break;
	case MANMEM_REMOVE:
		switch(list->remove(&list_head,(list_s*)pv))
		{
		case 1:
			break;
		case 0:
			utils.msgbox(msg.hWndMain,MB_ICONERROR,NULL,"�޴˽��:%p",pv);
			break;
		case 2:
			utils.msgbox(msg.hWndMain,MB_ICONERROR,NULL,"�ڴ�����Ϊ��!");
			break;
		}
		break;
	case MANMEM_FREE:
		debug_out(("����ڴ�й©...\n"));
		if(!list->is_empty(&list_head)){
			int i=1;
#ifdef _DEBUG
			utils.msgbox(msg.hWndMain,MB_ICONERROR,NULL,"����δ���ͷŵ��ڴ�!\n\n���������ύ�ڴ������Ϣ~");
#endif
			//����free_mem���Ƴ�������,��������ֻ�ܱ���,�����Ƴ�
			while(!list->is_empty(&list_head)){
				list_s* node = list_head.next;
				common_mem_context* pc = list_data(node,common_mem_context,entry);
				void* user_ptr = (void*)((unsigned char*)pc+sizeof(*pc));
#ifdef _DEBUG
				utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION,NULL,
					"�ڴ���%d:\n\n"
					"�������ڴ������Ϣ:\n\n"
					"�����С:%u\n"
					"�����ļ�:%s\n"
					"�ļ��к�:%d\n",
					i++,pc->size,pc->file,pc->line);
#endif
				memory.free_mem(&user_ptr,"MANMEM_FREE");
			}
			list->init(&list_head);
			DeleteCriticalSection(&critical_section);
			return;
		}
		return;
	}
	LeaveCriticalSection(&critical_section);
}


//#ifdef _DEBUG
	void* get_mem_debug(size_t size,char* file,int line)
//#else
//	void* get_mem(size_t size)
//#endif
{
	size_t all = sizeof(common_mem_context)+size+sizeof(common_mem_context_tail);
	void* pv = malloc(all);
	common_mem_context* pc = (common_mem_context*)pv;
	void* user_ptr = (unsigned char*)pc+sizeof(*pc);
	common_mem_context_tail* pct = (common_mem_context_tail*)((unsigned char*)user_ptr+size);
	if(!pv){
		utils.msgbox(msg.hWndMain,MB_ICONERROR,NULL,"�ڴ�������");
		return NULL;
	}

	memset(pv,0,all);

	pc->sign_head[0] = 'J';
	pc->sign_head[1] = 'J';

	pc->size = size;

//#ifdef _DEBUG
	pc->file = file;
	pc->line = line;
	//debug_out(("�����ڴ�:%u �ֽ�\n�����ļ�:%s\n�ļ��к�:%d\n\n",pc->size,pc->file,pc->line));
//#endif

	pct->sign_tail[0] = 'J';
	pct->sign_tail[1] = 'J';

	manage_mem(MANMEM_INTSERT,&pc->entry);


	return user_ptr;
}

/**************************************************
��  ��:free_mem@8
��  ��:�ͷ��ڴ�����,�쳣����
��  ��:void** ppv:user_ptrָ��,char* prefix:˵��
����ֵ:
˵  ��:
**************************************************/
void free_mem(void** ppv,char* prefix)
{
	common_mem_context* pc = NULL;
	common_mem_context_tail* pct = NULL;
	if(ppv==NULL || *ppv==NULL)
	{
//#ifdef _DEBUG
		utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION,NULL,"memory.free_mem �ͷſ�ָ��, ����:%s",prefix);
//#endif
		return;
	}

	__try{
		pc = (common_mem_context*)((size_t)*ppv-sizeof(*pc));
		pct = (common_mem_context_tail*)((unsigned char*)pc + sizeof(common_mem_context) + pc->size);

		if((pc->sign_head[0]=='J'&&pc->sign_head[1]=='J') &&
			pct->sign_tail[0]=='J'&&pct->sign_tail[1]=='J')
		{
			manage_mem(MANMEM_REMOVE,&pc->entry);
			free(pc);
			*ppv = NULL;			
		}else{

//#ifdef _DEBUG
			manage_mem(MANMEM_REMOVE,&pc->entry);
			utils.msgbox(msg.hWndMain,MB_ICONERROR,"debug error",
				"���ͷ��ڴ�ǩ������ȷ!\n\n"
				"�ļ�:%s\n"
				"����:%d",pc->file,pc->line);
//#else
//			utils.msgbox(msg.hWndMain,MB_ICONERROR,"debug error","���ͷ��ڴ�ǩ������ȷ!");
//#endif
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
//#ifdef _DEBUG
		utils.msgbox(msg.hWndMain,MB_ICONERROR,COMMON_NAME,
			"%s:ָ�뱻������ͷ�,�뱨���쳣!\n\n"
			"�ļ�:%s\n"
			"����:%d",prefix?prefix:"<null-function-name>",pc->file,pc->line);
//#else
//		utils.msgbox(msg.hWndMain,MB_ICONERROR,"debug error","%s:ָ�뱻������ͷ�,�뱨���쳣!",prefix?prefix:"<null-function-name>");
//#endif
	}
}

