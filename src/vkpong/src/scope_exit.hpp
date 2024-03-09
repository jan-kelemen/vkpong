#ifndef VKPONG_SCOPE_EXIT_INCLUDED
#define VKPONG_SCOPE_EXIT_INCLUDED

namespace vkpong
{
    template<typename Action>
    struct on_scope_exit final
    {
    public: // Construction
        on_scope_exit() = delete;

        constexpr explicit on_scope_exit(Action& action) noexcept
            : action_(action)
        {
        }

        on_scope_exit(on_scope_exit const&) = delete;

        on_scope_exit(on_scope_exit&&) noexcept = delete;

    public: // Destruction
        ~on_scope_exit() { action_(); }

    public: // Operators
        on_scope_exit& operator=(on_scope_exit const&) = delete;

        on_scope_exit& operator=(on_scope_exit&&) noexcept = delete;

    private: // Data
        Action& action_;
    };

} // namespace vkpong

#define VKPONG_TOKEN_PASTEx(x, y) x##y
#define VKPONG_TOKEN_PASTE(x, y) VKPONG_TOKEN_PASTEx(x, y)

#define VKPONG_ON_SCOPE_EXIT_INTERNAL1(lname, aname, ...) \
    auto const lname{[&]() { __VA_ARGS__; }}; \
    vkpong::on_scope_exit<decltype(lname)> const aname{lname};

#define VKPONG_ON_SCOPE_EXIT_INTERNAL2(ctr, ...) \
    VKPONG_ON_SCOPE_EXIT_INTERNAL1( \
        VKPONG_TOKEN_PASTE(ON_SCOPE_EXIT_func_, ctr), \
        VKPONG_TOKEN_PASTE(ON_SCOPE_EXIT_instance_, ctr), \
        __VA_ARGS__)

#define VKPONG_ON_SCOPE_EXIT(...) \
    VKPONG_ON_SCOPE_EXIT_INTERNAL2(__COUNTER__, __VA_ARGS__)

#endif
