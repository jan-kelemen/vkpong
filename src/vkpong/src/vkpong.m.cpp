#include <vkcontext.hpp>
#include <window.hpp>

#ifdef NDEBUG
constexpr bool enable_validation_layers{false};
#else
constexpr bool enable_validation_layers{true};
#endif

int main()
{
    vkpong::window window;
    {
        auto context{
            vkpong::create_context(window.handle(), enable_validation_layers)};
        window.loop();
    }
}
