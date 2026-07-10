#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>
#include <Engine/DataAsset.h>

#include "CkArchetype_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Archetype: a NAMED amalgamation of features (ECS debugger redesign,
// CkGameplayDebugger docs/specs/2026-07-10 §3.3).
//
// CkEcs owns storage + data model only. Resolution is split by consumer:
// - FeatureIds match through the DebugFeatureFlags bit cache when enabled
//   ((bits & required) == required — see CkArchetype_Registry.h).
// - RequiredLabel / NamePattern are carried as data and matched by consumers that can
//   see those systems (the ECS debugger) — CkEcs cannot depend on CkLabel.
// - Native registrations may supply a direct matcher (the CK_DEFINE_ARCHETYPE typed
//   struct wires its generated TryCast in — later phase).
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECS_API FCk_ArchetypeDescriptor
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ArchetypeDescriptor);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FText _DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FName> _FeatureIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _RequiredLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FString _NamePattern;

    // Relative to the owning plugin/project's Resources dir; consumers register the brush.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FString _IconSvgPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FLinearColor _Color = FLinearColor::White;

    // Precedence when several archetypes match: highest Priority, then most FeatureIds,
    // then registration order.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _Priority = 0;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY(_DisplayName);
    CK_PROPERTY(_FeatureIds);
    CK_PROPERTY(_RequiredLabel);
    CK_PROPERTY(_NamePattern);
    CK_PROPERTY(_IconSvgPath);
    CK_PROPERTY(_Color);
    CK_PROPERTY(_Priority);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ArchetypeDescriptor, _Name);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Data asset that defines an Archetype — a game-nameable amalgamation of features with a
 * bespoke debugger identity (icon + color). Mirrors UCkDynamic_HandleDefinition's
 * authoring shape; the expected authoring path is AngelScript:
 *
 *   asset Shelf_Archetype of UCk_ArchetypeDefinition
 *   {
 *       Name = n"Rewind99.Shelf";
 *       DisplayName = FText::FromString("Shelf");
 *       FeatureIds.Add(n"Transform");
 *       FeatureIds.Add(n"Inventory");
 *       FeatureIds.Add(n"Label");
 *       IconSvgPath = "Icons/Shelf.svg";
 *   }
 *
 * Definitions are pushed into ck::archetype_registry by consumers (the ECS debugger scans
 * the asset registry on open) or explicitly via UCk_Utils_Archetype_UE.
 */
UCLASS(BlueprintType)
class CKECS_API UCk_ArchetypeDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ArchetypeDefinition);

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype Definition")
    FName Name;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype Definition")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype Definition")
    TArray<FName> FeatureIds;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype Definition")
    FName RequiredLabel;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype Definition")
    FString NamePattern;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype Definition")
    FString IconSvgPath;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype Definition")
    FLinearColor Color = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype Definition")
    int32 Priority = 0;

public:
    UFUNCTION(BlueprintCallable,
        Category = "Archetype Definition")
    FCk_ArchetypeDescriptor
    Get_Descriptor() const;

    UFUNCTION(BlueprintCallable,
        Category = "Archetype Definition")
    bool
    IsValidDefinition() const;
};
