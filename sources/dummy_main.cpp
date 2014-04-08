#include <Windows.h>
#include <cassert>

#include "send_cmd.h"
#include "./struct/memory.h"
#include "utils.h"

//���ļ�����Ϊ�������ʹ��

int main(void)
{
	init_memory();
	init_utils();
	memory.manage_mem(MANMEM_INITIALIZE,NULL);

	//assert("����ʹ��" && 0);

	ASendCmd s1("E:\\Program\\Windows\\common\\bin\\command2.txt");
	ASendCmd s2("b.txt");

	MSG msg;
	while(GetMessage(&msg,NULL,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}
