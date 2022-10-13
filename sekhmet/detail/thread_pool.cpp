/*
 * Created by switchblade on 2021-12-24
 */

#include "../thread_pool.hpp"

#include "../debug/assert.hpp"
#include "../debug/logger.hpp"

namespace sek
{
	static void adjust_worker_count(std::size_t &n)
	{
		if (!n && !(n = std::thread::hardware_concurrency())) [[unlikely]]
			throw std::runtime_error("`std::thread::hardware_concurrency` returned 0");
	}

	thread_pool::control_block::control_block(std::size_t n, thread_pool::queue_mode mode) : dispatch_mode(mode)
	{
		adjust_worker_count(n);

		/* Initialize n workers. */
		realloc_workers(n);
		// clang-format off
		for (auto begin = workers_data, end = workers_data + n; begin != end; ++begin)
			std::construct_at(begin, this);
		// clang-format on
		workers_count = n;
	}
	thread_pool::control_block::~control_block()
	{
		for (auto task = queue_head.front, end = queue_head.back; task != end;)
			delete static_cast<task_base *>(std::exchange(task, task->next));	 // NOLINT

		/* Workers should be terminated by now, no need to destroy them again. */
		::operator delete[](static_cast<void *>(workers_data), workers_capacity * sizeof(worker_t));
	}

	void thread_pool::control_block::destroy_workers(worker_t *first, worker_t *last)
	{
		std::destroy(first, last);
		cv.notify_all();
	}
	void thread_pool::control_block::realloc_workers(std::size_t n)
	{
		auto new_workers = static_cast<worker_t *>(::operator new[](n * sizeof(worker_t)));
		if (workers_data)
		{
			for (auto src = workers_data, end = workers_data + workers_count, dst = new_workers; src < end; ++src, ++dst)
			{
				std::construct_at(dst, std::move(*src));
				std::destroy_at(src);
			}
			::operator delete[](static_cast<void *>(workers_data), n * sizeof(worker_t));
		}

		workers_data = new_workers;
		workers_capacity = n;
	}

	void thread_pool::control_block::resize(std::size_t n)
	{
		adjust_worker_count(n);

		if (n == workers_count) [[unlikely]]
			return;
		else if (n < workers_count) /* Terminate size - n threads. */
			destroy_workers(workers_data + n, workers_data + workers_count);
		else /* Start n - size threads. */
		{
			if (n > workers_capacity) [[unlikely]]
				realloc_workers(n);
			for (auto worker = workers_data + workers_count, end = workers_data + n; worker != end; ++worker)
				std::construct_at(worker, this);
		}
		workers_count = n;
	}
	void thread_pool::control_block::terminate() { destroy_workers(workers_data, workers_data + workers_count); }
	void thread_pool::control_block::acquire() { ref_count++; }
	void thread_pool::control_block::release()
	{
		if (ref_count-- == 1) [[unlikely]]
			delete this;
	}

	void thread_pool::worker_t::thread_main(std::stop_token st, control_block *cb) noexcept
	{
		cb->acquire();
		for (;;)
		{
			task_base *task;
			try
			{
				/* Wait for termination or available tasks. */
				std::unique_lock<std::mutex> lock(cb->mtx);
				cb->cv.wait(lock, [&]() { return st.stop_requested() || cb->queue_head.next != &cb->queue_head; });

				/* Break out of the loop if termination was requested, otherwise get the next task. */
				if (st.stop_requested()) [[unlikely]]
					break;
				else
					task = cb->pop_task();
			}
			catch (std::system_error &e) /* Mutex error. */
			{
				logger::error() << "Mutex error in worker thread: " << e.what();
			}

			/* Execute & delete the task. */
			delete task->invoke();
		}
		cb->release();
	}
}	 // namespace sek