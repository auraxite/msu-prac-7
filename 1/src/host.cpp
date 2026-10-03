#include "host.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <iterator>
#include <map>
#include <random>
#include <stdexcept>
#include <utility>

namespace {

constexpr std::array kSpecialRoles = {Role::Commissar, Role::Doctor, Role::Maniac};

SharedPtr<Player> make_player(Role role, int id, const std::string& name) {
	switch (role) {
		case Role::Civilian:  return SharedPtr<Player>(new Civilian(id, name));
		case Role::Mafia:     return SharedPtr<Player>(new Mafia(id, name));
		case Role::Commissar: return SharedPtr<Player>(new Commissar(id, name));
		case Role::Doctor:    return SharedPtr<Player>(new Doctor(id, name));
		case Role::Maniac:    return SharedPtr<Player>(new Maniac(id, name));
	}
	throw std::logic_error("unknown role");
}

}  // namespace

Host::Host(const std::vector<std::string>& names) {
	assign_roles(names);
}

GameState Host::make_state() const {
	GameState state;
	state.round = round_;
	for (const auto& player : players_) {
		if (player->is_alive()) {
			state.alive_ids.push_back(player->id());
		}
	}
	return state;
}

void Host::run() {
	while (true) {
		day_phase();
		if (check_winner()) {
			return;
		}
		night_phase();
		if (check_winner()) {
			return;
		}
		++round_;
	}
}

void Host::day_phase() {
	GameState state = make_state();
	std::vector<std::pair<int, int>> votes;
	for (const auto& p : players_) {
		if (p->is_alive()) {
			votes.push_back({p->id(), p->vote(state)});
		}
	}

	std::map<int, int> counts;
	for (auto [voter, target] : votes) {
		++counts[target];
	}

	int best = 0;
	for (auto [id, c] : counts) {
		best = std::max(best, c);
	}

	std::vector<int> leaders;
	for (auto [id, c] : counts) {
		if (c == best) {
			leaders.push_back(id);
		}
	}

	if (leaders.size() == 1) {
		auto& out = *players_[leaders[0]];
		out.kill();
		std::cout << out.name() << " был кикнут (" << role_to_string(out.role()) << ")\n";
	} else {
		std::cout << "Ничья\n";
	}
}

void Host::night_phase() {
	std::vector<NightAction> actions;
	for (const auto& p : players_) {
		if (p->is_alive()) {
			auto action = p->night_action(make_state());
			if (action) {
				actions.push_back(*action);
			}
		}
	}

	std::map<int, int> mafia_votes;
	for (const auto& a : actions) {
		if (a.type == ActionType::MafiaKill) {
			mafia_votes[a.actor] = a.target;
		}
	}

	if (!mafia_votes.empty()) {
		std::map<int, int> counts;
		for (auto [voter, target] : mafia_votes) {
			++counts[target];
		}

		int best = 0;
		for (auto [id, c] : counts) {
			best = std::max(best, c);
		}

		std::vector<int> leaders;
		for (auto [id, c] : counts) {
			if (c == best) {
				leaders.push_back(id);
			}
		}

		int victim = leaders.size() == 1 ? leaders[0] : mafia_votes.begin()->second;
		auto& dead = *players_[victim];
		dead.kill();
		std::cout << dead.name() << " погиб\n";
	}
}

bool Host::check_winner() const {
	int mafia = 0;
	int town = 0;
	bool maniac = false;
	for (const auto& p : players_) {
		if (!p->is_alive()) {
			continue;
		} else if (p->role() == Role::Mafia) {
			++mafia;
		} else {
			++town;
			if (p->role() == Role::Maniac) {
				maniac = true;
			}
		}
	}

	if (mafia == 0 && !maniac) {
		std::cout << "Победили мирные жители\n";
		return true;
	} else if (mafia == 0 && town == 2) {
		std::cout << "Победил маньяк\n";
		return true;
	} else if (mafia > town || (mafia == town && !maniac)) {
		std::cout << "Победила мафия\n";
		return true;
	}
	return false;
}

void Host::assign_roles(const std::vector<std::string>& names) {
	const int n = static_cast<int>(names.size());
	const int mafia_count = std::max(1, n / 3);
	const int special_count = static_cast<int>(kSpecialRoles.size());
	if (mafia_count + special_count > n) {
		throw std::invalid_argument("too few players");
	}

	std::vector<Role> roles(n, Role::Civilian);
	std::fill_n(roles.begin(), mafia_count, Role::Mafia);
	std::ranges::copy(kSpecialRoles, roles.begin() + mafia_count);

	std::mt19937 rng{std::random_device{}()};
	std::ranges::shuffle(roles, rng);

	players_.clear();
	players_.reserve(n);
	std::vector<int> mafia_ids;
	for (int id = 0; id < n; ++id) {
		players_.push_back(make_player(roles[id], id, names[id]));
		if (roles[id] == Role::Mafia) {
			mafia_ids.push_back(id);
		}
	}

	for (int id : mafia_ids) {
		auto& mafia = static_cast<Mafia&>(*players_[id]);
		std::vector<int> allies;
		std::ranges::copy_if(mafia_ids, std::back_inserter(allies), [id](int other) { return other != id; });
		mafia.set_allies(std::move(allies));
	}
}
