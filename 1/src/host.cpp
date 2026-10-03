#include "host.hpp"

#include <algorithm>
#include <iterator>
#include <random>
#include <stdexcept>

namespace {

constexpr int kSpecialRoles = 3;  // комиссар, доктор, маньяк

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
}

void Host::night_phase() {
}

bool Host::check_winner() const {
    return false;
}

void Host::assign_roles(const std::vector<std::string>& names) {
    const int n = static_cast<int>(names.size());
    const int mafia_count = std::max(1, n / 3);
    if (mafia_count + kSpecialRoles > n) {
        throw std::invalid_argument("too few players");
    }

    std::vector<Role> roles(n, Role::Civilian);
    std::fill_n(roles.begin(), mafia_count, Role::Mafia);
    roles[mafia_count] = Role::Commissar;
    roles[mafia_count + 1] = Role::Doctor;
    roles[mafia_count + 2] = Role::Maniac;

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
