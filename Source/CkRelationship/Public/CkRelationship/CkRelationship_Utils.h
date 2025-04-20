#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkRelationship/Team/CkTeam_Fragment_Data.h"

#include "CkRelationship_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RelationshipAttitude : uint8
{
    Neutral,
    Friendly,
    Hostile
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RelationshipAttitude);

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKRELATIONSHIP_API UCk_Utils_Relationship_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Relationship_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Relationship",
              DisplayName="[Ck][Relationship] Get Attitude Towards")
    ECk_RelationshipAttitude
    Get_AttitudeTowards(
        const FCk_Handle& InFrom,
        const FCk_Handle& InTo);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Relationship",
              DisplayName="[Ck][Relationship] Get Attitude Towards")
    ECk_RelationshipAttitude
    Get_AttitudeTowards_Exec(
        const FCk_Handle& InFrom,
        const FCk_Handle& InTo);
};

// --------------------------------------------------------------------------------------------------------------------
