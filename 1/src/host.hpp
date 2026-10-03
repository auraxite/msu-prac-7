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

private:
    void assign_roles(const std::vector<std::string>& names);

    std::vector<SharedPtr<Player>> players_;
};
