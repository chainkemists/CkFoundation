#pragma once

#include <CoreMinimal.h>
#include <Animation/AnimSequence.h>
#include <Animation/AnimBlueprint.h>
#include <Engine/DeveloperSettings.h>

#include "CkAnimationToolbox_Common.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AnimationToolbox_Status : uint8
{
    Unknown         UMETA(DisplayName = "Not scanned"),
    Clean           UMETA(DisplayName = "Clean"),
    Affected        UMETA(DisplayName = "Affected"),
    Skipped         UMETA(DisplayName = "Skipped"),
    Fixed           UMETA(DisplayName = "Fixed"),
    Failed          UMETA(DisplayName = "Failed"),
};

UENUM(BlueprintType)
enum class ECk_AnimationToolbox_ScanScope : uint8
{
    ProjectWide             UMETA(DisplayName = "Project-wide"),
    AnimBlueprintDeps       UMETA(DisplayName = "AnimBlueprint dependencies"),
    Path                    UMETA(DisplayName = "Content path"),
};

UENUM(BlueprintType)
enum class ECk_AnimationToolbox_RewriteStrategy : uint8
{
    TrimFirstFrame          UMETA(DisplayName = "A: Trim frame 0 (length shrinks)"),
    CopyFromFrameOne        UMETA(DisplayName = "B: Copy frame 1 onto frame 0 (length preserved)"),
    CopyFromLastFrame       UMETA(DisplayName = "C: Copy last frame onto frame 0 (seamless loop, recommended)"),
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATIONEDITOR_API FCk_AnimationToolbox_AnimInfo
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TSoftObjectPtr<UAnimSequence> AnimPtr;

    UPROPERTY()
    FString AssetName;

    UPROPERTY()
    FString PackagePath;

    UPROPERTY()
    int32 NumFrames = 0;

    UPROPERTY()
    int32 NumNotifies = 0;

    /** Pose-delta (translation cm + rotation deg summed over bones) between frame 0 and frame 1. */
    UPROPERTY()
    float Frame0To1Delta = 0.0f;
    /** Pose-delta from the last frame back to frame 0 — the loop-boundary jump. */
    UPROPERTY()
    float LoopBoundaryDelta = 0.0f;
    /** Median pose-delta across all frame-to-frame transitions i -> i+1, excluding the 0->1 transition. */
    UPROPERTY()
    float MedianDelta = 0.0f;
    /** Frame0To1Delta / MedianDelta. Values >> 1 indicate frame 0 is an outlier. */
    UPROPERTY()
    float Ratio = 0.0f;
    /** LoopBoundaryDelta / MedianDelta. Large values mean the loop doesn't close cleanly. */
    UPROPERTY()
    float LoopBoundaryRatio = 0.0f;

    /** Skeleton bones for which the anim has NO track; sampled from ref pose during detection. */
    UPROPERTY()
    int32 UnmappedBoneCount = 0;

    UPROPERTY()
    int32 MappedBoneCount = 0;
    /** First few unmapped bone names for tooltip/log output (capped to keep memory bounded). */
    UPROPERTY()
    TArray<FName> UnmappedBoneSample;

    UPROPERTY()
    ECk_AnimationToolbox_Status Status = ECk_AnimationToolbox_Status::Unknown;

    UPROPERTY()
    FString StatusMessage;

    // UI state (not persisted)
    UPROPERTY(Transient)
    bool bSelectedForFix = false;

public:
    FCk_AnimationToolbox_AnimInfo() = default;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANIMATIONEDITOR_API FCk_AnimationToolbox_ScanOptions
{
    GENERATED_BODY()

public:
    UPROPERTY()
    ECk_AnimationToolbox_ScanScope Scope = ECk_AnimationToolbox_ScanScope::AnimBlueprintDeps;

    /** When Scope == AnimBlueprintDeps */
    UPROPERTY()
    TSoftObjectPtr<UAnimBlueprint> AnimBlueprint;

    /** When Scope == Path */
    UPROPERTY()
    FString ContentPath = TEXT("/Game/");

    /** Frame-0-to-1 pose delta must exceed this multiple of the median transition delta to flag the anim. */
    UPROPERTY()
    float OutlierMultiplier = 3.0f;

    /** If true, also require the last-frame -> first-frame delta to exceed the multiplier.
     *  Skips non-looping anims whose frame 0 legitimately differs from frame 1. */
    UPROPERTY()
    bool bRequireLoopBoundaryMismatch = true;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Config = Editor, DefaultConfig, meta = (DisplayName = "CkAnimation Toolbox"))
class CKANIMATIONEDITOR_API UCk_AnimationToolbox_DeveloperSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category = "Scan")
    ECk_AnimationToolbox_ScanScope LastScope = ECk_AnimationToolbox_ScanScope::AnimBlueprintDeps;

    UPROPERTY(Config, EditAnywhere, Category = "Scan", meta = (AllowedClasses = "/Script/Engine.AnimBlueprint"))
    TSoftObjectPtr<UAnimBlueprint> LastAnimBlueprint;

    UPROPERTY(Config, EditAnywhere, Category = "Scan", meta = (ContentDir))
    FDirectoryPath LastPath;

    UPROPERTY(Config, EditAnywhere, Category = "Detection", meta = (ClampMin = "1.0", ClampMax = "20.0", UIMin = "1.0", UIMax = "10.0"))
    float OutlierMultiplier = 3.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Detection")
    bool bRequireLoopBoundaryMismatch = true;

    UPROPERTY(Config, EditAnywhere, Category = "Fix")
    ECk_AnimationToolbox_RewriteStrategy Strategy = ECk_AnimationToolbox_RewriteStrategy::CopyFromLastFrame;

public:
    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};

// --------------------------------------------------------------------------------------------------------------------
