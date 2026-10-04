#include "player.hpp"

#include <algorithm>
#include <cassert>
#include <random>
#include <stdexcept>
#include <vector>

namespace {

std::mt19937& rng() {
	static std::mt19937 gen{std::random_device{}()};
	return gen;
}

int random_of(const std::vector<int>& candidates) {
	assert(!candidates.empty());
	std::uniform_int_distribution<std::size_t> dist(0, candidates.size() - 1);
	return candidates[dist(rng())];
}

}  // namespace

std::string role_to_string(Role role) {
	switch (role) {
		case Role::Civilian:  return "Мирный житель";
		case Role::Mafia:     return "Мафия";
		case Role::Commissar: return "Комиссар";
		case Role::Doctor:    return "Доктор";
		case Role::Maniac:    return "Маньяк";
	}
	throw std::logic_error("unknown role");
}

Player::Player(int id, std::string name, Role role)
	: id_(id), name_(std::move(name)), role_(role) {}

int Player::vote(const GameState& state) const {
	return random_other(state);
}

int Player::random_other(const GameState& state) const {
	std::vector<int> candidates;
	for (int id : state.alive_ids) {
		if (id != id_) {
			candidates.push_back(id);
		}
	}
	return random_of(candidates);
}

std::optional<NightAction> Civilian::night_action(const GameState&) {
	return std::nullopt;
}

std::optional<NightAction> Mafia::night_action(const GameState& state) {
	std::vector<int> candidates;
	for (int id : state.alive_ids) {
		if (id != this->id() && std::ranges::find(allies_, id) == allies_.end()) {
			candidates.push_back(id);
		}
	}
	return NightAction{ActionType::MafiaKill, id(), random_of(candidates)};
}

std::optional<NightAction> Commissar::night_action(const GameState& state) {
	std::bernoulli_distribution shoot(0.5);
	ActionType type = shoot(rng()) ? ActionType::Shoot : ActionType::Check;
	return NightAction{type, id(), random_other(state)};
}

std::optional<NightAction> Doctor::night_action(const GameState& state) {
	return NightAction{ActionType::Heal, id(), random_of(state.alive_ids)};
}

std::optional<NightAction> Maniac::night_action(const GameState& state) {
	return NightAction{ActionType::ManiacKill, id(), random_other(state)};
}
