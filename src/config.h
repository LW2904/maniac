#pragma once

#include "common.h"

#include "dependencies/argh.h"

#include <cstdio>
#include <vector>
#include <cstring>
#include <utility>

struct Output {
	static constexpr auto PAGE_WIDTH = 75;

	// TODO: None of the text breaking functions handle newlines.

	static void print_text(const char *string, const int padding = 4) {
		std::string out;

		for (size_t i = 0; string[i]; i++) {
			if (!(i % (PAGE_WIDTH - padding))) {
				if (i != 0)
					out.append(1, '\n');

				out.append(padding, ' ');
			}

			out.append(1, string[i]);
		}

		puts(out.c_str());
	}

	// TODO: This is ugly and prone to bugs.
	static void print_option(const char *s_form, const char *l_form, const char *desc) {
		constexpr auto buf_size = 1024;
		constexpr auto long_arg_length = 22;

		char preamble[PAGE_WIDTH];
		snprintf(preamble, PAGE_WIDTH, "    %s / %-*s", s_form, long_arg_length,
			l_form);

		const auto padding = strlen(preamble);

		std::string broken_desc;
		for (int i = 0; desc[i]; i++) {
			if (!(i % (PAGE_WIDTH - padding)) && i != 0) {
				broken_desc.append("\n");
				broken_desc.append(padding, ' ');
			}

			broken_desc.append(1, desc[i]);
		}

		char out_buf[buf_size];
		snprintf(out_buf, buf_size, "%s%s\n", preamble, broken_desc.c_str());

		printf("%s", out_buf);
	}
};

class config : Output {
	argh::parser cmdl;

	template<typename T>
	T get_param(std::initializer_list<const char *const> name, T def, T imp) {
		if (cmdl[name]) {
			return imp;
		}

		auto value = cmdl(name, def);

		T temp;
		value >> temp;

		return temp;
	}

	// TODO: This should be more general.
	std::pair<int, int> string_to_pair(std::string string, std::string delim = ",") {
		std::pair<int, int> out;
		auto pos = string.find_first_of(delim);

		if (pos == std::string::npos) {
			throw std::runtime_error("no delimiter found");
		}

		out.first = std::stoi(string.substr(0, pos));
		out.second = std::stoi(string.substr(pos + 1, std::string::npos));

		return out;
	}

	static void print_help() {
		printf("\nUsage: maniac [options]\n");
		printf("\nOptions:\n");

		print_option("-h", "--help", "Show this message and exit.");
		print_option("-r", "--randomization [a,b]", "Add milliseconds in the range [a,b] to all key presses. If only `a` is provided, `b` implicitly equals `-a`. (default: 0,0, implicit: -5,5)");
		print_option("-u", "--humanization [a]", "For every key press, an offset calculated through (density at that point * (a / 100)) is added to the time. (default: 0, implicit: 100)");
		print_option("-c", "--compensation [a]", "Add static offset in milliseconds to every action to compensate for the time it takes maniac to send a keypress. (default: -20)");
		print_option("-m", "--mirror-mod", "Mirror the keys pressed (i.e.: mirror mod support). (default: false, implicit: true)");

		putchar('\n');

		print_text("Note that all options have both a default and an implicit value. The difference is best illustrated through an example:\n");

		printf("    command                       humanization\n");
		printf("    $ ./maniac                    0\n");
		printf("    $ ./maniac --humanization     100\n");
		printf("    $ ./maniac --humanization 50  50\n");
	}

public:
	bool should_exit = false;

	bool mirror_mod;
	int compensation_offset;
	int humanization_modifier;
	std::pair<int, int> randomization_range = { 0, 0 };

	void parse(int argc, char *argv[]) {
		cmdl.parse(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);

		if (cmdl[{ "-h", "--help" }]) {
			should_exit = true;
			print_help();

			return;
		}

		humanization_modifier = get_param({ "-u", "--humanization" }, 0, 100);
		randomization_range = string_to_pair(get_param<std::string>(
			{ "-r", "--randomization" }, "0,0", "-5,5"));
		compensation_offset = get_param({ "-c", "--compensation" }, -20, -20);
		mirror_mod = get_param({ "-m", "--mirror-mod" }, false, true);

		debug("humanization modifier: %d", humanization_modifier);
		debug("randomization range: [%d, %d]", randomization_range.first,
			randomization_range.second);
		debug("compensation offset: %d", compensation_offset);
		debug("mirror mod: %d", mirror_mod);
	}
};
