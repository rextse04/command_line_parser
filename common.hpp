#pragma once
#include <ranges>
#include <type_traits>
#include <concepts>

#define ITER_OF(iter_type, type) (\
    std::iter_type<std::remove_cvref_t<Iter>> &&\
    std::is_same_v<typename std::iterator_traits<std::remove_cvref_t<Iter>>::value_type, type>\
)
#define RANGE_OF(rng_type, type) (\
    ranges::rng_type<std::remove_cvref_t<Rng>> &&\
    std::is_same_v<ranges::range_value_t<std::remove_cvref_t<Rng>>, type>\
)
#define STALE_CLASS(name, ...)\
    name(const name&)__VA_OPT__( requires(__VA_ARGS__)) = delete;\
    name(name&&)__VA_OPT__( requires(__VA_ARGS__)) = delete;\
    name& operator=(const name&)__VA_OPT__( requires(__VA_ARGS__)) = delete;\
    name& operator=(name&&)__VA_OPT__( requires(__VA_ARGS__)) = delete;

namespace cmd {
    namespace ranges = std::ranges;
    namespace views = std::views;
    using namespace std::literals;

    template <typename Ref, typename T>
    requires (std::is_reference_v<Ref>)
    struct follow_const {
    private:
        using RefValue = std::remove_reference_t<Ref>;
        using TConstRefValue = std::add_const_t<std::remove_reference_t<T>>;
        using TConstPtrValue = std::add_const_t<std::remove_pointer_t<T>>;
        using TConstValue = std::add_const_t<T>;
        using TConst = std::conditional_t<std::is_reference_v<T>,
            std::add_lvalue_reference_t<TConstRefValue>,
            std::conditional_t<std::is_pointer_v<T>,
                std::add_pointer_t<TConstPtrValue>,
                TConstValue
            >
        >;

    public:
        using type = std::conditional_t<std::is_const_v<RefValue>, TConst, T>;
    };
    template <typename Ref, typename T>
    using follow_const_t = follow_const<Ref, T>::type;

    template <typename T>
    struct receiver_type {
        static constexpr bool owning = true;
        using type = T;
    };
    template <typename T>
    struct receiver_type<T&> {
        static constexpr bool owning = false;
        using type = const std::remove_reference_t<T>&;
    };
    template <typename T>
    struct receiver_type<T&&> {
        static constexpr bool owning = true;
        using type = std::remove_reference_t<T>;
    };
    template <typename T>
    using receiver_type_t = typename receiver_type<T>::type;

    template <template <typename...> typename, typename>
    struct is_template_instance : std::false_type {};
    template <template <typename...> typename Tmpl, typename... Ts>
    struct is_template_instance<Tmpl, Tmpl<Ts...>> : std::true_type {};
    template <template <typename...> typename Tmpl, typename T>
    constexpr bool is_template_instance_v = is_template_instance<Tmpl, std::remove_cvref_t<T>>::value;

    template <auto Value>
    struct constexpr_eval {};

    template <typename T, typename Tag>
    concept tagged = std::same_as<typename T::tag, Tag>;
    template <typename T, typename Ref>
    concept equiv_to = std::is_same_v<std::remove_cvref_t<Ref>, T>;
}