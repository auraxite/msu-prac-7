#pragma once

#include <string>
#include <utility>
#include <vector>

#include "player.hpp"

struct GameConfig {
	int mafia_divisor = 3;
	int ninja = 1;  // 0 или 1; ниндзя — один из мафии, если её ≥ 2
	// В порядке приоритета: при нехватке игроков отбрасываются с конца
	std::vector<std::pair<Role, int>> specials = {
		{Role::Commissar, 1},
		{Role::Doctor, 1},
		{Role::Maniac, 1},
		{Role::Hacker, 1},
		{Role::Elder, 1},
	};
};

// Нет файла — значения по умолчанию и сообщение в stderr.
// Ошибка в файле — std::runtime_error.
GameConfig load_game_config(const std::string& path);
