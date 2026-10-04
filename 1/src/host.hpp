#pragma once

#include <string>
#include <thread>
#include <vector>

#include "player.hpp"
#include "shared_ptr.hpp"

class Host {
public:
	explicit Host(const std::vector<std::string>& names, bool full_log = false, bool interactive = false);

	const std::vector<SharedPtr<Player>>& players() const noexcept { return players_; }

	GameState make_state() const;

	void run();

private:
	void assign_roles(const std::vector<std::string>& names);
	void announce(const std::string& text) const;
	void tell(const Player& player, const std::string& text) const;
	void debug(const std::string& text) const;
	void update_boss();
	bool is_human(int id) const;
	int ask_human_vote(const GameState& state) const;
	void day_phase();
	void night_phase();
	bool check_winner() const;

	std::vector<SharedPtr<Player>> players_;
	int round_ = 1;
	int boss_id_ = -1;
	bool full_log_ = false;
	bool interactive_ = false;
};
