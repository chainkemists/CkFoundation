#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkEditorOnly_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EditorMessage_Severity: uint8
{
    Info,
    Error,
    PerformanceWarning,
    Warning
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EditorMessage_Severity);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EditorMessage_ToastNotification_DisplayPolicy : uint8
{
    DoNotDisplay,
    Display
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EditorMessage_ToastNotification_DisplayPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EditorMessage_MessageLog_DisplayPolicy : uint8
{
    DoNotFocus,
    Focus
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EditorMessage_MessageLog_DisplayPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_TokenizedMessage
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_TokenizedMessage);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FString _Message;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FString _DocumentationLink;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<const UObject> _TargetObject;

public:
    CK_PROPERTY_GET(_Message);
    CK_PROPERTY(_DocumentationLink);
    CK_PROPERTY(_TargetObject);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_TokenizedMessage, _Message);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_MessageSegments
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_MessageSegments);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_TokenizedMessage> _MessageSegments;

public:
    CK_PROPERTY_GET(_MessageSegments);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_MessageSegments, _MessageSegments);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Utils_EditorOnly_PushNewEditorMessage_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Utils_EditorOnly_PushNewEditorMessage_Params);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _LoggerToUse;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_MessageSegments _MessageSegments;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EditorMessage_Severity _MessageSeverity = ECk_EditorMessage_Severity::Info;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EditorMessage_ToastNotification_DisplayPolicy _ToastNotificationDisplayPolicy =
        ECk_EditorMessage_ToastNotification_DisplayPolicy::DoNotDisplay;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EditorMessage_MessageLog_DisplayPolicy _MessageLogDisplayPolicy =
        ECk_EditorMessage_MessageLog_DisplayPolicy::DoNotFocus;

public:
    CK_PROPERTY_GET(_LoggerToUse);
    CK_PROPERTY_GET(_MessageSegments);

    CK_PROPERTY(_MessageSeverity);
    CK_PROPERTY(_ToastNotificationDisplayPolicy);
    CK_PROPERTY(_MessageLogDisplayPolicy);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Utils_EditorOnly_PushNewEditorMessage_Params, _LoggerToUse, _MessageSegments);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_EditorOnly_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EditorOnly_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EditorOnly",
              DisplayName = "[Ck] Request Push New Editor Message",
              meta = (DevelopmentOnly))
    static void
    Request_PushNewEditorMessage(
        const FCk_Utils_EditorOnly_PushNewEditorMessage_Params& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EditorOnly",
              DisplayName = "[Ck] Request Debug Pause Execution",
              meta = (DevelopmentOnly))
    static void
    Request_DebugPauseExecution();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EditorOnly",
              DisplayName = "[Ck] Request Debug Resume Execution",
              meta = (DevelopmentOnly))
    static void
    Request_DebugResumeExecution();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EditorOnly",
              DisplayName = "[Ck] Request Redraw Level Editing Viewports",
              meta = (DevelopmentOnly))
    static void
    Request_RedrawLevelEditingViewports();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EditorOnly",
              DisplayName = "[Ck] Get Is Commandlet/Cooking",
              meta = (DevelopmentOnly))
    static bool
    Get_IsCommandletOrCooking();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EditorOnly",
              DisplayName = "[Ck] Get Is Cooking By The Book",
              meta = (DevelopmentOnly))
    static bool
    Get_IsCookingByTheBook();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EditorOnly",
              DisplayName = "[Ck] Get PIE Net Mode Name Prefix",
              meta = (DevelopmentOnly, DefaultToSelf = "InContextObject"))
    static FString
    Get_PieNetModeNamePrefix(
        const UObject* InContextObject);
};

// --------------------------------------------------------------------------------------------------------------------
