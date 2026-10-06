#include <charconv>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "game_config.hpp"
#include "host.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

constexpr int kMinPlayers = 5;
constexpr int kMaxPlayers = 20;
constexpr const char* kConfigPath = "config/roles.yaml";

int usage_error(const std::string& message) {
	std::cerr << message << "\n"
	          << "Параметры запуска: [--full-log] [--interactive] [--log] [--open-announcements] [--players N | -n N] [--seed S]\n";
	return 1;
}

}  // namespace

int main(int argc, char* argv[]) {
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
#endif

	GameConfig config;
	try {
		config = load_game_config(kConfigPath);
	} catch (const std::exception& e) {
		std::cerr << "Ошибка конфигурации: " << e.what() << "\n";
		return 1;
	}

	int count = config.default_players;
	bool full_log = false;
	bool interactive = false;
	bool open_announcements = false;
	bool log = false;
	std::optional<unsigned> seed;
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--full-log") {
			full_log = true;
		} else if (arg == "--log") {
			log = true;
		} else if (arg == "--interactive") {
			interactive = true;
		} else if (arg == "--open-announcements") {
			open_announcements = true;
		} else if (arg == "--players" || arg == "-n") {
			if (i + 1 >= argc) {
				return usage_error("После " + std::string(arg) + " нужно указать число игроков");
			}
			std::string value = argv[++i];
			const char* end = value.data() + value.size();
			auto [ptr, ec] = std::from_chars(value.data(), end, count);
			if (ec != std::errc{} || ptr != end) {
				return usage_error("Число игроков должно быть целым числом: " + std::string(value));
			}
		} else if (arg == "--seed") {
			if (i + 1 >= argc) {
				return usage_error("После --seed нужно указать число");
			}
			std::string value = argv[++i];
			const char* end = value.data() + value.size();
			unsigned parsed = 0;
			auto [ptr, ec] = std::from_chars(value.data(), end, parsed);
			if (ec != std::errc{} || ptr != end) {
				return usage_error("Seed должен быть неотрицательным целым числом: " + value);
			}
			seed = parsed;
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

	try {
		std::cout << "\n";
		Host host(names, config, full_log, interactive, log, open_announcements, seed);
		host.run();
		std::cout << "\n";
	} catch (const std::invalid_argument& e) {
		std::cerr << "Не удалось раздать роли (" << e.what() << "): проверьте " << kConfigPath << " и -n\n";
		return 1;
	}
	return 0;
}
