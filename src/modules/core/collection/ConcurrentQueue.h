/**
 * @file
 */

#pragma once

#include "core/concurrent/Atomic.h"
#include "core/concurrent/Lock.h"
#include "core/concurrent/ConditionVariable.h"
#include "core/collection/DynamicArray.h"
#include "core/Trace.h"
#include "core/Common.h"
#include "core/Assert.h"
#include <stdint.h>

namespace core {

template<class Data>
class ConcurrentQueue {
private:
	using Collection = core::DynamicArray<Data>;
	Collection _data core_thread_guarded_by(_mutex);
	mutable core_trace_mutex(core::Lock,  _mutex, "ConcurrentQueue");
	core::ConditionVariable _conditionVariable;
	core::AtomicBool _abort { false };
public:
	using value_type = Data;
	using Key = Data;

	ConcurrentQueue(size_t reserve = 0u) {
		if (reserve) {
			_data.reserve(reserve);
		}
	}

	~ConcurrentQueue() {
		abortWait();
	}

	void abortWait() {
		_abort = true;
		_conditionVariable.notify_all();
	}

	void reset() {
		_abort = false;
	}

	void clear() {
		core::ScopedLock lock(_mutex);
		_data.clear();
	}

	void release() {
		core::ScopedLock lock(_mutex);
		_data.release();
	}

	template<typename ITER>
	void push(ITER first, ITER last) {
		{
			core::ScopedLock lock(_mutex);
			for (ITER i = first; i != last; ++i) {
				_data.push_back(*i);
			}
		}
		_conditionVariable.notify_one();
	}

	template<typename ITER, typename FUNC>
	void push(ITER first, ITER last, FUNC&& func) {
		{
			core::ScopedLock lock(_mutex);
			for (ITER i = first; i != last; ++i) {
				_data.push_back(func(*i));
			}
		}
		_conditionVariable.notify_one();
	}

	template<class COLLECTION>
	void pushAll(const COLLECTION& collection) {
		{
			core::ScopedLock lock(_mutex);
			_data.append(collection);
		}
		_conditionVariable.notify_one();
	}

	void push(Data const& data) {
		{
			core::ScopedLock lock(_mutex);
			_data.push_back(data);
		}
		_conditionVariable.notify_one();
	}

	void push(Data&& data) {
		{
			core::ScopedLock lock(_mutex);
			_data.push_back(core::move(data));
		}
		_conditionVariable.notify_one();
	}

	template<typename ... _Args>
	void emplace(_Args&&... __args) {
		{
			core::ScopedLock lock(_mutex);
			_data.emplace_back(core::forward<_Args>(__args)...);
		}
		_conditionVariable.notify_one();
	}

	inline bool empty() const {
		core::ScopedLock lock(_mutex);
		return _data.empty();
	}

	inline uint32_t size() const {
		core::ScopedLock lock(_mutex);
		return (uint32_t)_data.size();
	}

	bool pop(Data& poppedValue) {
		core::ScopedLock lock(_mutex);
		if (_data.empty()) {
			return false;
		}

		poppedValue = core::move(_data.front());
		_data.erase(_data.begin());
		return true;
	}

	template<class COLLECTION>
	bool popAll(COLLECTION& out) {
		core::ScopedLock lock(_mutex);
		if (_data.empty()) {
			return false;
		}

		out.append(_data);
		_data.clear();
		return true;
	}

	template<class COLLECTION>
	bool pop(COLLECTION& out, size_t n) {
		core::ScopedLock lock(_mutex);
		if (_data.empty()) {
			return false;
		}

		out.reserve(out.size() + n);
		for (size_t i = 0; i < n; ++i) {
			if (_data.size() <= i) {
				break;
			}
			out.emplace_back(core::move(_data[i]));
		}
		_data.erase(_data.begin(), n);
		return true;
	}

	bool waitAndPop(Data& poppedValue) {
		core::ScopedLock lock(_mutex);
		while (_data.empty() && !_abort) {
			if (!_conditionVariable.wait(_mutex)) {
				return false;
			}
		}
		if (_abort) {
			return false;
		}
		poppedValue = core::move(_data.front());
		_data.erase(_data.begin());
		return true;
	}
};

}
