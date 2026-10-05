#include "logger.hpp"

#include <algorithm>
#include <charconv>
#include <iostream>
#include <string_view>
#include <system_error>

namespace fs = std::filesystem;

namespace {

constexpr std::string_view kMatchPrefix = "match_";

std::string numbered(std::string_view prefix, int number) {
	std::string digits = std::to_string(number);
	if (digits.size() < 3) {
		digits.insert(0, 3 - digits.size(), '0');
	}
	return std::string(prefix) + digits;
}

int match_number(const fs::directory_entry& entry) {
	if (!entry.is_directory()) {
		return 0;
	}
	const std::string name = entry.path().filename().string();
	if (!name.starts_with(kMatchPrefix)) {
		return 0;
	}
	const char* begin = name.data() + kMatchPrefix.size();
	const char* end = name.data() + name.size();
	int number = 0;
	auto [ptr, ec] = std::from_chars(begin, end, number);
	return ec == std::errc{} && ptr == end ? number : 0;
}

}  // namespace

Logger::Logger(const fs::path& root) {
	std::error_code ec;
	fs::create_directories(root, ec);
	if (ec) {
		std::cerr << "Не удалось создать папку " << root.string() << " (" << ec.message() << ") — игра без лога\n";
		return;
	}

	int last = 0;
	for (const auto& entry : fs::directory_iterator(root, ec)) {
		last = std::max(last, match_number(entry));
	}
	if (ec) {
		std::cerr << "Не удалось прочитать папку " << root.string() << " (" << ec.message() << ") — игра без лога\n";
		return;
	}

	const fs::path dir = root / numbered(kMatchPrefix, last + 1);
	fs::create_directory(dir, ec);
	if (ec) {
		std::cerr << "Не удалось создать папку " << dir.string() << " (" << ec.message() << ") — игра без лога\n";
		return;
	}

	dir_ = dir;
	std::cout << "Лог матча: " << dir_.string() << "\n";
	start_round(0);
}

void Logger::start_round(int round) {
	if (!enabled()) {
		return;
	}
	round_.close();
	const fs::path file = dir_ / (numbered("round_", round) + ".txt");
	round_.open(file);
	if (!round_) {
		std::cerr << "Не удалось создать " << file.string() << " — раунд не будет записан\n";
	}
}

void Logger::write(const std::string& line) {
	if (round_.is_open()) {
		round_ << line << std::endl;
	}
}

void Logger::write_summary(const std::string& text) {
	if (!enabled()) {
		return;
	}
	round_.close();
	const fs::path file = dir_ / "summary.txt";
	std::ofstream summary(file);
	if (!summary) {
		std::cerr << "Не удалось создать " << file.string() << "\n";
		return;
	}
	summary << text;
}
