#pragma once
#include <optional>
#include <atomic>
#include <memory>

template<typename T>
struct Concurrent_Queue_t {
protected:
	struct Queue_Section_t {
	protected:
		T* data;
		size_t size;
		std::atomic_size_t start;
		std::atomic_size_t read_end;
		std::atomic_size_t write_end;

	public:
		std::atomic<std::shared_ptr<Queue_Section_t>> next;
		Queue_Section_t(size_t size) : size(size), start(0), write_end(0), read_end(0), next(std::shared_ptr<Queue_Section_t>(nullptr)) {
			data = reinterpret_cast<T*>(std::malloc(sizeof(T) * size));
		}

		~Queue_Section_t() {
			std::free(data);
		}

		void Push(const T& value) {
			size_t e = write_end.load();
			size_t n;

			do {
				if (e >= size) {
					std::shared_ptr<Queue_Section_t> next_section;

					do {
						next_section = next.load();
					} while (next_section == nullptr);

					next_section->Push(value);
					return;
				}
				n = e + 1;
			} while (!write_end.compare_exchange_weak(e, n, std::memory_order_release, std::memory_order_relaxed));
			data[e] = value;

			read_end.fetch_add(1);

			if (e == size / 2) {
				next.store(std::make_shared<Queue_Section_t>(size));
			}
		}

		std::optional<T> Pull() {
			size_t s = start.load();
			size_t e;

			do {
				e = read_end.load();
				if (s >= e) {
					return std::optional<T>();
				}
			} while (!start.compare_exchange_weak(s, s + 1, std::memory_order_release, std::memory_order_relaxed));

			return std::optional<T>(data[s]);
		}

		bool Full() const {
			return write_end.load() >= size;
		}

		bool Completed() const {
			return Full() && start.load() >= size;
		}
	};

	std::atomic<std::shared_ptr<Queue_Section_t>> first_section;
	std::atomic<std::shared_ptr<Queue_Section_t>> last_section;
	size_t chunk_size;

public:
	Concurrent_Queue_t(size_t chunk_size = 64U) : chunk_size(chunk_size), first_section(std::make_shared<Queue_Section_t>(chunk_size)) {
		last_section.store(first_section.load());
	}

	void Push(const T& item) {
		auto l = last_section.load();
		l->Push(item);
		auto next = l->next.load();

		if (l->Full()) {
			do {
				next = l->next.load();
				if (!l->Full() || next == nullptr)
					return;
			} while (!last_section.compare_exchange_weak(l, next, std::memory_order_release, std::memory_order_relaxed));
		}
	}

	std::optional<T> Pull() {
		auto f = first_section.load();
		std::optional<T> val = f->Pull();
		std::shared_ptr<Queue_Section_t> next;

		do {
			next = f->next.load();
			if ((!f->Completed()) || next == nullptr)
				break;
		} while (!first_section.compare_exchange_weak(f, next, std::memory_order_release, std::memory_order_relaxed));

		return val;
	}
};