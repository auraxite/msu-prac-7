#pragma once

#include <vector>

struct GameState {
	int round = 0;
	std::vector<int> alive_ids;
};
