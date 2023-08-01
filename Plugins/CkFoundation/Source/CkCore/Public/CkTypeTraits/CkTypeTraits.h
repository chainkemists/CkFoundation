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

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T>
    struct make_new_ptr;

    template <typename T>
    struct make_new_ptr<TSharedPtr<T>>
    {
        using type = T;

        template <typename... T_Args>
        auto operator()(T_Args&&... InArgs)
        {
            return MakeShared<T>(std::forward<T_Args>(InArgs)...);
        }
    };

    template <typename T>
    struct make_new_ptr<TUniquePtr<T>>
    {
        using type = T;

        template <typename... T_Args>
        auto operator()(T_Args&&... InArgs)
        {
            return MakeUnique<T>(std::forward<T_Args>(InArgs)...);
        }
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::policy
{
    struct All {};
    struct Any {};
}
