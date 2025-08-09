#pragma once

#include "CkCore/Format/CkFormat.h"

#include "CkCore/Enums/CkEnums.h"

#include "CkEnsure_Utils.generated.h"

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
class CKCORE_API UCk_Utils_Ensure_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Hidden in the editor through the DefaultCkFoundation.ini Config file (see: BlueprintEditor.Menu section)
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ensure|Private",
              DisplayName = "[Ck] Ensure (No Format)",
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