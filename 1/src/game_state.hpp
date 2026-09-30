#pragma once

#include <vector>

// TODO: заглушка. Что именно видят игроки, решит архитектура ведущего.
struct GameState {
    std::vector<int> alive_ids;
};
