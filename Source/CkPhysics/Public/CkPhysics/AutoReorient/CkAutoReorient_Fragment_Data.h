#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkAutoReorient_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKPHYSICS_API FCk_Handle_AutoReorient : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_AutoReorient); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_AutoReorient);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AutoReorient_Policy : uint8
{
    NoAutoReorient,
    OrientTowardsVelocity
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AutoReorient_Policy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPHYSICS_API FCk_Fragment_AutoReorient_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_AutoReorient_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_AutoReorient_Policy _ReorientPolicy = ECk_AutoReorient_Policy::NoAutoReorient;

    // TODO: Add tunables for how much we should reorient per DeltaT. For now, snap

public:
    CK_PROPERTY_GET(_ReorientPolicy);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_AutoReorient_ParamsData, _ReorientPolicy);
};

// --------------------------------------------------------------------------------------------------------------------
