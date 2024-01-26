#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include <Engine/Font.h>
#include <Kismet/BlueprintFunctionLibrary.h>
#include <Internationalization/Internationalization.h>

#include "CkIO_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Engine_TextFontSize : uint8
{
    Subtitle,
    Tiny,
    Small,
    Medium,
    Large
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Engine_TextFontSize);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_Utils_IO_AssetInfoFromPath_Result
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Utils_IO_AssetInfoFromPath_Result);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool        _AssetFound = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FAssetData  _AssetData;

public:
    CK_PROPERTY(_AssetFound);
    CK_PROPERTY(_AssetData);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKCORE_API UCk_Utils_IO_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IO_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Engine Default Text Font")
    static UFont*
    Get_Engine_DefaultTextFont(
        ECk_Engine_TextFontSize InFontSize);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Project Version")
    static FString
    Get_ProjectVersion();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Project Directory")
    static FString
    Get_ProjectDir();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Plugins Directory")
    static FString
    Get_PluginsDir(
        const FString& InPluginName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Project Content Directory")
    static FString
    Get_ProjectContentDir();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Project Plugins Directory")
    static FString
    Get_ProjectPluginsDir();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Engine Plugins Directory")
    static FString
    Get_EnginePluginsDir();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Asset Info From Path")
    static FCk_Utils_IO_AssetInfoFromPath_Result
    Get_AssetInfoFromPath(
        const FString& InAssetPath);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|IO",
              DisplayName = "[Ck] Get Extract Path")
    static FString
    Get_ExtractPath(
        const FString& InFullPath);
};

// --------------------------------------------------------------------------------------------------------------------
