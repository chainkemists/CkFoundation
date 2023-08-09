#pragma once

#include <CoreMinimal.h>

namespace ck::type_traits
{
    struct as_array {};
    struct as_string {};

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T>
    struct extract_value_type;

    template <typename T>
    struct extract_value_type<T*> { using type = T; };

    template <typename T>
    struct extract_value_type<TUniquePtr<T>> { using type = T; };

    template <typename T>
    struct extract_value_type<TSharedPtr<T>> { using type = T; };

    template <typename T>
    struct extract_value_type<TSharedPtr<T, ESPMode::NotThreadSafe>> { using type = T; };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T>
    struct make_new_ptr;

    template <typename T>
    struct make_new_ptr<TSharedPtr<T>>
    {
        template <typename... T_Args>
        auto operator()(T_Args&&... InArgs)
        {
            return MakeShared<T>(std::forward<T_Args>(InArgs)...);
        }
    };

    template <typename T>
    struct make_new_ptr<TSharedPtr<T, ESPMode::NotThreadSafe>>
    {
        template <typename... T_Args>
        auto operator()(T_Args&&... InArgs)
        {
            return MakeShared<T, ESPMode::NotThreadSafe>(std::forward<T_Args>(InArgs)...);
        }
    };

    template <typename T>
    struct make_new_ptr<TUniquePtr<T>>
    {
        template <typename... T_Args>
        auto operator()(T_Args&&... InArgs)
        {
            return MakeUnique<T>(std::forward<T_Args>(InArgs)...);
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T>
    struct move_or_copy_ptr;

    template <typename T>
    struct move_or_copy_ptr<TSharedPtr<T>>
    {
        auto operator()(const TSharedPtr<T>& InPtr) -> TSharedPtr<T>
        {
            return InPtr;
        }
    };

    template <typename T>
    struct move_or_copy_ptr<TSharedPtr<T, ESPMode::NotThreadSafe>>
    {
        auto operator()(const TSharedPtr<T, ESPMode::NotThreadSafe>& InPtr) -> TSharedPtr<T, ESPMode::NotThreadSafe>
        {
            return InPtr;
        }
    };

    template <typename T>
    struct move_or_copy_ptr<TUniquePtr<T>>
    {
        auto operator()(const TUniquePtr<T>& InOther) -> TUniquePtr<T>
        {
            return std::move(InOther);
        }
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::policy
{
    struct All {};
    struct Any {};

    struct TransientPackage {};
}
