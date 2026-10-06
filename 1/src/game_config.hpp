#pragma once

#include <array>
#include <string>
#include <vector>

#include "player.hpp"

// Обязательные роли: без них игра не начинается
inline constexpr std::array kRequiredSpecials = {Role::Commissar, Role::Doctor, Role::Maniac};

struct GameConfig {
	int default_players = 7;
	int mafia_divisor = 3;
	std::vector<Role> specials = {Role::Commissar, Role::Doctor, Role::Maniac, Role::Ninja, Role::Hacker, Role::Elder};
};

GameConfig load_game_config(const std::string& path);
