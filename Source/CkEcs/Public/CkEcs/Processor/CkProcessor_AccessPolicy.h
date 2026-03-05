#pragma once

#include "CkEcs/Registry/CkRegistry.h"

#include <type_traits>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_Fragment>
    struct TReadOnly
    {
        using FragmentType = T_Fragment;
        static constexpr auto IsReadOnly = true;
    };

    template <typename T_Fragment>
    struct TReadWrite
    {
        using FragmentType = T_Fragment;
        static constexpr auto IsReadOnly = false;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T>
    struct TAccessPolicyTraits
    {
        using FragmentType = T;
        static constexpr auto IsReadOnly = false;
    };

    template <typename T>
    struct TAccessPolicyTraits<TReadOnly<T>>
    {
        using FragmentType = T;
        static constexpr auto IsReadOnly = true;
    };

    template <typename T>
    struct TAccessPolicyTraits<TReadWrite<T>>
    {
        using FragmentType = T;
        static constexpr auto IsReadOnly = false;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T>
    using UnwrapFragmentType = typename TAccessPolicyTraits<T>::FragmentType;

    template <typename T>
    constexpr auto IsReadOnlyFragment = TAccessPolicyTraits<T>::IsReadOnly;

    // --------------------------------------------------------------------------------------------------------------------
    // Access policy detail utilities — shared between TProcessor and TParallelProcessor
    // --------------------------------------------------------------------------------------------------------------------

    namespace detail
    {
        // ---- Access policy unwrapping (strips TReadOnly/TReadWrite to get the raw fragment type) ----

        template <typename T>
        struct TUnwrapAccessPolicy
        {
            using Type = T;
        };

        template <typename T>
        struct TUnwrapAccessPolicy<ck::TReadOnly<T>>
        {
            using Type = T;
        };

        template <typename T>
        struct TUnwrapAccessPolicy<ck::TReadWrite<T>>
        {
            using Type = T;
        };

        template <typename T>
        using UnwrapAccessPolicy_T = typename TUnwrapAccessPolicy<T>::Type;

        // ---- Constness resolution for TReadOnly enforcement ----

        template <typename T_Policy, typename T_Fragment>
        struct TResolveConstness
        {
            using Type = T_Fragment&;
        };

        template <typename T_Inner, typename T_Fragment>
        struct TResolveConstness<ck::TReadOnly<T_Inner>, T_Fragment>
        {
            using Type = const T_Fragment&;
        };

        // ---- Filter policies: strip TExclude + empty tags to get only access-annotated fragments ----

        template <typename T_Policy>
        struct TIsExcludedPolicy : std::false_type {};

        template <typename... T_Args>
        struct TIsExcludedPolicy<ck::TExclude<T_Args...>> : std::true_type {};

        template <typename T_Policy>
        struct TIsEmptyPolicy
        {
            static constexpr auto value = std::is_empty_v<UnwrapAccessPolicy_T<T_Policy>>;
        };

        template <typename... T_Policies>
        using PoliciesOnly = entt::type_list_cat_t<
            std::conditional_t<
                TIsExcludedPolicy<T_Policies>::value || TIsEmptyPolicy<T_Policies>::value,
                entt::type_list<>,
                entt::type_list<T_Policies>
            >...
        >;

        // ---- Batch size detection via SFINAE ----

        template <typename T, typename = void>
        struct THasMinBatchSize : std::false_type {};

        template <typename T>
        struct THasMinBatchSize<T, std::void_t<decltype(T::MinBatchSize)>> : std::true_type {};

        // ---- Access policy wrapping detection (for TParallelProcessor static_assert) ----

        template <typename T>
        struct TIsAccessPolicyWrapped : std::false_type {};

        template <typename T>
        struct TIsAccessPolicyWrapped<ck::TReadOnly<T>> : std::true_type {};

        template <typename T>
        struct TIsAccessPolicyWrapped<ck::TReadWrite<T>> : std::true_type {};

    }
}

// --------------------------------------------------------------------------------------------------------------------
