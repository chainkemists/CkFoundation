#pragma once

#include "CoreMinimal.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Scene.h"          // FLightingChannels
#include "UObject/PerPlatformProperties.h"
#include "Animation/AnimInstance.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkIskmRenderer/Renderer/CkIskmRenderer_MeshDesc.h"

#include "CkIskmRenderer_Fragment_Data.generated.h"

class UCk_IskmAnimCollection_Data;

UENUM(BlueprintType)
enum class ECk_Iskm_ClusterMode : uint8
{
    None,    // Plan-1 default — proxies render directly off the manager actor.
    Tiled,   // Plan-2 — clustered scene proxy on a tile grid.
};

USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_IskmRenderer_RenderingInfo
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_IskmRenderer_RenderingInfo);
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    uint8 _bCastDynamicShadow : 1 = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    uint8 _bRenderInMainPass : 1 = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    uint8 _bRenderInDepthPass : 1 = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    uint8 _bReceivesDecals : 1 = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    uint8 _bUseAsOccluder : 1 = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    uint8 _bRenderCustomDepth : 1 = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    uint8 _bMayHaveNegativeDeterminant : 1 = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    uint8 _bPerInstanceLocalBounds : 1 = false;
public:
    // Bitfields require BY_COPY accessors — `CK_PROPERTY_GET` returns `const T&` and
    // bitfields have no addressable storage (compiler emits C4172 returning a ref).
    CK_PROPERTY_GET_BY_COPY(_bCastDynamicShadow);
    CK_PROPERTY_GET_BY_COPY(_bRenderInMainPass);
    CK_PROPERTY_GET_BY_COPY(_bRenderInDepthPass);
    CK_PROPERTY_GET_BY_COPY(_bReceivesDecals);
    CK_PROPERTY_GET_BY_COPY(_bUseAsOccluder);
    CK_PROPERTY_GET_BY_COPY(_bRenderCustomDepth);
    CK_PROPERTY_GET_BY_COPY(_bMayHaveNegativeDeterminant);
    CK_PROPERTY_GET_BY_COPY(_bPerInstanceLocalBounds);
};

USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_IskmRenderer_CullingInfo
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_IskmRenderer_CullingInfo);
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FPerPlatformFloat _MinDrawDistance;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FPerPlatformFloat _MaxDrawDistance;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FPerPlatformFloat _LODScale;
    // UHT: static C-style arrays cannot be `BlueprintReadWrite` — code-side reads go through `Get_LODDistance`.
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    float _LODDistances[7] = { 0.f };
public:
    CK_PROPERTY_GET(_MinDrawDistance);
    CK_PROPERTY_GET(_MaxDrawDistance);
    CK_PROPERTY_GET(_LODScale);
    auto Get_LODDistance(int32 InIndex) const -> float
    {
        return (InIndex >= 0 && InIndex < 7) ? _LODDistances[InIndex] : 0.f;
    }
};

USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_IskmRenderer_ClusterInfo
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(FCk_IskmRenderer_ClusterInfo);
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Iskm_ClusterMode _ClusterMode = ECk_Iskm_ClusterMode::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true,
                EditCondition = "_ClusterMode == ECk_Iskm_ClusterMode::Tiled",
                EditConditionHides, UIMin = 100.0, ClampMin = 100.0))
    float _ClusterCellSize = 5000.0f;
public:
    CK_PROPERTY_GET(_ClusterMode);
    CK_PROPERTY_GET(_ClusterCellSize);
};

UCLASS(BlueprintType)
class CKISKMRENDERER_API UCk_IskmRenderer_Data : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_IskmRenderer_Data);

protected:
#if WITH_EDITOR
    auto
    PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent) -> void override;

    auto
    IsDataValid(class FDataValidationContext& InContext) const -> EDataValidationResult override;
#endif

private:
    // ---- core wiring ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_IskmAnimCollection_Data> _AnimCollection;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation",
              meta = (AllowPrivateAccess = true))
    TSoftClassPtr<UAnimInstance> _DefaultAnimInstanceClass;

    // ---- modular outfit ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meshes",
              meta = (AllowPrivateAccess = true, TitleProperty = "{_Name}"))
    TArray<FCk_IskmRenderer_MeshDesc> _Submeshes;

    // Hard cap 15: Plan-2's cluster proxy packs mesh presence as a 4-bit bitmask. Plan-1 enforces the
    // same cap at Request_AttachSubmesh time so game code can't silently break under Plan-2.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Meshes",
              meta = (AllowPrivateAccess = true,
                      UIMin = 1, ClampMin = 1, UIMax = 15, ClampMax = 15))
    int32 _MaxSubmeshPerInstance = 15;

    // ---- per-instance custom data ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Data",
              meta = (AllowPrivateAccess = true,
                      UIMin = 0, ClampMin = 0, UIMax = 16, ClampMax = 16))
    int32 _NumCustomDataFloat = 0;

    // ---- render flags / culling / clustering / lighting / bounds ----
    // Plan-1 applies these to the SKMC at Setup; Plan-2 forwards them to the cluster scene proxy.

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rendering",
              meta = (AllowPrivateAccess = true))
    FCk_IskmRenderer_RenderingInfo _RenderingInfo;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Culling",
              meta = (AllowPrivateAccess = true))
    FCk_IskmRenderer_CullingInfo _CullingInfo;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clustering",
              meta = (AllowPrivateAccess = true))
    FCk_IskmRenderer_ClusterInfo _ClusterInfo;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rendering",
              meta = (AllowPrivateAccess = true,
                      UIMin = 0.1, ClampMin = 0.1, UIMax = 10.0, ClampMax = 10.0))
    float _BoundsScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rendering",
              meta = (AllowPrivateAccess = true))
    FLightingChannels _LightingChannels;

public:
    CK_PROPERTY_GET(_AnimCollection);
    CK_PROPERTY_GET(_DefaultAnimInstanceClass);
    CK_PROPERTY_GET(_Submeshes);
    CK_PROPERTY_GET(_MaxSubmeshPerInstance);
    CK_PROPERTY_GET(_NumCustomDataFloat);
    CK_PROPERTY_GET(_RenderingInfo);
    CK_PROPERTY_GET(_CullingInfo);
    CK_PROPERTY_GET(_ClusterInfo);
    CK_PROPERTY_GET(_BoundsScale);
    CK_PROPERTY_GET(_LightingChannels);

public:
    auto
    Find_SubmeshIndex_ByName(FName InName) const -> int32;
};

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKISKMRENDERER_API FCk_Handle_IskmRenderer : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_IskmRenderer);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_IskmRenderer);
