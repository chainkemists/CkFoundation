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
// Static tracker for per-handle-type registration

class CKECS_API FCkAngelScriptHandleBindingTracker
{
public:
    static auto
    TryRegisterHandleType(
        const FString& InTypeName) -> bool
    {
        auto& RegisteredTypes = Get_RegisteredHandleTypes();
        if (RegisteredTypes.Contains(InTypeName))
        {
            return false;
        }
        RegisteredTypes.Add(InTypeName);
        return true;
    }

    static auto
    TryRegisterBaseHandleMethod(
        const FString& InMethodSignature) -> bool
    {
        auto& RegisteredMethods = Get_RegisteredBaseHandleMethods();
        if (RegisteredMethods.Contains(InMethodSignature))
        {
            return false;
        }
        RegisteredMethods.Add(InMethodSignature);
        return true;
    }

private:
    static auto
    Get_RegisteredHandleTypes() -> TSet<FString>&
    {
        static TSet<FString> RegisteredTypes;
        return RegisteredTypes;
    }

    static auto
    Get_RegisteredBaseHandleMethods() -> TSet<FString>&
    {
        static TSet<FString> RegisteredMethods;
        return RegisteredMethods;
    }
};

// --------------------------------------------------------------------------------------------------------------------
// Empty macro - all bindings now done via CK_REGISTER_ANGELSCRIPT_HANDLE_CONVERSION

#define CK_DEFINE_ANGELSCRIPT_HANDLE_BINDINGS(_HandleType_)

// --------------------------------------------------------------------------------------------------------------------

#define CK_REGISTER_ANGELSCRIPT_HANDLE_CONVERSION(_HandleType_)                                                              \
    static void RegisterAngelScriptImplicitConversion()                                                                      \
    {                                                                                                                        \
        /* Use static tracking to prevent duplicate registration */                                                          \
        if (NOT FCkAngelScriptHandleBindingTracker::TryRegisterHandleType(TEXT(#_HandleType_)))                              \
        {                                                                                                                    \
            return;                                                                                                          \
        }                                                                                                                    \
                                                                                                                             \
        auto Bind = FAngelscriptBinds::ExistingClass(#_HandleType_);                                                         \
        if (Bind.GetTypeInfo() == nullptr)                                                                                   \
        {                                                                                                                    \
            return;                                                                                                          \
        }                                                                                                                    \
                                                                                                                             \
        /* Implicit conversions */                                                                                           \
        Bind.Method("FCk_Handle opImplConv() const", [](const _HandleType_& InOther) -> FCk_Handle                           \
        {                                                                                                                    \
            return InOther;                                                                                                  \
        });                                                                                                                  \
        Bind.Method("FCk_Handle& opImplCast()", [](_HandleType_& InOther) -> FCk_Handle&                                     \
        {                                                                                                                    \
            return InOther;                                                                                                  \
        });                                                                                                                  \
        Bind.Method("const FCk_Handle& opImplCast() const", [](_HandleType_ const& InOther) -> const FCk_Handle&             \
        {                                                                                                                    \
            return InOther;                                                                                                  \
        });                                                                                                                  \
        Bind.Method("FCk_Handle& H()", [](_HandleType_& InOther) -> FCk_Handle&                                              \
        {                                                                                                                    \
            return InOther;                                                                                                  \
        });                                                                                                                  \
                                                                                                                             \
        /* Core usability methods */                                                                                         \
        Bind.Method("bool IsValid() const", [](_HandleType_ const& Self) -> bool                                             \
        {                                                                                                                    \
            return ck::IsValid(Self);                                                                                        \
        });                                                                                                                  \
        Bind.Method("FString ToString() const", [](_HandleType_ const& Self) -> FString                                      \
        {                                                                                                                    \
            return Self.ToString();                                                                                          \
        });                                                                                                                  \
        Bind.Method("FString Debug() const", [](_HandleType_ const& Self) -> FString                                         \
        {                                                                                                                    \
            Self.DoFireEnsure();                                                                                             \
            return Self.ToString();                                                                                          \
        });                                                                                                                  \
                                                                                                                             \
        /* Equality operators on the handle type */                                                                          \
        Bind.Method("bool opEquals(const " #_HandleType_ "& Other) const", [](                                               \
            const _HandleType_& A, const _HandleType_& B) -> bool                                                            \
        {                                                                                                                    \
            return A == B;                                                                                                   \
        });                                                                                                                  \
        Bind.Method("bool opEquals(const FCk_Handle& Other) const", [](                                                      \
            const _HandleType_& A, const FCk_Handle& B) -> bool                                                              \
        {                                                                                                                    \
            return A == B;                                                                                                   \
        });                                                                                                                  \
                                                                                                                             \
        /* Methods on FCk_Handle base - use per-method tracking since multiple handle types add to FCk_Handle */            \
        auto BaseBind = FAngelscriptBinds::ExistingClass("FCk_Handle");                                                      \
        if (BaseBind.GetTypeInfo() != nullptr)                                                                               \
        {                                                                                                                    \
            /* To_HandleType conversion */                                                                                   \
            if (FCkAngelScriptHandleBindingTracker::TryRegisterBaseHandleMethod(                                             \
                TEXT("To_" #_HandleType_)))                                                                                  \
            {                                                                                                                \
                BaseBind.Method(#_HandleType_ " To_" #_HandleType_ "(ECk_SanityCheck InChecked = ECk_SanityCheck::UnChecked) const", \
                [](const FCk_Handle& InOther, ECk_SanityCheck InChecked) -> _HandleType_                                     \
                {                                                                                                            \
                    if (InChecked == ECk_SanityCheck::UnChecked)                                                             \
                    { return Cast(InOther); }                                                                                \
                    return CastChecked(InOther);                                                                             \
                });                                                                                                          \
            }                                                                                                                \
                                                                                                                             \
            /* opCast to this handle type */                                                                                 \
            if (FCkAngelScriptHandleBindingTracker::TryRegisterBaseHandleMethod(                                             \
                TEXT("opCast_" #_HandleType_)))                                                                              \
            {                                                                                                                \
                BaseBind.Method(#_HandleType_ " opCast() const",                                                             \
                [](const FCk_Handle& InOther) -> _HandleType_                                                                \
                {                                                                                                            \
                    return Cast(InOther);                                                                                    \
                });                                                                                                          \
            }                                                                                                                \
                                                                                                                             \
            /* opEquals with this handle type */                                                                             \
            if (FCkAngelScriptHandleBindingTracker::TryRegisterBaseHandleMethod(                                             \
                TEXT("opEquals_" #_HandleType_)))                                                                            \
            {                                                                                                                \
                BaseBind.Method("bool opEquals(const " #_HandleType_ "& in) const", [](                                      \
                    const FCk_Handle& A, const _HandleType_& B) -> bool                                                      \
                {                                                                                                            \
                    return A == B;                                                                                           \
                });                                                                                                          \
            }                                                                                                                \
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

// Note: Uses FCkAngelScriptHandleBindingTracker to prevent duplicates across translation units
inline AS_FORCE_LINK const FAngelscriptBinds::FBind BindEquals_FCk_Handle (FAngelscriptBinds::EOrder::Late, []
{
    /* Use handle type tracker with special key for base FCk_Handle */
    if (NOT FCkAngelScriptHandleBindingTracker::TryRegisterHandleType(TEXT("FCk_Handle_BASE")))
    {
        return;
    }

    auto Bind = FAngelscriptBinds::ExistingClass("FCk_Handle");
    if (Bind.GetTypeInfo() == nullptr)
    {
        return;
    }

    Bind.Method("bool opEquals(const FCk_Handle& Other) const",
        METHODPR_TRIVIAL(bool, FCk_Handle, operator==, (const FCk_Handle&) const));

    Bind.Method("FString ToString() const", [](FCk_Handle const& Self) -> FString
    {
        return Self.ToString();
    });

    Bind.Method("bool IsValid() const", [](FCk_Handle const& Self) -> bool
    {
        return ck::IsValid(Self);
    });

    Bind.Method("FString Debug() const", [](FCk_Handle const& Self) -> FString
    {
        Self.DoFireEnsure();
        return Self.ToString();
    });
});

#endif // WITH_ANGELSCRIPT_CK

// --------------------------------------------------------------------------------------------------------------------
