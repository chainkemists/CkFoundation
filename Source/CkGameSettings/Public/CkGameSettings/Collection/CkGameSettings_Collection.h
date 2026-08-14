#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include "CkGameSettings/CkGameSettings_Common.h"

#include "CkGameSettings_Collection.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Data-asset collection of setting definitions. Collections found in the project settings'
 * CollectionScanPaths are auto-registered at subsystem init; any collection can also be
 * registered explicitly via Request_RegisterCollection. Registration is atomic per collection.
 */
UCLASS(BlueprintType)
class CKGAMESETTINGS_API UCk_GameSettings_Collection_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GameSettings_Collection_PDA);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true, TitleProperty = "_Key"))
    TArray<FCk_GameSettings_SettingDefinition> _Settings;

public:
    CK_PROPERTY_GET(_Settings);
};

// --------------------------------------------------------------------------------------------------------------------
