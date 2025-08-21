#pragma once

#include "core/util.h"
#include <type_traits>

namespace nft
{

// === DETECTION IDIOM (C++17 style) ===

namespace detail {
    // Helper to detect if type has specific members/methods
    
    template<typename T>
    using get_code_t = decltype(T::GetCode());
    
    template<typename T>
    using notify_t = decltype(std::declval<T>().Notify());
    
    template<typename T>
    using key_member_t = decltype(std::declval<T>().key);
}

// Detectors using std::void_t
template<typename T>
using has_get_code = std::is_detected<detail::get_code_t, T>;

template<typename T>
using has_notify = std::is_detected<detail::notify_t, T>;

template<typename T>
using has_key_member = std::is_detected<detail::key_member_t, T>;

// === CONDITIONAL TYPE SELECTION ===

template<typename EventType>
using event_storage_t = std::conditional_t<
    std::is_trivially_copyable_v<EventType>,
    EventType,                    // Store by value if trivially copyable
    std::unique_ptr<EventType>   // Store by pointer otherwise
>;

// === TAG DISPATCH ===

struct event_tag {};
struct error_tag {};
struct unknown_tag {};

template<typename T>
constexpr auto get_type_tag() {
    if constexpr (is_valid_event_type_v<T>) {
        return event_tag{};
    } else if constexpr (/* your error type check */) {
        return error_tag{};
    } else {
        return unknown_tag{};
    }
}

// === SFINAE WITH CONCEPTS (C++20/23) ===

template<typename T>
concept ValidEvent = requires {
    typename T;                          // T must be a type
    { T::GetCode() } -> std::convertible_to<std::string>;
    requires std::is_base_of_v<IEventBase, T>;
    requires std::is_class_v<T>;
    requires !std::is_abstract_v<T>;
};

template<typename T>
concept KeyboardEvent = ValidEvent<T> && requires(T t) {
    t.key;
    t.scancode;
    t.action;
    t.mods;
};

template<typename T>
concept MouseEvent = ValidEvent<T> && requires(T t) {
    t.x;
    t.y;
    t.button;
};

} // namespace nft