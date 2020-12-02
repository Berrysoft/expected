#ifndef EXPECTED_HPP
#define EXPECTED_HPP

#ifndef __EXPORT_IMPL
    #ifdef _MSC_VER
        #define __EXPORT_IMPL __declspec(dllexport)
    #elif defined(__GNUC__)
        #ifdef __MINGW32__
            #define __EXPORT_IMPL __attribute__((dllexport))
        #else
            #define __EXPORT_IMPL __attribute__((visibility("default")))
        #endif // __MINGW32__
    #else
        #define __EXPORT_IMPL
    #endif // !__EXPORT_IMPL
#endif // !__EXPORT_IMPL

#ifndef __IMPORT_IMPL
    #ifdef _MSC_VER
        #define __IMPORT_IMPL __declspec(dllimport)
    #elif defined(__GNUC__) && defined(__MINGW32__)
        #define __IMPORT_IMPL __attribute__((dllimport))
    #else
        #define __IMPORT_IMPL
    #endif
#endif // !__IMPORT_IMPL

#ifdef EXPECTED_STATIC_DEFINE
    #ifndef __EXPECTED_EXPORT
        #define __EXPECTED_EXPORT
    #endif // !__EXPECTED_EXPORT
    #ifndef __EXPECTED_IMPORT
        #define __EXPECTED_IMPORT
    #endif // !__EXPECTED_IMPORT
#else
    #ifndef __EXPECTED_EXPORT
        #define __EXPECTED_EXPORT __EXPORT_IMPL
    #endif // !__EXPECTED_EXPORT
    #ifndef __EXPECTED_IMPORT
        #define __EXPECTED_IMPORT __IMPORT_IMPL
    #endif // !__EXPECTED_IMPORT
#endif // EXPECTED_STATIC_DEFINE

#ifndef EXPECTED_API
    #define EXPECTED_API __EXPECTED_IMPORT
#endif // !EXPECTED_API

#include <optional>
#include <string>
#include <variant>
#include <version>

#if __cpp_lib_coroutine >= 201902L || __has_include(<coroutine>)
    #include <coroutine>
#else
    #define EXPECTED_EXPERIMENTAL_COROUTINE
    #include <experimental/coroutine>
#endif

namespace expected
{
    namespace coro
    {
#ifdef EXPECTED_EXPERIMENTAL_COROUTINE
        using std::experimental::coroutine_handle;
        using std::experimental::noop_coroutine;
        using std::experimental::suspend_always;
        using std::experimental::suspend_never;
#else
        using std::coroutine_handle;
        using std::noop_coroutine;
        using std::suspend_always;
        using std::suspend_never;
#endif
    } // namespace coro

    namespace impl
    {
        struct result_base
        {
        private:
            std::string m_trace{};

        public:
            void set_stacktrace(std::string&& trace) { m_trace = std::move(trace); }
            constexpr std::string& stacktrace() { return m_trace; }
            constexpr std::string const& stacktrace() const { return m_trace; }
        };
    } // namespace impl

    // struct expected_result
    template <typename TValue, typename TError>
    requires(!std::is_void_v<TError>) struct result final : impl::result_base
    {
    private:
        std::variant<TValue, TError> m_value{};

    public:
        void set_value(TValue&& value) { m_value.template emplace<0>(std::move(value)); }
        void set_error(TError&& value) { m_value.template emplace<1>(std::move(value)); }

    private:
        template <std::size_t index>
        constexpr auto get() const
        {
            return m_value.index() == index ? std::make_optional(std::get<index>(m_value)) : std::nullopt;
        }

    public:
        constexpr std::optional<TValue> value() const { return get<0>(); }
        constexpr std::optional<TError> error() const { return get<1>(); }
    };

    template <typename TError>
    requires(!std::is_void_v<TError>) struct result<void, TError> final : impl::result_base
    {
    private:
        std::optional<TError> m_value{};

    public:
        void set_value() { m_value = std::nullopt; }
        void set_error(TError&& value) { m_value = std::move(value); }

        constexpr bool value() const noexcept { return m_value == std::nullopt; }
        constexpr std::optional<TError> error() const { return m_value; }
    };

    // Tag for void-return functions.
    struct void_t
    {
    };

    inline constexpr void_t evoid{};

    // Wrapper for errors, when return value and error are the same type.
    template <typename TError>
    requires(!std::is_void_v<TError>) struct error_wrapper
    {
        TError m_value;
    };

    struct error_t
    {
    };

    inline constexpr error_t eerror{};

    template <typename TError>
    error_wrapper<TError> operator,(error_t, TError&& error)
    {
        return error_wrapper<TError>{ std::forward<TError>(error) };
    }

    template <typename T>
    struct is_error : std::false_type
    {
    };

    template <typename T>
    struct is_error<error_wrapper<T>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool is_error_v = is_error<T>::value;

    template <typename T>
    concept error_concept = is_error_v<T>;

    template <typename TValue, typename TError>
    requires(!std::is_void_v<TError>) struct task;

    namespace impl
    {
        EXPECTED_API std::string get_stacktrace();

        template <typename D, typename TValue, typename TError>
        requires(!std::is_void_v<TError>) struct promise_base
        {
            result<TValue, TError> m_value;

            using handle_type = coro::coroutine_handle<D>;

            task<TValue, TError> get_return_object() { return task<TValue, TError>{ handle_type::from_promise(*static_cast<D*>(this)) }; }
            constexpr coro::suspend_never initial_suspend() noexcept { return {}; }
            constexpr coro::suspend_always final_suspend() noexcept { return {}; }
            void unhandled_exception() { throw; }

            template <typename TOther>
            struct awaitable
            {
                using other_handle_type = typename task<TOther, TError>::handle_type;

                other_handle_type m_handle{};

                explicit awaitable(other_handle_type handle) : m_handle(handle) {}

                bool await_ready() { return false; }

                coro::coroutine_handle<> await_suspend(handle_type h)
                {
                    auto& value = m_handle.promise().m_value;
                    if (value.error())
                    {
                        h.promise().return_value(*value.error());
                        h.promise().m_value.set_stacktrace(std::move(value.stacktrace()));
                        return std::noop_coroutine();
                    }
                    else
                    {
                        return h;
                    }
                }

                TOther await_resume()
                {
                    if constexpr (!std::is_void_v<TOther>)
                    {
                        return m_handle.promise().m_value.value().value_or(TOther{});
                    }
                }
            };

            template <typename TOther>
            awaitable<TOther> await_transform(task<TOther, TError> const& e)
            {
                return awaitable<TOther>{ e.m_handle };
            }

            void return_value(TError error, std::string trace = get_stacktrace()) requires(!std::is_same_v<TValue, TError>)
            {
                this->m_value.set_error(std::move(error));
                this->m_value.set_stacktrace(std::move(trace));
            }

            template <error_concept TExpected>
            void return_value(TExpected error, std::string trace = get_stacktrace())
            {
                this->m_value.set_error(std::move(error.m_value));
                this->m_value.set_stacktrace(std::move(trace));
            }
        };

        template <typename TValue, typename TError>
        requires(!std::is_void_v<TError>) struct promise : promise_base<promise<TValue, TError>, TValue, TError>
        {
            void return_value(TValue value) { this->m_value.set_value(std::move(value)); }
            using promise_base<promise<TValue, TError>, TValue, TError>::return_value;
        };

        template <typename TError>
        requires(!std::is_void_v<TError>) struct promise<void, TError> : promise_base<promise<void, TError>, void, TError>
        {
            void return_value(void_t) { this->m_value.set_value(); }
            using promise_base<promise<void, TError>, void, TError>::return_value;
        };
    } // namespace impl

    template <typename TValue, typename TError>
    requires(!std::is_void_v<TError>) struct task
    {
        using promise_type = impl::promise<TValue, TError>;

        using handle_type = coro::coroutine_handle<promise_type>;

        handle_type m_handle{};

        task() noexcept = default;
        explicit task(handle_type handle) noexcept : m_handle(handle) {}
        ~task()
        {
            if (m_handle)
            {
                m_handle.destroy();
            }
        }

        task(const task&) = delete;
        task& operator=(const task&) = delete;

        task(task&& e) noexcept : m_handle(e.m_handle) { e.m_handle = {}; }
        task& operator=(task&& e) noexcept
        {
            if (this != std::addressof(e))
            {
                m_handle = e.m_handle;
                e.m_handle = {};
            }
            return *this;
        }

        auto run() const
        {
            auto& value = m_handle.promise().m_value;
            while (!m_handle.done() && !value.error())
            {
                m_handle.resume();
            }
            return value;
        }
    };
} // namespace expected

#endif // !EXPECTED_HPP
