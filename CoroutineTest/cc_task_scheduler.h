#pragma once
#include <coroutine>
#include <thread>
#include <memory>
#include <optional>
#include "cc_queue.h"

struct BaseTaskScheduler {
	
};

thread_local std::shared_ptr<BaseTaskScheduler> current_scheduler = nullptr;