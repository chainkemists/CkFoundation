#pragma once

#include "CoreMinimal.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkIskmRenderer_MeshDesc.generated.h"

USTRUCT(BlueprintType)
struct CKISKMRENDERER_API FCk_IskmRenderer_MeshDesc
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_IskmRenderer_MeshDesc);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FName _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FName _GroupName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<USkeletalMesh> _Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<TObjectPtr<UMaterialInterface>> _OverrideMaterials;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _LODScreenSizeScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _AttachByDefault = false;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_GroupName);
    CK_PROPERTY_GET(_Mesh);
    CK_PROPERTY_GET(_OverrideMaterials);
    CK_PROPERTY_GET(_LODScreenSizeScale);
    CK_PROPERTY_GET(_AttachByDefault);
};
