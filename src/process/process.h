#pragma once

#include "common.h"

#include <map>
#include <string>
#include <vector>
#include <climits>
#include <stdexcept>

#ifdef ON_WINDOWS
	#include <tlhelp32.h>
#endif

#ifdef ON_LINUX
	#include <sys/uio.h>
	#include <X11/Xlib.h>
	#include <X11/extensions/XTest.h>
#endif

class Process {
	#ifdef ON_WINDOWS
		using id_t = DWORD;
	#endif
	#ifdef ON_LINUX
		using id_t = pid_t;
	#endif

	struct Context {
		#ifdef ON_WINDOWS
			HANDLE handle;
		#endif

		#ifdef ON_LINUX
			id_t process_id;
			Display *display;
		#endif
	} context;

	static Context get_context(const std::string &name);
	static id_t find_process(const std::string &name);

public:
	explicit Process(const std::string &name);

	~Process();

	/**
	 * Reads `sizeof(T) * count` bytes of memory at the specified address and writes it to `*out`.
	 * Returns the number of bytes read. Generally a faster alternative to `T read_memory(...)`
	 * as it does not throw exceptions and is a bare ReadProcessMemory wrapper.
	 */
	template<typename T>
	inline size_t read_memory(uintptr_t address, T *out, size_t count = 1);

	/**
	 * Reads `sizeof(T)` bytes of memory at the specified address and returns it. *As
	 * opposed to reporting errors through the return value this function throws a runtime
	 * exception on failed read!*
	 */
	template<typename T>
	inline T read_memory(uintptr_t address);

	/**
	 * See `T read_memory`, except that it throws exceptions with the given name included in the
	 * error message.
	 */
	template<typename T, typename Any = uintptr_t>
	T read_memory_safe(const char *name, Any address);

	/**
	 * Expects `pattern` to be in "IDA-Style", i.e. to group bytes in pairs of two and to denote
	 * wildcards by a single question mark. Returns 0 if the pattern couldn't be found.
	 */
	uintptr_t find_pattern(const char *pattern);

	void send_keypress(char key, bool down) const;
};

template<typename T>
inline size_t Process::read_memory(uintptr_t address, T *out, size_t count) {
	size_t read = 0;

	#ifdef ON_WINDOWS
		if (!ReadProcessMemory(handle, reinterpret_cast<LPCVOID>(address),
			reinterpret_cast<LPVOID>(out), count * sizeof(T),
			reinterpret_cast<SIZE_T *>(&read))) {
			return 0;
		}
	#endif

	#ifdef ON_LINUX
		struct iovec local[1];
		struct iovec remote[1];

		local[0].iov_len = count * sizeof(T);
		local[0].iov_base = out;

		remote[0].iov_len = count * sizeof(T);
		remote[0].iov_base = (void *)address;

		read = process_vm_readv(context.process_id, local, 1, remote, 1, 0);
	#endif

	return read;
}

template<typename T>
inline T Process::read_memory(uintptr_t address) {
	T out;

	if (!read_memory(address, &out, 1)) {
		throw std::runtime_error("failed reading memory");
	}

	return out;
}

template<typename T, typename Any>
T Process::read_memory_safe(const char *name, Any addr) {
	// TODO: So much for "safe".
	auto address = (uintptr_t)(void *)addr;

	if (!address) {
		// TODO: Get rid of this ASAP once std::format is out.
		char msg[128];
		msg[127] = '\0';

		snprintf(msg, 128, "pointer to %s was invalid", name);

		throw std::runtime_error(msg);
	}

	T out;

	if (!read_memory(address, &out, 1)) {
		// TODO: See above.
		char msg[128];
		msg[127] = '\0';

		snprintf(msg, 128, "failed reading %s at %#x", name, address);

		throw std::runtime_error(msg);
	}

	debug_short("%s: %#x", name, (unsigned int)address);

	return out;
}

inline void Process::send_keypress(char key, bool down) const {
	#ifdef ON_WINDOWS
		// TODO: Look into KEYEVENTF_SCANCODE (see esp. KEYBDINPUT remarks section).

		static INPUT in;
		static auto layout = GetKeyboardLayout(0);

		in.type = INPUT_KEYBOARD;
		in.ki.time = 0;
		in.ki.wScan = 0;
		in.ki.dwExtraInfo = 0;
		in.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
		// TODO: Populate an array of scan codes for the keys that are going to be
		//	 pressed to avoid calculating them all the time.
		in.ki.wVk = VkKeyScanEx(key, layout) & 0xFF;

		if (!SendInput(1, &in, sizeof(INPUT))) {
			debug("failed sending input: %lu", GetLastError());
		}
	#endif

	#ifdef ON_LINUX
		KeyCode keycode = XKeysymToKeycode(context.display, key);

		if (!keycode)
			return;

		XTestFakeKeyEvent(context.display, (unsigned)keycode, down, CurrentTime);

		XFlush(context.display);
	#endif
}
