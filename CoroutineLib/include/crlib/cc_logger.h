//
// Created by John Stefanelli on 22/02/23.
//

#ifndef COROUTINELIB_CC_LOGGER_H
#define COROUTINELIB_CC_LOGGER_H

#include <thread>
#if defined(_DEBUG) || defined(DEBUG)
#include "cc_queue_config.h"
#include <string>
#include <iostream>
#include <sstream>

namespace crlib::internal {

	default_queue<std::string> log_values;

	inline void dump_logs() {
		auto v = log_values.pull();
		while(v.has_value()) {
			std::cout << "[LOG]" << v.value() << std::endl;

			v = log_values.pull();
		}
	}
}

#define CC_LOG(s) crlib::internal::log_values.push(s)
#define CC_LOG2(s) { std::stringstream ss; ss << std::hex << "[" << CC_CTID() << "]" << std::dec << s; CC_LOG(ss.str()); }
#else
#define CC_LOG(s)
#define CC_LOG2(s)
#define CC_CTID()
#endif //_DEBUG || DEBUG

#define CC_CTID() std::this_thread::get_id()

#endif //COROUTINELIB_CC_LOGGER_H
