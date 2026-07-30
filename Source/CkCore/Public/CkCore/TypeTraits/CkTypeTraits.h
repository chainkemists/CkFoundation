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
            // Copying a null ptr crashes when the debugger copies handles with freed registries.
            if (NOT InPtr.IsValid())
            { return nullptr; }
            return InPtr;
        }
    };

    template <typename T>
    struct MoveOrCopyPtr<TSharedPtr<T, ESPMode::NotThreadSafe>>
    {
        auto operator()(const TSharedPtr<T, ESPMode::NotThreadSafe>& InPtr) -> TSharedPtr<T, ESPMode::NotThreadSafe>
        {
            // Copying a null ptr crashes when the debugger copies handles with freed registries.
            if (NOT InPtr.IsValid())
            { return nullptr; }
            return InPtr;
        }
    };

    template <typename T>
    struct MoveOrCopyPtr<TUniquePtr<T>>
    {
        // A TUniquePtr cannot be copied from a const source; reject at compile time instead of
        // std::move-ing a const lvalue (which cannot bind the move ctor either).
        static_assert(sizeof(T) == 0, "TPtrWrapper over a TUniquePtr is not copyable");
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T>
    using AddConstUnlessAlready = std::conditional_t<
        std::is_const_v<std::remove_pointer_t<T>>,
        T,
        std::add_const_t<T>
    >;

    template<typename T>
    using Binding_Param_T = std::conditional_t<
        std::is_copy_constructible_v<T> && std::is_trivially_copyable_v<T>,
        T,
        const T&
    >;

    template<typename T>
    struct TIsRefOrPointerOrSmallTrivial
    {
        using DecayedType = std::remove_const<T>::type;

        enum { Value =
            TIsReferenceType<T>::Value ||
            TIsPointer<T>::Value ||
            (TIsTrivial<DecayedType>::Value && sizeof(DecayedType) <= 16)
        };
    };

    template<typename Signature>
    struct TFuncArgsAreAllRefOrPointerOrSmallTrivial;

    template<typename RetType>
    struct TFuncArgsAreAllRefOrPointerOrSmallTrivial<RetType()>
    {
        enum { Value = true };
    };

    template<typename RetType, typename FirstArg, typename... RestArgs>
    struct TFuncArgsAreAllRefOrPointerOrSmallTrivial<RetType(FirstArg, RestArgs...)>
    {
        enum { Value =
            TIsRefOrPointerOrSmallTrivial<FirstArg>::Value &&
            TFuncArgsAreAllRefOrPointerOrSmallTrivial<RetType(RestArgs...)>::Value
        };
    };

    template<typename RetType, typename... Args>
    struct TFuncArgsAreAllRefOrPointerOrSmallTrivial<RetType(Args...) const>
        : TFuncArgsAreAllRefOrPointerOrSmallTrivial<RetType(Args...)>
    {
    };

    template<typename Class, typename RetType, typename... Args>
    struct TFuncArgsAreAllRefOrPointerOrSmallTrivial<RetType(Class::*)(Args...)>
        : TFuncArgsAreAllRefOrPointerOrSmallTrivial<RetType(Args...)>
    {
    };

    template<typename Class, typename RetType, typename... Args>
    struct TFuncArgsAreAllRefOrPointerOrSmallTrivial<RetType(Class::*)(Args...) const>
        : TFuncArgsAreAllRefOrPointerOrSmallTrivial<RetType(Args...)>
    {
    };
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
