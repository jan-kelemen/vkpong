#ifndef VKPONG_GAME_INCLUDED
#define VKPONG_GAME_INCLUDED

#include <utility>

namespace vkpong
{
    enum class action
    {
        up,
        down
    };

    class [[nodiscard]] game final
    {
    public:
        float player_position{};
        float npc_position{};
        std::pair<float, float> ball_position{};
        std::pair<float, float> ball_vector{0.01f, 0.01f};

        void tick();

        void update(action act);
    };
} // namespace vkpong

#endif
