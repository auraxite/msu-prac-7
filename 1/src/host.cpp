#include "host.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <iostream>
#include <iterator>
#include <map>
#include <random>
#include <stdexcept>
#include <thread>
#include <utility>

namespace {

constexpr std::array kSpecialRoles = {Role::Commissar, Role::Doctor, Role::Maniac};
constexpr int kHumanId = 0;  // Игрок 1

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

std::string action_to_string(ActionType type) {
	switch (type) {
		case ActionType::MafiaKill:  return "убийство (мафия)";
		case ActionType::Check:      return "проверка";
		case ActionType::Shoot:      return "выстрел";
		case ActionType::Heal:       return "лечение";
		case ActionType::ManiacKill: return "убийство (маньяк)";
	}
	throw std::logic_error("unknown action");
}

}  // namespace

Host::Host(const std::vector<std::string>& names, bool full_log, bool interactive)
	: full_log_(full_log), interactive_(interactive) {
	assign_roles(names);
}

bool Host::is_human(int id) const {
	return interactive_ && id == kHumanId;
}

int Host::ask_human_vote(const GameState& state) const {
	std::vector<int> options;
	for (int id : state.alive_ids) {
		if (id != kHumanId) {
			options.push_back(id);
		}
	}

	std::cout << "Ваш голос. Против кого?\n";
	for (std::size_t i = 0; i < options.size(); ++i) {
		std::cout << "  " << i + 1 << ") " << players_[options[i]]->name() << "\n";
	}

	std::string line;
	while (true) {
		std::cout << "> " << std::flush;
		if (!std::getline(std::cin, line)) {
			std::cout << "\nВвод закончился — голос выбран случайно\n";
			return players_[kHumanId]->vote(state);
		}

		const auto first = line.find_first_not_of(" \t\r");
		const auto last = line.find_last_not_of(" \t\r");
		const std::string trimmed = first == std::string::npos ? "" : line.substr(first, last - first + 1);

		int choice = 0;
		const char* end = trimmed.data() + trimmed.size();
		auto [ptr, ec] = std::from_chars(trimmed.data(), end, choice);
		if (ec == std::errc{} && ptr == end && choice >= 1 && choice <= static_cast<int>(options.size())) {
			return options[choice - 1];
		}
		std::cout << "Введите число от 1 до " << options.size() << "\n";
	}
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

void Host::announce(const std::string& text) const {
	std::cout << text << "\n";
}

void Host::tell(const Player& player, const std::string& text) const {
	if (is_human(player.id())) {
		std::cout << "[лично] " << text << "\n";
	} else {
		debug("лично → " + player.name() + ": " + text);
	}
}

void Host::debug(const std::string& text) const {
	if (full_log_) {
		std::cout << "[debug] " << text << "\n";
	}
}

void Host::update_boss() {
	int boss = -1;
	for (const auto& p : players_) {
		if (p->is_alive() && p->role() == Role::Mafia) {
			boss = p->id();
			break;
		}
	}

	if (boss == boss_id_) {
		return;
	}
	boss_id_ = boss;
	if (boss_id_ == -1) {
		return;
	}

	for (const auto& p : players_) {
		if (p->is_alive() && p->role() == Role::Mafia) {
			tell(*p, "Босс мафии — " + players_[boss_id_]->name());
		}
	}
}

void Host::run() {
	update_boss();

	while (true) {
		announce("\n=== День " + std::to_string(round_) + " ===");
		day_phase();
		if (check_winner()) {
			return;
		}
		update_boss();
		announce("\n=== Ночь " + std::to_string(round_) + " ===");
		night_phase();
		if (check_winner()) {
			return;
		}
		update_boss();
		++round_;
	}
}

void Host::day_phase() {
	GameState state = make_state();
	const std::vector<int>& alive = state.alive_ids;
	std::vector<int> targets(alive.size());
	{
		std::vector<std::jthread> threads;
		for (std::size_t i = 0; i < alive.size(); ++i) {
			if (is_human(alive[i])) {
				continue;
			}
			threads.push_back(std::jthread([&, i] {
				targets[i] = players_[alive[i]]->vote(state);
			}));
		}
	}

	for (std::size_t i = 0; i < alive.size(); ++i) {
		if (is_human(alive[i])) {
			targets[i] = ask_human_vote(state);
		}
	}

	std::vector<std::pair<int, int>> votes;
	for (std::size_t i = 0; i < alive.size(); ++i) {
		votes.push_back({alive[i], targets[i]});
	}

	for (auto [voter, target] : votes) {
		debug(players_[voter]->name() + " голосует против " + players_[target]->name());
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
		announce(out.name() + " был кикнут (" + role_to_string(out.role()) + ")");
	} else {
		announce("Ничья");
	}
}

void Host::night_phase() {
	GameState state = make_state();
	const std::vector<int>& alive = state.alive_ids;
	std::vector<std::optional<NightAction>> results(alive.size());
	{
		std::vector<std::jthread> threads;
		for (std::size_t i = 0; i < alive.size(); ++i) {
			threads.push_back(std::jthread([&, i] {
				results[i] = players_[alive[i]]->night_action(state);
			}));
		}
	}

	std::vector<NightAction> actions;
	for (const auto& action : results) {
		if (action) {
			actions.push_back(*action);
		}
	}

	for (const auto& a : actions) {
		debug(players_[a.actor]->name() + ": " + action_to_string(a.type) + " → " + players_[a.target]->name());
	}

	int healed = -1;
	for (const auto& a : actions) {
		if (a.type == ActionType::Heal) {
			healed = a.target;
		} else if (a.type == ActionType::Check) {
			const auto& suspect = *players_[a.target];
			const bool is_mafia = suspect.role() == Role::Mafia;
			tell(*players_[a.actor], suspect.name() + (is_mafia ? " — мафия" : " — не мафия"));
		}
	}

	std::map<int, int> mafia_votes;
	for (const auto& a : actions) {
		if (a.type == ActionType::MafiaKill) {
			mafia_votes[a.actor] = a.target;
		}
	}

	std::vector<int> targets;

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

		int victim = leaders.size() == 1 ? leaders[0] : mafia_votes.at(boss_id_);
		targets.push_back(victim);
	}

	for (const auto& a : actions) {
		if (a.type == ActionType::ManiacKill || a.type == ActionType::Shoot) {
			targets.push_back(a.target);
		}
	}

	bool someone_died = false;
	for (int t : targets) {
		auto& dead = *players_[t];
		if (t == healed || !dead.is_alive()) {
			continue;
		}
		dead.kill();
		announce(dead.name() + " погиб");
		someone_died = true;
	}
	if (!someone_died) {
		announce("Этой ночью никто не погиб");
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

	if (mafia + town == 0) {
		announce("Все погибли — ничья");
		return true;
	} else if (mafia == 0 && !maniac) {
		announce("Победили мирные жители");
		return true;
	} else if (mafia == 0 && town <= 2) {
		announce("Победил маньяк");
		return true;
	} else if (mafia > town || (mafia == town && !maniac)) {
		announce("Победила мафия");
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

	for (const auto& p : players_) {
		tell(*p, "Ваша роль: " + role_to_string(p->role()));
	}
}
