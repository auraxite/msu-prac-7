#include <charconv>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "host.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

constexpr int kDefaultPlayers = 7;
constexpr int kMinPlayers = 5;
constexpr int kMaxPlayers = 20;

int usage_error(const std::string& message) {
	std::cerr << message << "\n"
	          << "Использование: mafia [--players N | -n N] [--full-log] [--interactive] [--open-announcements]\n";
	return 1;
}

}  // namespace

int main(int argc, char* argv[]) {
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
#endif

	int count = kDefaultPlayers;
	bool full_log = false;
	bool interactive = false;
	bool open_announcements = false;
	for (int i = 1; i < argc; ++i) {
		std::string_view arg = argv[i];
		if (arg == "--full-log") {
			full_log = true;
		} else if (arg == "--interactive") {
			interactive = true;
		} else if (arg == "--open-announcements") {
			open_announcements = true;
		} else if (arg == "--players" || arg == "-n") {
			if (i + 1 >= argc) {
				return usage_error("После " + std::string(arg) + " нужно указать число игроков");
			}
			std::string_view value = argv[++i];
			const char* end = value.data() + value.size();
			auto [ptr, ec] = std::from_chars(value.data(), end, count);
			if (ec != std::errc{} || ptr != end) {
				return usage_error("Число игроков должно быть целым числом: " + std::string(value));
			}
		} else {
			return usage_error("Неизвестный аргумент: " + std::string(arg));
		}
	}

	if (count < kMinPlayers || count > kMaxPlayers) {
		return usage_error("Число игроков должно быть от " + std::to_string(kMinPlayers) + " до "
		                   + std::to_string(kMaxPlayers) + ", получено " + std::to_string(count));
	}

	std::vector<std::string> names;
	for (int i = 1; i <= count; ++i) {
		names.push_back("Игрок " + std::to_string(i));
	}

	Host host(names, full_log, interactive, open_announcements);
	host.run();
	return 0;
}
