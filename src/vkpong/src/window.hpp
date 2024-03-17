#ifndef VKPONG_WINDOW_INCLUDED
#define VKPONG_WINDOW_INCLUDED

#include <functional>
#include <memory>

struct GLFWwindow;

namespace vkpong
{
    class [[nodiscard]] window final
    {
    public: // constants
        static constexpr int default_width = 800;
        static constexpr int default_height = 600;

    public: // construction
        window() : window{default_width, default_height} { }

        window(int width, int height);

        window(window const&) = delete;

        window(window&&) noexcept = delete;

    public: // Interface
        void loop(std::function<void()> const& callback);

        [[nodiscard]] GLFWwindow* handle() const noexcept;

    public: // Operators
        window& operator=(window const&) = delete;

        window& operator=(window&&) noexcept = delete;

    public: // Destruction
        ~window();

    private: // Data
        std::unique_ptr<GLFWwindow, void (*)(GLFWwindow*)> impl_;
    };
} // namespace vkpong

inline GLFWwindow* vkpong::window::handle() const noexcept
{
    return impl_.get();
}

#endif
