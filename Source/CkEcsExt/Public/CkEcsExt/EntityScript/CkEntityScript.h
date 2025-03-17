#pragma once

#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include "CkEntityScript.generated.h"

// -----------------------------------------------------------------------------------------------------------

UCLASS(Abstract, EditInlineNew, Blueprintable, BlueprintType)
class CKECSEXT_API UCk_EntityScript_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityScript_PDA);
};
