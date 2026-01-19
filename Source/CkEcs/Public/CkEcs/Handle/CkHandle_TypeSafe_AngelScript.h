#pragma once

#include "CkHandle_TypeSafe.h"

#if WITH_ANGELSCRIPT_CK

#include "CkCore/Format/CkFormat.h"

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
// Cross-handle conversion registry for As_/Is_ methods between all derived handle types

class CKECS_API FCkAngelScriptHandleTypeRegistry
{
public:
    // Function signatures for Has/Cast operations
    using FHasFunction = TFunction<bool(const FCk_Handle&)>;
    using FCastFunction = TFunction<FCk_Handle(const FCk_Handle&)>;
    using FCastCheckedFunction = TFunction<FCk_Handle(const FCk_Handle&)>;

    struct FHandleTypeInfo
    {
        FString TypeName;       // e.g., "FCk_Handle_Probe"
        FString ShortName;      // e.g., "Probe"
        FHasFunction HasFunc;
        FCastFunction CastFunc;
        FCastCheckedFunction CastCheckedFunc;
    };

    static auto
    RegisterHandleType(
        const FString& InTypeName,
        const FString& InShortName,
        FHasFunction InHasFunc,
        FCastFunction InCastFunc,
        FCastCheckedFunction InCastCheckedFunc) -> void;

    static auto
    GetRegisteredHandleTypes() -> const TArray<FHandleTypeInfo>&;

    static auto
    FindHandleTypeByShortName(
        const FString& InShortName) -> const FHandleTypeInfo*;

    static auto
    BindCrossHandleConversions() -> void;

private:
    static auto
    GetMutableRegisteredHandleTypes() -> TArray<FHandleTypeInfo>&;

    static auto
    GetBoundPairs() -> TSet<TPair<FString, FString>>&;
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

    static auto
    TryRegisterDerivedHandleMethod(
        const FString& InSourceType,
        const FString& InMethodSignature) -> bool
    {
        auto& RegisteredMethods = Get_RegisteredDerivedHandleMethods();
        auto Key = FString::Printf(TEXT("%s::%s"), *InSourceType, *InMethodSignature);
        if (RegisteredMethods.Contains(Key))
        {
            return false;
        }
        RegisteredMethods.Add(Key);
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

    static auto
    Get_RegisteredDerivedHandleMethods() -> TSet<FString>&
    {
        static TSet<FString> RegisteredMethods;
        return RegisteredMethods;
    }
};

// --------------------------------------------------------------------------------------------------------------------
// Helper to extract short name from handle type (e.g., "FCk_Handle_Probe" -> "Probe")

inline auto
ExtractHandleShortName(
    const FString& InFullTypeName) -> FString
{
    static const FString Prefix = TEXT("FCk_Handle_");
    if (InFullTypeName.StartsWith(Prefix))
    {
        return InFullTypeName.RightChop(Prefix.Len());
    }
    return InFullTypeName;
}

// --------------------------------------------------------------------------------------------------------------------
// Empty macro - all bindings now done via CK_REGISTER_ANGELSCRIPT_HANDLE_CONVERSION

#define CK_DEFINE_ANGELSCRIPT_HANDLE_BINDINGS(_HandleType_)

// --------------------------------------------------------------------------------------------------------------------

#define CK_REGISTER_ANGELSCRIPT_HANDLE_CONVERSION(_HandleType_)                                                                    \
    static void RegisterAngelScriptImplicitConversion()                                                                            \
    {                                                                                                                              \
        /* Use static tracking to prevent duplicate registration */                                                                \
        if (NOT FCkAngelScriptHandleBindingTracker::TryRegisterHandleType(TEXT(#_HandleType_)))                                    \
        {                                                                                                                          \
            return;                                                                                                                \
        }                                                                                                                          \
                                                                                                                                   \
        auto Bind = FAngelscriptBinds::ExistingClass(#_HandleType_);                                                               \
        if (Bind.GetTypeInfo() == nullptr)                                                                                         \
        {                                                                                                                          \
            return;                                                                                                                \
        }                                                                                                                          \
                                                                                                                                   \
        /* Implicit conversions */                                                                                                 \
        Bind.Method("FCk_Handle opImplConv() const", [](const _HandleType_& InOther) -> FCk_Handle                                 \
        {                                                                                                                          \
            return InOther;                                                                                                        \
        });                                                                                                                        \
        Bind.Method("FCk_Handle& opImplCast()", [](_HandleType_& InOther) -> FCk_Handle&                                           \
        {                                                                                                                          \
            return InOther;                                                                                                        \
        });                                                                                                                        \
        Bind.Method("const FCk_Handle& opImplCast() const", [](_HandleType_ const& InOther) -> const FCk_Handle&                   \
        {                                                                                                                          \
            return InOther;                                                                                                        \
        });                                                                                                                        \
        Bind.Method("FCk_Handle& H()", [](_HandleType_& InOther) -> FCk_Handle&                                                    \
        {                                                                                                                          \
            return InOther;                                                                                                        \
        });                                                                                                                        \
                                                                                                                                   \
        /* Core usability methods */                                                                                               \
        Bind.Method("bool IsValid() const", [](_HandleType_ const& Self) -> bool                                                   \
        {                                                                                                                          \
            return ck::IsValid(Self);                                                                                              \
        });                                                                                                                        \
        Bind.Method("FString ToString() const", [](_HandleType_ const& Self) -> FString                                            \
        {                                                                                                                          \
            return Self.ToString();                                                                                                \
        });                                                                                                                        \
        Bind.Method("FString Debug() const", [](_HandleType_ const& Self) -> FString                                               \
        {                                                                                                                          \
            Self.DoFireEnsure();                                                                                                   \
            return Self.ToString();                                                                                                \
        });                                                                                                                        \
                                                                                                                                   \
        /* Equality operators on the handle type */                                                                                \
        Bind.Method("bool opEquals(const " #_HandleType_ "& Other) const", [](                                                     \
            const _HandleType_& A, const _HandleType_& B) -> bool                                                                  \
        {                                                                                                                          \
            return A == B;                                                                                                         \
        });                                                                                                                        \
        Bind.Method("bool opEquals(const FCk_Handle& Other) const", [](                                                            \
            const _HandleType_& A, const FCk_Handle& B) -> bool                                                                    \
        {                                                                                                                          \
            return A == B;                                                                                                         \
        });                                                                                                                        \
                                                                                                                                   \
        /* Register this handle type to the cross-handle registry */                                                               \
        auto ShortName = ExtractHandleShortName(TEXT(#_HandleType_));                                                              \
        FCkAngelScriptHandleTypeRegistry::RegisterHandleType(                                                                      \
            TEXT(#_HandleType_),                                                                                                   \
            ShortName,                                                                                                             \
            [](const FCk_Handle& InHandle) -> bool { return Has(InHandle); },                                                      \
            [](const FCk_Handle& InHandle) -> FCk_Handle { return Cast(InHandle); },                                               \
            [](const FCk_Handle& InHandle) -> FCk_Handle { return CastChecked(InHandle); }                                         \
        );                                                                                                                         \
                                                                                                                                   \
        /* Methods on FCk_Handle base - use per-method tracking since multiple handle types add to FCk_Handle */                   \
        auto BaseBind = FAngelscriptBinds::ExistingClass("FCk_Handle");                                                            \
        if (BaseBind.GetTypeInfo() != nullptr)                                                                                     \
        {                                                                                                                          \
            /* As_ShortName conversion (e.g., As_Probe) */                                                                         \
            auto ToMethodKey = FString::Printf(TEXT("As_%s"), *ShortName);                                                         \
            if (FCkAngelScriptHandleBindingTracker::TryRegisterBaseHandleMethod(ToMethodKey))                                      \
            {                                                                                                                      \
                auto ToMethodSig = FString::Printf(TEXT("%hs As_%s(ECk_SanityCheck InChecked = ECk_SanityCheck::Checked) const"),  \
                    #_HandleType_, *ShortName);                                                                                    \
                BaseBind.Method(TCHAR_TO_ANSI(*ToMethodSig),                                                                       \
                [](const FCk_Handle& InOther, ECk_SanityCheck InChecked) -> _HandleType_                                           \
                {                                                                                                                  \
                    if (InChecked == ECk_SanityCheck::UnChecked)                                                                   \
                    { return Cast(InOther); }                                                                                      \
                    return CastChecked(InOther);                                                                                   \
                });                                                                                                                \
            }                                                                                                                      \
                                                                                                                                   \
            /* Is_ShortName check (e.g., Is_Probe) */                                                                              \
            auto HasMethodKey = FString::Printf(TEXT("Is_%s"), *ShortName);                                                        \
            if (FCkAngelScriptHandleBindingTracker::TryRegisterBaseHandleMethod(HasMethodKey))                                     \
            {                                                                                                                      \
                auto HasMethodSig = FString::Printf(TEXT("bool Is_%s() const"), *ShortName);                                       \
                BaseBind.Method(TCHAR_TO_ANSI(*HasMethodSig),                                                                      \
                [](const FCk_Handle& InOther) -> bool                                                                              \
                {                                                                                                                  \
                    return ck::IsValid(Cast(InOther));                                                                             \
                });                                                                                                                \
            }                                                                                                                      \
                                                                                                                                   \
            /* opCast to this handle type */                                                                                       \
            if (FCkAngelScriptHandleBindingTracker::TryRegisterBaseHandleMethod(                                                   \
                TEXT("opCast_" #_HandleType_)))                                                                                    \
            {                                                                                                                      \
                BaseBind.Method(#_HandleType_ " opCast() const",                                                                   \
                [](const FCk_Handle& InOther) -> _HandleType_                                                                      \
                {                                                                                                                  \
                    return Cast(InOther);                                                                                          \
                });                                                                                                                \
            }                                                                                                                      \
                                                                                                                                   \
            /* opEquals with this handle type */                                                                                   \
            if (FCkAngelScriptHandleBindingTracker::TryRegisterBaseHandleMethod(                                                   \
                TEXT("opEquals_" #_HandleType_)))                                                                                  \
            {                                                                                                                      \
                BaseBind.Method("bool opEquals(const " #_HandleType_ "& Other) const", [](                                         \
                    const FCk_Handle& A, const _HandleType_& B) -> bool                                                            \
                {                                                                                                                  \
                    return A == B;                                                                                                 \
                });                                                                                                                \
            }                                                                                                                      \
        }                                                                                                                          \
    }                                                                                                                              \
                                                                                                                                   \
private:                                                                                                                           \
    static inline bool AngelScriptRegistered = []() -> bool                                                                        \
    {                                                                                                                              \
        FCkAngelScriptHandleRegistration::RegisterHandleConversion(                                                                \
            &RegisterAngelScriptImplicitConversion);                                                                               \
        return true;                                                                                                               \
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
