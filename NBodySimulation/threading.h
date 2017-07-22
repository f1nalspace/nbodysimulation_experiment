#ifndef THREADING_H
#define THREADING_H

#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <deque>
#include <atomic>

template <typename T>
class ConcurrentQueue {
private:
	std::deque<T> _queue;
	std::mutex _queueMutex;
public:
	inline void Push(T value) {
		std::unique_lock<std::mutex> lock(_queueMutex);
		_queue.push_back(value);
	}

	inline bool Pop(T &out) {
		std::unique_lock<std::mutex> lock(_queueMutex, std::defer_lock);
		lock.lock();
		if (!_queue.empty()) {
			T value  = _queue.front();
			_queue.pop_front();
			out = value;
		}
		lock.unlock();
	}
};

typedef std::function<void(const size_t, const size_t, const float)> thread_pool_task_function;
struct ThreadPoolTask {
	size_t startIndex;
	size_t endIndex;
	float deltaTime;
	uint8_t padding0[4];
	thread_pool_task_function func;
};

class ThreadPool {
private:
	bool _stopped;
	std::vector<std::thread> _threads;
	std::atomic_size_t _pendingCount;
	std::deque<ThreadPoolTask> _queue;
	std::mutex _queueMutex;
	std::condition_variable _queueSignal;
public:
	ThreadPool(const size_t threadCount = std::thread::hardware_concurrency()) :
		_stopped(false) {
		_pendingCount.store(0, std::memory_order_relaxed);
		_threads.reserve(threadCount);
		for (size_t workerIndex = 0; workerIndex < threadCount; ++workerIndex) {
			_threads.push_back(std::thread(&ThreadPool::WorkerThreadProc, this));
		}
	}
	~ThreadPool() {
		std::unique_lock<std::mutex> lock(_queueMutex);
		_stopped = true;
		_queueSignal.notify_all();
		lock.unlock();
		for (size_t workerIndex = 0; workerIndex < _threads.size(); ++workerIndex) {
			_threads[workerIndex].join();
		}
	}

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&) = delete;
	ThreadPool& operator=(ThreadPool&&) = delete;

	inline void AddTask(const ThreadPoolTask &task) {
		{
			std::unique_lock<std::mutex> lock(_queueMutex);
			_queue.push_back(task);
		}
		_pendingCount++;
	}

	inline void WaitUntilDone() {
		while (_pendingCount.load(std::memory_order_relaxed) > 0) {
			std::this_thread::yield();
		}
	}

	inline void WorkerThreadProc() {
		ThreadPoolTask group;
		std::unique_lock<std::mutex> lock(_queueMutex, std::defer_lock);

		while (true) {
			lock.lock();
			while (true) {
				if (!_queue.empty()) break;
				if (_stopped) return;
				_queueSignal.wait(lock);
			}
			group = _queue.front();
			_queue.pop_front();
			lock.unlock();

			group.func(group.startIndex, group.endIndex, group.deltaTime);
			_pendingCount.fetch_sub(1, std::memory_order_relaxed);
		}
	}

	inline void CreateTasks(const size_t itemCount, const thread_pool_task_function &function, const float deltaTime) {
		if (itemCount == 0) return;

		const size_t threads_size = _threads.size();
		const size_t itemsPerTask = std::max((size_t)1, itemCount / threads_size);

		ThreadPoolTask task = {};
		task.func = function;
		task.deltaTime = deltaTime;

		std::lock_guard<std::mutex> lock(_queueMutex);
		size_t tasks_added = 0;
		for (size_t itemIndex = 0; itemIndex < itemCount; itemIndex += itemsPerTask, ++tasks_added) {
			task.startIndex = itemIndex;
			task.endIndex = std::min(itemIndex + itemsPerTask - 1, itemCount - 1);
			_queue.push_back(task);
		}
		_queueSignal.notify_all();

		_pendingCount.fetch_add(tasks_added, std::memory_order_relaxed);
	}

	inline size_t GetThreadCount() {
		return _threads.size();
	}
};

#endif