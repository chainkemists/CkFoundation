#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment_Data.h"

#include <InstancedStruct.h>

#include "CkDecal_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGRAPHICS_API FCk_Handle_Decal : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Decal); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Decal);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Decal_Lifetime : uint8
{
    Persistent,

    // Automatically destroys the decal after fully fading out.
    DestroyAfterFadeOut
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRAPHICS_API FCk_Decal_FadeInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Decal_FadeInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _FadeInStartDelay = FCk_Time::ZeroSecond();

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _FadeInDuration = FCk_Time::ZeroSecond();

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _HasFadeOut = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_HasFadeOut"))
    FCk_Time _FadeOutStartDelay = FCk_Time::ZeroSecond();

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_HasFadeOut"))
    FCk_Time _FadeOutDuration = FCk_Time::ZeroSecond();

public:
    CK_PROPERTY(_FadeInStartDelay);
    CK_PROPERTY(_FadeInDuration);
    CK_PROPERTY(_HasFadeOut);
    CK_PROPERTY(_FadeOutStartDelay);
    CK_PROPERTY(_FadeOutDuration);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRAPHICS_API FCk_Decal_VisualInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Decal_VisualInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<class UMaterialInterface> _DecalMaterial;

    /** Decal size in local space (does not include the component scale), technically redundant but there for convenience */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, AllowPreserveRatio = true))
    FVector _DecalSize = FVector{ 128.0f, 256.0f, 256.0f };

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FLinearColor _DecalColor = FLinearColor::White;

    /**
     * Controls the order in which decal elements are rendered.  Higher values draw later (on top).
     * Setting many different sort orders on many different decals prevents sorting by state and can reduce performance.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 SortOrder = 0;

public:
    CK_PROPERTY_GET(_DecalMaterial);
    CK_PROPERTY(_DecalSize);
    CK_PROPERTY(_DecalColor);
    CK_PROPERTY(SortOrder);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Decal_VisualInfo, _DecalMaterial);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRAPHICS_API FCk_Fragment_Decal_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Decal_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Decal_Lifetime _DecalLifetime = ECk_Decal_Lifetime::DestroyAfterFadeOut;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Decal_VisualInfo _DecalVisualInfo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Decal_FadeInfo _DecalFadeInfo;

public:
    CK_PROPERTY(_DecalLifetime);
    CK_PROPERTY(_DecalVisualInfo);
    CK_PROPERTY(_DecalFadeInfo);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRAPHICS_API FCk_Request_Decal_Resize
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Decal_Resize);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, AllowPreserveRatio = true))
    FVector _NewDecalSize = FVector{ 128.0f, 256.0f, 256.0f };

public:
    CK_PROPERTY(_NewDecalSize);
};

// --------------------------------------------------------------------------------------------------------------------
