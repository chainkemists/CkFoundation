#pragma once

#include "CkHandle_TypeSafe.h"

#if WITH_ANGELSCRIPT_CK

#include <AngelscriptBinds.h>
#include <AngelscriptManager.h>

// --------------------------------------------------------------------------------------------------------------------

class CKECS_API FCkAngelScriptHandleRegistration
{
public:
    using FRegistrationFunction = TFunction<void()>;

    static auto
    RegisterHandleConversion(
        const FRegistrationFunction& InRegistrationFunc) -> void;

private:
    static auto
    EnsureCallbackRegistered() -> void;

    static auto
    RegisterAllHandleConversions() -> void;

    static auto
    GetRegistrationFunctions() -> TArray<FRegistrationFunction>&;

private:
    static inline FDelegateHandle _PreCompileDelegateHandle;
};

// --------------------------------------------------------------------------------------------------------------------
// AngelScript binding macros for handle types

#define CK_DEFINE_ANGELSCRIPT_HANDLE_BINDINGS(_HandleType_)                                                                  \
    inline AS_FORCE_LINK const FAngelscriptBinds::FBind BindEquals_##_HandleType_##_To_##_HandleType_ (                      \
        FAngelscriptBinds::EOrder::Early, []                                                                                \
    {                                                                                                                        \
        const FBindFlags Flags;                                                                                              \
        auto FBindingHandle_ = FAngelscriptBinds::ValueClass<_HandleType_>(#_HandleType_, Flags);                            \
        FBindingHandle_.Method("bool opEquals(const " #_HandleType_ "& Other) const",                                        \
        [](const _HandleType_& In1, const _HandleType_& In2)                                                                 \
	    {                                                                                                                    \
		    return In1.ConvertToHandle() == In2.ConvertToHandle();                                                           \
	    });                                                                                                                  \
    });                                                                                                                      \
    inline AS_FORCE_LINK const FAngelscriptBinds::FBind BindEquals_##_HandleType_##_To_FCk_Handle (                         \
        FAngelscriptBinds::EOrder::Early, []                                                                                \
    {                                                                                                                        \
        const FBindFlags Flags;                                                                                              \
        auto FBindingHandle_ = FAngelscriptBinds::ValueClass<_HandleType_>(#_HandleType_, Flags);                            \
        FBindingHandle_.Method("bool opEquals(const FCk_Handle& Other) const",                                               \
        METHODPR_TRIVIAL(bool, _HandleType_, operator==, (const FCk_Handle&) const));                                        \
    });                                                                                                                      \
    inline AS_FORCE_LINK const FAngelscriptBinds::FBind BindEquals_FCk_Handle_To_##_HandleType_ (                           \
        FAngelscriptBinds::EOrder::Early, []                                                                                \
    {                                                                                                                        \
        const FBindFlags Flags;                                                                                              \
        auto FBindingHandle_ = FAngelscriptBinds::ValueClass<FCk_Handle>("FCk_Handle", Flags);                               \
        FBindingHandle_.Method("bool opEquals(const " #_HandleType_ "& Other) const",                                        \
        METHODPR_TRIVIAL(bool, FCk_Handle, operator==, (const _HandleType_&) const));                                        \
    });

// --------------------------------------------------------------------------------------------------------------------

#define CK_REGISTER_ANGELSCRIPT_HANDLE_CONVERSION(_HandleType_)                                                             \
    static void RegisterAngelScriptImplicitConversion()                                                                     \
    {                                                                                                                        \
        auto Bind = FAngelscriptBinds::ExistingClass(#_HandleType_);                                                         \
        if (NOT Bind.HasMethod("FCk_Handle opImplConv() const"))                                                             \
        {                                                                                                                    \
            Bind.Method("FCk_Handle opImplConv() const", [](const _HandleType_& InOther) -> FCk_Handle                       \
            {                                                                                                                \
                return InOther;                                                                                              \
            });                                                                                                              \
        }                                                                                                                    \
        if (NOT Bind.HasMethod("FCk_Handle& opImplCast()"))                                                                  \
        {                                                                                                                    \
            Bind.Method("FCk_Handle& opImplCast()", [](_HandleType_& InOther) -> FCk_Handle&                                 \
            {                                                                                                                \
                return InOther;                                                                                              \
            });                                                                                                              \
        }                                                                                                                    \
        if (NOT Bind.HasMethod("const FCk_Handle& opImplCast() const"))                                                      \
        {                                                                                                                    \
            Bind.Method("const FCk_Handle& opImplCast() const", [](_HandleType_ const& InOther) -> const FCk_Handle&         \
            {                                                                                                                \
                return InOther;                                                                                              \
            });                                                                                                              \
        }                                                                                                                    \
        if (NOT Bind.HasMethod("FCk_Handle& H()"))                                                                           \
        {                                                                                                                    \
            Bind.Method("FCk_Handle& H()", [](_HandleType_& InOther) -> FCk_Handle&                                          \
            {                                                                                                                \
                return InOther;                                                                                              \
            });                                                                                                              \
        }                                                                                                                    \
                                                                                                                             \
        /* Add core scripting usability methods */                                                                           \
        if (NOT Bind.HasMethod("bool IsValid() const"))                                                                      \
        {                                                                                                                    \
            Bind.Method("bool IsValid() const", [](_HandleType_ const& Self) -> bool                                         \
            {                                                                                                                \
                return ck::IsValid(Self);                                                                                    \
            });                                                                                                              \
        }                                                                                                                    \
                                                                                                                             \
        if (NOT Bind.HasMethod("FString ToString() const"))                                                                  \
        {                                                                                                                    \
            Bind.Method("FString ToString() const", [](_HandleType_ const& Self) -> FString                                  \
            {                                                                                                                \
                return Self.ToString();                                                                                      \
            });                                                                                                              \
        }                                                                                                                    \
                                                                                                                             \
        if (NOT Bind.HasMethod("FString Debug() const"))                                                                     \
        {                                                                                                                    \
            Bind.Method("FString Debug() const", [](_HandleType_ const& Self) -> FString                                     \
            {                                                                                                                \
                Self.DoFireEnsure();                                                                                         \
                return Self.ToString();                                                                                      \
            });                                                                                                              \
        }                                                                                                                    \
                                                                                                                             \
        if (NOT Bind.HasMethod("bool opEquals(const " #_HandleType_ "& in) const"))                                          \
        {                                                                                                                    \
            Bind.Method("bool opEquals(const " #_HandleType_ "& in) const", [](                                              \
                const _HandleType_& A, const _HandleType_& B) -> bool                                                        \
            {                                                                                                                \
                return A == B;                                                                                               \
            });                                                                                                              \
        }                                                                                                                    \
                                                                                                                             \
        if (NOT Bind.HasMethod("bool opEquals(const FCk_Handle& in) const"))                                                 \
        {                                                                                                                    \
            Bind.Method("bool opEquals(const FCk_Handle& in) const", [](                                                     \
                const _HandleType_& A, const FCk_Handle& B) -> bool                                                          \
            {                                                                                                                \
                return A == B;                                                                                               \
            });                                                                                                              \
        }                                                                                                                    \
                                                                                                                             \
        auto BaseBind = FAngelscriptBinds::ExistingClass("FCk_Handle");                                                      \
        if (NOT BaseBind.HasMethod(#_HandleType_ " To_" #_HandleType_ "(ECk_SanityCheck InChecked = ECk_SanityCheck::UnChecked) const")) \
        {                                                                                                                    \
            BaseBind.Method(#_HandleType_ " To_" #_HandleType_ "(ECk_SanityCheck InChecked = ECk_SanityCheck::UnChecked) const", \
            [](const FCk_Handle& InOther, ECk_SanityCheck InChecked) -> _HandleType_                                         \
            {                                                                                                                \
                if (InChecked == ECk_SanityCheck::UnChecked)                                                                 \
                { return Cast(InOther); }                                                                                    \
                                                                                                                             \
                return CastChecked(InOther);                                                                                 \
            });                                                                                                              \
        }                                                                                                                    \
        if (NOT BaseBind.HasMethod(#_HandleType_ " opCast() const"))                                                         \
        {                                                                                                                    \
            BaseBind.Method(#_HandleType_ " opCast() const",                                                                 \
            [](const FCk_Handle& InOther) -> _HandleType_                                                                    \
            {                                                                                                                \
                return Cast(InOther);                                                                                        \
            });                                                                                                              \
        }                                                                                                                    \
        if (NOT BaseBind.HasMethod("bool opEquals(const " #_HandleType_ "& in) const"))                                      \
        {                                                                                                                    \
            BaseBind.Method("bool opEquals(const " #_HandleType_ "& in) const", [](                                          \
                const FCk_Handle& A, const _HandleType_& B) -> bool                                                          \
            {                                                                                                                \
                return A == B;                                                                                               \
            });                                                                                                              \
        }                                                                                                                    \
        if (NOT BaseBind.HasMethod("bool opEquals(const FCk_Handle& in) const"))                                             \
        {                                                                                                                    \
            BaseBind.Method("bool opEquals(const FCk_Handle& in) const", [](                                                 \
                const FCk_Handle& A, const FCk_Handle& B) -> bool                                                            \
            {                                                                                                                \
                return A == B;                                                                                               \
            });                                                                                                              \
        }                                                                                                                    \
    }                                                                                                                        \
                                                                                                                             \
private:                                                                                                                     \
    static inline bool AngelScriptRegistered = []() -> bool                                                                  \
    {                                                                                                                        \
        FCkAngelScriptHandleRegistration::RegisterHandleConversion(                                                          \
            &RegisterAngelScriptImplicitConversion);                                                                         \
        return true;                                                                                                         \
    }();

// --------------------------------------------------------------------------------------------------------------------
// Base FCk_Handle bindings

inline AS_FORCE_LINK const FAngelscriptBinds::FBind BindEquals_FCk_Handle (FAngelscriptBinds::EOrder::Early, []
{
    const FBindFlags Flags;
    auto Bind = FAngelscriptBinds::ValueClass<FCk_Handle>("FCk_Handle", Flags);
    if (NOT Bind.HasMethod("bool opEquals(const FCk_Handle& Other) const"))
    {
        Bind.Method("bool opEquals(const FCk_Handle& Other) const",
          METHODPR_TRIVIAL(bool, FCk_Handle, operator==, (const FCk_Handle&) const));
    }

    if (NOT Bind.HasMethod("FString ToString() const"))
    {
        Bind.Method("FString ToString() const", [](FCk_Handle const& Self) -> FString
        {
            return Self.ToString();
        });
    }

    if (NOT Bind.HasMethod("bool IsValid() const"))
    {
        Bind.Method("bool IsValid() const", [](FCk_Handle const& Self) -> bool
        {
            return ck::IsValid(Self);
        });
    }

    if (NOT Bind.HasMethod("FString Debug() const"))
    {
        Bind.Method("FString Debug() const", [](FCk_Handle const& Self) -> FString
        {
            Self.DoFireEnsure();
            return Self.ToString();
        });
    }
});

#endif // WITH_ANGELSCRIPT_CK

// --------------------------------------------------------------------------------------------------------------------
