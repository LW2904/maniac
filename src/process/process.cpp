#include "process.h"

Process::Process(const std::string &name) {
	context = get_context(name);
}

Process::~Process() {
	#ifdef ON_WINDOWS
		CloseHandle(context.handle);
	#endif

	#ifdef ON_LINUX
		debug("stub");
	#endif
}

Process::id_t Process::find_process(const std::string &name) {
	#ifdef ON_WINDOWS
		PROCESSENTRY32 processInfo;
		processInfo.dwSize = sizeof(PROCESSENTRY32);

		HANDLE snapshot = CreateToolhelp32Snapshot(
			TH32CS_SNAPPROCESS, 0);
		if (!snapshot || snapshot == INVALID_HANDLE_VALUE) {
			throw std::runtime_error(
				"failed to get process list");
		}

		Process32First(snapshot, &processInfo);
		if (name == processInfo.szExeFile) {
			CloseHandle(snapshot);
			return processInfo.th32ProcessID;
		}

		while (Process32Next(snapshot, &processInfo)) {
			if (name == processInfo.szExeFile) {
				CloseHandle(snapshot);
				return processInfo.th32ProcessID;
			}
		}

		CloseHandle(snapshot);
		return 0ul;
	#endif

	#ifdef ON_LINUX
		// TODO: Straight outta old maniac.
		char *cmd = (char *)calloc(1, 200);
		sprintf(cmd, "pidof %s", name.c_str());

		FILE *f = popen(cmd, "r");
		size_t read = fread(cmd , 1, 200, f);
		fclose(f);

		auto process_id = read
			? (unsigned long)strtol(cmd, nullptr, 10)
			: 0;
		free(cmd);

		return process_id;
	#endif
}

Process::Context Process::get_context(const std::string &name) {
	auto process_id = find_process(name);
	if (!process_id) {
		throw std::runtime_error("failed finding process");
	}

	debug("%s '%s' %s %d", "found process", name.c_str(), "with id", process_id);

	#ifdef ON_WINDOWS
		HANDLE handle = OpenProcess(PROCESS_VM_READ, FALSE, process_id);
		if (!handle || handle == INVALID_HANDLE_VALUE) {
			throw std::runtime_error("failed opening handle to process");
		}

		debug("%s '%s' (%#x)", "got handle to process", name.c_str(),
			(uintptr_t)handle);

		Context ctx{};
		ctx.handle = handle:
		return ctx;
	#endif

	#ifdef ON_LINUX
		Display* display;
		if (!(display = XOpenDisplay(nullptr))) {
			throw std::runtime_error("failed opening X display");
		}

		debug("%s %#x", "opened X display", (unsigned)(uintptr_t)display);

		Context ctx{};
		ctx.display = display;
		ctx.process_id = process_id;
		return ctx;
	#endif
}

uintptr_t Process::find_pattern(const char *pattern) {
	auto pattern_bytes = std::vector<int>{ };

	for (auto cur = pattern; *cur; cur++) {
		if (*cur == '?') {
			// If the current byte is a wildcard push a dummy byte
			pattern_bytes.push_back(-1);
		} else if (*cur == ' ') {
			continue;
		} else {
			// This is somewhat hacky: strtol parses *as many characters as
			// possible* and sets cur to the first character it couldn't parse.
			// This is the reason why patterns *must* follow the structure
			// "AB CD EF ? ? FF"; "ABCDEF??FF" would cause it to fail later (!).
			pattern_bytes.push_back(strtol(cur, const_cast<char **>(&cur), 16));
		}
	}

	auto pattern_size = pattern_bytes.size();

	const size_t chunk_size = 4096;
	std::byte chunk_bytes[chunk_size];

	uintptr_t i;
	uintptr_t failed_reads = 0;

	for (i = 1; i < INT_MAX; i += chunk_size - pattern_size) {
		if (!read_memory<std::byte>(i, chunk_bytes, chunk_size)) {
			failed_reads++;

			continue;
		}

		for (size_t j = 0; j < chunk_size; j++) {
			bool hit = true;

			for (size_t k = 0; k < pattern_size; k++) {
				if (pattern_bytes[k] == -1) {
					continue;
				}

				if (chunk_bytes[j + k] !=
					static_cast<std::byte>(pattern_bytes[k])) {
					hit = false;
					break;
				}
			}

			if (hit) {
				return i + j;
			}
		}
	}

	debug("%f%% of reads failed (%lu/%lu)", ((double)failed_reads / (double)i) * 100,
       		failed_reads, i)

	throw std::runtime_error("pattern not found");
}
