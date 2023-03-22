# CoroutineLib
A rudimentary library for C#-like Task functionality on Cpp

## Dependencies
A cpp-20 compatible compiler.

The library has been compiled successfully on Windows with *MSVC 2022* and on macOS/Linux with *GCC*.

## Usage

### Basics

To start a task, simply define a function/method/lambda that returns a `crlib::Task<T>` object and call it: 
```c++
#include <crlib/cc_task.h>

crlib::Task<int> asyncSum(int x, int y) {
	co_return x + y;
}

int main() {
	auto task = asyncSum(1, 2);
	
	return 0;
}
```

To wait for a task's result from outside an asynchronous context, use the `wait()` method

```c++
// (asyncSum from the previous example)

int main() {
	auto task = asyncSum(1, 2);
	
	auto result = task.wait();
	
	return result;
}
```

To wait for a task from inside another task, you can simply `co_await` for it:

```c++
// (asyncSum from the first example, again)

crlib::Task<int> asyncMultiply(int x, int y) {
	int val = 0;
	for(int i = 0; i < y; i++) {
		val = co_await asyncSum(val, x);
	}
	
	co_return val;
}

int main() {
	auto result = asyncMultiply(2, 2).wait();
	
	return result;
}
```

### Where's `co_yield`?

To yield values, you can use a `crlib::GeneratorTask<T>` as such:

```c++
#include <iostream>
#include <crlib/cc_task.h>

crlib::GeneratorTask<int> asyncFibonacci(int iterations) {
	int a = 0, b = 1;
	
	for(int i = 0; i < iterations; i++) {
		if (i == 0) {
			co_yield 0;
		} else {
			co_yield b;
			int next = a + b;
			a = b;
			b = next;
		}
	}
}

crlib::Task<void> printer() {
	auto fib = asyncFibonacci(10);
	
	std::optional<int> n;
	do {
		n = co_await fib;
		if (n.has_value()) {
			std::cout << n.value() << " ";
		}
 	} while(n.has_value());
	
	std::cout << std::endl;
}

int main() {
	printer().wait();
}
```

### Waiting for multiple tasks

To wait for multiple tasks, use the `crlib::WhenAll()` function:
```c++
#include <thread>
#include <chorno>
#include <crlib/cc_task.h>

crlib::Task<void> sleeper(int seconds) {
	std::this_thread::sleep_for(std::chrono::seconds(seconds));
	co_return;
}

crlib::Task<void> waiter() {
	auto t0 = sleeper(2);
	auto t1 = sleeper(5);
	auto t2 = sleeper(7); //WARNING: These are not guaranteed to start on separate threads
	
	co_await crlib::WhenAll<>(t0, t1, t2);
}

int main() {
	waiter().wait();
}

```

### Synchronization Tools

This library offers a couple of synchronization tools:

#### AsyncMutex

A mutex for coroutines, guarantees that only a single coroutine is executed at the same time:

```c++
#include <crlib/cc_task.h>
#include <crlib/cc_sync_utils.h>
#include <queue>

crlib::AsyncMutex mutex;
std::queue<int> queue;

crlib::Task<> producer(int amount) {
	for(int i = 0; i < amount; i++) {
		auto guard = co_await mutex.await();

		queue.push(i);
		//Mutex is released when 'guard' goes out of scope, or when calling 'guard->release()'
	}
}

crlib::Task<> consumer(int max) {
	int val = 0;
	do {
		auto guard = co_await mutex.await();

		if (queue.empty()) {
			continue;
		}
		val = queue.front();
		queue.pull();
	} while(val < max - 1);
}

int main() {
	auto p = producer(10);
	auto c = consumer(10);
	
	[p, c]() -> crlib::Task<> {
		co_await crlib::WhenAll(p, c);
	}().wait();
}
```

#### AsyncConditionVariable

To have a set of coroutines wait for "a signal", `crlib::AsyncConditionVariable` can be used:

```c++
#include <crlib/cc_task.h>
#include <crlib/cc_sync_utils.h>
#include <iostream>
#include <thread>
#include <chrono>

crlib::AsyncConditionVariable cv;

crlib::Task<> waiter(int i) {
	co_await cv.await();
	std::cout << "Task " << i << " resuming" << std::endl;
}

int main() {
	auto w0 = waiter(0);
	auto w1 = waiter(1);
	
	std::this_thread::sleep_for(std::chrono::seconds(5));
	
	cv.notify_all();
	
	[w0, w1]() -> crlib::Task<> {
		co_await crlib::WhenAll(w0, w1);
	}().wait();
	
	return 0;
}
```

## Under the hood

By default, every `crlib::Task<T>`, `crlib::GeneratorTask<T>` are executed on a "default thread pool", created when the first coroutine is scheduled for execution.

You can supply a custom "task scheduler" with the second template parameter for `Task`s and `GeneratorTask`s.

You can find an example in the `CoroutineTest/SchedulerTest.cpp` file 

## Important TODOs

 - a `crlib::WhenAny()` method
 - a `crlib::Delay(int milliseconds)` method/task

## License

MIT License

Copyright (c) 2023 Giovanni Stefanelli

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
