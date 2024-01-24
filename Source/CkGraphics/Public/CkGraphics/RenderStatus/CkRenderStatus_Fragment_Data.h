#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment_Data.h"

#include <InstancedStruct.h>

#include "CkRenderStatus_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RenderStatus_Group : uint8
{
    Unspecified,
    WorldStatic,
    WorldDynamic,
    Pawn,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RenderStatus_Group);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRAPHICS_API FCk_RenderedActorsList
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RenderedActorsList);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_RenderStatus_Group _RenderGroup = ECk_RenderStatus_Group::Unspecified;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_EntityOwningActor_BasicDetails> _RenderedEntityWithActors;

public:
    CK_PROPERTY(_RenderGroup);
    CK_PROPERTY(_RenderedEntityWithActors);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRAPHICS_API FCk_Fragment_RenderStatus_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_RenderStatus_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_RenderStatus_Group _RenderGroup = ECk_RenderStatus_Group::Unspecified;

public:
    CK_PROPERTY_GET(_RenderGroup)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_RenderStatus_ParamsData, _RenderGroup);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRAPHICS_API FCk_Request_RenderStatus_QueryRenderedActors
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_RenderStatus_QueryRenderedActors);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_RenderStatus_Group _RenderGroup = ECk_RenderStatus_Group::Unspecified;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _TimeTolerance = FCk_Time::ZeroSecond();

public:
    CK_PROPERTY(_RenderGroup);
    CK_PROPERTY(_TimeTolerance);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_RenderStatus_OnRenderedActorsQueried_MC,
    const FCk_RenderedActorsList&, InRenderedActorsList,
    const FInstancedStruct&, InOptionalPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_RenderStatus_OnRenderedActorsQueried,
    const FCk_RenderedActorsList&, InRenderedActorsList,
    const FInstancedStruct&, InOptionalPayload);

// --------------------------------------------------------------------------------------------------------------------
