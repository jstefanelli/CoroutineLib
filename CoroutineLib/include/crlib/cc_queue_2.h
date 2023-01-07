#pragma once
#include <thread>
#include <memory>
#include <atomic>
#include <vector>
#include <optional>

namespace crlib {
	template<typename T>
	class SingleProducerQueue {
	protected:
		std::vector<T> data;
		std::atomic_size_t next_writable;
		std::atomic<size_t> next_readable;
	public:
		explicit SingleProducerQueue(size_t queue_size) : data(queue_size), next_writable(0), next_readable(0) {

		}

		bool write(const T& val) {
			auto free_slot = next_writable.load();
			auto readable = next_readable.load();

			auto next_free = (free_slot + 1) % data.size();
			if (next_free == readable) {
				return false;
			}

			data[free_slot] = val;
			next_writable.store(next_free);

			return true;
		}

		std::optional<T> read() {
			auto readable_slot = next_readable.load();
			size_t readable_next;
			T val;
			do {
				readable_next = (readable_slot + 1) % data.size();
				auto writable = next_writable.load();

				if (readable_slot == writable) {
					return std::nullopt;
				}

				val = data[readable_slot];
			} while(!next_readable.compare_exchange_weak(readable_slot, readable_next, std::memory_order_seq_cst, std::memory_order_seq_cst));

			return val;
		}

		bool empty() const {
			auto readable_slot = next_readable.load();
			auto writable_slot = next_writable.load();

			return readable_slot == writable_slot;
		}

		bool full() const {
			auto readable_slot = next_readable.load();
			auto writable_slot = next_writable.load();
			auto next_free = (writable_slot + 1) % data.size();

			return next_free == readable_slot;
		}

		size_t size() {
			return data.size();
		}
	};
}