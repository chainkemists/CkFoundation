#pragma once

#include "CkHandle_TypeSafe.h"

#if WITH_ANGELSCRIPT_CK

#include "CkCore/Format/CkFormat.h"
#include "CkEcs/Handle/CkHandle_AngelScript_Registry.h"

#include <AngelscriptBinds.h>
#include <AngelscriptManager.h>

// --------------------------------------------------------------------------------------------------------------------

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

/**
 * Macro to register a static (C++ USTRUCT) handle type with AngelScript.
 *
 * Uses deferred registration pattern:
 * 1. Static initializer registers a callback
 * 2. Callback is executed during PreCompile when Has/Cast/CastChecked are available
 * 3. Registry handles cross-handle conversions and base method propagation
 *
 * Requirements:
 *   - The handle type must have static Has(), Cast(), and CastChecked() methods
 *   - The handle type name must follow the pattern FCk_Handle_XYZ
 */
#define CK_REGISTER_ANGELSCRIPT_HANDLE_CONVERSION(_HandleType_)                                                        \
    static void RegisterAngelScript_##_HandleType_##_Complete()                                                        \
    {                                                                                                                  \
        auto Bind = FAngelscriptBinds::ExistingClass(#_HandleType_);                                                   \
        if (Bind.GetTypeInfo() == nullptr)                                                                             \
        { return; }                                                                                                    \
                                                                                                                       \
        Bind.Method("FCk_Handle opImplConv() const", [](const _HandleType_& InOther) -> FCk_Handle                     \
        { return InOther; });                                                                                          \
        Bind.Method("FCk_Handle& opImplCast()", [](_HandleType_& InOther) -> FCk_Handle&                               \
        { return InOther; });                                                                                          \
        Bind.Method("const FCk_Handle& opImplCast() const", [](_HandleType_ const& InOther) -> const FCk_Handle&       \
        { return InOther; });                                                                                          \
        Bind.Method("FCk_Handle& H()", [](_HandleType_& InOther) -> FCk_Handle&                                        \
        { return InOther; });                                                                                          \
                                                                                                                       \
        Bind.Method("bool IsValid() const", [](_HandleType_ const& Self) -> bool                                       \
        { return ck::IsValid(Self); });                                                                                \
        Bind.Method("FString ToString() const", [](_HandleType_ const& Self) -> FString                                \
        { return Self.ToString(); });                                                                                  \
        Bind.Method("FString Debug() const", [](_HandleType_ const& Self) -> FString                                   \
        { Self.DoFireEnsure(); return Self.ToString(); });                                                             \
                                                                                                                       \
        Bind.Method("bool opEquals(const " #_HandleType_ "& Other) const",                                             \
            [](const _HandleType_& A, const _HandleType_& B) -> bool { return A == B; });                              \
        Bind.Method("bool opEquals(const FCk_Handle& Other) const",                                                    \
            [](const _HandleType_& A, const FCk_Handle& B) -> bool { return A == B; });                                \
                                                                                                                       \
        FCkAngelScript_HandleRegistry::RegisterStaticHandle(                                                           \
            TEXT(#_HandleType_),                                                                                       \
            ExtractHandleShortName(TEXT(#_HandleType_)),                                                               \
            [](const FCk_Handle& H) -> bool { return Has(H); },                                                        \
            [](const FCk_Handle& H) -> FCk_Handle { return Cast(H); },                                                 \
            [](const FCk_Handle& H) -> FCk_Handle { return CastChecked(H); }                                           \
        );                                                                                                             \
    }                                                                                                                  \
                                                                                                                       \
private:                                                                                                               \
    static inline bool AngelScriptRegistered_##_HandleType_ = []() -> bool                                             \
    {                                                                                                                  \
        FCkAngelScript_HandleRegistry::RegisterDeferredCallback(                                                       \
            &RegisterAngelScript_##_HandleType_##_Complete);                                                           \
        return true;                                                                                                   \
    }();

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_ANGELSCRIPT_CK