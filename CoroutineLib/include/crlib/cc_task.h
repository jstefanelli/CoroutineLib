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
#include "cc_value_task.h"


template<typename ... Args, typename Scheduler>
struct std::coroutine_traits<crlib::Task<void, Scheduler>, Args...> {
    struct promise_type : public crlib::BasePromise<crlib::Task<void, Scheduler>, crlib::Task_lock<void>> {
        void return_void() {

        }
    };
};

template<typename T, typename ... Args, typename Scheduler>
struct std::coroutine_traits<crlib::Task<T, Scheduler>, Args...> {
    struct promise_type : public crlib::BasePromise<crlib::Task<T, Scheduler>, crlib::Task_lock<T>> {
        void return_value(T&& val) {
            this->lock->set_result(std::move(val));
		}
	};
};
