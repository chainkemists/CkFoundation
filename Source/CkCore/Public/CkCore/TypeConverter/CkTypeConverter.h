#pragma once

#include "CkCore/Algorithms/CkAlgorithms.h"

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

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    template <typename T>
    struct TTypeConverter<TWeakObjectPtr<T>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TWeakObjectPtr<T>& InPayload) const
        {
            return ck::IsValid(InPayload) ? InPayload.Get() : static_cast<T*>(nullptr);
        }
    };

    template <typename T>
    struct TTypeConverter<TArray<TWeakObjectPtr<T>>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TArray<TWeakObjectPtr<T>>& InPayload) const
        {
            const auto ToRet = ck::algo::Transform<TArray<T*>>(InPayload, [&](const TWeakObjectPtr<T>& InPtr)
            {
                return ck::IsValid(InPtr) ? InPtr.Get() : static_cast<T*>(nullptr);
            });

            return ToRet;
        }
    };

    // --------------------------------------------------------------------------------------------------------------------
}
