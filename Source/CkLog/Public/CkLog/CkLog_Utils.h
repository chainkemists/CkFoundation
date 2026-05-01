#pragma once

#include "CkLog/CkLog_Category.h"

#include <CoreMinimal.h>
#include <functional>
#include <Kismet/BlueprintFunctionLibrary.h>

#include <Framework/Notifications/NotificationManager.h>
#include <Widgets/Notifications/SNotificationList.h>
#include <Runtime/Launch/Resources/Version.h>

#if !UE_BUILD_SHIPPING
#include <Logging/MessageLog.h>
#endif

#if WITH_EDITOR
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 2
#include <MessageLog/Public/IMessageLogListing.h>
#include <MessageLog/Public/MessageLogModule.h>
#else
#include "Developer/MessageLog/Public/MessageLogModule.h"
#endif
#endif

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptBinds.h>
// Direct ctti include is intentional. CkLog cannot depend on CkCore's wrapper
// (CkCore already depends on CkLog), so we include the underlying header
// directly for the one purpose of auto-deriving the AS namespace inside
// CK_REGISTER_LOG_FUNCTIONS below.
#include "CkThirdParty/ctti/include/ctti/nameof.hpp"
#endif

#include "CkLog_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FCk_LogCategory;

namespace ck::log
{
    using InvokeLogMapType   = TMap<FName, TFunction<void(const FString&)>>;
    using IsLogActiveMapType = TMap<FName, TFunction<bool()>>;

    CKLOG_API auto Get_LogMap()         -> InvokeLogMapType&;
    CKLOG_API auto Get_DisplayMap()     -> InvokeLogMapType&;
    CKLOG_API auto Get_WarningMap()     -> InvokeLogMapType&;
    CKLOG_API auto Get_ErrorMap()       -> InvokeLogMapType&;
    CKLOG_API auto Get_FatalMap()       -> InvokeLogMapType&;
    CKLOG_API auto Get_VerboseMap()     -> InvokeLogMapType&;
    CKLOG_API auto Get_VeryVerboseMap() -> InvokeLogMapType&;

    CKLOG_API auto Get_LogMap_IsActive()         -> IsLogActiveMapType&;
    CKLOG_API auto Get_DisplayMap_IsActive()     -> IsLogActiveMapType&;
    CKLOG_API auto Get_WarningMap_IsActive()     -> IsLogActiveMapType&;
    CKLOG_API auto Get_ErrorMap_IsActive()       -> IsLogActiveMapType&;
    CKLOG_API auto Get_FatalMap_IsActive()       -> IsLogActiveMapType&;
    CKLOG_API auto Get_VerboseMap_IsActive()     -> IsLogActiveMapType&;
    CKLOG_API auto Get_VeryVerboseMap_IsActive() -> IsLogActiveMapType&;

    CKLOG_API auto Get_AllRegisteredCategories() -> TArray<FName>;

#if WITH_ANGELSCRIPT_CK
    // Strip the trailing "::SuffixName" from a fully-qualified type name to
    // recover its enclosing C++ namespace. Returns "" for global-scope types.
    //
    // Also strips any compiler-prefix junk left in by ctti (e.g. "struct ",
    // "class ", or partial off-by-one fragments like "truct " on some MSVC
    // versions) by discarding everything up to and including the last
    // whitespace — valid namespace paths contain none.
    inline auto Get_NamespaceFrom_QualifiedTypeName(const FString& InQualifiedTypeName) -> FString
    {
        const auto LastSep = InQualifiedTypeName.Find(
            TEXT("::"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
        if (LastSep == INDEX_NONE)
        { return FString{}; }
        auto NamespacePath = InQualifiedTypeName.Left(LastSep);
        const auto LastSpace = NamespacePath.Find(
            TEXT(" "), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
        if (LastSpace != INDEX_NONE)
        { NamespacePath.RightChopInline(LastSpace + 1); }
        return NamespacePath;
    }

    // Returns the C++ namespace surrounding type T as a string (e.g. "ck::crowd").
    // Used by CK_REGISTER_LOG_FUNCTIONS to auto-derive the AS namespace from a
    // sentinel type defined inside the user's namespace.
    template <typename T>
    auto Get_NamespaceOf() -> FString
    {
        constexpr auto Name = ctti::nameof<T>();
        const auto NameStr = FString{static_cast<int32>(Name.length()), Name.begin()};
        return Get_NamespaceFrom_QualifiedTypeName(NameStr);
    }
#endif
}

// --------------------------------------------------------------------------------------------------------------------

/*
 * Define module specific log functions using the following macro. Wrap the macro in a namespace to avoid symbol collisions.
 * A working example can be found in Ck_Log.h
 */

#define CK_DEFINE_LOG_FUNCTIONS(_LogCategory_)                                                                                   \
                                                                                                                                 \
template <typename T, typename ... TArgs>                                                                                        \
auto                                                                                                                             \
    Fatal(                                                                                                                       \
        const T&  InString,                                                                                                      \
        TArgs&&...InArgs)                                                                                                        \
    -> void                                                                                                                      \
{                                                                                                                                \
    UE_LOG(_LogCategory_, Fatal, TEXT("[PIE-ID %d] %s"), UE::GetPlayInEditorID() - 1, *ck::Format_UE(InString, InArgs...));      \
}                                                                                                                                \
                                                                                                                                 \
template <typename T, typename ... TArgs>                                                                                        \
auto                                                                                                                             \
    Error(                                                                                                                       \
        const T&  InString,                                                                                                      \
        TArgs&&...InArgs)                                                                                                        \
    -> void                                                                                                                      \
{                                                                                                                                \
    UE_LOG(_LogCategory_, Error, TEXT("[PIE-ID %d] %s"), UE::GetPlayInEditorID() - 1, *ck::Format_UE(InString, InArgs...));      \
}                                                                                                                                \
                                                                                                                                 \
template <typename T, typename ... TArgs>                                                                                        \
auto                                                                                                                             \
    Warning(                                                                                                                     \
        const T&  InString,                                                                                                      \
        TArgs&&...InArgs)                                                                                                        \
    -> void                                                                                                                      \
{                                                                                                                                \
    UE_LOG(_LogCategory_, Warning, TEXT("[PIE-ID %d] %s"), UE::GetPlayInEditorID() - 1, *ck::Format_UE(InString, InArgs...));    \
}                                                                                                                                \
                                                                                                                                 \
template <typename T, typename ... TArgs>                                                                                        \
auto                                                                                                                             \
    Display(                                                                                                                     \
        const T&  InString,                                                                                                      \
        TArgs&&...InArgs)                                                                                                        \
    -> void                                                                                                                      \
{                                                                                                                                \
    UE_LOG(_LogCategory_, Display, TEXT("[PIE-ID %d] %s"), UE::GetPlayInEditorID() - 1, *ck::Format_UE(InString, InArgs...));    \
}                                                                                                                                \
                                                                                                                                 \
template <typename T, typename ... TArgs>                                                                                        \
auto                                                                                                                             \
    Log(                                                                                                                         \
        const T&  InString,                                                                                                      \
        TArgs&&...InArgs)                                                                                                        \
    -> void                                                                                                                      \
{                                                                                                                                \
    UE_LOG(_LogCategory_, Log, TEXT("Trace: [PIE-ID %d] %s"), UE::GetPlayInEditorID() - 1, *ck::Format_UE(InString, InArgs...)); \
}                                                                                                                                \
                                                                                                                                 \
template <typename T, typename ... TArgs>                                                                                        \
auto                                                                                                                             \
    Verbose(                                                                                                                     \
        const T&  InString,                                                                                                      \
        TArgs&&...InArgs)                                                                                                        \
    -> void                                                                                                                      \
{                                                                                                                                \
    UE_LOG(_LogCategory_, Verbose, TEXT("[PIE-ID %d] %s"), UE::GetPlayInEditorID() - 1, *ck::Format_UE(InString, InArgs...));    \
}                                                                                                                                \
                                                                                                                                 \
template <typename T, typename ... TArgs>                                                                                        \
auto                                                                                                                             \
    VeryVerbose(                                                                                                                 \
        const T&  InString,                                                                                                      \
        TArgs&&...InArgs)                                                                                                        \
    -> void                                                                                                                      \
{                                                                                                                                \
    UE_LOG(_LogCategory_, VeryVerbose, TEXT("[PIE-ID %d] %s"), UE::GetPlayInEditorID() - 1, *ck::Format_UE(InString, InArgs...));\
}                                                                                                                                \
                                                                                                                                 \
template <typename T, typename ... TArgs>                                                                                        \
auto                                                                                                                             \
    FatalIf(                                                                                                                     \
        bool InExpression,                                                                                                       \
        const T&  InString,                                                                                                      \
        TArgs&&...InArgs)                                                                                                        \
    -> void                                                                                                                      \
{                                                                                                                                \
    if (InExpression)                                                                                                            \
    { Fatal(InString, InArgs...); }                                                                                              \
}                                                                                                                                \
                                                                                                                                 \
template <typename T, typename ... TArgs>                                                                                        \
auto                                                                                                                             \
    ErrorIf(                                                                                                                     \
        bool InExpression,                                                                                                       \
        const T&  InString,                                                                                                      \
        TArgs&&...InArgs)                                                                                                        \
    -> ECk_LogResults                                                                                                            \
{                                                                                                                                \
    if (InExpression)                                                                                                            \
    {                                                                                                                            \
        Error(InString, InArgs...);                                                                                              \
        return ECk_LogResults::Logged;                                                                                           \
    }                                                                                                                            \
    return ECk_LogResults::NotLogged;                                                                                            \
}                                                                                                                                \
                                                                                                                                 \
template <typename T, typename ... TArgs>                                                                                        \
auto                                                                                                                             \
    WarningIf(                                                                                                                   \
        bool InExpression,                                                                                                       \
        const T&  InString,                                                                                                      \
        TArgs&&...InArgs)                                                                                                        \
    -> ECk_LogResults                                                                                                            \
{                                                                                                                                \
    if (InExpression)                                                                                                            \
    {                                                                                                                            \
        Warning(InString, InArgs...);                                                                                            \
        return ECk_LogResults::Logged;                                                                                           \
    }                                                                                                                            \
    return ECk_LogResults::NotLogged;                                                                                            \
}                                                                                                                                \
                                                                                                                                 \
template <typename T, typename ... TArgs>                                                                                        \
auto                                                                                                                             \
    DisplayIf(                                                                                                                   \
        bool InExpression,                                                                                                       \
        const T&  InString,                                                                                                      \
        TArgs&&...InArgs)                                                                                                        \
    -> ECk_LogResults                                                                                                            \
{                                                                                                                                \
    if (InExpression)                                                                                                            \
    {                                                                                                                            \
        Display(InString, InArgs...);                                                                                            \
        return ECk_LogResults::Logged;                                                                                           \
    }                                                                                                                            \
    return ECk_LogResults::NotLogged;                                                                                            \
}                                                                                                                                \
                                                                                                                                 \
template <typename T, typename ... TArgs>                                                                                        \
auto                                                                                                                             \
    LogIf(                                                                                                                       \
        bool InExpression,                                                                                                       \
        const T&  InString,                                                                                                      \
        TArgs&&...InArgs)                                                                                                        \
    -> ECk_LogResults                                                                                                            \
{                                                                                                                                \
    if (InExpression)                                                                                                            \
    {                                                                                                                            \
        Log(InString, InArgs...);                                                                                                \
        return ECk_LogResults::Logged;                                                                                           \
    }                                                                                                                            \
    return ECk_LogResults::NotLogged;                                                                                            \
}                                                                                                                                \
                                                                                                                                 \
template <typename T, typename ... TArgs>                                                                                        \
auto                                                                                                                             \
    VerboseIf(                                                                                                                   \
        bool InExpression,                                                                                                       \
        const T&  InString,                                                                                                      \
        TArgs&&...InArgs)                                                                                                        \
    -> ECk_LogResults                                                                                                            \
{                                                                                                                                \
    if (InExpression)                                                                                                            \
    {                                                                                                                            \
        Verbose(InString, InArgs...);                                                                                            \
        return ECk_LogResults::Logged;                                                                                           \
    }                                                                                                                            \
    return ECk_LogResults::NotLogged;                                                                                            \
}                                                                                                                                \
                                                                                                                                 \
template <typename T, typename ... TArgs>                                                                                        \
auto                                                                                                                             \
    VeryVerboseIf(                                                                                                               \
        bool InExpression,                                                                                                       \
        const T&  InString,                                                                                                      \
        TArgs&&...InArgs)                                                                                                        \
    -> ECk_LogResults                                                                                                            \
{                                                                                                                                \
    if (InExpression)                                                                                                            \
    {                                                                                                                            \
        VeryVerbose(InString, InArgs...);                                                                                        \
        return ECk_LogResults::Logged;                                                                                           \
    }                                                                                                                            \
    return ECk_LogResults::NotLogged;                                                                                            \
}                                                                                                                                \
                                                                                                                                 \
inline auto                                                                                                                      \
    Get_Fatal_IsActive()                                                                                                         \
    -> bool                                                                                                                      \
{                                                                                                                                \
    return UE_LOG_ACTIVE(_LogCategory_, Fatal);                                                                                  \
}                                                                                                                                \
                                                                                                                                 \
inline auto                                                                                                                      \
    Get_Error_IsActive()                                                                                                         \
    -> bool                                                                                                                      \
{                                                                                                                                \
    return UE_LOG_ACTIVE(_LogCategory_, Error);                                                                                  \
}                                                                                                                                \
                                                                                                                                 \
inline auto                                                                                                                      \
    Get_Warning_IsActive()                                                                                                       \
    -> bool                                                                                                                      \
{                                                                                                                                \
    return UE_LOG_ACTIVE(_LogCategory_, Warning);                                                                                \
}                                                                                                                                \
                                                                                                                                 \
inline auto                                                                                                                      \
    Get_Display_IsActive()                                                                                                       \
    -> bool                                                                                                                      \
{                                                                                                                                \
    return UE_LOG_ACTIVE(_LogCategory_, Display);                                                                                \
}                                                                                                                                \
                                                                                                                                 \
inline auto                                                                                                                      \
    Get_Log_IsActive()                                                                                                           \
    -> bool                                                                                                                      \
{                                                                                                                                \
    return UE_LOG_ACTIVE(_LogCategory_, Log);                                                                                    \
}                                                                                                                                \
                                                                                                                                 \
inline auto                                                                                                                      \
    Get_Verbose_IsActive()                                                                                                       \
    -> bool                                                                                                                      \
{                                                                                                                                \
    return UE_LOG_ACTIVE(_LogCategory_, Verbose);                                                                                \
}                                                                                                                                \
                                                                                                                                 \
inline auto                                                                                                                      \
    Get_VeryVerbose_IsActive()                                                                                                   \
    -> bool                                                                                                                      \
{                                                                                                                                \
    return UE_LOG_ACTIVE(_LogCategory_, VeryVerbose);                                                                            \
}                                                                                                                                \
static constexpr auto LogCategory = #_LogCategory_

// --------------------------------------------------------------------------------------------------------------------

#define CK_REGISTER_LOG_FUNCTIONS(_LogCategory_)                                                                                        \
inline struct _ ##_LogCategory_## LogMapInjector                                                                                        \
{                                                                                                                                       \
    _ ##_LogCategory_## LogMapInjector()                                                                                                \
    {                                                                                                                                   \
        ck::log::Get_LogMap().Emplace(TEXT(#_LogCategory_),         [](const FString& InString) { Log(TEXT("{}"), InString); });        \
        ck::log::Get_DisplayMap().Emplace(TEXT(#_LogCategory_),     [](const FString& InString) { Display(TEXT("{}"), InString); });    \
        ck::log::Get_WarningMap().Emplace(TEXT(#_LogCategory_),     [](const FString& InString) { Warning(TEXT("{}"), InString); });    \
        ck::log::Get_ErrorMap().Emplace(TEXT(#_LogCategory_),       [](const FString& InString) { Error(TEXT("{}"), InString); });      \
        ck::log::Get_FatalMap().Emplace(TEXT(#_LogCategory_),       [](const FString& InString) { Fatal(TEXT("{}"), InString); });      \
        ck::log::Get_VerboseMap().Emplace(TEXT(#_LogCategory_),     [](const FString& InString) { Verbose(TEXT("{}"), InString); });    \
        ck::log::Get_VeryVerboseMap().Emplace(TEXT(#_LogCategory_), [](const FString& InString) { VeryVerbose(TEXT("{}"), InString); });\
                                                                                                                                        \
        ck::log::Get_LogMap_IsActive().Emplace(TEXT(#_LogCategory_),         []() { return Get_Log_IsActive(); });                      \
        ck::log::Get_DisplayMap_IsActive().Emplace(TEXT(#_LogCategory_),     []() { return Get_Display_IsActive(); });                  \
        ck::log::Get_WarningMap_IsActive().Emplace(TEXT(#_LogCategory_),     []() { return Get_Warning_IsActive(); });                  \
        ck::log::Get_ErrorMap_IsActive().Emplace(TEXT(#_LogCategory_),       []() { return Get_Error_IsActive(); });                    \
        ck::log::Get_FatalMap_IsActive().Emplace(TEXT(#_LogCategory_),       []() { return Get_Fatal_IsActive(); });                    \
        ck::log::Get_VerboseMap_IsActive().Emplace(TEXT(#_LogCategory_),     []() { return Get_Verbose_IsActive(); });                  \
        ck::log::Get_VeryVerboseMap_IsActive().Emplace(TEXT(#_LogCategory_), []() { return Get_VeryVerbose_IsActive(); });              \
    }                                                                                                                                   \
} _LogCategory_ ##_LogMapInjector;                                                                                                      \
CK__REGISTER_LOG_AS_BINDS(_LogCategory_)

// --------------------------------------------------------------------------------------------------------------------
// AngelScript log binding (always wired up by CK_REGISTER_LOG_FUNCTIONS — no opt-out).
//
// Binds the major log levels (Log/Display/Warning/Error/Verbose/VeryVerbose)
// as global functions in the C++ namespace surrounding the macro expansion.
// E.g. CK_REGISTER_LOG_FUNCTIONS(CkCrowd) inside `namespace ck::crowd {}`
// exposes ck::crowd::Log("msg") etc. to AngelScript.
//
// Skipped intentionally: Fatal (risky from script), *If variants (return enum
// — use AS `if` instead), Get_*_IsActive introspection.

#if WITH_ANGELSCRIPT_CK

// Helper: bind one log level. The "{}" placeholder in the lambda passes the
// already-formatted AS string straight through ck::Format_UE.
#define CK__AS_LOG_BIND_ONE(_Level_)                                                 \
    FAngelscriptBinds::BindGlobalFunction(                                           \
        "void " #_Level_ "(const FString& in InMsg)",                                \
        [](const FString& InMsg) { _Level_(TEXT("{}"), InMsg); })

// AS namespace is auto-derived: a sentinel struct is declared inside the
// surrounding C++ namespace, then ctti reads its fully-qualified name and we
// strip the sentinel suffix to recover the namespace. Zero per-module config.
#define CK__REGISTER_LOG_AS_BINDS(_LogCategory_)                                                       \
    struct CK__ASNsHelper_##_LogCategory_ {};                                                          \
    AS_FORCE_LINK const FAngelscriptBinds::FBind _LogCategory_##_AS_LogBind(                           \
        FAngelscriptBinds::EOrder::Late, []                                                            \
    {                                                                                                  \
        const auto NsString = ck::log::Get_NamespaceOf<CK__ASNsHelper_##_LogCategory_>();              \
        FAngelscriptBinds::FNamespace Ns(NsString);                                                    \
        CK__AS_LOG_BIND_ONE(Log);                                                                      \
        CK__AS_LOG_BIND_ONE(Display);                                                                  \
        CK__AS_LOG_BIND_ONE(Warning);                                                                  \
        CK__AS_LOG_BIND_ONE(Error);                                                                    \
        CK__AS_LOG_BIND_ONE(Verbose);                                                                  \
        CK__AS_LOG_BIND_ONE(VeryVerbose);                                                              \
    })

#else

#define CK__REGISTER_LOG_AS_BINDS(_LogCategory_)

#endif

// --------------------------------------------------------------------------------------------------------------------

#define CK_LOG_ERROR_IF_NOT(_Namespace_, _Expression_, _Format_, ...)\
    if (_Namespace_::ErrorIf(NOT (_Expression_), _Format_, ##__VA_ARGS__) == ECk_LogResults::Logged)

#if WITH_EDITOR
#define CK_LOG_ERROR_NOTIFY_IF_NOT(_Namespace_, _Expression_, _Format_, ...)                                                       \
    if ([&]() -> bool                                                                                                              \
    {                                                                                                                              \
        if (_Expression_)                                                                                                          \
        { return false; }                                                                                                          \
                                                                                                                                   \
        const auto& FormattedString = ck::Format_UE(_Format_, ##__VA_ARGS__);                                                      \
        const auto& FormattedText = FText::FromString(FormattedString);                                                            \
                                                                                                                                   \
        if (auto& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>(TEXT("MessageLog"));                     \
            MessageLogModule.IsRegisteredLogListing(_Namespace_::LogCategory) == false)                                            \
        {                                                                                                                          \
            FMessageLogInitializationOptions InitOptions;                                                                          \
            InitOptions.bShowFilters = true;                                                                                       \
                                                                                                                                   \
            MessageLogModule.RegisterLogListing(_Namespace_::LogCategory, FText::FromName(_Namespace_::LogCategory), InitOptions); \
        }                                                                                                                          \
                                                                                                                                   \
        auto EditorInfo = FMessageLog{_Namespace_::LogCategory};                                                                   \
        EditorInfo.Error(FormattedText);                                                                                           \
                                                                                                                                   \
        auto Info = FNotificationInfo{FormattedText};                                                                              \
        Info.bFireAndForget = true;                                                                                                \
        Info.bUseThrobber = false;                                                                                                 \
        Info.FadeOutDuration = 0.5f;                                                                                               \
        Info.ExpireDuration = 5.0f;                                                                                                \
                                                                                                                                   \
        if (const auto& Notification = FSlateNotificationManager::Get().AddNotification(Info))                                     \
        {                                                                                                                          \
            Notification->SetCompletionState(SNotificationItem::CS_Fail);                                                          \
        }                                                                                                                          \
        _Namespace_::Error(TEXT("{}"), FormattedString);                                                                           \
                                                                                                                                   \
        return true;                                                                                                               \
    }())
#else
#define CK_LOG_ERROR_NOTIFY_IF_NOT(_Namespace_, _Expression_, _Format_, ...)\
    CK_LOG_ERROR_IF_NOT(_Namespace_, _Expression_, _Format_, ##__VA_ARGS__)
#endif

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_LogResults : uint8
{
    Logged,
    NotLogged
};

// --------------------------------------------------------------------------------------------------------------------
UENUM(BlueprintType)
enum class ECk_LogVerbosity : uint8
{
    Log,
    Display,
    Verbose,
    VeryVerbose,
    Warning,
    Error,
    Fatal
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKLOG_API UCk_Utils_Log_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly,
              DisplayName = "[Ck] Log (Fatal)",
              Category = "Ck|Utils|Log")
    static void
    Log_Fatal(
        FText InMsg,
        FCk_LogCategory InLogCategory);

    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly,
              DisplayName = "[Ck] Log (Error)",
              Category = "Ck|Utils|Log")
    static void
    Log_Error(
        FText InMsg,
        FCk_LogCategory InLogCategory);

    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly,
              DisplayName = "[Ck] Log (Warning)",
              Category = "Ck|Utils|Log")
    static void
    Log_Warning(
        FText InMsg,
        FCk_LogCategory InLogCategory);

    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly,
              DisplayName = "[Ck] Log (Display)",
              Category = "Ck|Utils|Log")
    static void
    Log_Display(
        FText InMsg,
        FCk_LogCategory InLogCategory);

    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly,
              DisplayName = "[Ck] Log",
              Category = "Ck|Utils|Log")
    static void
    Log(
        FText InMsg,
        FCk_LogCategory InLogCategory);

    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly,
              DisplayName = "[Ck] Log (Verbose)",
              Category = "Ck|Utils|Log")
    static void
    Log_Verbose(
        FText InMsg,
        FCk_LogCategory InLogCategory);

    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly,
              DisplayName = "[Ck] Log (VeryVerbose)",
              Category = "Ck|Utils|Log")
    static void
    Log_VeryVerbose(
        FText InMsg,
        FCk_LogCategory InLogCategory);

public:
    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly,
              DisplayName = "[Ck] Log If (Fatal)",
              Category = "Ck|Utils|Log")
    static ECk_LogResults
    Log_Fatal_If(
        bool InExpression,
        FText InMsg,
        FCk_LogCategory InLogCategory);

    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly,
              DisplayName = "[Ck] Log If (Error)",
              Category = "Ck|Utils|Log")
    static ECk_LogResults
    Log_Error_If(
        bool InExpression,
        FText InMsg,
        FCk_LogCategory InLogCategory);

    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly,
              DisplayName = "[Ck] Log If (Warning)",
              Category = "Ck|Utils|Log")
    static ECk_LogResults
    Log_Warning_If(
        bool InExpression,
        FText InMsg,
        FCk_LogCategory InLogCategory);

    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly,
              DisplayName = "[Ck] Log If (Display)",
              Category = "Ck|Utils|Log")
    static ECk_LogResults
    Log_Display_If(
        bool InExpression,
        FText InMsg,
        FCk_LogCategory InLogCategory);

    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly,
              DisplayName = "[Ck] Log If",
              Category = "Ck|Utils|Log")
    static ECk_LogResults
    Log_If(
        bool InExpression,
        FText InMsg,
        FCk_LogCategory InLogCategory);

    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly,
              DisplayName = "[Ck] Log If (Verbose)",
              Category = "Ck|Utils|Log")
    static ECk_LogResults
    Log_Verbose_If(
        bool InExpression,
        FText InMsg,
        FCk_LogCategory InLogCategory);

    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly,
              DisplayName = "[Ck] Log If (VeryVerbose)",
              Category = "Ck|Utils|Log")
    static ECk_LogResults
    Log_VeryVerbose_If(
        bool InExpression,
        FText InMsg,
        FCk_LogCategory InLogCategory);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Log Active (Fatal)",
              Category = "Ck|Utils|Log")
    static bool
    Get_Fatal_IsActive(
        FCk_LogCategory InLogCategory);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Log Active (Error)",
              Category = "Ck|Utils|Log")
    static bool
    Get_Error_IsActive(
        FCk_LogCategory InLogCategory);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Log Active (Warning)",
              Category = "Ck|Utils|Log")
    static bool
    Get_Warning_IsActive(
        FCk_LogCategory InLogCategory);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Log Active (Display)",
              Category = "Ck|Utils|Log")
    static bool
    Get_Display_IsActive(
        FCk_LogCategory InLogCategory);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Log Active",
              Category = "Ck|Utils|Log")
    static bool
    Get_Log_IsActive(
        FCk_LogCategory InLogCategory);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Log Active (Verbose)",
              Category = "Ck|Utils|Log")
    static bool
    Get_Verbose_IsActive(
        FCk_LogCategory InLogCategory);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Log Active (VeryVerbose)",
              Category = "Ck|Utils|Log")
    static bool
    Get_VeryVerbose_IsActive(
        FCk_LogCategory InLogCategory);

public:
    static auto
    Get_IsLogActive_ForVerbosity(
        const FCk_LogCategory& InLogCategory,
        ECk_LogVerbosity InVerbosity) -> bool;

private:
    static auto
    DoInvokeLog(
        const TMap<FName, TFunction<void(const FString&)>>& InMap,
        FName InLogger,
        FText InMsg) -> void;

    static auto
    DoGet_IsLogActive(
        const TMap<FName, TFunction<bool()>>& InMap,
        FName InLogger) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
