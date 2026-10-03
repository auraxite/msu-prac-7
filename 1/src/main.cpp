#include <iostream>

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

	Host host({"Alice", "Bob", "Carol", "Dave", "Eve", "Frank", "Grace"});
	for (const auto& p : host.players()) {
		std::cout << p->name() << " — " << role_to_string(p->role()) << "\n";
	}
	host.run();
	return 0;
}
