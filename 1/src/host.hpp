#pragma once

#include <map>
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
	     bool interactive = false, bool log = false, bool open_announcements = false);

	const std::vector<SharedPtr<Player>>& players() const noexcept { return players_; }

	GameState make_state() const;

	void run();

private:
	void assign_roles(const std::vector<std::string>& names, const GameConfig& config);
	void announce(const std::string& text) const;
	void tell(const Player& player, const std::string& text) const;
	void debug(const std::string& text) const;
	std::string status(const Player& player) const;
	void update_boss();
	void tell_allies() const;
	bool is_human(int id) const;
	std::optional<std::size_t> ask_choice(const std::string& question, const std::vector<std::string>& items) const;
	std::optional<int> ask_target(const std::string& question, const std::vector<int>& ids) const;
	int ask_human_vote(const GameState& state) const;
	std::optional<NightAction> ask_human_night_action(const GameState& state) const;
	void day_phase();
	void night_phase();
	bool check_winner();
	std::string make_summary() const;

	// Статистика матча для summary.txt
	struct Fate {
		int round = 0;    // 0 — жив
		std::string how;  // "кикнут днём", "убит ночью (мафия)"
	};
	struct Stats {
		std::vector<Fate> fates;  // по id игрока
		int kicked = 0;
		int ties = 0;
		int night_kills = 0;
		int saved = 0;
		std::map<ActionType, int> kills_by;
		std::string result;
	};

	std::vector<SharedPtr<Player>> players_;
	int round_ = 1;
	int boss_id_ = -1;
	bool full_log_ = false;
	bool interactive_ = false;
	bool open_announcements_ = false;
	mutable Logger logger_;  // mutable: пишем из const-методов announce/tell/debug
	Stats stats_;
};
