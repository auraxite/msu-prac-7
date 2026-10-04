#include <string>
#include <vector>

#include "host.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

int main() {
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
#endif

	std::vector<std::string> names;
	for (int i = 1; i <= 7; ++i) {
		names.push_back("Игрок " + std::to_string(i));
	}

	Host host(names);
	host.run();
	return 0;
}
