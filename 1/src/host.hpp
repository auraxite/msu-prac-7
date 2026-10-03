#pragma once

#include <string>
#include <vector>

#include "player.hpp"
#include "shared_ptr.hpp"

class Host {
public:
    // Id игрока = его индекс в names
    explicit Host(const std::vector<std::string>& names);

    const std::vector<SharedPtr<Player>>& players() const noexcept { return players_; }

    GameState make_state() const;

    void run();

private:
    void assign_roles(const std::vector<std::string>& names);
    void day_phase();
    void night_phase();
    bool check_winner() const;

    std::vector<SharedPtr<Player>> players_;
    int round_ = 1;
};
