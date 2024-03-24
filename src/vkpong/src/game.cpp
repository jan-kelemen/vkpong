#include <game.hpp>

#include <algorithm>

namespace
{
    constexpr float vertical_delta{0.05f};
} // namespace

void vkpong::game::tick()
{
    if (auto current_diff{std::abs(npc_position - ball_position.second)};
        current_diff > vertical_delta)
    {
        npc_position = std::clamp(ball_position.second, -0.8f, 0.8f);
    }
    else
    {
        npc_position = ball_position.second;
    }

    std::pair<float, float> new_ball_position{
        ball_position.first + ball_vector.first,
        ball_position.second + ball_vector.second};

    if (new_ball_position.first <= -0.86f)
    {
        if (std::abs(npc_position - ball_position.second) >= 0.2f)
        {
            ball_position = {0, 0};
            return;
        }
        else
        {
            ball_vector = {ball_vector.first * -1, ball_vector.second * -1};
        }
    }
    else if (new_ball_position.first >= 0.86f)
    {
        if (std::abs(player_position - ball_position.second) >= 0.2f)
        {
            ball_position = {0, 0};
            return;
        }
        else
        {
            ball_vector = {ball_vector.first * -1, ball_vector.second * -1};
        }
    }

    if (new_ball_position.second <= -1.f || new_ball_position.second >= 1.f)
    {
        ball_vector.second *= -1;
    }
    else
    {
        ball_position = new_ball_position;
    }
}

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

    player_position = std::clamp(player_position, -0.8f, 0.8f);
}
