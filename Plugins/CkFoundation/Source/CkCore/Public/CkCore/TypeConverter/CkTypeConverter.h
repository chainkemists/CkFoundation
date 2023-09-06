#pragma once

#include <utility>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    enum class TypeConverterPolicy
    {
        TypeToUnreal,
        UnrealToType
    };

    template <typename T, TypeConverterPolicy T_Policy>
    struct TTypeConverter
    {
        T&& operator()(T&& InPayload) const
        {
            return std::forward<T>(InPayload);
        }
    };
}

// --------------------------------------------------------------------------------------------------------------------