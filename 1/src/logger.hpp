#pragma once

#include <filesystem>
#include <fstream>
#include <string>

class Logger {
public:
	Logger() = default;
	explicit Logger(const std::filesystem::path& root);

	bool enabled() const noexcept { return !dir_.empty(); }
	const std::filesystem::path& dir() const noexcept { return dir_; }

	void start_round(int round);
	void write(const std::string& line);
	void write_summary(const std::string& text);

private:
	std::filesystem::path dir_;
	std::ofstream round_;
};
