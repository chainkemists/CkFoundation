#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"
#include "CkSettings/UserSettings/CkUserSettings.h"

#include "CkCore_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EnsureDisplay_Policy : uint8
{
    /* Ensures are processed and a modal dialog displayed to give users a chance to break, ignore once or ignore all. */
    ModalDialog,
    /* Ensures are processed once per game session and sent to the MessageLog window. Will break into Blueprint Script if possible. No modal dialog is displayed. */
    MessageLog,
    /* Ensures are processed once per game session and logged. Will NOT break into Blueprint script. Only use when streaming!*/
    StreamerMode,
    LogOnly UMETA(Hidden)
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EnsureDisplay_Policy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EnsureDetails_Policy : uint8
{
    MessageOnly,
    MessageAndStackTrace
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EnsureDetails_Policy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EnsureBreakInBlueprints_Policy : uint8
{
    AlwaysBreak,
    // Unreal 'Break on Exception' is disabled by default in Editor Settings
    UseUnrealExceptionBehavior
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EnsureBreakInBlueprints_Policy);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Core"))
class CKCORE_API UCk_Core_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Core_ProjectSettings_UE);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Core"))
class CKCORE_API UCk_Core_UserSettings_UE : public UCk_Plugin_UserSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Core_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug",
              meta = (AllowPrivateAccess = true, InvalidEnumValues="Default"))
    ECk_DebugNameVerbosity_Policy _DefaultDebugNameVerbosity = ECk_DebugNameVerbosity_Policy::Compact;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ensures",
              meta = (AllowPrivateAccess = true))
    ECk_EnsureDisplay_Policy _EnsureDisplayPolicy = ECk_EnsureDisplay_Policy::ModalDialog;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Ensures",
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_EnsureDisplayPolicy != ECk_EnsureDisplay_Policy::StreamerMode"))
    ECk_EnsureBreakInBlueprints_Policy _EnsureBreakInBlueprintsPolicy = ECk_EnsureBreakInBlueprints_Policy::AlwaysBreak;

    UPROPERTY(Config, VisibleAnywhere, BlueprintReadOnly, Category = "Ensures",
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_EnsureDisplayPolicy != ECk_EnsureDisplay_Policy::StreamerMode"))
    ECk_EnsureDetails_Policy _EnsureDetailsPolicy = ECk_EnsureDetails_Policy::MessageAndStackTrace;

public:
    CK_PROPERTY_GET(_DefaultDebugNameVerbosity);
    CK_PROPERTY_GET(_EnsureDisplayPolicy);
    CK_PROPERTY_GET(_EnsureDetailsPolicy);
    CK_PROPERTY_GET(_EnsureBreakInBlueprintsPolicy);
};

// --------------------------------------------------------------------------------------------------------------------

class CKCORE_API UCk_Utils_Core_ProjectSettings_UE
{
};

// --------------------------------------------------------------------------------------------------------------------

class CKCORE_API UCk_Utils_Core_UserSettings_UE
{
public:
    static auto Get_DefaultDebugNameVerbosity() -> ECk_DebugNameVerbosity_Policy;
    static auto Get_EnsureBreakInBlueprintsPolicy() -> ECk_EnsureBreakInBlueprints_Policy;
    static auto Get_EnsureDisplayPolicy() -> ECk_EnsureDisplay_Policy;
    static auto Get_EnsureDetailsPolicy() -> ECk_EnsureDetails_Policy;
};

// --------------------------------------------------------------------------------------------------------------------
