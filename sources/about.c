#define __ABOUT_C__
#include "about.h"
#include "msg.h"
#include "utils.h"
#include "../res/resource.h"

#include <Windows.h>
#include <WinInet.h>

#pragma comment(lib,"WinInet")

struct about_s about;
static char* __THIS_FILE__ = __FILE__;

void soft_update(void);

void init_about(void)
{
	memset(&about,0,sizeof(about));
	about.show   = show_about;
	about.update = soft_update;
}

void show_about(void)
{
	char* help_msg = 
		"软件说明:\r\n"
		"    1.软件用C语言/C++/SDK方式完成, 绿色小巧, 不会给系统带来任何的垃圾文件\r\n"
		"    2.能实现对串口通过的读取与写入,16进制与字符方式两种\r\n"
		"    3.支持文件发送,保存到文本\r\n"
		"    4.支持自动发送,自定义发送周期\r\n"
		"    5.自动识别已有串口,检测串口插入与移除\r\n"
		"    6.支持串口超时设置\r\n"
		"    7.支持DTR/RTS引脚电平控制\r\n"
		"    8.提供对驱动设置的支持\r\n"
		"    9.提供128个ASCII码表供代码转换时参考\r\n"
		"    10.提供小工具:字符串转16进制数组\r\n"
		"    11.提供表达式求值计算器,包括基本算术/逻辑运算\r\n"
		"    12.接收字符数据时, 对'\b'控制字符支持\r\n"
		"-----------------------------------------------------\r\n"
		"软件参数:\r\n"
		"    最大发送文件大小:1MB(1048576字节)实际文件大小.\r\n"
		"    发送区最大文本大小:(同<最大发送文件大小)\r\n"
		"    数据接收区缓冲区大小(16进制与文本字符一样):10MB\r\n"
		"    停止显示后,内部保存数据缓冲区大小:1MB\r\n"
		"    读串口时,一次性最大读取大小:1MB\r\n"
		"-----------------------------------------------------\r\n"
		"使用帮助:\r\n"
		"    先设置好各个串口的参数之后打开串口进行读与写\r\n"
		"    一般设置(需参考硬件):\r\n"
		"        波特率:9600\r\n"
		"        校验位:无\r\n"
		"        数据位:8位\r\n"
		"        停止位:1位\r\n\r\n"
		"-----------------------------------------------------\r\n"
		"其它:\r\n"
		"    自动发送时间间隔范围:10ms~60000ms\r\n"
		"    开启自动发送后将不允许手动发送\r\n"
		"    若修改发送数据, 自动发送将被取消\r\n"
		"    16进制发送格式为:2个16进制位+1个空格\r\n"
		"-----------------------------------------------------\r\n"
		"更新:\r\n"
		"2012-12-24 1.0.0.0:\r\n"
		"    发布第1个版本\r\n"
		"2012-12-26:\r\n"
		"    自动识别当前存在,插入,移除的串口号\r\n"
		//"    关闭串口后接收与发送缓冲计数器现已自动清零(2013-01-05)"
		"2013-01-11 1.0.0.1:\r\n"
		"    增加保存接收区数据到文件(16进制/文本形式)\r\n"
		"    增加从文件读数据到发送区(16进制/文本形式)\r\n"
		"    增加暂停显示功能\r\n"
		"    增加复制发送/接收区数据到剪贴板\r\n"
		"2013-01-18 1.0.0.2:\r\n"
		"    修复:文本文件,16二进制文件读取错误\r\n"
		"    修复:程序内部缓冲区满后使程序进入死循环\r\n"
		"    修复:文本字符方式显示接收的数据时产生不正确的换行符的错误,若要产生换行符, 请使用\"\\n\"\r\n"
		"2013-02-08 1.0.0.3:\r\n"
		"    内部程序作了许多的优化工作,包含数据的发送方式等\r\n"
		"    修复接收数据时鼠标在接收区的文本选择造成的干扰\r\n"
		"2013-02-14 1.0.0.4:\r\n"
		"    增加显示出0~127号ASCII对应8,10,16进制功能\r\n"
		"2013-02-24 1.0.0.5,今天元宵节:\r\n"
		"    更改原来的1~64串口列表到自动检测计算机上的可用串口\r\n"
		"2013-02-27 1.0.0.6:\r\n"
		"    若发送文本,则自动发送被自动取消(若自动发送选项已打开)\r\n"
		"    在显示模式下不允许对接收区数据进行选择操作\r\n"
		"    提供硬件支持的串口设备设置\r\n"
		"    为用户提供串口超时设置\r\n"
		"    提供手动设置DTR/RTS引脚电平\r\n"
		"2013-03-01  1.0.0.7:\r\n"
		"    修改原计算器(系统)为表达式求值计算器(简单版本)\r\n"
		"2013-03-03:\r\n"
		"    添加:<其它>菜单添加<设备管理器>\r\n"
		"    修改:在关闭串口后自动发送前面的钩不再自动取消(如果已经选中)\r\n"
		"    修改:串口被关闭/移除后串口列表回到第一个串口设备的BUG\r\n"
		"2013-03-04:\r\n"
		"    修改:现在在串口列表中可以显示串口在设备管理器中的名字了\r\n"
		"    修正:无法显示 MSP430-FETUIF Debugger 的串口号(现在调用SetupApi更新列表)\r\n"
		"2013-03-05:\r\n"
		"    为了方便数据的统计与显示,16进制内容与字符内容被显示到不同的编辑框中\r\n"
		"2013-03-09 1.0.0.8:\r\n"
		"    修正在使用SetupApi枚举串口设备时未检测并口设备而造成的内存异常访问错误\r\n"
		"    减少在某些波特率(如:19200bps)下丢包严重的情况(如:MSP430串口),有时候还是会发生,等待修复.某些软件(如:SComAssistant"
		"采用每次只读一个字节的办法效果还行, 就是速度有点慢. 我改成了WaitCommEvent函数调用了(原来是Pending ReadFile),减少了CPU占用(有些串口驱动并不总是支持同步操作).\r\n"
		"    以前只管ReadFile+输出nRead字节,这里错误,ReadFile并不保证读取到要求的数据量后才返回,这里会导致严重丢包,WriteFile亦然.\r\n"
		"    速度减慢,但数据更完整\r\n"
		"2013-03-10 1.0.0.9:\r\n"
		"    修正:因为在格式化字符串的最后少写了一句 *pb = \'\\0\',导致接收区数据显示错误!"
		"    修复:对utils.hex2chs和add_text作了大量修改,大大减少数据丢包,貌似没有丢包?,细节处理参见源程序\r\n"
		"    1.0.0.8版本因为内部原因速度严重减慢, 1.0.0.9回到原来的快速!\r\n"
		"2013-03-18:\r\n"
		"    更正:若为字符显示方式,16进制方式保存不被允许,因为格式基本上不满足!\r\n"
		"2013-03-23 1.10:\r\n"
		"    添加:工作模式中,右键点击接收区字符文本框可以使能中文显示模式(不推荐),由于中文字符由两个字节构成,所以:一旦在某一次接收过程中只"
		"接收到了中文字符的一个字节,那么数据就会显示出错, 这个无法避免, 所以建议尽量不使能中文显示模式.\r\n"
		"    修正:用C语言的人们都习惯使用'\\n'作为换行符,我也这样使用,"
		"但偏偏Windows的编辑框以'\\r\\n'作为换行符,没有办法,我不得"
		"不把所有的'\\n'换成'\\r\\n',效率必然会下降,而且我不得不计算出"
		"\\n的个数先 --> 为了计算所需缓冲区的大小.\r\n"
		"    添加:现在可以显示出还未被发送出去的数据计数.\r\n"
		"    添加:新增计时器,打开串口后开始计时,关闭后停止计时.\r\n"
		"2013-03-25:\r\n"
		"    修正:大大减少中文乱码的问题.细节处理见代码.现在应该可以放心地使能中文显示模式了.\r\n"
		"    增加:字符串转16进制数组功能,工具菜单里面.\r\n"
		"2013-04-04:\r\n"
		"    修正:无法复制接收区字符文本的BUG.\r\n"
		"    小提示:在选择串口时,如果没有任何可用的串口,则进行提示更新.\r\n"
		"2013-04-07:\r\n"
		"    修改:完全修改了utils.str2hex的实现,大大增加了16进制输入的灵活性.\r\n"
		"现在的要求:每个16进制值由两个相邻字符组成,无其它限制.(以前是2个相邻字符+一个空格)\r\n"
		"2013-04-11:\r\n"
		"    发送字符数据时,对于换行,只发送\'\\n\',不再发送\'\\r\\n\',注意:结尾的\'\\0\\\'不被发送!\r\n"
		"2013-04-13:\r\n"
		"    修正:更改发送与接收方式为异步方式.\r\n"
		"    添加:简单波形显示.\r\n"
		"2013-04-23:\r\n"
		"    修正:发送与接收方式改回同步方式!坑~\r\n"
		"    修正:当发送操作达到100次时无法继续发送的BUG!\r\n"
		"    修改:优化内部线程同步机制,避免程序停止工作(失去响应)!\r\n"
		"    优化:优化自动发送数据的方式,提高精度,减小内存/CPU占用!\r\n"
		"    小提示:在加载/保存文件时,若不清楚打开/保存方式,可以查看简单的帮助信息!\r\n"
		"2013-05-11:明天母亲节\r\n"
		"    修正:终于找到一个比较好的办法来处理自动发送用到的重复数据了,呵呵,时间下限减少到10ms\r\n"
		"2013-07-05:\r\n"
		"    临时修正:选择从文件加载并取消后, 串口号选择的ComboBox会消失不见,不知道原因,临时已解决\r\n"
		"2013-07-14:\r\n"
		"    改进:程序内部改进内存分配算法,避免因程序错误造成内存泄漏\r\n"
		"2013-07-20 1.12:\r\n"
		"    (过渡版本,以后更新)\r\n"
		"2013-07-27:\r\n"
		"    细节:主窗口最小化后其它子窗口不自动最小化的问题\r\n"
		"2013-08-29:\r\n"
		"    修正:在设备管理器中更改串口的端口号后, 自动刷新串口列表\r\n"
		"    细节:修正在串口打开且允许显示接收到的数据时无法使用鼠标滚轮的小问题\r\n"
		"2013-08-30:\r\n"
		"    细节:根据用户要求,窗口大小现在可以变化; 如果不满意于接收/发送区的文本框过小, 可以左右拖动窗口以改变窗口大小\r\n"
		"    细节:由于原来接收区没有水平滚动条,所以数据可能自动被换行, 现在已纠正,数据不再自动换行, 要换行, 请使用 \'\\n\'\r\n"
		"2013-09-10 1.13:\r\n"
		"	 增加:现在可以手动编写待发送的命令文件,并发送命令了 - 在发送文件时选择 命令文件, 格式见博客后面的介绍\r\n"
		"    增加:字符发送模式下,可以选择取消回车换行符的发送,可以选择插入转义字符\r\n"
		"        1.支持的字符型转义字符:\r\n"
		"            \\r,\\n,\\t,\\v,\\a,\\b,\\\\\r\n"
		"        2.支持的16进制转义字符格式:\r\n"
		"            \\x?? - 其中一个问号代表一个16进制字符, 不可省略其一,\r\n"
		"            必需保证4个字符的格式\r\n"
		"        3.\'\?\',\'\'\',\'\"\', 等print-able字符不需要转义\r\n"
		"2013-11-02 1.14:\r\n"
		"    修改:完全修改了命令发送的界面,比原来方便了很多~\r\n"
		"2013-11-06:\r\n"
		"    修正:如果接收缓冲区有未显示的数据,则会在按下继续显示时进行提示,而不是原来的在接收到下一次的数据时进行提示;感谢网友lin0119的反馈\r\n"
		"2014-03-03: 1.15\r\n"
		"    增加:支持输入非标准的波特率, 但是驱动是否能够支持, 要看具体的驱动了\r\n"
		"    改进:加入了一些快捷键, 比如Alt+S为发送...\r\n"
		"2014-07-05: 1.16\r\n"
		"    ①字符接收数据时,增加对控制字符Backspace的支持(即'\b'),效果就是向前删除一个字符\r\n"
		"    ②修复一处中文检测错误(原来是对的, 不知道什么时候改错了\r\n"
		"    ③删除了窗体大小调整(下个版本即将使用自动布局)\r\n"
		"2014-07-06: 1.17\r\n"
		"    ①增加:允许从接收区输入字符并发送(更友好的类交互模式)\r\n"
		"    ②更改:更改了字符接收区/发送区的字体为Consolas等宽字体,不再使用原来的Courier字体\r\n"
		"    ③增加:简洁模式 - 此模式下大部分界面元素会被隐藏, 有时候这样更舒服\r\n"
		"    ④增加:主窗口的自动界面布局(允许拖动改变窗口大小,控件坐标自动调整)\r\n"
		"    ⑤更改:默认使用的模式改为:字符接收+字符发送\r\n"
		;
	DialogBoxParam(msg.hInstance, MAKEINTRESOURCE(IDD_MSG), msg.hWndMain, DialogProc, (LPARAM)help_msg);
}

INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HFONT hf;
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			//soft_update();
			SetWindowText(hwndDlg,"关于 "COMMON_NAME_AND_VERSION);
			SetDlgItemText(hwndDlg,IDC_STATIC_VERSION,COMMON_NAME_AND_VERSION"  编译时间:"__DATE__" - "__TIME__);
			SetDlgItemText(hwndDlg, IDC_MSG_EDIT_MSG, (char*)lParam);
			SetFocus(GetDlgItem(hwndDlg, IDC_MSG_OK));
			utils.center_window(hwndDlg,msg.hWndMain);
			
			{
				//TODO:我只是觉得那字体太难看, 没别的意思
				OSVERSIONINFO osver = {0};
				osver.dwOSVersionInfoSize = sizeof(osver);
				GetVersionEx(&osver);
				if(osver.dwMajorVersion >= 6){ //Windows Vista Or Later
					NONCLIENTMETRICS ncm = {0};
					ncm.cbSize = sizeof(ncm);
					SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(ncm),(PVOID)&ncm,0);
					hf = CreateFontIndirect(&ncm.lfCaptionFont);
					SendDlgItemMessage(msg.hWndMain,IDC_MSG_EDIT_MSG,WM_SETFONT,(WPARAM)hf,MAKELPARAM(TRUE,0));
				}
			}
			return 0;
		}
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HICON hIcon;
			HDC hDC = BeginPaint(hwndDlg, &ps);
			hIcon = LoadIcon(msg.hInstance, MAKEINTRESOURCE(IDI_ICON1));
			DrawIcon(hDC, 10, 10 ,hIcon);
			EndPaint(hwndDlg, &ps);
			DestroyIcon(hIcon);
			return 0;
		}
	case WM_CLOSE:
		DeleteObject((HGDIOBJ)hf);
		hf=NULL;
		EndDialog(hwndDlg, 0);
		return 0;
	case WM_COMMAND:
		if(LOWORD(wParam) == IDC_MSG_OK && HIWORD(wParam)==BN_CLICKED){
			SendMessage(hwndDlg, WM_CLOSE, 0, 0);
			return 0;
		}else if(LOWORD(wParam) == IDC_MSG_HOTLINK && HIWORD(wParam) == BN_CLICKED){
			char* web = "http://www.cnblogs.com/nbsofer/archive/2012/12/24/2831700.html";
			ShellExecute(NULL, "open", web, NULL, NULL, SW_SHOWNORMAL);
			return 0;
		}
		return 0;
	default:
		return 0;
	}
}


void soft_update(void)
{
	HINTERNET hInternet = NULL;
	HINTERNET hFile = NULL;
	DWORD dwBytesRead = 0;
	const char* log = "http://files.cnblogs.com/nbsofer/common_update.xml";

	char buffer[1024] = {0};
	char link[128] = {0};
	char version[5]={0};
	char* p = NULL;
	int i;
	int sehcode=0;

	hInternet = InternetOpen("common",INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
	if(hInternet==NULL) return;
	do{
		hFile = InternetOpenUrl(hInternet,log,NULL,0,0,0);
		if(hFile == NULL) break;

		if(!InternetReadFile(hFile,buffer,sizeof(buffer)-1,&dwBytesRead))
			break;
		InternetCloseHandle(hFile);

		if(memcmp(buffer,"common",6) != 0)
			break;

		p = buffer+8;
		memcpy(version,p,4);
		p += 6;

		i=0;
		while(*p && *p!='\r' && i<sizeof(link)-1){
			link[i++] = *p++;
		}
		p += 2;

		i = strcmp(version,COMMON_VERSION);
		if(i==0){
			MessageBox(msg.hWndMain,
				"恭喜, 您目前使用的是最新版本,无需更新~\n\n"
				"如果您有什么建议,或是发现了BUG, 请查看帮助并联系作者~",
				COMMON_NAME_AND_VERSION,MB_OK|MB_ICONINFORMATION);
		}else if(i>0){
			i = utils.msgbox(msg.hWndMain,MB_ICONINFORMATION|MB_YESNO,
				"检查到新版本",
				"检查到软件有新版本更新, 点击是进入下载页面\n\n"
				"当前版本: " COMMON_VERSION " "
				"更新版本: %s\n\n"
				"新版本内容:\n%s",version,p);
			if(i == IDYES){
				ShellExecute(NULL,"open",link,NULL,NULL,SW_SHOWNORMAL);
			}
		}else{
			utils.msgbox(msg.hWndMain,MB_ICONINFORMATION,
				COMMON_NAME_AND_VERSION,
				"咦~ 貌似, 貌似你跟我有*fei*一般的关系, 您已经提前使用了更新版本, 无需再作更新啦~ 享用吧~");
		}
		sehcode = 1;
	}while(0);

	InternetCloseHandle(hInternet);

	if(sehcode == 0 ){
		utils.msgbox(msg.hWndMain,MB_ICONEXCLAMATION,
			COMMON_NAME_AND_VERSION,
			"很不幸, 但是在检查新版本的过程中貌似遇到了错误~");
	}
}
