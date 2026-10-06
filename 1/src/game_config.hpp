#pragma once

#include <string>
#include <vector>

#include "player.hpp"

struct GameConfig {
	int default_players = 7;
	int mafia_divisor = 3;
	std::vector<Role> specials = {Role::Commissar, Role::Doctor, Role::Maniac, Role::Ninja, Role::Hacker, Role::Elder};
};

GameConfig load_game_config(const std::string& path);
