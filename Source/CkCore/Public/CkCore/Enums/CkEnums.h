#pragma once

#include "CkCore/Format/CkFormat.h"

#include "CkThirdParty/bitwise-enum/bitwise_enum.hpp"

#include "CkEnums.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/*
 * All common enums go in this file
*/

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template<typename T_Enum>
    auto
    Get_EnumValueByName(
        FName InEnumValueAsName,
        const TCHAR* InEnum)
    -> TOptional<T_Enum>
    {
        const auto* EnumType = FindFirstObjectSafe<UEnum>(InEnum, EFindFirstObjectOptions::ExactClass);

        if (ck::Is_NOT_Valid(EnumType, ck::IsValid_Policy_NullptrOnly{}))
        { return {}; }

        const auto& EnumValue = EnumType->GetValueByName(InEnumValueAsName);
        return static_cast<T_Enum>(EnumValue);
    }

    template<typename T_Enum>
    auto
    Get_EnumValueByName(
        FName InEnumValueAsName)
    -> TOptional<T_Enum>
    {
        return Get_EnumValueByName<T_Enum>(InEnumValueAsName, FEnumToString<T_Enum>::Get_AsTChar());
    }
}

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Mobility : uint8
{
    Static,
    Stationary,
    Movable
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Mobility);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_SourceOrTarget : uint8
{
    Source,
    Target,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SourceOrTarget);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_PendingKill_Policy : uint8
{
    ExcludePendingKill,
    IncludePendingKill,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PendingKill_Policy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_SelfOrParent : uint8
{
    Self,
    Parent
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SelfOrParent);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RowFoundOrNot : uint8
{
    RowFound,
    RowNotFound
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RowFoundOrNot);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_CardinalDirection : uint8
{
    North,
    West,
    South,
    East
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_CardinalDirection);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_OrdinalDirection : uint8
{
    NorthEast,
    SouthEast,
    SouthWest,
    NorthWest
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_OrdinalDirection);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_CardinalAndOrdinalDirection : uint8
{
    North,
    NorthEast,
    East,
    SouthEast,
    South,
    SouthWest,
    West,
    NorthWest
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_CardinalAndOrdinalDirection);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Replication : uint8
{
    Replicates,
    DoesNotReplicate
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Replication);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ScaledUnscaled : uint8
{
    Scaled,
    Unscaled
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ScaledUnscaled);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Recursion : uint8
{
    NotRecursive,
    Recursive
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Recursion);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_SucceededFailed : uint8
{
    Succeeded,
    Failed
};

static inline ECk_SucceededFailed IgnoreSucceededFailed;

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SucceededFailed);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ForwardReverse : uint8
{
    Forward,
    Reverse
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ForwardReverse);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AddedOrNot : uint8
{
    Added,
    AlreadyExists,
    NotAdded
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AddedOrNot);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ValidInvalid : uint8
{
    Valid,
    Invalid
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ValidInvalid);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RelativeAbsolute : uint8
{
    Relative,
    Absolute
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RelativeAbsolute);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_LocalWorld : uint8
{
    Local,
    World
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_LocalWorld);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Edges : uint8
{
    Left,
    Right,
    Top,
    Bottom
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Edges);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Direction_2D : uint8
{
    Left,
    Right,
    Up,
    Down
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Direction_2D);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Direction_3D : uint8
{
    Forward,
    Left,
    Right,
    Back,
    Up,
    Down
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Direction_3D);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_PitchYawRoll : uint8
{
    Pitch,
    Yaw,
    Roll
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PitchYawRoll);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Logic_And_Or : uint8
{
    And,
    Or
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Logic_And_Or);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EnableDisable : uint8
{
    Enable,
    Disable
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EnableDisable);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_TargetInstigator : uint8
{
    Target,
    Instigator
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_TargetInstigator);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_BeginEndOverlap : uint8
{
    BeginOverlap,
    EndOverlap,
    Both
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_BeginEndOverlap);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Vector_Axis : uint8
{
    X UMETA (DisplayName = "X (Forward)"),
    Y UMETA (DisplayName = "Y (Right)"),
    Z UMETA (DisplayName = "Z (Up)"),
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Vector_Axis);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Vector_Axis_Swizzle : uint8
{
    YXZ,
    ZYX,
    XZY,
    YZX,
    ZXY
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Vector_Axis_Swizzle);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Plane_Axis : uint8
{
    XY,
    XZ,
    YZ
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Plane_Axis);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = true))
enum class ECk_TransformComponents : uint8
{
    None = 0 UMETA(Hidden),
    Location = 1 << 0,
    Rotation = 1 << 1,
    Scale = 1 << 2
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_TransformComponents);
ENABLE_ENUM_BITWISE_OPERATORS(ECk_TransformComponents);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Interpolation_Strategy : uint8
{
    Linear,
    Exponential
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Interpolation_Strategy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ArithmeticOperations_Basic : uint8
{
    Add,
    Subtract,
    Multiply,
    Divide
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ArithmeticOperations_Basic);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ArithmeticOperations_Complete : uint8
{
    Add,
    Subtract,
    Multiply,
    Divide,
    Exponent,
    Modulus
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ArithmeticOperations_Complete);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ModifierOperation_RevocablePolicy : uint8
{
    NotRevocable,
    Revocable
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ModifierOperation_RevocablePolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_NormalizationPolicy : uint8
{
    None,
    ZeroToOne UMETA(DisplayName = "Normalize [0, 1]"),
    MinusOneToOne UMETA(DisplayName = "Normalize [-1, 1]")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_NormalizationPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EasingMethod : uint8
{
    EaseIn,
    EaseOut,
    EaseInAndOut
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EasingMethod);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RoundingMethod : uint8
{
    Ceiling,
    Floor,
    Closest UMETA(Description=">= 0.5 is ceiling and < 0.5 is floor")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RoundingMethod);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Inclusiveness : uint8
{
    Inclusive,
    Exclusive
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Inclusiveness);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ComparisonOperators : uint8
{
    EqualTo              UMETA(DisplayName = "=="),
    NotEqualTo           UMETA(DisplayName = "!="),
    GreaterThan          UMETA(DisplayName = ">"),
    GreaterThanOrEqualTo UMETA(DisplayName = ">="),
    LessThan             UMETA(DisplayName = "<"),
    LessThanOrEqualTo    UMETA(DisplayName = "<="),
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ComparisonOperators);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ExtentScope : uint8
{
    // Applies globally or with infinite extents
    Global,

    // Applies within defined bounds
    Bounded
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ExtentScope);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_PerformanceRating : uint8
{
    VeryGood,
    Good,
    Okay,
    Bad,
    VeryBad
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PerformanceRating);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ScoreSortingPolicy : uint8
{
    SmallestToLargest,
    LargestToSmallest,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ScoreSortingPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_DistanceSortingPolicy : uint8
{
    ClosestToFarthest,
    FarthestToClosest,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_DistanceSortingPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EntityFragmentRequirementPolicy : uint8
{
    EnsureAllFragments,
    IgnoreIfFragmentMissing,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EntityFragmentRequirementPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_MinMax : uint8
{
    None,
    Min UMETA(DisplayName = "Has Min"),
    Max UMETA(DisplayName = "Has Max"),
    MinMax UMETA(DisplayName = "Has Min & Max")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_MinMax);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_MinMaxCurrent : uint8
{
    Current,
    Min,
    Max,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_MinMaxCurrent);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Override : uint8
{
    Override,
    DoNotOverride,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Override);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Unique : uint8
{
    Unique,
    NotUnique,
    DoesNotExist
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Unique);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Lifetime : uint8
{
    UntilDestroyed,
    AfterOneFrame,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Lifetime);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ForwardVectorSource : uint8
{
    CharacterForward,
    Velocity,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ForwardVectorSource);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_TaskDoneBehavior : uint8
{
    Deactivate,
    Nothing,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_TaskDoneBehavior);

// --------------------------------------------------------------------------------------------------------------------
UENUM(BlueprintType)
enum class ECk_ClientServer : uint8
{
    Client,
    Server,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ClientServer);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ActorTraversalPolicy : uint8
{
    RootActorOnly,
    RootActorAndAllAttachedActors,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ActorTraversalPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ConstructionPhase : uint8
{
    DuringConstruction,
    AfterConstruction,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ConstructionPhase);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Collision : uint8
{
    NoCollision UMETA(DisplayName = "No Collision"),
    QueryOnly UMETA(DisplayName = "Query Only (No Physics Collision)"),
    PhysicsOnly UMETA(DisplayName = "Physics Only (No Query Collision)"),
    QueryAndPhysics UMETA(DisplayName = "Collision Enabled (Query and Physics)")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Collision);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Enum_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Enum_UE);

public:
    static auto
    ConvertToECollisionEnabled(
        ECk_Collision InEnum) -> ECollisionEnabled::Type;

    static auto
    ConvertFromECollisionEnabled(
        ECollisionEnabled::Type InEnum) -> ECk_Collision;
};

// --------------------------------------------------------------------------------------------------------------------
