#include "host.hpp"

#include <algorithm>
#include <charconv>
#include <iostream>
#include <iterator>
#include <map>
#include <random>
#include <stdexcept>
#include <thread>
#include <utility>

namespace {

constexpr int kHumanId = 0;  // Игрок 1

SharedPtr<Player> make_player(Role role, int id, const std::string& name) {
	switch (role) {
		case Role::Civilian:  return SharedPtr<Player>(new Civilian(id, name));
		case Role::Mafia:     return SharedPtr<Player>(new Mafia(id, name));
		case Role::Commissar: return SharedPtr<Player>(new Commissar(id, name));
		case Role::Doctor:    return SharedPtr<Player>(new Doctor(id, name));
		case Role::Maniac:    return SharedPtr<Player>(new Maniac(id, name));
		case Role::Ninja:     return SharedPtr<Player>(new Ninja(id, name));
		case Role::Hacker:    return SharedPtr<Player>(new Hacker(id, name));
		case Role::Elder:     return SharedPtr<Player>(new Elder(id, name));
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
		case ActionType::Hack:       return "взлом";
	}
	throw std::logic_error("unknown action");
}

std::string killer_to_string(ActionType type) {
	switch (type) {
		case ActionType::MafiaKill:  return "мафия";
		case ActionType::ManiacKill: return "маньяк";
		case ActionType::Shoot:      return "комиссар";
		default:                     break;
	}
	throw std::logic_error("not a kill action");
}

std::vector<int> without(const std::vector<int>& ids, int excluded) {
	std::vector<int> result;
	std::ranges::copy_if(ids, std::back_inserter(result), [excluded](int id) { return id != excluded; });
	return result;
}

std::vector<int> find_leaders(const std::map<int, int>& counts) {
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
	return leaders;
}

}  // namespace

Host::Host(const std::vector<std::string>& names, const GameConfig& config, bool full_log, bool interactive,
           bool log, bool open_announcements, std::optional<unsigned> seed)
	: full_log_(full_log), interactive_(interactive), open_announcements_(open_announcements) {
	if (log) {
		logger_ = Logger("logs");
	}
	assign_roles(names, config, seed);
}

std::string Host::status(const Player& player) const {
	if (open_announcements_) {
		return role_to_string(player.role());
	}
	return is_mafia(player.role()) ? "мафия" : "мирный";
}

bool Host::is_human(int id) const {
	return interactive_ && id == kHumanId;
}

std::optional<std::size_t> Host::ask_choice(const std::string& question, const std::vector<std::string>& items) const {
	std::cout << question << "\n";
	for (std::size_t i = 0; i < items.size(); ++i) {
		std::cout << "  " << i + 1 << ") " << items[i] << "\n";
	}

	std::string line;
	while (true) {
		std::cout << "> " << std::flush;
		if (!std::getline(std::cin, line)) {
			std::cout << "\n";
			return std::nullopt;
		}

		const auto first = line.find_first_not_of(" \t\r");
		const auto last = line.find_last_not_of(" \t\r");
		const std::string trimmed = first == std::string::npos ? "" : line.substr(first, last - first + 1);

		int choice = 0;
		const char* end = trimmed.data() + trimmed.size();
		auto [ptr, ec] = std::from_chars(trimmed.data(), end, choice);
		if (ec == std::errc{} && ptr == end && choice >= 1 && choice <= static_cast<int>(items.size())) {
			return static_cast<std::size_t>(choice - 1);
		}
		std::cout << "Введите число от 1 до " << items.size() << "\n";
	}
}

std::optional<int> Host::ask_target(const std::string& question, const std::vector<int>& ids) const {
	std::vector<std::string> names;
	for (int id : ids) {
		names.push_back(players_[id]->name());
	}
	auto choice = ask_choice(question, names);
	if (!choice) {
		return std::nullopt;
	}
	return ids[*choice];
}

int Host::ask_human_vote(const GameState& state) const {
	auto target = ask_target("Ваш голос. Против кого?", without(state.alive_ids, kHumanId));
	if (!target) {
		std::cout << "Ввод закончился — голос выбран случайно\n";
		return players_[kHumanId]->vote(state);
	}
	return *target;
}

std::optional<NightAction> Host::ask_human_night_action(const GameState& state) const {
	const Player& me = *players_[kHumanId];
	const std::vector<int> others = without(state.alive_ids, kHumanId);

	std::optional<int> target;
	ActionType type{};
	switch (me.role()) {
		case Role::Civilian:
		case Role::Elder:
			std::cout << "Вы спите\n";
			return std::nullopt;

		case Role::Mafia:
		case Role::Ninja: {
			const auto& allies = static_cast<const Mafia&>(me).allies();
			std::vector<int> victims;
			for (int id : others) {
				if (std::ranges::find(allies, id) == allies.end()) {
					victims.push_back(id);
				}
			}
			type = ActionType::MafiaKill;
			target = ask_target("Кого убить?", victims);
			break;
		}

		case Role::Commissar: {
			auto action = ask_choice("Что делаете?", {"Проверить", "Выстрелить"});
			if (!action) {
				break;
			}
			type = *action == 0 ? ActionType::Check : ActionType::Shoot;
			target = ask_target(type == ActionType::Check ? "Кого проверить?" : "В кого выстрелить?", others);
			break;
		}

		case Role::Doctor: {
			const int last = static_cast<const Doctor&>(me).last_patient();
			type = ActionType::Heal;
			target = ask_target("Кого лечить?", without(state.alive_ids, last));
			break;
		}

		case Role::Maniac:
			type = ActionType::ManiacKill;
			target = ask_target("Кого убить?", others);
			break;

		case Role::Hacker:
			type = ActionType::Hack;
			target = ask_target("Кого взломать?", others);
			break;
	}

	if (!target) {
		std::cout << "Ввод закончился — ход выбран случайно\n";
		return players_[kHumanId]->night_action(state);
	}
	return NightAction{type, kHumanId, *target};
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
	logger_.write(text);
}

void Host::tell(const Player& player, const std::string& text) const {
	const std::string private_message = "[лично → " + player.name() + "] " + text;
	if (is_human(player.id())) {
		std::cout << "[лично] " << text << "\n";
	} else if (full_log_) {
		std::cout << private_message << "\n";
	}
	logger_.write(private_message);
}

void Host::debug(const std::string& text) const {
	if (full_log_) {
		std::cout << "[debug] " << text << "\n";
	}
	logger_.write("[debug] " + text);
}

void Host::update_boss() {
	const auto it = std::ranges::find_if(players_, [](const auto& p) { return p->is_alive() && is_mafia(p->role()); });
	const int boss = it == players_.end() ? -1 : (*it)->id();

	if (boss == boss_id_) {
		return;
	}
	boss_id_ = boss;
	if (boss_id_ == -1) {
		return;
	}

	for (const auto& p : players_) {
		if (p->is_alive() && is_mafia(p->role())) {
			tell(*p, "Босс мафии: " + players_[boss_id_]->name());
		}
	}
}

void Host::tell_allies() const {
	for (const auto& p : players_) {
		if (!p->is_alive() || !is_mafia(p->role())) {
			continue;
		}
		std::string names;
		for (int id : static_cast<const Mafia&>(*p).allies()) {
			if (players_[id]->is_alive()) {
				names += (names.empty() ? "" : ", ") + players_[id]->name();
			}
		}
		tell(*p, names.empty() ? "Живых сообщников нет" : "Ваши сообщники: " + names);
	}
}

void Host::run() {
	update_boss();
	tell_allies();

	while (true) {
		logger_.start_round(round_);
		announce("\n=== День " + std::to_string(round_) + " ===");
		day_phase();
		if (check_winner()) {
			break;
		}
		update_boss();
		announce("\n=== Ночь " + std::to_string(round_) + " ===");
		tell_allies();
		night_phase();
		if (check_winner()) {
			break;
		}
		update_boss();
		++round_;
	}

	logger_.write_summary(make_summary());
}

std::string Host::make_summary() const {
	std::string s = "Итог: " + result_ + "\nРаундов: " + std::to_string(round_) + "\n\nИгроки:\n";
	for (const auto& p : players_) {
		s += "  " + p->name() + " - " + role_to_string(p->role()) + " - " + fates_[p->id()] + "\n";
	}
	return s;
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

	std::map<int, int> counts;
	for (std::size_t i = 0; i < alive.size(); ++i) {
		debug(players_[alive[i]]->name() + " голосует против " + players_[targets[i]]->name());
		++counts[targets[i]];
	}

	const std::vector<int> leaders = find_leaders(counts);
	if (leaders.size() == 1) {
		auto& out = *players_[leaders[0]];
		if (out.role() == Role::Elder) {
			announce(out.name() + " - старейшина, его нельзя казнить");
		} else {
			out.kill();
			announce(out.name() + " был кикнут (" + status(out) + ")");
			tell(out, "Вы погибли ☠");
			fates_[out.id()] = "кикнут днём, раунд " + std::to_string(round_);
		}
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
			if (is_human(alive[i])) {
				continue;
			}
			threads.push_back(std::jthread([&, i] {
				results[i] = players_[alive[i]]->night_action(state);
			}));
		}
	}

	for (std::size_t i = 0; i < alive.size(); ++i) {
		if (is_human(alive[i])) {
			results[i] = ask_human_night_action(state);
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
			static_cast<Doctor&>(*players_[a.actor]).set_last_patient(a.target);
		} else if (a.type == ActionType::Check) {
			const auto& suspect = *players_[a.target];
			const bool looks_mafia = is_mafia(suspect.role()) && suspect.role() != Role::Ninja;
			tell(*players_[a.actor], suspect.name() + (looks_mafia ? " — мафия" : " — не мафия"));
		}
	}

	std::map<int, int> mafia_votes;
	for (const auto& a : actions) {
		if (a.type == ActionType::MafiaKill) {
			mafia_votes[a.actor] = a.target;
		}
	}

	std::vector<std::pair<int, ActionType>> targets;

	if (!mafia_votes.empty()) {
		std::map<int, int> counts;
		for (auto [voter, target] : mafia_votes) {
			++counts[target];
		}
		const std::vector<int> leaders = find_leaders(counts);
		const int victim = leaders.size() == 1 ? leaders[0] : mafia_votes.at(boss_id_);
		targets.push_back({victim, ActionType::MafiaKill});
	}

	for (const auto& a : actions) {
		if (a.type == ActionType::ManiacKill || a.type == ActionType::Shoot) {
			targets.push_back({a.target, a.type});
		}
	}

	bool someone_died = false;
	bool someone_saved = false;
	for (auto [t, cause] : targets) {
		auto& dead = *players_[t];
		if (!dead.is_alive()) {
			continue;
		}
		if (t == healed) {
			someone_saved = true;
			continue;
		}
		dead.kill();
		std::string text = dead.name() + " погиб (" + status(dead) + ")";
		if (open_announcements_) {
			text += ", убийца — " + killer_to_string(cause);
		}
		announce(text);
		tell(dead, "Вы погибли ☠");
		someone_died = true;
		fates_[t] = "убит ночью (" + killer_to_string(cause) + "), раунд " + std::to_string(round_);
	}
	if (!someone_died) {
		announce("Этой ночью никто не погиб");
	}
	if (someone_saved && open_announcements_) {
		announce(players_[healed]->name() + " был спасён доктором");
	}

	for (const auto& a : actions) {
		if (a.type == ActionType::Hack) {
			const auto& victim = *players_[a.target];
			announce("Взлом хакера: " + victim.name() + (is_mafia(victim.role()) ? " - мафия" : " - не мафия"));
		}
	}
}

bool Host::check_winner() {
	int mafia = 0;
	int town = 0;
	bool maniac = false;
	for (const auto& p : players_) {
		if (!p->is_alive()) {
			continue;
		} else if (is_mafia(p->role())) {
			++mafia;
		} else {
			++town;
			if (p->role() == Role::Maniac) {
				maniac = true;
			}
		}
	}

	if (mafia + town == 0) {
		result_ = "Все погибли - ничья";
	} else if (mafia == 0 && !maniac) {
		result_ = "Победили мирные жители";
	} else if (mafia == 0 && town <= 2) {
		result_ = "Победил маньяк";
	} else if (mafia > town || (mafia == town && !maniac)) {
		result_ = "Победила мафия";
	} else {
		return false;
	}
	announce(result_);
	return true;
}

void Host::assign_roles(const std::vector<std::string>& names, const GameConfig& config,
                        std::optional<unsigned> seed) {
	const int n = static_cast<int>(names.size());
	const int mafia_count = std::max(1, n / config.mafia_divisor);

	std::vector<Role> roles(n, Role::Civilian);
	std::fill_n(roles.begin(), mafia_count, Role::Mafia);

	int next = mafia_count;
	for (Role role : config.specials) {
		if (role == Role::Ninja) {
			roles[0] = Role::Ninja;
		} else if (next < n) {
			roles[next++] = role;
		} else if (std::ranges::find(kRequiredSpecials, role) != kRequiredSpecials.end()) {
			throw std::invalid_argument("too few players");
		}
	}

	std::mt19937 rng{seed.value_or(std::random_device{}())};
	std::ranges::shuffle(roles, rng);

	players_.clear();
	players_.reserve(n);
	fates_.assign(n, "жив");
	std::vector<int> mafia_ids;
	for (int id = 0; id < n; ++id) {
		players_.push_back(make_player(roles[id], id, names[id]));
		if (is_mafia(roles[id])) {
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
