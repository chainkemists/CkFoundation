#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include <GameplayTagContainer.h>

#include "Ck2dGridObject_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_GridObject_Centering : uint8
{
    Anchor,
    Center
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GridObject_Centering);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGRID_API FCk_Handle_2dGridObject : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_2dGridObject); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_2dGridObject);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRID_API FCk_2dGridObject_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_2dGridObject_Spec);

private:
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FIntPoint _FootprintExtent = FIntPoint(1, 1);

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_GridObject_Centering _Centering = ECk_GridObject_Centering::Anchor;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FIntPoint _AnchorOffset = FIntPoint::ZeroValue;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Categories = "2dGridObject"))
    FGameplayTagContainer _RequiredCellTags;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, Categories = "2dGridObject"))
    FGameplayTagContainer _ForbiddenCellTags;

public:
    CK_PROPERTY(_FootprintExtent);
    CK_PROPERTY(_Centering);
    CK_PROPERTY(_AnchorOffset);
    CK_PROPERTY(_RequiredCellTags);
    CK_PROPERTY(_ForbiddenCellTags);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_2dGridObject_Spec, _FootprintExtent);
};

// --------------------------------------------------------------------------------------------------------------------
