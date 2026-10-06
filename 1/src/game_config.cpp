#include "game_config.hpp"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <set>
#include <stdexcept>

namespace {

std::string trim(const std::string& s) {
	const auto first = s.find_first_not_of(" \t\r");
	if (first == std::string::npos) {
		return "";
	}
	const auto last = s.find_last_not_of(" \t\r");
	return s.substr(first, last - first + 1);
}

constexpr Role kRequired[] = {Role::Commissar, Role::Doctor, Role::Maniac};

bool is_required(Role role) {
	return std::ranges::find(kRequired, role) != std::end(kRequired);
}

std::optional<Role> special_from_name(const std::string& name) {
	if (name == "commissar") return Role::Commissar;
	if (name == "doctor")    return Role::Doctor;
	if (name == "maniac")    return Role::Maniac;
	if (name == "ninja")     return Role::Ninja;
	if (name == "hacker")    return Role::Hacker;
	if (name == "elder")     return Role::Elder;
	return std::nullopt;
}

int parse_int(const std::string& value, const auto& fail) {
	int result = 0;
	const char* end = value.data() + value.size();
	auto [ptr, ec] = std::from_chars(value.data(), end, result);
	if (ec != std::errc{} || ptr != end) {
		fail("ожидалось целое число, а не '" + value + "'");
	}
	return result;
}

}  // namespace

GameConfig load_game_config(const std::string& path) {
	GameConfig config;

	std::ifstream file(path);
	if (!file) {
		std::cerr << "Нет " << path << " - используются значения по умолчанию\n";
		return config;
	}

	std::set<std::string> seen;
	bool in_specials = false;
	std::vector<Role> specials;

	std::string raw;
	int number = 0;
	auto fail = [&](const std::string& message) {
		throw std::runtime_error(path + ", строка " + std::to_string(number) + ": " + message);
	};

	while (std::getline(file, raw)) {
		++number;

		const std::string text = raw.substr(0, raw.find('#'));
		if (trim(text).empty()) {
			continue;
		}
		const bool indented = text[0] == ' ' || text[0] == '\t';

		const auto colon = text.find(':');
		if (colon == std::string::npos) {
			fail("ожидалось 'ключ: значение'");
		}
		const std::string key = trim(text.substr(0, colon));
		const std::string value = trim(text.substr(colon + 1));

		if (!seen.insert(key).second) {
			fail("'" + key + "' повторяется");
		}

		if (!indented) {
			in_specials = false;
			if (key == "default_players") {
				config.default_players = parse_int(value, fail);
				if (config.default_players < 5 || config.default_players > 20) {
					fail("default_players должен быть от 5 до 20");
				}
			} else if (key == "mafia_divisor") {
				config.mafia_divisor = parse_int(value, fail);
				if (config.mafia_divisor < 3) {
					fail("mafia_divisor должен быть не меньше 3");
				}
			} else if (key == "specials") {
				if (!value.empty()) {
					fail("после 'specials:' роли перечисляются на следующих строках с отступом");
				}
				in_specials = true;
			} else {
				fail("неизвестный ключ '" + key + "'");
			}
		} else {
			if (!in_specials) {
				fail("лишний отступ");
			}
			const auto role = special_from_name(key);
			if (!role) {
				fail("неизвестная роль: '" + key + "'");
			}
			const int count = parse_int(value, fail);
			if (count != 0 && count != 1) {
				fail(key + ": допустимо только 0 или 1");
			}
			if (count == 1) {
				specials.push_back(*role);
			} else if (is_required(*role)) {
				fail(key + " - обязательная роль, должно быть 1");
			}
		}
	}

	if (!seen.contains("specials")) {
		return config;
	}
	for (Role role : kRequired) {
		if (std::ranges::find(specials, role) == specials.end()) {
			throw std::runtime_error(path + ": нет обязательной роли «" + role_to_string(role) + "»");
		}
	}
	config.specials = std::move(specials);
	return config;
}
