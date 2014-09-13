#pragma once

namespace Common {
	// ����, ����, δ�������ݼ������ӿ�
	class i_data_counter
	{
	public:
		// ��, д, δд
		virtual void update_counter(long rd, long wr, long uw) = 0;
	};

	// ���ݼ�������
	class c_data_counter
	{
	public:
		c_data_counter()
			: _updater(0)
		{
			reset_all();
		}

		~c_data_counter()
		{
			reset_all();
		}

		void set_updater(i_data_counter* udt) { _updater = udt; }
		void call_updater(){
			SMART_ASSERT(_updater != NULL).Warning();
			_lock.lock();
			_updater->update_counter(_n_read, _n_write, _n_unwr);
			_lock.unlock();
		}

		void reset_all(){
			InterlockedExchange(&_n_read, 0);
			InterlockedExchange(&_n_unwr, 0);
			InterlockedExchange(&_n_write, 0);
		}

		void reset_wr_rd(){
			InterlockedExchange(&_n_read, 0);
			InterlockedExchange(&_n_write, 0);
		}

		void reset_unsend()			{ InterlockedExchange(&_n_unwr, 0); }
		void add_send(int n)		{ InterlockedExchangeAdd(&_n_write, n); }
		void add_recv(int n)		{ InterlockedExchangeAdd(&_n_read, n); }
		void add_unsend(int n)		{ InterlockedExchangeAdd(&_n_unwr, n); }
		void sub_unsend(int n)		{ InterlockedExchangeAdd(&_n_unwr, -n); }

	protected:
		c_critical_locker	_lock;
		i_data_counter*		_updater;
		volatile long		_n_write;
		volatile long		_n_read;
		volatile long		_n_unwr;
	};

	//////////////////////////////////////////////////////////////////////////
	// ���¶��� �������ݷ�װ�����ͽṹ

	// Ĭ�Ϸ��ͻ�������С, �����˴�С���Զ����ڴ����
	const int csdp_def_size = 1024;
	enum csdp_type{
		csdp_local,		// ���ذ�, ����Ҫ�ͷ�
		csdp_alloc,		// �����, ������������Ĭ�ϻ��������ڲ�����������ʱ������
		csdp_exit,		
	};

#pragma warning(push)
#pragma warning(disable:4200)	// nonstandard extension used : zero-sized array in struct/union
#pragma pack(push,1)
	// �����������ݰ�, ������������
	struct c_send_data_packet{
		csdp_type		type;			// ������
		list_s			_list_entry;	// ������ӵ����Ͷ���
		bool			used;			// �Ƿ��ѱ�ʹ��
		int				cb;				// ���ݰ����ݳ���
		unsigned char	data[0];
	};

	// ��չ�������ݰ�, ��һ�� csdp_def_size ��С�Ļ�����
	struct c_send_data_packet_extended{
		csdp_type		type;			// ������
		list_s			_list_entry;	// ������ӵ����Ͷ���
		bool			used;			// �Ƿ��ѱ�ʹ��
		int				cb;				// ���ݰ����ݳ���
		unsigned char	data[csdp_def_size];
	};
#pragma pack(pop)
#pragma warning(pop)

	// �������ݰ�������, ���������͵����ݰ��ųɶ���
	// ���������ᱻ����߳�ͬʱ����
	class c_data_packet_manager
	{
	public:
		c_data_packet_manager();
		~c_data_packet_manager();
		void					empty();
		c_send_data_packet*		alloc(int size);						// ͨ���˺�����ȡһ����������ָ����С���ݵİ�
		void					release(c_send_data_packet* psdp);		// ����һ����
		void					put(c_send_data_packet* psdp);			// ���Ͷ���β����һ���µ����ݰ�
		void					put_front(c_send_data_packet* psdp);	// ����һ���������ݰ���������, ���ȴ���
		c_send_data_packet*		get();									// ����ȡ�ߵ��ô˽ӿ�ȡ�����ݰ�, û�а�ʱ�ᱻ����
		c_send_data_packet*		query_head();
		HANDLE					get_event() const { return _hEvent; }

	private:
		c_send_data_packet_extended	_data[100];	// Ԥ����ı��ذ��ĸ���
		c_critical_locker			_lock;		// ���߳���
		HANDLE						_hEvent;	// ����get()�ɲ�������, ��Ϊ�����ط�Ҫput()!
		list_s						_list;		// �������ݰ�����
	};

	//////////////////////////////////////////////////////////////////////////
	// ���¹�������ص�һЩ����, ��: �������б� ...

	// ���ڶ���
	class t_com_item
	{
	public:
		t_com_item(int i,const char* s){_s = s; _i=i;}

		// �����ַ�������: ����: ��У��λ
		std::string get_s() const {return _s;}
		// ������������ : ����: NOPARITY(��)
		int get_i() const {return _i;}

	protected:
		std::string _s;
		int _i;
	};

	// ˢ�´��ڶ����б�ʱ��Ҫ�õ��Ļص���������
	typedef void t_list_callback(void* ud, const t_com_item* t);

	// ���ڶ���ˢ��ʱ�Ļص����ͽӿ�
	class i_com_list
	{
	public:
		virtual void callback(t_list_callback* cb, void* ud) = 0;
	};

	// ���ڶ�������: ���� ����ϵͳ���еĴ����б�
	template<class T>
	class t_com_list : public i_com_list
	{
	public:
		void empty() {_list.clear();}
		const T& add(T t) { _list.push_back(t); return _list[_list.size() - 1]; }
		int size() {return _list.size();}
		const T& operator[](int i) {return _list[i];}

		// ���¶����б�, �������ϵͳ�����б�
		virtual i_com_list* update_list(){return this;}

		virtual operator i_com_list*() {return static_cast<i_com_list*>(this);}
		virtual void callback(t_list_callback* cb, void* ud)
		{
			for(int i=0,c=_list.size(); i<c; i++){
				cb(ud, &_list[i]);
			}
		}

	protected:
		std::vector<T> _list;
	};

	// ���ڶ˿��б�, �̳е�ԭ����: �˿���һ����ν�� "�Ѻ���"
	// ���糣����: Prolific USB-to-Serial Comm Port
	// ���µ������б�ؼ���ʱ��Ҫ��������һ��
	class c_comport : public t_com_item
	{
	public:
		c_comport(int id,const char* s)
			: t_com_item(id, s)
		{}

		std::string get_id_and_name() const;
	};

	// ���ڶ˿�����: Ҫ��ϵͳȡ���б�, ������д
	class c_comport_list : public t_com_list<c_comport>
	{
	public:
		virtual i_com_list* update_list();
	};

	// ���ڲ����ʿ����ⲿ�ֶ����, ���Զ��һ����Ա
	class c_baudrate : public t_com_item
	{
	public:
		c_baudrate(int id, const char* s, bool inner)
			: t_com_item(id, s)
			, _inner(inner)
		{}

		bool is_added_by_user() const { return !_inner; }
	protected:
		bool _inner;
	};
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// �Ҳ�֪�����������֪ͨ���ڱ��ر���һ�¼�? �ɴ�д���ӿڰ�
	class i_com_event_handler
	{
	public:
		virtual void com_closed() = 0;
	};

	// �����¼��������ӿ�
	class i_com_event_listener
	{
	public:
		virtual void do_event(DWORD evt) = 0;
	};

	// �����¼��������ӿ� ������
	class c_com_event_listener
	{
		struct item{
			item(i_com_event_listener* p, DWORD _mask)
				: listener(p)
				, mask(_mask)
			{}

			i_com_event_listener* listener;
			DWORD mask;
		};
	public:
		void call_listeners(DWORD dwEvt){
			_lock.lock();
			for (auto& item : _listeners){
				if (dwEvt & item.mask){
					item.listener->do_event(dwEvt);
				}
			}
			_lock.unlock();
		}
		void add_listener(i_com_event_listener* pcel, DWORD mask){
			_lock.lock();
			_listeners.push_back(item(pcel, mask));
			_lock.unlock();
		}
		void remove_listener(i_com_event_listener* pcel){
			_lock.lock();
			for (auto it = _listeners.begin(); it != _listeners.end(); it++){
				if (it->listener == pcel){
					_listeners.erase(it);
					break;
				}
			}
			_lock.unlock();
		}

	protected:
		c_critical_locker	_lock;
		std::vector<item>	_listeners;
	};

	// ������
	class CComm
	{
	// UI֪ͨ��
	public:
		void set_notifier(i_notifier* noti) { _notifier = noti;	}
	private:
		i_notifier*	_notifier;

	// �������ݰ�����
	private:
		c_send_data_packet*		get_packet()	{ return _send_data.get(); }
	public:
		bool					put_packet(c_send_data_packet* psdp, bool bfront=false){
			if (is_opened()){
				if (bfront)
					_send_data.put_front(psdp);
				else
					_send_data.put(psdp);
				
				switch (psdp->type)
				{
				case csdp_type::csdp_alloc:
				case csdp_type::csdp_local:
					_data_counter.add_unsend(psdp->cb);
					_data_counter.call_updater();
					break;
				}
				return true;
			}
			else{
				_notifier->msgbox(MB_ICONERROR, NULL, "����δ��!");
				return false;
			}
		}
		c_send_data_packet*		alloc_packet(int size) { return _send_data.alloc(size); }
		void					release_packet(c_send_data_packet* psdp) { _send_data.release(psdp); }
	private:	
		c_data_packet_manager	_send_data;

	// ������
	public:
		c_data_counter*			counter() { return &_data_counter; }
	private:
		c_data_counter			_data_counter;

	// �¼�������
	private:
		c_com_event_listener	_event_listener;

	// ��������¼�������
	public:
		void					set_event_handler(i_com_event_handler* handler) { _event_handler = handler; }
	private:
		i_com_event_handler*	_event_handler;
	// ���ݽ�����
	public:
		void add_data_recerver(i_data_receiver* receiver);
		void remove_data_recerver(i_data_receiver* receiver);
		void call_data_receivers(const unsigned char* ba, int cb);
	private:
		c_ptr_array<i_data_receiver>	_data_receivers;
		c_critical_locker				_data_receiver_lock;

	// �ڲ������߳�
	private:
		bool _begin_threads();
		bool _end_threads();

		struct thread_helper_context
		{
			CComm* that;
			enum e_which{
				kRead,
				kWrite,
			}which;
		};
		unsigned int thread_read();
		//void wait_read_event() { ::WaitForSingleObject(_hevent_continue_to_read, INFINITE); }
		unsigned int thread_write();
		static unsigned int __stdcall thread_helper(void* pv);
	public:
		//void resume_read_thread() { ::SetEvent(_hevent_continue_to_read); }
		//void suspend_read_thread() { ::ResetEvent(_hevent_continue_to_read); }
	private:
		HANDLE		_hthread_read;				// ���߳̾��
		HANDLE		_hthread_write;				// д�߳̾��

		HANDLE		_hevent_read_start;			// �Զ�����,֪ͨ���߳̿�ʼ��������¼�
		HANDLE		_hevent_read_end;			// �ֶ�����,֪ͨ���߳̿�ʼ��������¼�

		HANDLE		_hevent_write_start;		// �Զ�����,֪ͨд�߳̿�ʼ��������¼�
		HANDLE		_hevent_write_end;			// �ֶ�����,֪ͨд�߳̿�ʼ��������¼�

	// �������ýṹ��
	private:
		//COMMPROP			_commprop;
		//COMMCONFIG			_commconfig;
		COMMTIMEOUTS		_timeouts;
		DCB					_dcb;

	// ��������(���ⲿ����)
	public:
		struct s_setting_comm{
			DWORD	baud_rate;
			BYTE	parity;
			BYTE	stopbit;
			BYTE	databit;
		};
		bool setting_comm(s_setting_comm* pssc);

	// ���ڶ����б�
	public:
		c_comport_list*			comports()	{ return &_comport_list; }
		t_com_list<c_baudrate>*	baudrates()	{ return &_baudrate_list; }
		t_com_list<t_com_item>*	parities()	{ return &_parity_list; }
		t_com_list<t_com_item>*	stopbits()	{ return &_stopbit_list; }
		t_com_list<t_com_item>*	databits()	{ return &_databit_list; }
	private:
		c_comport_list			_comport_list;
		t_com_list<c_baudrate>	_baudrate_list;
		t_com_list<t_com_item>	_parity_list;
		t_com_list<t_com_item>	_stopbit_list;
		t_com_list<t_com_item>	_databit_list;

	// �ⲿ��ز����ӿ�
	private:
		HANDLE		_hComPort;
	public:
		bool		open(int com_id);
		bool		close();
		HANDLE		get_handle() { return _hComPort; }
		bool		is_opened() { 
			return !!_hComPort; 
		}
		bool		begin_threads();
		bool		end_threads();

	public:
		CComm();
		~CComm();
	};
}

#define COMMON_MAX_LOAD_SIZE			((unsigned long)1<<20)
#define COMMON_LINE_CCH_SEND			16
#define COMMON_LINE_CCH_RECV			16
#define COMMON_SEND_BUF_SIZE			COMMON_MAX_LOAD_SIZE
#define COMMON_RECV_BUF_SIZE			0 // un-limited //(((unsigned long)1<<20)*10)
#define COMMON_INTERNAL_RECV_BUF_SIZE	((unsigned long)1<<20)
#define COMMON_READ_BUFFER_SIZE			((unsigned long)1<<20)
