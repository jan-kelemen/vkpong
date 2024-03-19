#include <game.hpp>

#include <algorithm>

namespace
{
    constexpr float vertical_delta{0.05f};
} // namespace

void vkpong::game::update(vkpong::action act)
{
    switch (act)
    {
    case action::up:
        player_position -= vertical_delta;
        break;
    case action::down:
        player_position += vertical_delta;
    }

    player_position = std::clamp(-1.f, player_position, 1.f);
}
