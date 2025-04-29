#pragma once

#include "CkCore/Build/CkBuild_Macros.h"
#include "CkCore/Macros/CkMacros.h"

// the following includes are needed if using the macros defined in this file
#include "CkCore/CkCoreLog.h"
#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/MessageDialog/CkMessageDialog_Utils.h"
#include "CkCore/Settings/CkCore_Settings.h"

#include <CoreMinimal.h>
#include <Windows/WindowsPlatformApplicationMisc.h> // required for clipboard copy

#include "CkEnsure.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Ensure_Entry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ensure_Entry);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName _FileName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 _LineNumber = 0;

public:
    CK_PROPERTY_GET(_FileName);
    CK_PROPERTY_GET(_LineNumber);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Ensure_Entry, _FileName, _LineNumber);
};

auto CKCORE_API GetTypeHash(const FCk_Ensure_Entry& InA) -> uint8;



// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Ensure_IgnoredEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ensure_IgnoredEntry);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName _FileName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 _LineNumber = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText _Message;

public:
    CK_PROPERTY_GET(_FileName);
    CK_PROPERTY_GET(_LineNumber);
    CK_PROPERTY(_Message);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Ensure_IgnoredEntry, _FileName, _LineNumber);
};

auto CKCORE_API GetTypeHash(const FCk_Ensure_IgnoredEntry& InA) -> uint8;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Payload_OnEnsureIgnored
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Payload_OnEnsureIgnored);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FCk_Ensure_IgnoredEntry _IgnoredEnsure;

public:
    CK_PROPERTY_GET(_IgnoredEnsure);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Payload_OnEnsureIgnored, _IgnoredEnsure);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_OnEnsureIgnored,
    const FCk_Payload_OnEnsureIgnored&, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_OnEnsureIgnored_MC,
    const FCk_Payload_OnEnsureIgnored&, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_OnEnsureCountChanged,
    int32, InNewTotalCount,
    int32, InNewUniqueCount);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_OnEnsureCountChanged_MC,
    int32, InNewTotalCount,
    int32, InNewUniqueCount);

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCORE_API UCk_Utils_Ensure_UE
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ensure",
              DisplayName = "[Ck] Ensure",
              meta     = (DevelopmentOnly, ExpandEnumAsExecs = "OutHitStatus", DefaultToSelf = "InContext", HidePin = "InContext"))
    static void
    EnsureMsgf(
        bool InExpression,
        FText InMsg,
        ECk_ValidInvalid& OutHitStatus,
        const UObject* InContext = nullptr);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ensure",
              DisplayName = "[Ck] Ensure IsValid",
              meta     = (DevelopmentOnly, ExpandEnumAsExecs = "OutHitStatus", DefaultToSelf = "InContext", HidePin = "InContext"))
    static void
    EnsureMsgf_IsValid(
        UObject* InObject,
        FText InMsg,
        ECk_ValidInvalid& OutHitStatus,
        const UObject* InContext = nullptr);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Trigger Ensure",
              Category = "Ck|Utils|Ensure",
              meta     = (DevelopmentOnly, DefaultToSelf = "InContext", HidePin = "InContext"))
    static void
    TriggerEnsure(
        FText InMsg,
        const UObject* InContext = nullptr);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get All Ignored Ensures",
              Category = "Ck|Utils|Ensure")
    static TArray<FCk_Ensure_IgnoredEntry>
    Get_AllIgnoredEnsures();

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Ensure Count",
              Category = "Ck|Utils|Ensure")
    static int32
    Get_EnsureCount();

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Unique Ensure Count",
              Category = "Ck|Utils|Ensure")
    static int32
    Get_UniqueEnsureCount();

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Request Clear All Ignored Ensures",
              Category = "Ck|Utils|Ensure")
    static void
    Request_ClearAllIgnoredEnsures();

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Bind To OnEnsureIgnored",
              Category = "Ck|Utils|Ensure")
    static void
    BindTo_OnEnsureIgnored(
        const FCk_Delegate_OnEnsureIgnored& InDelegate);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Unbind From OnEnsureIgnored",
              Category = "Ck|Utils|Ensure")
    static void
    UnbindFrom_OnEnsureIgnored(
        const FCk_Delegate_OnEnsureIgnored& InDelegate);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Bind To OnEnsureCountChanged",
              Category = "Ck|Utils|Ensure")
    static void
    BindTo_OnEnsureCountChanged(
        const FCk_Delegate_OnEnsureCountChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Unbind From OnEnsureCountChanged",
              Category = "Ck|Utils|Ensure")
    static void
    UnbindFrom_OnEnsureCountChanged(
        const FCk_Delegate_OnEnsureCountChanged& InDelegate);

public:
    static auto
    Get_IsEnsureIgnored(
        FName InFile,
        int32 InLine) -> bool;

    static auto
    Get_IsEnsureIgnored_WithCallstack(
        const FString& InCallstack) -> bool;

    static auto
    Request_IncrementEnsureCountAtFileAndLine(
        FName InFile,
        int32 InLine) -> void;

    static auto
    Request_IncrementEnsureCountWithCallstack(
        const FString& InCallstack) -> void;

    static auto
    Request_IgnoreEnsureAtFileAndLineWithMessage(
        FName InFile,
        const FText& InMessage,
        int32 InLine) -> void;

    static auto
    Request_IgnoreEnsureAtFileAndLine(
        FName InFile,
        int32 InLine) -> void;

    static auto
    Request_IgnoreEnsure_WithCallstack(
        const FString& InCallstack) -> void;

    static auto
    Request_IgnoreAllEnsures() -> void;
};

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
// ReSharper disable once CppInconsistentNaming
#define _DETAILS_CK_ENSURE_LOG_OR_PUSHMESSAGE(_Category_, _Msg_, _ContextObject_)                                                          \
    UCk_Utils_EditorOnly_UE::Request_PushNewEditorMessage                                                                                  \
    (                                                                                                                                      \
        FCk_Utils_EditorOnly_PushNewEditorMessage_Params                                                                                   \
        {                                                                                                                                  \
            TEXT(_Category_),                                                                                                              \
            FCk_MessageSegments                                                                                                            \
            {                                                                                                                              \
                {                                                                                                                          \
                    FCk_TokenizedMessage{_Msg_}.Set_TargetObject(_ContextObject_)                                                          \
                }                                                                                                                          \
            }                                                                                                                              \
        }                                                                                                                                  \
        .Set_MessageSeverity(ECk_EditorMessage_Severity::Error)                                                                            \
        .Set_ToastNotificationDisplayPolicy(ECk_EditorMessage_ToastNotification_DisplayPolicy::DoNotDisplay)                               \
        .Set_MessageLogDisplayPolicy(ECk_EditorMessage_MessageLog_DisplayPolicy::DoNotFocus)                                               \
    );                                                                                                                                     \
    if (UCk_Utils_Core_UserSettings_UE::Get_EnsureDisplayPolicy() == ECk_EnsureDisplay_Policy::LogOnly)                                    \
    { return false; }
#else
// ReSharper disable once CppInconsistentNaming
#define _DETAILS_CK_ENSURE_LOG_OR_PUSHMESSAGE(_Category_, _Msg_, _ContextObject_)                                                          \
    if (UCk_Utils_Core_UserSettings_UE::Get_EnsureDisplayPolicy() == ECk_EnsureDisplay_Policy::LogOnly)                                    \
    {                                                                                                                                      \
        UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLineWithMessage(__FILE__, FText::FromString(_Msg_), __LINE__);                   \
        UE_LOG(CkCore, Error, TEXT("%s"), *_Msg_);                                                                                         \
        return false;                                                                                                                      \
    }
#endif

#define CK_ENSURE(InExpression, InString, ...)                                                                                             \
[&]() -> bool                                                                                                                              \
{                                                                                                                                          \
    const auto ExpressionResult = InExpression;                                                                                            \
                                                                                                                                           \
    if (LIKELY(ExpressionResult))                                                                                                          \
    { return true; }                                                                                                                       \
                                                                                                                                           \
    UCk_Utils_Ensure_UE::Request_IncrementEnsureCountAtFileAndLine(__FILE__, __LINE__);                                                    \
                                                                                                                                           \
    if (UCk_Utils_Ensure_UE::Get_IsEnsureIgnored(__FILE__, __LINE__))                                                                      \
    { return false; }                                                                                                                      \
                                                                                                                                           \
    const auto IsMessageOnly = UCk_Utils_Core_UserSettings_UE::Get_EnsureDetailsPolicy() == ECk_EnsureDetails_Policy::MessageOnly;         \
                                                                                                                                           \
    const auto& Message = ck::Format_UE(InString, ##__VA_ARGS__);                                                                          \
    const auto& Title = ck::Format_UE(TEXT("Frame#[{}] PIE-ID[{}]"), GFrameCounter, UCk_Utils_EditorOnly_UE::Get_DebugStringForWorld());   \
    const auto& StackTraceWith2Skips = IsMessageOnly ?                                                                                     \
        TEXT("[StackTrace DISABLED]") :                                                                                                    \
        UCk_Utils_Debug_StackTrace_UE::Get_StackTrace(2);                                                                                  \
    const auto& BpStackTrace = IsMessageOnly ?                                                                                             \
        TEXT("[BP StackTrace DISABLED]") :                                                                                                 \
        UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Blueprint(ck::type_traits::AsString{});                                              \
    const auto& MessagePlusBpCallStack = ck::Format_UE(                                                                                    \
        TEXT("[{}] {}\n{}\n\n == BP CallStack ==\n{}"),                                                                                    \
        GPlayInEditorID - 1 < 0 ? TEXT("Server") : TEXT("Client"),                                                                         \
        TEXT(#InExpression),                                                                                                               \
        Message,                                                                                                                           \
        BpStackTrace);                                                                                                                     \
                                                                                                                                           \
    const auto& MessagePlusBpCallStackStr = FText::FromString(MessagePlusBpCallStack);                                                     \
                                                                                                                                           \
    if (UCk_Utils_Core_UserSettings_UE::Get_EnsureDisplayPolicy() == ECk_EnsureDisplay_Policy::StreamerMode)                               \
    {                                                                                                                                      \
        ck::core::Error(TEXT("{}"), MessagePlusBpCallStack);                                                                               \
        UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLineWithMessage(__FILE__, MessagePlusBpCallStackStr, __LINE__);                  \
        return false;                                                                                                                      \
    }                                                                                                                                      \
                                                                                                                                           \
    _DETAILS_CK_ENSURE_LOG_OR_PUSHMESSAGE("CkEnsures", MessagePlusBpCallStack, nullptr);                                                   \
                                                                                                                                           \
    if (UCk_Utils_Core_UserSettings_UE::Get_EnsureDisplayPolicy() == ECk_EnsureDisplay_Policy::MessageLog)                                 \
    {                                                                                                                                      \
        UCk_Utils_Debug_StackTrace_UE::Try_BreakInScript(nullptr, MessagePlusBpCallStackStr);                                              \
        UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLineWithMessage(__FILE__, MessagePlusBpCallStackStr, __LINE__);                  \
        return false;                                                                                                                      \
    }                                                                                                                                      \
                                                                                                                                           \
    const auto& CallstackPlusMessage = ck::Format_UE(                                                                                      \
        TEXT("{}\nExpression: {}\nMessage: {}\n\n == BP CallStack ==\n{}\n == CallStack ==\n{}\n"),                                        \
        Title,                                                                                                                             \
        TEXT(#InExpression),                                                                                                               \
        Message,                                                                                                                           \
        BpStackTrace,                                                                                                                      \
        StackTraceWith2Skips);                                                                                                             \
    const auto& DialogMessage = FText::FromString(CallstackPlusMessage);                                                                   \
                                                                                                                                           \
    auto ReturnResult = false;                                                                                                             \
    using DialogButton = UCk_Utils_MessageDialog_UE::DialogButton;                                                                         \
    auto Buttons = TArray<DialogButton>{};                                                                                                 \
                                                                                                                                           \
    Buttons.Add(DialogButton{FText::FromString(TEXT("Ignore Once")), FSimpleDelegate::CreateLambda([&]()                                   \
    {                                                                                                                                      \
        ReturnResult = false;                                                                                                              \
    })}.Set_Color(FLinearColor{0.22f, 0.22f, 0.22f, 1.0f}));                                                                               \
                                                                                                                                           \
    Buttons.Add(DialogButton{FText::FromString(TEXT("Ignore All")), FSimpleDelegate::CreateLambda([&]()                                    \
    {                                                                                                                                      \
        UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLineWithMessage(__FILE__, DialogMessage, __LINE__);                              \
        ReturnResult = false;                                                                                                              \
    })}.Set_Color(FLinearColor{1.0f, 0.62f, 0.27f, 1.0f}).Set_IsPrimary(true).Set_ShouldFocus(true));                                      \
                                                                                                                                           \
                                                                                                                                           \
    Buttons.Add(DialogButton{FText::FromString(TEXT("Break")), {}}                                                                         \
    .Set_Color(FLinearColor{0.22f, 0.22f, 0.22f, 1.0f})                                                                                    \
    .Set_EnableDisable(StackTraceWith2Skips.IsEmpty() ? ECk_EnableDisable::Disable : ECk_EnableDisable::Enable));                          \
                                                                                                                                           \
    if (GIsEditor)                                                                                                                         \
    {                                                                                                                                      \
    Buttons.Add(DialogButton{FText::FromString(TEXT("Break in BP")), FSimpleDelegate::CreateLambda([&]()                                   \
    {                                                                                                                                      \
        UCk_Utils_Debug_StackTrace_UE::Try_BreakInScript(nullptr);                                                                         \
    })}.Set_Color(FLinearColor{0.34f, 0.34f, 0.59f, 1.0f})                                                                                 \
    .Set_EnableDisable(BpStackTrace.IsEmpty() ? ECk_EnableDisable::Disable : ECk_EnableDisable::Enable));                                  \
    }                                                                                                                                      \
                                                                                                                                           \
    if (GIsEditor)                                                                                                                         \
    {                                                                                                                                      \
        Buttons.Add(DialogButton{FText::FromString(TEXT("Abort PIE")), FSimpleDelegate::CreateLambda([&]()                                 \
        {                                                                                                                                  \
            UCk_Utils_Ensure_UE::Request_IgnoreAllEnsures();                                                                               \
            UCk_Utils_EditorOnly_UE::Request_AbortPIE();                                                                                   \
        })}.Set_Color(FLinearColor{1.0f, 0.1f, 0.1f, 1.0f}));                                                                              \
    }                                                                                                                                      \
                                                                                                                                           \
    auto ButtonIndex = UCk_Utils_MessageDialog_UE::CustomDialog(DialogMessage, FText::FromString(Title), Buttons);                         \
    if (ButtonIndex == 2)                                                                                                                  \
    {                                                                                                                                      \
        ReturnResult = ensureAlwaysMsgf(false, TEXT("[DEBUG BREAK HIT]"));                                                                 \
        if (NOT BpStackTrace.IsEmpty())                                                                                                    \
        {                                                                                                                                  \
            UCk_Utils_Debug_StackTrace_UE::Try_BreakInScript(nullptr);                                                                     \
        }                                                                                                                                  \
    }                                                                                                                                      \
    return ReturnResult;                                                                                                                   \
}()

#if CK_BYPASS_ENSURES
#define CK_ENSURE_IF_NOT(InExpression, InFormat, ...)\
if constexpr(false)
#elif CK_BYPASS_ENSURE_DEBUGGING
#define CK_ENSURE_IF_NOT(InExpression, InFormat, ...)\
if (NOT InExpression)
#else
#define CK_ENSURE_IF_NOT(InExpression, InFormat, ...)\
if (NOT CK_ENSURE(InExpression, InFormat, ##__VA_ARGS__))
#endif

#define CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(InWorldContextObject)\
if(\
NOT [InWorldContextObject]()\
{\
    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject), TEXT("Invalid World Context object used to validate UWorld"))\
    { return false; }\
\
    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject->GetWorld()), TEXT("Invalid UWorld"))\
    { return false; }\
\
    return true;\
}())

#define CK_TRIGGER_ENSURE(InString, ...)\
CK_ENSURE(false, InString, ##__VA_ARGS__)

// Technically, the same as CK_ENSURE(...), but semantically it's different i.e. we WANT the ensure to be triggered by
// an expression that is not really part of the ensure itself
#define CK_TRIGGER_ENSURE_IF(InExpression, InString, ...)\
if(InExpression) { CK_TRIGGER_ENSURE(InString, ##__VA_ARGS__); }

#define CK_INVALID_ENUM(InEnsure)\
CK_TRIGGER_ENSURE(TEXT("Encountered an invalid value for Enum [{}]"), InEnsure)

// --------------------------------------------------------------------------------------------------------------------