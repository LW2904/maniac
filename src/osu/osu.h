#pragma once

#include "common.h"
#include "../process/process.h"
#include "signatures.h"

#include <chrono>
#include <thread>
#include <vector>
#include <random>
#include <cstring>
#include <algorithm>

// TODO: I don't like the osu namespace since it leads to the ugly `osu::Osu` but
//	 I also don't want `Action` and `internal` to be in the global namespace.

namespace osu {
	namespace internal {
		// Still not sure whether I like this approach, seems kind of hacky.
		// Beats having a internal.cpp with only the declarations living
		// here because templates would have to be implemented here, which sucks.
		#include "internal_imp.h"
	};

	struct Action {
		char key;
		bool down;
		int32_t time;

		Action(char key, bool down, int32_t time) : key(key), down(down),
			time(time) { };

		bool operator < (const Action &action) const {
			return time < action.time;
		};

		bool operator == (const Action &action) const {
			return action.key == key && action.down == down
				&& action.time == time;
		};

		// Only used for debugging
		void log() const;
	};

	class Osu : public Process {
		int tap_time = 50;

		// TODO: Generic pointers are bad in the long run.
		uintptr_t time_address = 0;
		uintptr_t player_pointer = 0;

	public:
		Osu();

		~Osu();

		int32_t get_game_time();

		bool is_playing();

		internal::map_player get_map_player();

		void execute_action(Action *action);

		static std::string get_key_subset(int column_count);
	};

	inline int32_t Osu::get_game_time() {
		int32_t time = -1;

		if (!read_memory<int32_t>(time_address, &time)) {
			debug("%s %#x", "failed getting game time at",
				(unsigned int)(time_address));
		}

		return time;
	}

	inline bool Osu::is_playing() {
		uintptr_t address = 0;

		size_t read = read_memory<uintptr_t>(player_pointer, &address, 1);

		if (!read) {
			debug("%s %#x", "failed getting player address at", address);
		}

		return address != 0;
	}

	inline void Osu::execute_action(Action *action) {
		send_keypress(action->key, action->down);
	}
}
