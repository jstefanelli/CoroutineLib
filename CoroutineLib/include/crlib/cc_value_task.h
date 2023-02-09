#ifndef COROUTINELIB_CC_VALUE_TASK_H
#define COROUTINELIB_CC_VALUE_TASK_H

#include "cc_task_locks.h"
#include "cc_task_types.h"
#include "cc_base_promise.h"

namespace crlib {

}

template<crlib::NotVoid T, typename ... Args>
struct std::coroutine_traits<crlib::ValueTask<T>, Args...> {
struct promise_type : public crlib::BasePromise<crlib::ValueTask<T>, crlib::Single_Awaitable_Task_lock<T>> {
	void return_value(T value) {
		this->lock->set_result(std::move(value));
	}
};
};

#endif //COROUTINELIB_CC_VALUE_TASK_H
