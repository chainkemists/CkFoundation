#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CkUsf_LookDefinition.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Usf_Domain : uint8
{
    SurfaceLit,
    SurfaceUnlit,
    PostProcess,
    UI,
    Decal
};

UENUM(BlueprintType)
enum class ECk_Usf_ParamType : uint8
{
    Scalar,
    Vector,
    Texture2D,
    TextureCube
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUSF_API FCk_Usf_ParamDesc
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    FName _Name = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    ECk_Usf_ParamType _Type = ECk_Usf_ParamType::Scalar;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    float _DefaultScalar = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    FLinearColor _DefaultVector = FLinearColor::Black;

    // Object path for Texture2D / TextureCube params, e.g.
    // "/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap".
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    FString _DefaultTexturePath;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKUSF_API UCkUsf_LookDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    // Include path resolvable from a Custom node, e.g. "/CkUsf/Looks/Hologram.ush"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    FString _UshIncludePath;

    // HLSL function name inside the include, e.g. "CkUsf_Look_Hologram"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    FName _UshFunctionName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    ECk_Usf_Domain _Domain = ECk_Usf_Domain::SurfaceLit;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    TArray<FCk_Usf_ParamDesc> _Parameters;

    // Logical look name; defaults to asset name if None. Drives the generated master path.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkUsf")
    FName _LookName = NAME_None;

    UFUNCTION(BlueprintCallable, Category = "CkUsf",
              DisplayName = "[Ck][Usf] Get Effective Look Name")
    FName Get_EffectiveLookName() const;
};
