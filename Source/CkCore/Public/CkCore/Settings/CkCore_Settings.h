#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkCore_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Core_EnsureDisplayPolicy : uint8
{
    ModalDialog,
    LogOnly
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Core_EnsureDisplayPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Core_EnsureDetailsPolicy : uint8
{
    MessageOnly,
    MessageAndStackTrace
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Core"))
class CKCORE_API UCk_Core_ProjectSettings_UE : public UCk_Engine_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Core_ProjectSettings_UE);

private:
    UPROPERTY(Config, VisibleAnywhere, BlueprintReadOnly, Category = "Ensures",
              meta = (AllowPrivateAccess = true))
    ECk_Core_EnsureDisplayPolicy _EnsureDisplayPolicy = ECk_Core_EnsureDisplayPolicy::ModalDialog;

    UPROPERTY(Config, VisibleAnywhere, BlueprintReadOnly, Category = "Ensures",
              meta = (AllowPrivateAccess = true))
    ECk_Core_EnsureDetailsPolicy _EnsureDetailsPolicy = ECk_Core_EnsureDetailsPolicy::MessageAndStackTrace;

public:
    CK_PROPERTY_GET(_EnsureDisplayPolicy);
    CK_PROPERTY_GET(_EnsureDetailsPolicy);
};

// --------------------------------------------------------------------------------------------------------------------

class CKCORE_API UCk_Utils_Core_ProjectSettings_UE
{
public:
    static auto Get_EnsureDisplayPolicy() -> ECk_Core_EnsureDisplayPolicy;
    static auto Get_EnsureDetailsPolicy() -> ECk_Core_EnsureDetailsPolicy;
};

// --------------------------------------------------------------------------------------------------------------------
