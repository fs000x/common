#include "StdAfx.h"

static char* __THIS_FILE__ = __FILE__;


namespace Common{
	//////////////////////////////////////////////////////////////////////////
	std::string c_comport::get_id_and_name() const
	{
		char idstr[17] = {0};
		_snprintf(idstr, sizeof(idstr), "COM%-13d", _i);
		std::stringstream ss;
		ss << idstr << "\t\t" << _s;
		return std::string(ss.str());
	}

	//////////////////////////////////////////////////////////////////////////
	i_com_list* c_comport_list::update_list()
	{
		HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;
		SP_DEVINFO_DATA spdata = {0};

		empty();

		hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, 0, 0, DIGCF_PRESENT);
		if(hDevInfo == INVALID_HANDLE_VALUE){
			return this;
		}

		spdata.cbSize = sizeof(spdata);
		for(int i=0; SetupDiEnumDeviceInfo(hDevInfo, i, &spdata); i++){
			char buff[1024] = {0};
			if(SetupDiGetDeviceRegistryProperty(hDevInfo, &spdata, SPDRP_FRIENDLYNAME, NULL, 
				PBYTE(buff), _countof(buff), NULL))
			{
				// Prolific com port (COMxx)
				char* p = strstr(buff, "(COM");
				if(p){
					int id = atoi(p + 4);
					if(p != buff) *(p-1) = '\0';
					add(c_comport(id, buff));
				}
			}
		}
		SetupDiDestroyDeviceInfoList(hDevInfo);

		return this;
	}

	CComm::CComm()
		: _notifier(NULL)
		, _hComPort(NULL)
		, _hthread_read(0)
		, _hthread_write(0)
		//, _hevent_continue_to_read(0)
		, _hevent_read_start(0)
		, _hevent_write_start(0)
		, _hevent_read_end(0)
		, _hevent_write_end(0)

	{
		_begin_threads();
		static char* aBaudRate[]={"110","300","600","1200","2400","4800","9600","14400","19200","38400","57600","115200","128000","256000", NULL};
		static DWORD iBaudRate[]={CBR_110,CBR_300,CBR_600,CBR_1200,CBR_2400,CBR_4800,CBR_9600,CBR_14400,CBR_19200,CBR_38400,CBR_57600,CBR_115200,CBR_128000,CBR_256000};
		static char* aParity[] = {"��","��У��","żУ��", "���", "�ո�", NULL};
		static BYTE iParity[] = { NOPARITY, ODDPARITY,EVENPARITY, MARKPARITY, SPACEPARITY };
		static char* aStopBit[] = {"1λ", "1.5λ","2λ", NULL};
		static BYTE iStopBit[] = {ONESTOPBIT,ONE5STOPBITS,TWOSTOPBITS};
		static char* aDataSize[] = {"8λ","7λ","6λ","5λ",NULL};
		static BYTE iDataSize[] = {8,7,6,5};

		for(int i=0; aBaudRate[i]; i++)
			_baudrate_list.add(c_baudrate(iBaudRate[i],aBaudRate[i], true));
		for(int i=0; aParity[i]; i++)
			_parity_list.add(t_com_item(iParity[i],aParity[i]));
		for(int i=0; aStopBit[i]; i++)
			_stopbit_list.add(t_com_item(iStopBit[i], aStopBit[i]));
		for(int i=0; aDataSize[i]; i++)
			_databit_list.add(t_com_item(iDataSize[i], aDataSize[i]));

		_timeouts.ReadIntervalTimeout = MAXDWORD;
		_timeouts.ReadTotalTimeoutMultiplier = 0;
		_timeouts.ReadTotalTimeoutConstant = 0;
		_timeouts.WriteTotalTimeoutMultiplier = 0;
		_timeouts.WriteTotalTimeoutConstant = 0;

	}


	CComm::~CComm()
	{
		_end_threads();
	}

	bool CComm::open(int com_id)
	{
		if (is_opened()){
			SMART_ASSERT("com was opened!" && 0).Fatal();
			return false;
		}

		char str[64];
		sprintf(str, "\\\\.\\COM%d", com_id);
		_hComPort = ::CreateFile(str, GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		if (_hComPort == INVALID_HANDLE_VALUE){
			_hComPort = NULL;
			DWORD dwErr = ::GetLastError();
			_notifier->msgerr();
			if (dwErr == ERROR_FILE_NOT_FOUND){
				//TODO
			}
			return false;
		}


		return true;
	}

	bool CComm::close()
	{
		SMART_ENSURE(::CloseHandle(_hComPort), != 0).Fatal();
		_hComPort = NULL;
		_data_counter.reset_all();
		_send_data.empty();

		return true;
	}

	bool CComm::begin_threads()
	{
		::ResetEvent(_hevent_read_end);
		::ResetEvent(_hevent_write_end);
		::SetEvent(_hevent_write_start);
		::SetEvent(_hevent_read_start);
		return true;
	}

	bool CComm::end_threads()
	{
		::ResetEvent(_hevent_read_start);
		::ResetEvent(_hevent_write_start);
		::SetEvent(_hevent_write_end);
		::SetEvent(_hevent_read_end);
		
		// �ڶ�д�߳��˳�֮ǰ, ����end��Ϊ����״̬
		// ����ȵ������߳̾��˳�����״̬��������������
		debug_out(("%s: �ȴ� [���߳�] ����...\n", __FUNCTION__));
		while (::WaitForSingleObject(_hevent_read_end, 0) == WAIT_OBJECT_0)
			;
		debug_out(("%s: �ȴ� [д�߳�] ����...\n", __FUNCTION__));
		while (::WaitForSingleObject(_hevent_write_end, 0) == WAIT_OBJECT_0)
			;
		return true;
	}

	unsigned int __stdcall CComm::thread_helper(void* pv)
	{
		thread_helper_context* pctx = (thread_helper_context*)pv;
		CComm* comm = pctx->that;
		thread_helper_context::e_which which = pctx->which;

		delete pctx;

		switch (which)
		{
		case thread_helper_context::e_which::kRead:
			return comm->thread_read();
		case thread_helper_context::e_which::kWrite:
			return comm->thread_write();
		default:
			SMART_ASSERT(0 && "unknown thread").Fatal();
			return 1;
		}
	}

	unsigned int CComm::thread_write()
	{
		BOOL bRet;
		DWORD dw;
		bool bNormalExit = false;

		debug_out(("[д�߳�] ����\n"));
	_wait_for_work:
		dw = ::WaitForSingleObject(_hevent_write_start, INFINITE);
		debug_out(("[д�߳�] ��ʼ��ʼ���빤��״̬\n"));

		SMART_ASSERT(dw == WAIT_OBJECT_0)(dw).Fatal();

		// ���빤���߳�, �����������Ϊ���ڶ�����, ����ζ�Ÿý�����
		if (!is_opened()){
			debug_out(("[д�߳�] û������, д�߳��˳�\n"));
			::SetEvent(_hevent_write_end);
			return 0;
		}
		 

		OVERLAPPED overlap = { 0 };
		overlap.hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

		for (;;)
		{
			debug_out(("[д�߳�] ȡ���ݰ���...\n"));
			c_send_data_packet* psdp = _send_data.get();	// get@1 
			if (psdp->type == csdp_type::csdp_alloc || psdp->type == csdp_type::csdp_local){
				debug_out(("[д�߳�] ȡ��һ���������ݰ�, ����Ϊ %d �ֽ�\n", psdp->cb));

				DWORD	nWritten = 0;		// д����һ��д��ĳ���
				int		nWrittenData;	// ��ǰѭ���ܹ�д�볤��

				for (nWrittenData = 0; nWrittenData < psdp->cb;)
				{
					BOOL bSucceed = FALSE;

					bRet = ::WriteFile(_hComPort, &psdp->data[0] + nWrittenData, psdp->cb - nWrittenData, NULL, &overlap);
					if (bRet != FALSE){ // I/O is completed
						bSucceed = ::GetOverlappedResult(_hComPort, &overlap, &nWritten, FALSE);
						debug_out(("[д�߳�] I/O completed immediately: bytes : %d\n", nWritten));
					}
					else{ // I/O is pending						
						if (::GetLastError() == ERROR_IO_PENDING){
							HANDLE handles[2];
							handles[0] = _hevent_write_end;
							handles[1] = overlap.hEvent;

							switch (::WaitForMultipleObjects(2, &handles[0], FALSE, INFINITE))
							{
							case WAIT_FAILED:		// what error happened ? the serial port removed ?
								debug_out(("[д�߳�] �ȴ�ʧ��\n"));
								bSucceed = FALSE;
								break;
							case WAIT_OBJECT_0 + 0: // now we exit
								debug_out(("[д�߳�] ����׼���˳�����״̬\n"));
								nWritten = 0;
								bSucceed = TRUE;
								goto exit_main_for;
								break;
							case WAIT_OBJECT_0 + 1: // the I/O operation is now completed
								bSucceed = ::GetOverlappedResult(_hComPort, &overlap, &nWritten, FALSE);
								break;
							}
						}
						else{
							bSucceed = FALSE;
						}
					}

					if (bSucceed){
						nWrittenData += nWritten;
						_data_counter.add_send(nWritten);
						_data_counter.sub_unsend(nWritten);
						_data_counter.call_updater();
					}
					else{
						_notifier->msgerr("[д����ʧ��]");
						goto exit_main_for;
					}
				}
				_send_data.release(psdp);
			}
			else if (psdp->type == csdp_type::csdp_exit){
				debug_out(("[д�߳�] �յ��˳�����\n"));
				_send_data.release(psdp);
				bNormalExit = true;
				goto exit_main_for;
			}
		}
		exit_main_for:
		;
		debug_out(("[д�߳�] PurgeComm()...\n"));
		if (!::PurgeComm(_hComPort, PURGE_TXABORT | PURGE_TXCLEAR)){
			// The main thread is waiting for us!!!
			//_notifier->msgerr("PurgeComm");
		}
		debug_out(("[д�߳�] CancelIO()...\n"));

		if (!::CancelIo(_hComPort)){
			//_notifier->msgerr("CancelIO");
		}

		::CloseHandle(overlap.hEvent);

		if (!bNormalExit){
			_event_handler->com_closed();
		}

		// Do just like the thread_read do.
		::WaitForSingleObject(_hevent_write_end, INFINITE);
		::ResetEvent(_hevent_write_end);

		debug_out(("[д�߳�] ���¾���!\n"));
		goto _wait_for_work;

		return 0;
	}

	unsigned int CComm::thread_read()
	{
		BOOL bRet;
		DWORD dw;
		bool bNormalExit =false;

		unsigned char* block_data = NULL;
		block_data = new unsigned char[COMMON_READ_BUFFER_SIZE];

		debug_out(("[���߳�] ����\n"));
	_wait_for_work:
		dw = ::WaitForSingleObject(_hevent_read_start, INFINITE);
		debug_out(("[���߳�] ��ʼ��ʼ���빤��״̬\n"));

		SMART_ASSERT(dw == WAIT_OBJECT_0)(dw).Fatal();

		// ���빤���߳�, �����������Ϊ���ڶ�����, ����ζ�Ÿý�����
		if (!is_opened()){
			debug_out(("[���߳�] û������, ���߳��˳�\n"));
			delete[] block_data;
			::SetEvent(_hevent_read_end);
			return 0;
		}


		OVERLAPPED overlap = { 0 };
		overlap.hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

		OVERLAPPED owaitevent = { 0 };
		owaitevent.hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL); // MSDN˵WaitCommEvent�������ֶ������¼�

		for (;;)
		{
			BOOL	bSucceed = FALSE;

			//debug_out(("[���߳�] �ȴ������¼���...\n"));

			::ResetEvent(owaitevent.hEvent);
			bRet = ::WaitCommEvent(_hComPort, &dw, &owaitevent);
			if (bRet == FALSE){
				if (::GetLastError() == ERROR_IO_PENDING){
					HANDLE handles[2];
					handles[0] = _hevent_read_end;
					handles[1] = owaitevent.hEvent;
					switch (::WaitForMultipleObjects(2, &handles[0], FALSE, INFINITE))
					{
					case WAIT_FAILED:
						debug_out(("[���߳�] WaitCommEvent ʧ��...\n"));
						bSucceed = FALSE;
						break;
					case WAIT_OBJECT_0 + 0:
						debug_out(("[���߳�] �յ����ڹر��¼�֪ͨ\n"));
						bNormalExit = true;
						goto _exit_main_for;
					case WAIT_OBJECT_0 + 1:
						// MSDN��˵�������ͨ��GetOverlappedResult��ȡ�ý��
						// �����Թ�, ����ڵȴ������аѴ��ڰ��˵Ļ�, GOR��Ȼ����TRUE
						// û���ĸ��ط���ʾ��������, �����Ҿ�ֻ���ȼٶ���"�ɹ�"����
						// Then I should call ClearCommError() and detect the errors.
						bSucceed = TRUE;
						break;
					}
				}
				else{
					bSucceed = FALSE;
				}
			}
			else{
				bSucceed = TRUE;
			}

			if (bSucceed){
				DWORD nBytesToRead, nRead, nTotalRead;
				DWORD	comerr;
				COMSTAT	comsta;
				// for some reasons, such as comport has been removed
				if (!::ClearCommError(_hComPort, &comerr, &comsta)){
					_notifier->msgerr("ClearCommError()");
					goto _exit_main_for;
				}
				nBytesToRead = comsta.cbInQue;
				if (nBytesToRead == 0) 
					nBytesToRead++; // would never happen

				if (nBytesToRead > COMMON_READ_BUFFER_SIZE)
					nBytesToRead = COMMON_READ_BUFFER_SIZE;

				for (nTotalRead = 0; nTotalRead < nBytesToRead;)
				{
					bRet = ::ReadFile(_hComPort, block_data + nTotalRead, nBytesToRead - nTotalRead, &nRead, &overlap);
					if (bRet != FALSE){
						bSucceed = ::GetOverlappedResult(_hComPort, &overlap, &nRead, FALSE);
						if (bSucceed) {
							//debug_out(("[���߳�] ��ȡ %d �ֽ�, bRet==TRUE, nBytesToRead: %d\n", nRead, nBytesToRead));
						}
					}
					else{
						if (::GetLastError() == ERROR_IO_PENDING){
							HANDLE handles[2];
							handles[0] = _hevent_read_end;
							handles[1] = overlap.hEvent;

							switch (::WaitForMultipleObjects(2, &handles[0], FALSE, INFINITE))
							{
							case WAIT_FAILED:
								debug_out(("[���߳�] �ȴ�ʧ��!\n"));
								bSucceed = FALSE;
								break;
							case WAIT_OBJECT_0 + 0:
								debug_out(("[���߳�] ����׼���˳�����״̬!\n"));
								if (!::PurgeComm(_hComPort, PURGE_TXABORT | PURGE_TXCLEAR)){
									_notifier->msgerr("PurgeComm");
								}
								bNormalExit = true;
								goto _exit_main_for;
								break;
							case WAIT_OBJECT_0 + 1:
								bSucceed = ::GetOverlappedResult(_hComPort, &overlap, &nRead, FALSE);
								//debug_out(("[���߳�] ��ȡ %d �ֽ�, bRet==FALSE\n", nRead));
								break;
							}
						}
						else{
							bSucceed = FALSE;
						}
					}

					if (bSucceed){
						if (nRead > 0){
							nTotalRead += nRead;
							_data_counter.add_recv(nRead);
							_data_counter.call_updater();
						}
						else{
							nBytesToRead--;
						}
					}
					else{
						_notifier->msgerr("[������ʧ��]");
						goto _exit_main_for;
					}
				}
				call_data_receivers(block_data, nBytesToRead);
			}
			else{
				_notifier->msgerr("[������ʧ��]");
				goto _exit_main_for;
			}
		}
	_exit_main_for:
		if (!::PurgeComm(_hComPort, PURGE_RXABORT | PURGE_RXCLEAR)){
			// The main thread is waiting for us!!!
			//_notifier->msgerr("PurgeComm");
		}

		if (!::CancelIo(_hComPort)){
			//_notifier->msgerr("CancelIO");
		}

		::CloseHandle(owaitevent.hEvent);
		::CloseHandle(overlap.hEvent);

		if (!bNormalExit){
			_event_handler->com_closed();
		}

		// Sometimes we got here not because of we've got a exit signal
		// Maybe something wrong
		// And if something wrong, the following handle is still non-signal.
		// The main thread notify this thread to exit by signaling the event and then wait
		// this thread Reset it, since the event is a Manual reset event handle.
		// So, let's wait whatever the current signal-state the event is, just before the
		// main thread  really want we do that.
		::WaitForSingleObject(_hevent_read_end, INFINITE);
		::ResetEvent(_hevent_read_end);

		debug_out(("[���߳�] ���¾���!\n"));
		goto _wait_for_work;

		return 0;
	}

	void CComm::call_data_receivers(const unsigned char* ba, int cb)
	{
		_data_receiver_lock.lock();
		for (int i = 0; i < _data_receivers.size(); i++){
			_data_receivers[i]->receive(ba, cb);
		}
		_data_receiver_lock.unlock();
	}

	void CComm::remove_data_recerver(i_data_receiver* receiver)
	{
		_data_receiver_lock.lock();
		_data_receivers.remove(receiver);
		_data_receiver_lock.unlock();

	}

	void CComm::add_data_recerver(i_data_receiver* receiver)
	{
		_data_receiver_lock.lock();
		_data_receivers.add(receiver);
		_data_receiver_lock.unlock();
	}

	bool CComm::setting_comm(s_setting_comm* pssc)
	{
		SMART_ASSERT(is_opened()).Fatal();

		if (!::GetCommState(get_handle(), &_dcb)){
			_notifier->msgerr("GetCommState()����");
			return false;
		}

		_dcb.fBinary = TRUE;
		_dcb.BaudRate = pssc->baud_rate;
		_dcb.fParity = pssc->parity == NOPARITY ? FALSE : TRUE;
		_dcb.Parity = pssc->parity;
		_dcb.ByteSize = pssc->databit;
		_dcb.StopBits = pssc->stopbit;

		if (!::SetCommState(_hComPort, &_dcb)){
			_notifier->msgerr("SetCommMask()����");
			return false;
		}

		if (!::SetCommMask(get_handle(), EV_RXCHAR)){
			_notifier->msgerr("SetCommMask()����");
			return false;
		}
		if (!::SetCommTimeouts(get_handle(), &_timeouts)){
			_notifier->msgerr("���ô��ڳ�ʱ����");
			return false;
		}

		PurgeComm(_hComPort, PURGE_TXCLEAR | PURGE_TXABORT);
		PurgeComm(_hComPort, PURGE_RXCLEAR | PURGE_RXABORT);

		return true;
	}

	bool CComm::_begin_threads()
	{
		SMART_ASSERT(!_hthread_read && !_hthread_write
			&& !_hevent_read_start && !_hevent_write_start
			&& !_hevent_read_end && !_hevent_write_end).Fatal();


		// �ڿ�����д�߳�֮ǰӦ���ȴ����¼�, ��Ϊ�̸߳��������ж��Ƿ�ʼ����
		_hevent_read_start  = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		_hevent_read_end    = ::CreateEvent(NULL, TRUE, FALSE, NULL);
		_hevent_write_start = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		_hevent_write_end   = ::CreateEvent(NULL, TRUE, FALSE, NULL);

		if (!_hevent_read_start || !_hevent_write_start
			|| !_hevent_read_end || !_hevent_read_end)
		{
			::MessageBox(NULL, "Ӧ�ó����ʼ��ʧ��, �����˳�!", NULL, MB_ICONHAND);
			::exit(1);
		}

		thread_helper_context* ctx = NULL;;

		ctx = new thread_helper_context;
		ctx->that = this;
		ctx->which = thread_helper_context::kRead;
		_hthread_read = (HANDLE)_beginthreadex(NULL, 0, thread_helper, ctx, 0, NULL);

		ctx = new thread_helper_context;
		ctx->that = this;
		ctx->which = thread_helper_context::kWrite;
		_hthread_write = (HANDLE)_beginthreadex(NULL, 0, thread_helper, ctx, 0, NULL);

		if (!_hthread_read || !_hthread_write){
			::MessageBox(NULL, "Ӧ�ó����ʼ��ʧ��, �����˳�!", NULL, MB_ICONHAND);
			::exit(1);
		}

		return true;
	}

	bool CComm::_end_threads()
	{
		SMART_ASSERT(is_opened() == false).Fatal();

		// �ɶ�д�߳����ò��õ�ǰ�̵߳ȴ����ǵĽ���
		::ResetEvent(_hevent_read_end);
		::ResetEvent(_hevent_write_end);

		// ��ʱ�����ǹرյ�, �յ����¼���׼���˳��߳�
		::SetEvent(_hevent_read_start);
		::SetEvent(_hevent_write_start);

		// �ȴ���д�߳���ȫ�˳�
		::WaitForSingleObject(_hevent_read_end, INFINITE);
		::WaitForSingleObject(_hevent_write_end, INFINITE);

		::CloseHandle(_hevent_read_end);
		::CloseHandle(_hevent_read_start);
		::CloseHandle(_hevent_write_end);
		::CloseHandle(_hevent_write_start);

		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	c_data_packet_manager::c_data_packet_manager()
		: _hEvent(0)
	{
		list_init(&_list);
		_hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		SMART_ASSERT(_hEvent != NULL).Fatal();

		for (int i = 0; i < sizeof(_data) / sizeof(_data[0]); i++)
			_data[i].used = false;

	}

	c_data_packet_manager::~c_data_packet_manager()
	{
		SMART_ASSERT(list_is_empty(&_list)).Fatal();
		::CloseHandle(_hEvent);
	}

	c_send_data_packet* c_data_packet_manager::alloc(int size)
	{
		SMART_ASSERT(size >= 0)(size).Fatal();
		_lock.lock();

		c_send_data_packet* psdp = NULL;

		if (size <= csdp_def_size){
			for (int i = 0; i < sizeof(_data) / sizeof(_data[0]); i++){
				if (_data[i].used == false){
					psdp = (c_send_data_packet*)&_data[i];
					break;
				}
			}
			if (psdp != NULL){
				psdp->used = true;
				psdp->type = csdp_type::csdp_local;
				psdp->cb = size;
				goto _exit;
			}
			// no left
		}

		while (psdp == NULL){
			psdp = (c_send_data_packet*)GET_MEM(sizeof(c_send_data_packet) + size);
		}
		psdp->type = csdp_type::csdp_alloc;
		psdp->used = true;
		psdp->cb = size;
		goto _exit;

	_exit:
		_lock.unlock();
		return psdp;
	}

	void c_data_packet_manager::release(c_send_data_packet* psdp)
	{
		SMART_ASSERT(psdp != NULL).Fatal();

		switch (psdp->type)
		{
		case csdp_type::csdp_alloc:
			memory.free((void**)&psdp, "");
			break;
		case csdp_type::csdp_local:
		case csdp_type::csdp_exit:
			_lock.lock();
			psdp->used = false;
			_lock.unlock();
			break;
		default:
			SMART_ASSERT(0).Fatal();
		}
	}

	void c_data_packet_manager::put(c_send_data_packet* psdp)
	{
		_lock.lock();
		list_insert_tail(&_list, &psdp->_list_entry);
		_lock.unlock();
		::SetEvent(_hEvent); // singal get() proc
	}

	c_send_data_packet* c_data_packet_manager::get()
	{
		c_send_data_packet* psdp = NULL;

		for (;;){ // ���޵ȴ�, ֱ���յ�һ�����ݰ�
			_lock.lock();
			list_s* pls = list_remove_head(&_list);
			_lock.unlock();

			if (pls != NULL){
				psdp = list_data(pls, c_send_data_packet, _list_entry);
				return psdp;
			}
			else{
				::WaitForSingleObject(_hEvent, INFINITE);
			}
		}
	}

	void c_data_packet_manager::put_front(c_send_data_packet* psdp)
	{
		_lock.lock();
		list_insert_head(&_list, &psdp->_list_entry);
		_lock.unlock();
		::SetEvent(_hEvent);
	}

	void c_data_packet_manager::empty()
	{
		// TODO
	}

	c_send_data_packet* c_data_packet_manager::query_head()
	{
		c_send_data_packet* psdp = NULL;
		_lock.lock();
		if (list_is_empty(&_list)){
			psdp = NULL;
		}
		else{
			psdp = list_data(_list.next, c_send_data_packet, _list_entry);
		}
		_lock.unlock();
		return psdp;
	}

}
