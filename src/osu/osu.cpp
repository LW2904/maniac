#include "osu.h"

using namespace osu;

Osu::Osu() : Process("osu!.exe") {
	using namespace signatures;

	// TODO: Run this in parallel.

	auto time_location = find_pattern(TIME_SIG) + TIME_SIG_OFFSET;
	debug("time pattern found at %#lx", time_location);

	auto player_location = find_pattern(PLAYER_SIG) + PLAYER_SIG_OFFSET;
	debug("player pattern found at %#lx", player_location);

	time_address = read_memory<uintptr_t>(time_location);
	debug("got time address: %#lx", time_address);

	player_pointer = read_memory<uintptr_t>(player_location);
	debug("got player pointer: %#lx", player_pointer);
}

Osu::~Osu() = default;

std::string Osu::get_key_subset(int column_count) {
	// TODO: This pile of shit is copied pretty much straight from maniac 0.x and needs to
	// 	 be refactored.

	if (column_count > 9) {
		throw std::runtime_error("maps with more than 9 columns are not supported");
	}

	if (column_count <= 0) {
		throw std::runtime_error("got negative column count");
	}

	const char *keys = "asdfjkl[";
	constexpr auto keys_len = 8;

	const size_t key_subset_len = column_count + 1;
	char *const key_subset = reinterpret_cast<char *>(malloc(key_subset_len));

	if (!key_subset) {
		throw std::runtime_error("failed allocating memory");
	}

	key_subset[column_count] = '\0';

	const size_t subset_offset = (keys_len / 2) - (column_count / 2);

	memmove(key_subset, reinterpret_cast<const void *>(keys + subset_offset),
		keys_len - (subset_offset * 2));

	if (column_count % 2) {
		auto offset = column_count / 2;
		memmove(key_subset + offset + 1, key_subset + offset,
			offset);

		key_subset[column_count / 2] = ' ';
	}

	auto out_string = std::string(key_subset);
	free(key_subset);

	return out_string;
}

internal::map_player Osu::get_map_player() {
	internal::process = this;

	auto player_address = read_memory_safe<uintptr_t>("player", player_pointer);

	return internal::map_player(player_address);
}

void Action::log() const {
#ifdef DEBUG
	debug("action:");
	debug("    time: %d", time);
	debug("    key: %c", key);
	debug("    down: %d", down);
#else
	return;
#endif
}
