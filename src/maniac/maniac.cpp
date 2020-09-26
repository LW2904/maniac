#include "maniac.h"

namespace maniac {
	void block_until_playing() {
		while (true) {
			if (!osu->is_playing())
				std::this_thread::sleep_for(
					std::chrono::milliseconds(250));
			else break;
		}
	}

	void play(std::vector<osu::Action> &actions) {
		auto keys = osu::Osu::get_key_subset(9);
		for (auto key : keys) {
			osu->send_keypress(key, false);
		}

		auto cur_i = 0;
		auto cur_time = 0;
		auto raw_actions = actions.data();
		auto total_actions = actions.size();

		while (cur_i < total_actions) {
			if (!osu->is_playing())
				return;

			cur_time = osu->get_game_time();
			while (cur_i < total_actions && (raw_actions + cur_i)->time <= cur_time) {
				osu->execute_action(raw_actions + cur_i);

				cur_i++;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	}

	std::vector<osu::Action> get_actions(int32_t min_time) {
		auto player = osu->get_map_player();
		auto hit_objects = player.manager.list.content;

		auto columns = std::max_element(hit_objects.begin(),
			hit_objects.end(), [](auto a, auto b) {
				return a.column < b.column; })->column + 1;
		auto keys = osu::Osu::get_key_subset(columns);

		if (config.mirror_mod)
			std::reverse(keys.begin(), keys.end());

		std::vector<osu::Action> actions;
		actions.reserve(hit_objects.size() * 2);

		for (auto &hit_object : hit_objects) {
			if (hit_object.start_time < min_time)
				continue;

			// TODO: Make the tap time an option or something.
			if (hit_object.start_time == hit_object.end_time)
				hit_object.end_time += 20;

			actions.emplace_back(keys[hit_object.column], true,
				hit_object.start_time + config.compensation_offset);
			actions.emplace_back(keys[hit_object.column], false,
				hit_object.end_time + config.compensation_offset);
		}

		debug("got %d actions", actions.size());

		std::sort(actions.begin(), actions.end());
		actions.erase(std::unique(actions.begin(), actions.end()), actions.end());

		return actions;
	}
}
