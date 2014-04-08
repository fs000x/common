#include "../debug.h"
#include "../utils.h"
#include "memory.h"
#include "parse_cmd.h"

/**********************************************************
�ļ�����:parse_cmd.c/parse_cmd.h
�ļ�·��:./common/
����ʱ��:2013-07-26 21:20
�ļ�����:Ů������
�ļ�˵��:���ļ��������������͵����ݵ������ļ�,ʹ�÷������ļ�ĩβ��main����
	ע��:
		1.������ַ�����ʽ, Ĭ�Ͽ���ת���ַ�����
		2.�������ݱ���д��һ��, ���ܻ���,Ҫ������ʹ�� '\n'

	�����ļ���ʽ:
		��ʽ:����:��������(��д��ͬһ��)
	��ʽ:
		��������Ϊ16��������,��:00 12 34 56
		��������Ϊ�ַ�����,��:abcdefg\r\n12345
	����:
		��ǰ���������
	��������:
		16����������ַ�����

	��ʽ,����,�������ݵ���󳤶ȼ��궨��
**********************************************************/

static char* __THIS_FILE__ = __FILE__;

#define FREE_MEM(p,s) memory.free_mem((void**)(p),s)

int read_file_content(char* file,char** pbuf,size_t* size)
{
	FILE* fp = NULL;
	size_t file_size = 0;
	char* buffer = NULL;

	fp = fopen(file,"rb");	//Must be "rb", never use "rt"
	if(fp == NULL){
		return 0;
	}
	fseek(fp,0,SEEK_END);
	file_size = ftell(fp);
	fseek(fp,0,SEEK_SET);
	
	//�������ı��ļ�,��һ���ֽ��Ա�֤���һ���ֽ�Ϊ'\0'
	file_size++;
	buffer = (char*)GET_MEM(file_size);
	if(buffer == NULL){
		fclose(fp);
		return 0;
	}
	memset(buffer,0,file_size);
	fread(buffer,1,file_size-1,fp);
	fclose(fp);
	*pbuf = buffer;
	*size = file_size-1;/////2013��11��2�� 18:38:16 BUGBUGBUGBUG  MBMB :((( file_size
	return 1;
}



#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*(a)))


int parse_command_list(char* buffer,size_t size,command_item** ppci,size_t* nitems)
{
	char* ptr = NULL;
	char ch;

	enum{
		PARSE_INVALID,
		PARSE_METHOD,
		PARSE_NAME,
		PARSE_COMMAND,
		PARSE_COMMENT,
	};
	//int flag = PARSE_METHOD;
	int flag_last = PARSE_METHOD;
	int it=0;

	int it_of_item = 0;
	//command_item* *ppci = (command_item**) .... ���浱���ṹ��ʹ��ֻ����һ���ڴ�
	command_item* pci = (command_item*)GET_MEM(sizeof(command_item)*CMDI_MAX_LINES);
	if(pci == NULL){
		return 0;
	}

	*ppci = NULL;
	*nitems = 0;

	memset(pci,0,sizeof(command_item)*CMDI_MAX_LINES);
	for(ptr=buffer;;){
		ch = *ptr;
		switch(flag_last)
		{
		case PARSE_METHOD:
			{
				if(it_of_item >= CMDI_MAX_LINES){
					goto _exit_for;
				}
				switch(ch)
				{
				case 'C':
				case 'H':
					if(it < ARRAY_SIZE(pci[0].method)-1){
						pci[it_of_item].method[it++] = ch;
					}
					ptr++;
					break;
				case '#':
					if(it == 0){
						flag_last = PARSE_COMMENT;
					}
					ptr++;
					break;
				case '\r':
				case '\n':
					ptr++;
					if(*ptr=='\r' || *ptr=='\n'){
						ptr++;
					}
					it = 0;
					flag_last = PARSE_METHOD;
					break;
				case '\0':
					goto _exit_for;
				case ':':
					if(it <= ARRAY_SIZE(pci[0].method)-1){
						pci[it_of_item].method[it++] = 0;
					}
					flag_last = PARSE_NAME;
					it = 0;
					ptr++;
					break;
				default:
					if(it < ARRAY_SIZE(pci[0].method)-1){
						pci[it_of_item].method[it++] = ch;
					}else if(it == ARRAY_SIZE(pci[0].method)-1){
						pci[it_of_item].method[it++] = 0;
					}
					ptr++;
					break;
				}
				continue;
			}
		case PARSE_NAME:
			{
				switch(ch)
				{
				case ':':
					if(it <= ARRAY_SIZE(pci[0].name)-1){
						pci[it_of_item].name[it++] = 0;
					}
					flag_last = PARSE_COMMAND;
					ptr++;
					it = 0;
					break;
				case '\r':
				case '\n':
					ptr++;
					if(*ptr=='\r' || *ptr=='\n'){
						ptr++;
					}
					if(it <= ARRAY_SIZE(pci[0].name)-1){
						pci[it_of_item].name[it++] = 0;
					}
					it = 0;
					flag_last = PARSE_METHOD;
					break;
				case '\0':
					if(it <= ARRAY_SIZE(pci[0].name)-1){
						pci[it_of_item].name[it++] = 0;
					}
					it_of_item++;
					goto _exit_for;
					break;
				default:
					if(it < ARRAY_SIZE(pci[0].name)-1){
						pci[it_of_item].name[it++] = ch;
					}else if(it == ARRAY_SIZE(pci[0].name)-1){
						pci[it_of_item].name[it++] = 0;
					}
					ptr++;
					break;
				}
				continue;
			}
		case PARSE_COMMAND:
			{
				if(ch=='\r' || ch=='\n'){
					ptr++;
					if(ptr[0] == '\r' || ptr[0] == '\n'){
						ptr++;
					}
					if(it <= ARRAY_SIZE(pci[0].command)-1){
						pci[it_of_item].command[it++] = 0;
					}
					it = 0;
					it_of_item++;
					flag_last = PARSE_METHOD;
					continue;
				}else if(ch==0){
					if(it <= ARRAY_SIZE(pci[0].command)-1){
						pci[it_of_item].command[it++] = 0;
					}
					it_of_item++;
					goto _exit_for;
				}else{
					if(it < ARRAY_SIZE(pci[0].command)-1){
						pci[it_of_item].command[it++] = ch;
					}else if(it == ARRAY_SIZE(pci[0].command)-1){
						pci[it_of_item].command[it++] = 0;
					}
					ptr++;
					continue;
				}
			}
		case PARSE_COMMENT:
		case PARSE_INVALID:
			{
				if(ch=='\r' || ch=='\n'){
					ptr++;
					if(ptr[0]=='\r' || ptr[0]=='\n'){
						ptr++;
					}
					flag_last = PARSE_METHOD;
					continue;
				}else if(ch==0){
					goto _exit_for;
				}else{
					ptr++;
					continue;
				}
			}
		}//switch
	}//for
_exit_for:
	if(it_of_item < CMDI_MAX_LINES){
		//re-alloc
	}
	//todo:
	{//////////////////////////////////////////////////////////////////////////
		//2013��11��2�� 14:35:20
		//���ڿ��ǵ��û����ܻ��ڱ༭������ʱ�޸���������, ��������Ч��Ҳû�����˼, ע�͵����ִ���~
		// 

// 		int x;
// 		for(x=0; x<it_of_item; x++){
// 			make_command_item(pci+x);
// 		}

	}//////////////////////////////////////////////////////////////////////////
	*ppci = pci;
	*nitems = it_of_item;
	return 1;
}

int make_command_item(command_item* pci)
{
	int ret;
	unsigned char hexa[CMDI_MAX_COMMAND];//tmp hex array
	unsigned char* hex_ptr = hexa;

	if(pci->method[0] == 'H'){//16
		ret = utils.str2hex((char*)pci->command,&hex_ptr,sizeof(hexa));
		if(ret & 0x80000000){
			size_t len = ret & 0x7FFFFFFF;
			pci->valid = 1;
			pci->bytes = len;
			//memcpy(pci->command,hex_ptr,len);
			memcpy(pci->data,hex_ptr,len);

			if(hex_ptr != hexa){
				memory.free_mem((void**)&hex_ptr,"make_command_item");
			}
		}else{
			pci->valid = 0;
			utils.msgbox(NULL,MB_ICONERROR,NULL,"���� %s ���������ݲ�����ȷ��16��������!",pci->name);
		}

	}else if(pci->method[0] == 'C'){//char
		unsigned int ret;
		int len;

		memcpy(pci->data,pci->command,sizeof(pci->command));

		ret = utils.parse_string_escape_char((char*)pci->data);
			
		len = ret & 0x7FFFFFFF;

		if(ret & 0x80000000){
			pci->bytes = len;//bytes��ֵΪ��Чʱ��ֵ, ����Ч����Ч, ����valid�ж�
			pci->valid = 1;
		}else{
			//pci->bytes = strlen((char*)pci->command);
			utils.msgbox(NULL,MB_ICONEXCLAMATION, NULL, "�������� %s ʱ����, �������Ч!\n\n"
				"�ڵ� %d ���ַ����������﷨��������!",pci->name,len);
			pci->valid = 0;
		}
	}else{
		pci->valid = 0;
		utils.msgbox(NULL,MB_ICONERROR,NULL,"���� %s ���������Ͳ���ȷ!",pci->name);
	}
	return pci->valid != 0;
}

int parse_cmd(char* file,command_item** ppci,size_t* pnItems)
{
	char* buf = NULL;
	size_t size = 0;
	if(read_file_content(file,&buf,&size)){
		parse_command_list(buf,size,ppci,pnItems);
		FREE_MEM(&buf,"");
		return 1;
	}else{
		*ppci = NULL;
		return 0;
	}
}

#if 0
int main(void)
{
	command_item* pci = NULL;
	size_t size = 0;
	size_t i;
	if(parse_cmd("command.txt",&pci,&size)){
		for(i=0; i<size; i++){
			printf("method:%c\nname:%s\ncommand:%s\n\n",pci[i].method[0],pci[i].name,pci[i].command);
		}
		FREE_MEM(&pci,"");
	}
	return 0;
}
#endif
