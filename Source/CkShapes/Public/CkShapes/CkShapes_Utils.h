#pragma once

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkShapes_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Shape_Type : uint8
{
    Box,
    Capsule,
    Cylinder,
    Sphere,

    None UMETA(DisplayName = "No Shape"),
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Shape_Type);

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKSHAPES_API UCk_Utils_Shapes_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Shapes_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][Shapes] Has Feature",
              Category = "Ck|Utils|Shapes")
    static bool
    Has_Any(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck][Shapes] Get Type",
              Category = "Ck|Utils|Shapes")
    static ECk_Shape_Type
    Get_ShapeType(
        const FCk_Handle& InHandle);
};

// --------------------------------------------------------------------------------------------------------------------