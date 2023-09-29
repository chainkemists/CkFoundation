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
enum class ECk_EditorMessage_DisplayPolicy: uint8
{
    ToastNotification,
    ToastNotificationAndOpenMessageLogWindow
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EditorMessage_DisplayPolicy);

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
    TObjectPtr<UObject> _TargetObject;

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
    ECk_EditorMessage_DisplayPolicy _DisplayPolicy = ECk_EditorMessage_DisplayPolicy::ToastNotificationAndOpenMessageLogWindow;

public:
    CK_PROPERTY_GET(_LoggerToUse);
    CK_PROPERTY_GET(_MessageSegments);
    CK_PROPERTY_GET(_MessageSeverity);
    CK_PROPERTY_GET(_DisplayPolicy);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Utils_EditorOnly_PushNewEditorMessage_Params, _LoggerToUse, _MessageSegments, _MessageSeverity, _DisplayPolicy);
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
              meta = (DevelopmentOnly))
    static void
    Request_PushNewEditorMessage(
        const FCk_Utils_EditorOnly_PushNewEditorMessage_Params& InParams);
};

// --------------------------------------------------------------------------------------------------------------------
