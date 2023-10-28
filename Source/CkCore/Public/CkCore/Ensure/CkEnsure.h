#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Build/CkBuild_Macros.h"

// the following includes are needed if using the macros defined in this file
#include "CkCore/Format/CkFormat.h"
#include "CkCore/MessageDialog/CkMessageDialog.h"

#include <CoreMinimal.h>

#include "CkEnsure.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Ensure_IgnoredEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ensure_IgnoredEntry);

public:
    FCk_Ensure_IgnoredEntry() = default;
    FCk_Ensure_IgnoredEntry(FName InName, int32 InLineNumber, const FText& InMessage = FText::GetEmpty());

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
    CK_PROPERTY_GET(_Message);
};

auto CKCORE_API GetTypeHash(const FCk_Ensure_IgnoredEntry& InA) -> uint8;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Ensure_OnEnsureIgnored_Payload
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ensure_OnEnsureIgnored_Payload);

public:
    FCk_Ensure_OnEnsureIgnored_Payload() = default;
    explicit FCk_Ensure_OnEnsureIgnored_Payload(const FCk_Ensure_IgnoredEntry& InIgnoredEnsure);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FCk_Ensure_IgnoredEntry _IgnoredEnsure;

public:
    CK_PROPERTY_GET(_IgnoredEnsure);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam
(
    FCk_Ensure_OnEnsureIgnored_Delegate_MC,
    const FCk_Ensure_OnEnsureIgnored_Payload&,
    InPayload
);

DECLARE_DYNAMIC_DELEGATE_OneParam
(
    FCk_Ensure_OnEnsureIgnored_Delegate,
    const FCk_Ensure_OnEnsureIgnored_Payload&,
    InPayload
);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ensure_HitStatus : uint8
{
    Hit,
    NotHit
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCORE_API UCk_Utils_Ensure_UE
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Core|Ensure",
              meta     = (ExpandEnumAsExecs = "OutHitStatus", DefaultToSelf = "InContext", HidePin = "InContext"))
    static void
    EnsureMsgf(bool InExpression,
        FText InMsg,
        ECk_Ensure_HitStatus& OutHitStatus,
        const UObject* InContext = nullptr);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ensure")
    static TArray<FCk_Ensure_IgnoredEntry>
    Get_AllIgnoredEnsures();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ensure")
    static void
    Request_ClearAllIgnoredEnsures();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ensure")
    static void
    Bind_To_OnEnsureIgnored(const FCk_Ensure_OnEnsureIgnored_Delegate& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ensure")
    static void
    Unbind_From_OnEnsureIgnored(const FCk_Ensure_OnEnsureIgnored_Delegate& InDelegate);

public:
    static auto Get_IsEnsureIgnored(FName InFile, int32 InLine) -> bool;
    static auto Get_IsEnsureIgnored_WithCallstack(const FString& InCallstack) -> bool;

    static auto Request_IgnoreEnsureAtFileAndLineWithMessage(FName InFile, const FText& InMessage, int32 InLine) -> void;
    static auto Request_IgnoreEnsureAtFileAndLine(FName InFile, int32 InLine) -> void;
    static auto Request_IgnoreEnsure_WithCallstack(const FString& InCallstack) -> void;

private:
    static TMap<FName, TSet<FCk_Ensure_IgnoredEntry>> _IgnoredEnsures;
    static TSet<FString>                              _IgnoredEnsures_BP;
    static FCk_Ensure_OnEnsureIgnored_Delegate_MC     _OnIgnoredEnsure_MC;
};

// --------------------------------------------------------------------------------------------------------------------

#define CK_ENSURE(InExpression, InString, ...)                                                                                \
[&]() -> bool                                                                                                                 \
{                                                                                                                             \
    if (LIKELY(InExpression))                                                                                                 \
    { return true; }                                                                                                          \
                                                                                                                              \
    if (UCk_Utils_Ensure_UE::Get_IsEnsureIgnored(__FILE__, __LINE__))                                                         \
    { return false; }                                                                                                         \
                                                                                                                              \
    const auto& message = ck::Format_UE(InString, ##__VA_ARGS__);                                                             \
    const auto& stackTraceWith2Skips = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace(2);                                      \
    const auto& bpStackTrace = UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Blueprint(ck::type_traits::AsString{});          \
    const auto& callstackPlusMessage = ck::Format_UE(                                                                         \
        TEXT("[{}]\nExpression: '{}'\nMessage: '{}'\n\n == BP CallStack ==\n{}\n == CallStack ==\n{}\n"),                     \
        GFrameCounter,                                                                                                        \
        TEXT(#InExpression),                                                                                                  \
        message,                                                                                                              \
        bpStackTrace,                                                                                                         \
        stackTraceWith2Skips                                                                                                  \
    );                                                                                                                        \
                                                                                                                              \
    const auto& dialogMessage = FText::FromString(callstackPlusMessage);                                                      \
    const auto& ans = UCk_Utils_MessageDialog_UE::YesNoYesAll(dialogMessage, FText::FromString(TEXT("Ignore and Continue?")));\
    switch(ans)                                                                                                               \
    {                                                                                                                         \
        case ECk_MessageDialog_YesNoYesAll::Yes:                                                                              \
        {                                                                                                                     \
            return false;                                                                                                     \
        }                                                                                                                     \
        case ECk_MessageDialog_YesNoYesAll::No:                                                                               \
        {                                                                                                                     \
            const auto& ensureAns = ensureAlwaysMsgf(false, TEXT("[DEBUG BREAK HIT] %s"), *message);                          \
            return ensureAns;                                                                                                 \
        }                                                                                                                     \
        case ECk_MessageDialog_YesNoYesAll::YesAll:                                                                           \
        {                                                                                                                     \
            UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLineWithMessage(__FILE__, dialogMessage, __LINE__);             \
            return false;                                                                                                     \
        }                                                                                                                     \
        default:                                                                                                              \
        {                                                                                                                     \
            return ensureMsgf(false, TEXT("Encountered an invalid value for Enum [{}]"), ans);                                \
        }                                                                                                                     \
    }                                                                                                                         \
}()

#if CK_BYPASS_ENSURES
#define CK_ENSURE_IF_NOT(InExpression, InFormat, ...)\
if constexpr(false)
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
