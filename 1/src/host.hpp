#pragma once

#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "game_config.hpp"
#include "logger.hpp"
#include "player.hpp"
#include "shared_ptr.hpp"

class Host {
public:
	Host(const std::vector<std::string>& names, const GameConfig& config, bool full_log = false,
	     bool interactive = false, bool log = false, bool open_announcements = false,
	     std::optional<unsigned> seed = std::nullopt);

	const std::vector<SharedPtr<Player>>& players() const noexcept { return players_; }

	GameState make_state() const;

	void run();

private:
	void assign_roles(const std::vector<std::string>& names, const GameConfig& config, std::optional<unsigned> seed);
	void update_boss();
	void tell_allies() const;
	void day_phase();
	void night_phase();
	bool check_winner();
	std::string make_summary() const;
	bool is_human(int id) const;
	std::optional<std::size_t> ask_choice(const std::string& question, const std::vector<std::string>& items) const;
	std::optional<int> ask_target(const std::string& question, const std::vector<int>& ids) const;
	int ask_human_vote(const GameState& state) const;
	std::optional<NightAction> ask_human_night_action(const GameState& state) const;
	void announce(const std::string& text) const;
	void tell(const Player& player, const std::string& text) const;
	void debug(const std::string& text) const;
	std::string status(const Player& player) const;

	std::vector<SharedPtr<Player>> players_;
	int round_ = 1;
	int boss_id_ = -1;
	bool full_log_ = false;
	bool interactive_ = false;
	bool open_announcements_ = false;
	mutable Logger logger_;
	std::vector<std::string> fates_;  // по id игрока: "жив" или как и в каком раунде погиб
	std::string result_;
};
