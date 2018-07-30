#include "stdafx.h"
#include <cstdint>
#include <thread>
#include <atomic>
#include <mutex>
#include <iostream>


using std::atomic_flag;
using std::lock_guard;
using std::thread;
using std::atomic;
using std::cout;
using std::endl;





class spinlock
{
private:
	std::atomic_flag atom_flag;
public:
	spinlock() { atom_flag.clear(); }
	void lock()
	{
		while(atom_flag.test_and_set(std::memory_order_acquire));
	}
	void unlock()
	{
		atom_flag.clear(std::memory_order_release);
	}
};






class threaded_reader_writer
{
private:
	spinlock spin;
	uint64_t value = 0;
	uint64_t n_errors = 0;
	bool b_value_ready = false;
	atomic<bool> b_stop_thread = false;

private:
	void thread_writer();
	void thread_reader();

public:
	threaded_reader_writer() : value(0), n_errors(0), b_value_ready(false), b_stop_thread(false) {}
	virtual ~threaded_reader_writer(){}

	void run();
};





// thread writer: increments value
//
void threaded_reader_writer::thread_writer()
{
	while(!b_stop_thread.load())
	{
		lock_guard<spinlock> locker(spin);
		while(!b_value_ready)
		{
			++value;
			cout << "write: " << value;
			b_value_ready = true;
		}
	}
}





// thread reader: reads value
//
void threaded_reader_writer::thread_reader()
{
	uint64_t pred = 0;
	while(!b_stop_thread.load())
	{
		lock_guard<spinlock> locker(spin);
		if(b_value_ready)
		{
			if(value != pred + 1) // counter control
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(5000));
				++n_errors;
				cout << " read:  " << value << " (" << n_errors << ")" << endl;
			}
			pred = value;
			cout << " read:  " << value << " errors: " << n_errors << endl;

			b_value_ready = false;
		}
	}
}





void threaded_reader_writer::run()
{
	thread t_writer(&threaded_reader_writer::thread_writer, this);
	thread t_reader(&threaded_reader_writer::thread_reader, this);

	std::this_thread::sleep_for(std::chrono::seconds(10));

	b_stop_thread.store(true);	

	t_writer.join();
	t_reader.join();
}






int main()
{
	threaded_reader_writer().run();

	return 0;
}

