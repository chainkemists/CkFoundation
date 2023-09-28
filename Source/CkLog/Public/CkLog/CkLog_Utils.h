#pragma once

#include "CkCore/Format/CkFormat.h"

#include <CoreMinimal.h>
#include <Kismet/BlueprintFunctionLibrary.h>
#include <functional>

#include "CkLog_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::log
{
    using LoggingMapType = TMap<FName, TFunction<void(const FString&)>>;

    CKLOG_API auto Get_LogMap()         -> LoggingMapType&;
    CKLOG_API auto Get_DisplayMap()     -> LoggingMapType&;
    CKLOG_API auto Get_WarningMap()     -> LoggingMapType&;
    CKLOG_API auto Get_ErrorMap()       -> LoggingMapType&;
    CKLOG_API auto Get_FatalMap()       -> LoggingMapType&;
    CKLOG_API auto Get_VerboseMap()     -> LoggingMapType&;
    CKLOG_API auto Get_VeryVerboseMap() -> LoggingMapType&;
}

// --------------------------------------------------------------------------------------------------------------------

/*
 * Define module specific log functions using the following macro. Wrap the macro in a namespace to avoid symbol collisions.
 * A working example can be found in Ck_Log.h
 */

#define CK_DEFINE_LOG_FUNCTIONS(_LogCategory_)                                                     \
                                                                                                   \
template <typename T, typename ... TArgs>                                                          \
auto Fatal(const T&  InString, TArgs&&...InArgs) -> void {                                         \
    UE_LOG(_LogCategory_, Fatal, TEXT("%s"), *ck::Format_UE(InString, InArgs...));                 \
}                                                                                                  \
                                                                                                   \
template <typename T, typename ... TArgs>                                                          \
auto                                                                                               \
    Error(const T&  InString, TArgs&&...InArgs)                                                    \
    -> void                                                                                        \
{                                                                                                  \
    UE_LOG(_LogCategory_, Error, TEXT("%s"), *ck::Format_UE(InString, InArgs...));                 \
}                                                                                                  \
                                                                                                   \
template <typename T, typename ... TArgs>                                                          \
auto                                                                                               \
    Warning(const T&  InString, TArgs&&...InArgs)                                                  \
    -> void                                                                                        \
{                                                                                                  \
    UE_LOG(_LogCategory_, Warning, TEXT("%s"), *ck::Format_UE(InString, InArgs...));               \
}                                                                                                  \
                                                                                                   \
template <typename T, typename ... TArgs>                                                          \
auto                                                                                               \
    Display(const T&  InString, TArgs&&...InArgs)                                                  \
    -> void                                                                                        \
{                                                                                                  \
    UE_LOG(_LogCategory_, Display, TEXT("%s"), *ck::Format_UE(InString, InArgs...));               \
}                                                                                                  \
                                                                                                   \
template <typename T, typename ... TArgs>                                                          \
auto                                                                                               \
    Log(const T&  InString, TArgs&&...InArgs)                                                      \
    -> void                                                                                        \
{                                                                                                  \
    UE_LOG(_LogCategory_, Log, TEXT("Trace: %s"), *ck::Format_UE(InString, InArgs...));            \
}                                                                                                  \
                                                                                                   \
template <typename T, typename ... TArgs>                                                          \
auto                                                                                               \
    Verbose(const T&  InString,                                                                    \
            TArgs&&...InArgs)                                                                      \
    -> void                                                                                        \
{                                                                                                  \
    UE_LOG(_LogCategory_, Verbose, TEXT("%s"), *ck::Format_UE(InString, InArgs...));               \
}                                                                                                  \
                                                                                                   \
template <typename T, typename ... TArgs>                                                          \
auto                                                                                               \
    VeryVerbose(const T&  InString,                                                                \
                TArgs&&...InArgs)                                                                  \
    -> void                                                                                        \
{                                                                                                  \
    UE_LOG(_LogCategory_, VeryVerbose, TEXT("%s"), *ck::Format_UE(InString, InArgs...));           \
}                                                                                                  \
                                                                                                                          \
template <typename T, typename ... TArgs>                                                                                 \
auto                                                                                                                      \
    FatalIf(bool      InExpression,                                                                                       \
          const T&  InString,                                                                                             \
          TArgs&&...InArgs)                                                                                               \
    -> void                                                                                                               \
{                                                                                                                         \
    if (InExpression)                                                                                                     \
    { Fatal(InString, InArgs...); }                                                                                       \
}                                                                                                                         \
                                                                                                                          \
template <typename T, typename ... TArgs>                                                                                 \
auto                                                                                                                      \
    ErrorIf(bool      InExpression,                                                                                       \
          const T&  InString,                                                                                             \
          TArgs&&...InArgs)                                                                                               \
    -> void                                                                                                               \
{                                                                                                                         \
    if (InExpression)                                                                                                     \
    { Error(InString, InArgs...); }                                                                                       \
}                                                                                                                         \
                                                                                                                          \
template <typename T, typename ... TArgs>                                                                                 \
auto                                                                                                                      \
    WarningIf(bool      InExpression,                                                                                     \
            const T&  InString,                                                                                           \
            TArgs&&...InArgs)                                                                                             \
    -> void                                                                                                               \
{                                                                                                                         \
    if (InExpression)                                                                                                     \
    { Warning(InString, InArgs...); }                                                                                     \
}                                                                                                                         \
                                                                                                                          \
template <typename T, typename ... TArgs>                                                                                 \
auto                                                                                                                      \
    DisplayIf(bool      InExpression,                                                                                     \
            const T&  InString,                                                                                           \
            TArgs&&...InArgs)                                                                                             \
    -> void                                                                                                               \
{                                                                                                                         \
    if (InExpression)                                                                                                     \
    { Display(InString, InArgs...); }                                                                                     \
}                                                                                                                         \
                                                                                                                          \
template <typename T, typename ... TArgs>                                                                                 \
auto                                                                                                                      \
    LogIf(bool      InExpression,                                                                                         \
        const T&  InString,                                                                                               \
        TArgs&&...InArgs)                                                                                                 \
    -> void                                                                                                               \
{                                                                                                                         \
    if (InExpression)                                                                                                     \
    { Log(InString, InArgs...); }                                                                                         \
}                                                                                                                         \
                                                                                                                          \
template <typename T, typename ... TArgs>                                                                                 \
auto                                                                                                                      \
    VerboseIf(bool      InExpression,                                                                                     \
            const T&  InString,                                                                                           \
            TArgs&&...InArgs)                                                                                             \
    -> void                                                                                                               \
{                                                                                                                         \
    if (InExpression)                                                                                                     \
    { Verbose(InString, InArgs...); }                                                                                     \
}                                                                                                                         \
                                                                                                                          \
template <typename T, typename ... TArgs>                                                                                 \
auto                                                                                                                      \
    VeryVerboseIf(bool      InExpression,                                                                                 \
                const T&  InString,                                                                                       \
                TArgs&&...InArgs)                                                                                         \
    -> void                                                                                                               \
{                                                                                                                         \
    if (InExpression)                                                                                                     \
    { VeryVerbose(InString, InArgs...); }                                                                                 \
}

// --------------------------------------------------------------------------------------------------------------------

#define CK_REGISTER_LOG_FUNCTIONS(_LogCategory_)                                                                         \
inline struct _ ##_LogCategory_## LogMapInjector                                                                         \
{                                                                                                                        \
    _ ##_LogCategory_## LogMapInjector()                                                                                 \
    {                                                                                                                                   \
        ck::log::Get_LogMap().Emplace(TEXT(#_LogCategory_), [](const FString& InString) { Log(TEXT("{}"), InString); });                \
        ck::log::Get_DisplayMap().Emplace(TEXT(#_LogCategory_), [](const FString& InString) { Display(TEXT("{}"), InString); });        \
        ck::log::Get_WarningMap().Emplace(TEXT(#_LogCategory_), [](const FString& InString) { Warning(TEXT("{}"), InString); });        \
        ck::log::Get_ErrorMap().Emplace(TEXT(#_LogCategory_), [](const FString& InString) { Error(TEXT("{}"), InString); });            \
        ck::log::Get_FatalMap().Emplace(TEXT(#_LogCategory_), [](const FString& InString) { Fatal(TEXT("{}"), InString); });            \
        ck::log::Get_VerboseMap().Emplace(TEXT(#_LogCategory_), [](const FString& InString) { Verbose(TEXT("{}"), InString); });        \
        ck::log::Get_VeryVerboseMap().Emplace(TEXT(#_LogCategory_), [](const FString& InString) { VeryVerbose(TEXT("{}"), InString); });\
    }                                                                                                                                   \
} ##_LogCategory_## _LogMapInjector

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_LogResults : uint8
{
    Logged,
    NotLogged
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKLOG_API UCk_Log_Utils_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Log|Utils")
    static void
    Log_Fatal(FText InMsg, FName InLogger = NAME_None);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Log|Utils")
    static void
    Log_Error(FText InMsg, FName InLogger = NAME_None);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Log|Utils")
    static void
    Log_Warning(FText InMsg, FName InLogger = NAME_None);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Log|Utils")
    static void
    Log_Display(FText InMsg, FName InLogger = NAME_None);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Log|Utils")
    static void
    Log(FText InMsg, FName InLogger = NAME_None);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Log|Utils")
    static void
    Log_Verbose(FText InMsg, FName InLogger = NAME_None);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Log|Utils")
    static void
    Log_VeryVerbose(FText InMsg, FName InLogger = NAME_None);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Log|Utils")
    static ECk_LogResults
    Log_Fatal_If(bool InExpression, FText InMsg, FName InLogger = NAME_None);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Log|Utils")
    static ECk_LogResults
    Log_Error_If(bool InExpression, FText InMsg, FName InLogger = NAME_None);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Log|Utils")
    static ECk_LogResults
    Log_Warning_If(bool InExpression, FText InMsg, FName InLogger = NAME_None);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Log|Utils")
    static ECk_LogResults
    Log_Display_If(bool InExpression, FText InMsg, FName InLogger = NAME_None);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Log|Utils")
    static ECk_LogResults
    Log_If(bool InExpression, FText InMsg, FName InLogger = NAME_None);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Log|Utils")
    static ECk_LogResults
    Log_Verbose_If(bool InExpression, FText InMsg, FName InLogger = NAME_None);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Log|Utils")
    static ECk_LogResults
    Log_VeryVerbose_If(bool InExpression, FText InMsg, FName InLogger = NAME_None);

private:
    static auto
    DoInvokeLog(TMap<FName,
        TFunction<void(const FString&)>>& InMap,
        FName InLogger, FText InMsg) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
