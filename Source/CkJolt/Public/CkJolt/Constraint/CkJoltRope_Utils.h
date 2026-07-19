#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkJolt/Body/CkJoltBody_Fragment_Data.h"
#include "CkJolt/Constraint/CkJoltConstraint_Fragment_Data.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkJoltRope_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// How consecutive rope segments link: Rigid = point constraints at the segment boundaries (a chain —
// inextensible, rotation free); Springy = auto-distance constraints with soft spring limits between
// segment centers (stretchy — bungees, hair strands).
UENUM(BlueprintType)
enum class ECk_JoltRope_LinkMode : uint8
{
    Rigid,
    Springy
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_JoltRope_LinkMode);

// --------------------------------------------------------------------------------------------------------------------

// Config for a rope built from a chain of Dynamic sphere JoltBodies linked by JoltConstraints. The rope
// hangs from _AnchorLocation toward _Direction; segment i sits at Anchor + Direction * Length * (i + 0.5).
// Essentials = the anchor location. Anchoring: _AnchorBody (a JoltBody entity, e.g. a kinematic scalp)
// when set, otherwise the WORLD at _AnchorLocation.
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_JoltRope_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_JoltRope_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _AnchorLocation = FVector::ZeroVector;

    // Invalid = anchor to the WORLD at _AnchorLocation.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _AnchorBody;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _Direction = FVector{0.0, 0.0, -1.0};

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 1, ClampMax = 200))
    int32 _SegmentCount = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 1.0))
    float _SegmentLength = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.5))
    float _SegmentRadius = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.001))
    float _SegmentMassKg = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_JoltRope_LinkMode _LinkMode = ECk_JoltRope_LinkMode::Rigid;

    // Springy links only (Hz + damping ratio).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.0))
    float _SpringFrequency = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.0))
    float _SpringDamping = 0.7f;

    // Body knobs. Higher-than-Jolt-default damping keeps ropes from jittering forever.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.0))
    float _LinearDamping = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0.0))
    float _AngularDamping = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _GravityFactor = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _CollisionProfileName = TEXT("PhysicsActor");

public:
    CK_PROPERTY_GET(_AnchorLocation);
    CK_PROPERTY(_AnchorBody);
    CK_PROPERTY(_Direction);
    CK_PROPERTY(_SegmentCount);
    CK_PROPERTY(_SegmentLength);
    CK_PROPERTY(_SegmentRadius);
    CK_PROPERTY(_SegmentMassKg);
    CK_PROPERTY(_LinkMode);
    CK_PROPERTY(_SpringFrequency);
    CK_PROPERTY(_SpringDamping);
    CK_PROPERTY(_LinearDamping);
    CK_PROPERTY(_AngularDamping);
    CK_PROPERTY(_GravityFactor);
    CK_PROPERTY(_CollisionProfileName);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_JoltRope_ParamsData, _AnchorLocation);
};

// --------------------------------------------------------------------------------------------------------------------

// The built rope: segment bodies root-to-tip and the links between them (link 0 = anchor link).
USTRUCT(BlueprintType)
struct CKJOLT_API FCk_JoltRope_Result
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_JoltRope_Result);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Handle_JoltBody> _Segments;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Handle_JoltConstraint> _Links;

public:
    CK_PROPERTY(_Segments);
    CK_PROPERTY(_Links);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_JoltRope_Result, _Segments);
};

// --------------------------------------------------------------------------------------------------------------------

// Builder for rope/chain/strand content on top of JoltBody + JoltConstraint. Not a feature — it owns no
// fragments; the returned handles ARE the rope (segments are child entities of InOwner and cascade-die
// with it). "Hair" is a usage pattern: many short Springy strands anchored to a kinematic body.
UCLASS(NotBlueprintable)
class CKJOLT_API UCk_Utils_JoltRope_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_JoltRope_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltRope",
              DisplayName="[Ck][JoltRope] Create Rope")
    static FCk_JoltRope_Result
    Create_Rope(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_JoltRope_ParamsData& InParams);
};

// --------------------------------------------------------------------------------------------------------------------
