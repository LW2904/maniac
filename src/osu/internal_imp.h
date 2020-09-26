#pragma once

#include "../process/process.h"

inline Process *process;

struct hit_object {
	uintptr_t base;

	int32_t start_time;
	int32_t end_time;
	int32_t type;
	int32_t column;

	hit_object() : base(0), start_time(0), end_time(0), type(0),
		column(0) {}

	explicit hit_object(uintptr_t base) : base(base) {
		start_time = get_start_time();
		end_time = get_end_time();
		type = get_type();
		column = get_column();
	}

	[[nodiscard]] int32_t get_start_time() const {
		return process->read_memory<int32_t>(base + 0x10);
	}

	[[nodiscard]] int32_t get_end_time() const {
		return process->read_memory<int32_t>(base + 0x14);
	}

	[[nodiscard]] int32_t get_type() const {
		return process->read_memory<int32_t>(base + 0x18);
	}

	[[nodiscard]] int32_t get_column() const {
		return process->read_memory<int32_t>(base + 0x9C);
	}
};

template<typename T>
struct list_container {
	uintptr_t base;

	size_t size;
	std::vector<T> content;

	explicit list_container(uintptr_t base) : base(base), size(get_size()),
		content(get_content()) {}

	[[nodiscard]] size_t get_size() {
		return process->read_memory_safe<size_t>("list contents size",
			base + 0xC);
	}

	[[nodiscard]] std::vector<T> get_content() {
		auto contents_address = process->read_memory_safe<uintptr_t>(
			"list contents address", base + 0x4);

		auto size = get_size();

		std::vector<T> vector;
		vector.reserve(size);

		for (size_t i = 0; i < size; i++) {
			vector.push_back(T(process->read_memory<uintptr_t>(
				contents_address + 0x8 + (i * 0x4))));
		}

		return vector;
	}
};

struct hit_manager_headers {
	uintptr_t base;

	int column_count;

	explicit hit_manager_headers(uintptr_t base) : base(base),
		column_count(get_column_count()) {}

	[[nodiscard]] int get_column_count() const {
		return static_cast<int>(process->read_memory_safe<float>(
			"column count", base + 0x30));
	}
};

struct hit_manager {
	uintptr_t base;

	hit_manager_headers headers;
	list_container<hit_object> list;

	explicit hit_manager(uintptr_t base) : base(base), headers(get_headers()),
		list(get_list()) {}

	[[nodiscard]] hit_manager_headers get_headers() const {
		return hit_manager_headers(
			process->read_memory_safe<uintptr_t>("hit manager headers",
				base + 0x30));
	}

	[[nodiscard]] list_container<hit_object> get_list() const {
		return list_container<hit_object>(process->read_memory_safe<uintptr_t>(
			"hitpoint list container", base + 0x48));
	}
};

struct map_player {
	uintptr_t base;

	hit_manager manager;

	explicit map_player(uintptr_t base) : base(base), manager(get_hit_manager()) {}

	[[nodiscard]] hit_manager get_hit_manager() const {
		return hit_manager(process->read_memory_safe<uintptr_t>("hit manager",
			base + 0x40));
	}
};
