#pragma once

#include <CoreMinimal.h>
#include <variant>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::type_traits
{
    struct AsArray {};
    struct AsString {};

    struct Const{};
    struct NonConst{};

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T>
    struct ExtractValueType;

    template <typename T>
    struct ExtractValueType<T*> { using type = T; };

    template <typename T>
    struct ExtractValueType<TUniquePtr<T>> { using type = T; };

    template <typename T>
    struct ExtractValueType<TSharedPtr<T>> { using type = T; };

    template <typename T>
    struct ExtractValueType<TSharedPtr<T, ESPMode::NotThreadSafe>> { using type = T; };

    // --------------------------------------------------------------------------------------------------------------------

    // TODO: rename MakeNewPtr to TMakeNewPtr
    template <typename T>
    struct MakeNewPtr;

    template <typename T>
    struct MakeNewPtr<TSharedPtr<T>>
    {
        template <typename... T_Args>
        auto operator()(T_Args&&... InArgs)
        {
            return MakeShared<T>(std::forward<T_Args>(InArgs)...);
        }
    };

    template <typename T>
    struct MakeNewPtr<TSharedPtr<T, ESPMode::NotThreadSafe>>
    {
        template <typename... T_Args>
        auto operator()(T_Args&&... InArgs)
        {
            return MakeShared<T, ESPMode::NotThreadSafe>(std::forward<T_Args>(InArgs)...);
        }
    };

    template <typename T>
    struct MakeNewPtr<TUniquePtr<T>>
    {
        template <typename... T_Args>
        auto operator()(T_Args&&... InArgs)
        {
            return MakeUnique<T>(std::forward<T_Args>(InArgs)...);
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T>
    struct MoveOrCopyPtr;

    template <typename T>
    struct MoveOrCopyPtr<TSharedPtr<T>>
    {
        auto operator()(const TSharedPtr<T>& InPtr) -> TSharedPtr<T>
        {
            return InPtr;
        }
    };

    template <typename T>
    struct MoveOrCopyPtr<TSharedPtr<T, ESPMode::NotThreadSafe>>
    {
        auto operator()(const TSharedPtr<T, ESPMode::NotThreadSafe>& InPtr) -> TSharedPtr<T, ESPMode::NotThreadSafe>
        {
            return InPtr;
        }
    };

    template <typename T>
    struct MoveOrCopyPtr<TUniquePtr<T>>
    {
        auto operator()(const TUniquePtr<T>& InOther) -> TUniquePtr<T>
        {
            return std::move(InOther);
        }
    };

    template <typename T>
    using AddConstUnlessAlready = std::conditional_t<
        std::is_const_v<std::remove_pointer_t<T>>,
        T,
        std::add_const_t<T>
    >;

    template<typename T>
    using Binding_Param_T = std::conditional_t<
        std::is_copy_constructible_v<T> && std::is_trivially_copyable_v<T>,
        T,           // Pass by value if copyable
        const T&     // Pass by const reference if not copyable
    >;
}

// --------------------------------------------------------------------------------------------------------------------

// TODO: move this to its own file
namespace ck::policy
{
    struct All {};
    struct Any {};

    struct TransientPackage {};

    struct ReturnOptional {};
    struct DontResetContainer {};

    template <typename T>
    struct TMutability
    {
        static_assert
        (
            std::is_same<type_traits::Const, T>() || std::is_same<type_traits::NonConst, T>(),
            "Mutability can only accept `Const` or `NonConst` as policy params"
        );
    };

    template<>
    struct TMutability<type_traits::Const>{};

    template<>
    struct TMutability<type_traits::NonConst>{};

    using Mutability_Const = TMutability<type_traits::Const>;
    using Mutability_NonConst = TMutability<type_traits::NonConst>;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_Func>
    struct TVisitor
    {
        TVisitor(T_Func InFunc)
            : _Func(InFunc)
        {
        }

        template <typename T_TypeToVisit>
        void operator()(T_TypeToVisit& InVariant)
        {
            std::visit([&](auto& InRequest)
            {
                _Func(InRequest);
            }, InVariant);
        }

        T_Func _Func;
    };

    template <typename T_Func>
    auto
    Visitor(T_Func InFunc) -> TVisitor<T_Func>
    {
        auto V = TVisitor<T_Func>{InFunc};
        return V;
    }
}

// --------------------------------------------------------------------------------------------------------------------
