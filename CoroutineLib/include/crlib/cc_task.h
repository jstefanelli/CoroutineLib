#pragma once
#include <memory>
#include <semaphore>
#include <coroutine>
#include <optional>
#include <functional>
#include "cc_task_locks.h"
#include "cc_generic_queue.h"
#include "cc_api.h"
#include "cc_task_scheduler.h"
#include "cc_task_types.h"
#include "cc_awaitables.h"
#include "cc_generator_task.h"
#include "cc_base_promise.h"


template<typename ... Args>
struct std::coroutine_traits<crlib::Task<void>, Args...> {
    struct promise_type : public crlib::BasePromise<crlib::Task<void>, crlib::Task_lock<void>> {
        void return_void() {

        }
    };
};

template<typename T, typename ... Args>
struct std::coroutine_traits<crlib::Task<T>, Args...> {
    struct promise_type : public crlib::BasePromise<crlib::Task<T>, crlib::Task_lock<T>> {
        void return_value(T val) {
            this->lock->set_result(val);
		}
	};
};
