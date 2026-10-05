#include "game_config.hpp"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace {

std::string trim(std::string_view s) {
	const auto first = s.find_first_not_of(" \t\r");
	if (first == std::string_view::npos) {
		return "";
	}
	const auto last = s.find_last_not_of(" \t\r");
	return std::string(s.substr(first, last - first + 1));
}

constexpr Role kRequired[] = {Role::Commissar, Role::Doctor, Role::Maniac};

bool is_required(Role role) {
	return std::ranges::find(kRequired, role) != std::end(kRequired);
}

std::optional<Role> special_from_name(std::string_view name) {
	if (name == "commissar") return Role::Commissar;
	if (name == "doctor")    return Role::Doctor;
	if (name == "maniac")    return Role::Maniac;
	if (name == "hacker")    return Role::Hacker;
	if (name == "elder")     return Role::Elder;
	return std::nullopt;
}

class ConfigError {
public:
	ConfigError(const std::string& path, int line) : prefix_(path + ", строка " + std::to_string(line) + ": ") {}

	[[noreturn]] void operator()(const std::string& message) const {
		throw std::runtime_error(prefix_ + message);
	}

private:
	std::string prefix_;
};

int parse_int(const std::string& value, const ConfigError& fail) {
	int result = 0;
	const char* end = value.data() + value.size();
	auto [ptr, ec] = std::from_chars(value.data(), end, result);
	if (ec != std::errc{} || ptr != end) {
		fail("ожидалось целое число, а не '" + value + "'");
	}
	return result;
}

int parse_flag(const std::string& key, const std::string& value, const ConfigError& fail) {
	const int result = parse_int(value, fail);
	if (result != 0 && result != 1) {
		fail(key + ": допустимо только 0 или 1");
	}
	return result;
}

}  // namespace

GameConfig load_game_config(const std::string& path) {
	GameConfig config;

	std::ifstream file(path);
	if (!file) {
		std::cerr << "Нет " << path << " — используются значения по умолчанию\n";
		return config;
	}

	bool seen_divisor = false;
	bool seen_ninja = false;
	bool seen_specials = false;
	bool in_specials = false;
	std::vector<std::pair<Role, int>> specials;

	std::string raw;
	int number = 0;
	while (std::getline(file, raw)) {
		++number;
		if (number == 1 && raw.starts_with("\xEF\xBB\xBF")) {
			raw.erase(0, 3);
		}
		const ConfigError fail(path, number);

		const std::string text = raw.substr(0, raw.find('#'));
		if (trim(text).empty()) {
			continue;
		}
		if (text[0] == '\t') {
			fail("табы в отступах запрещены");
		}
		const bool indented = text[0] == ' ';

		const auto colon = text.find(':');
		if (colon == std::string::npos) {
			fail("ожидалось 'ключ: значение'");
		}
		const std::string key = trim(std::string_view(text).substr(0, colon));
		const std::string value = trim(std::string_view(text).substr(colon + 1));

		if (!indented) {
			in_specials = false;
			if (key == "mafia_divisor") {
				if (seen_divisor) {
					fail("mafia_divisor повторяется");
				}
				seen_divisor = true;
				config.mafia_divisor = parse_int(value, fail);
				if (config.mafia_divisor < 3) {
					fail("mafia_divisor должен быть не меньше 3");
				}
			} else if (key == "ninja") {
				if (seen_ninja) {
					fail("ninja повторяется");
				}
				seen_ninja = true;
				config.ninja = parse_flag(key, value, fail);
			} else if (key == "specials") {
				if (seen_specials) {
					fail("specials повторяется");
				}
				if (!value.empty()) {
					fail("после 'specials:' роли перечисляются на следующих строках с отступом");
				}
				seen_specials = true;
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
				fail("неизвестная роль '" + key + "' (есть: commissar, doctor, maniac, hacker, elder)");
			}
			if (std::ranges::find(specials, *role, &std::pair<Role, int>::first) != specials.end()) {
				fail("роль '" + key + "' повторяется");
			}
			const int count = parse_flag(key, value, fail);
			if (count == 0 && is_required(*role)) {
				fail(key + " — обязательная роль, должно быть 1");
			}
			specials.push_back({*role, count});
		}
	}

	if (seen_specials) {
		// Не указанные обязательные роли добавляются в начало списка,
		// чтобы им гарантированно хватило мест
		std::vector<std::pair<Role, int>> missing;
		for (Role role : kRequired) {
			if (std::ranges::find(specials, role, &std::pair<Role, int>::first) == specials.end()) {
				missing.push_back({role, 1});
			}
		}
		specials.insert(specials.begin(), missing.begin(), missing.end());
		config.specials = std::move(specials);
	}
	return config;
}
