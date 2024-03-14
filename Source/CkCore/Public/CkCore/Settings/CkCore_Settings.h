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
    ModalDialog,
    MessageLog,
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

    UPROPERTY(Config, VisibleAnywhere, BlueprintReadOnly, Category = "Ensures",
              meta = (AllowPrivateAccess = true))
    ECk_EnsureDetails_Policy _EnsureDetailsPolicy = ECk_EnsureDetails_Policy::MessageAndStackTrace;

public:
    CK_PROPERTY_GET(_DefaultDebugNameVerbosity);
    CK_PROPERTY_GET(_EnsureDisplayPolicy);
    CK_PROPERTY_GET(_EnsureDetailsPolicy);
};

// --------------------------------------------------------------------------------------------------------------------

class CKCORE_API UCk_Utils_Core_ProjectSettings_UE
{
public:
    static auto Get_EnsureDisplayPolicy() -> ECk_EnsureDisplay_Policy;
    static auto Get_EnsureDetailsPolicy() -> ECk_EnsureDetails_Policy;
};

// --------------------------------------------------------------------------------------------------------------------

class CKCORE_API UCk_Utils_Core_UserSettings_UE
{
public:
    static auto Get_DefaultDebugNameVerbosity() -> ECk_DebugNameVerbosity_Policy;
};

// --------------------------------------------------------------------------------------------------------------------
