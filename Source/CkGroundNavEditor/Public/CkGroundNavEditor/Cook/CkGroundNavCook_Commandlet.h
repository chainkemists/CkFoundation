#pragma once

#include <Commandlets/Commandlet.h>

#include "CkGroundNavCook_Commandlet.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Bakes every authored GroundNav volume in a loaded map. Volumes live on Ck Entity Spawners, so this
 * commandlet reads their script params directly and never constructs an ECS entity. `-DryRun` performs
 * the same Jolt extraction, pure field bake and serialization plan without creating a package or saving.
 */
UCLASS()
class CKGROUNDNAVEDITOR_API UCk_GroundNavCook_Commandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GroundNavCook_Commandlet);

public:
    auto
    Main(
        const FString& InParams) -> int32 override;
};

// --------------------------------------------------------------------------------------------------------------------
