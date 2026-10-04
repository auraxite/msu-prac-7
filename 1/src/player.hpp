#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "game_state.hpp"

enum class Role { Civilian, Mafia, Commissar, Doctor, Maniac, Ninja, Hacker, Elder };

std::string role_to_string(Role role);
bool is_mafia(Role role);

enum class ActionType { MafiaKill, Check, Shoot, Heal, ManiacKill, Hack };

struct NightAction {
	ActionType type;
	int actor;
	int target;
};

class Player {
public:
	Player(int id, std::string name, Role role);
	virtual ~Player() = default;

	// Полиморфный базовый класс: копирование привело бы к срезке
	Player(const Player&) = delete;
	Player& operator=(const Player&) = delete;

	int id() const noexcept { return id_; }
	const std::string& name() const noexcept { return name_; }
	Role role() const noexcept { return role_; }
	bool is_alive() const noexcept { return alive_; }
	void kill() noexcept { alive_ = false; }

	// std::nullopt — ночью игрок ничего не делает
	virtual std::optional<NightAction> night_action(const GameState& state) = 0;

	// Id того, против кого голос. Воздержаться и голосовать за себя нельзя.
	int vote(const GameState& state) const;

protected:
	// Случайный живой игрок, кроме себя
	int random_other(const GameState& state) const;

private:
	int id_;
	std::string name_;
	Role role_;
	bool alive_ = true;
};

class Civilian final : public Player {
public:
	Civilian(int id, std::string name) : Player(id, std::move(name), Role::Civilian) {}
	std::optional<NightAction> night_action(const GameState& state) override;
};

class Mafia : public Player {
public:
	Mafia(int id, std::string name) : Player(id, std::move(name), Role::Mafia) {}
	std::optional<NightAction> night_action(const GameState& state) override;

	// Id сообщников. Знает только сам мафиози, в GameState не попадает.
	void set_allies(std::vector<int> ids) { allies_ = std::move(ids); }
	const std::vector<int>& allies() const noexcept { return allies_; }

protected:
	Mafia(int id, std::string name, Role role) : Player(id, std::move(name), role) {}

private:
	std::vector<int> allies_;
};

class Ninja final : public Mafia {
public:
	Ninja(int id, std::string name) : Mafia(id, std::move(name), Role::Ninja) {}
};

class Commissar final : public Player {
public:
	Commissar(int id, std::string name) : Player(id, std::move(name), Role::Commissar) {}
	std::optional<NightAction> night_action(const GameState& state) override;
};

class Doctor final : public Player {
public:
	Doctor(int id, std::string name) : Player(id, std::move(name), Role::Doctor) {}
	std::optional<NightAction> night_action(const GameState& state) override;
};

class Maniac final : public Player {
public:
	Maniac(int id, std::string name) : Player(id, std::move(name), Role::Maniac) {}
	std::optional<NightAction> night_action(const GameState& state) override;
};

class Hacker final : public Player {
public:
	Hacker(int id, std::string name) : Player(id, std::move(name), Role::Hacker) {}
	std::optional<NightAction> night_action(const GameState& state) override;
};

class Elder final : public Player {
public:
	Elder(int id, std::string name) : Player(id, std::move(name), Role::Elder) {}
	std::optional<NightAction> night_action(const GameState& state) override;
};
