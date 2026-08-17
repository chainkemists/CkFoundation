#pragma once

#include "CkDebugScene/CkDebugScene_Mesh.h"

#include <Components/InstancedStaticMeshComponent.h>
#include <CoreMinimal.h>
#include <SceneTypes.h>
#include <Templates/PimplPtr.h>
#include <UObject/StrongObjectPtr.h>
#include <UObject/WeakObjectPtr.h>

class UInstancedStaticMeshComponent;
class ULineBatchComponent;
class UMaterialInterface;
class UWorld;

struct FCk_DebugScene_Line
{
    FVector _From = FVector::ZeroVector;
    FVector _To = FVector::ZeroVector;
    FLinearColor _Color = FLinearColor::White;
    float _Thickness = 1.0f;
};

struct FCk_DebugScene_Label
{
    FVector _WorldPosition = FVector::ZeroVector;
    FString _Text;
    FLinearColor _Color = FLinearColor::White;
    float _Scale = 1.0f;
};

struct FCk_DebugScene_Vector
{
    FVector _Origin = FVector::ZeroVector;
    FVector _NormalizedDirection = FVector::ForwardVector;
    float _Length = 100.0f;
    float _ArrowHeadScale = 10.0f;
    FLinearColor _Color = FLinearColor::White;
};

enum class ECk_DebugScene_RenderClass : uint8
{
    Opaque,
    Transparent,
};

enum class ECk_DebugScene_WireframeMode : uint8
{
    None,
    TransparentOnly,
    All,
};

class CKDEBUGSCENE_API FCk_DebugScene_Appearance
{
  public:
    auto
    Set_BaseMaterial(UMaterialInterface* InMaterial) -> FCk_DebugScene_Appearance&;
    auto
    Set_RenderClass(ECk_DebugScene_RenderClass InRenderClass) -> FCk_DebugScene_Appearance&;
    auto
    Set_RenderClassId(uint8 InRenderClassId) -> FCk_DebugScene_Appearance&;
    auto
    Set_Color(FLinearColor InColor) -> FCk_DebugScene_Appearance&;
    auto
    Set_Opacity(float InOpacity) -> FCk_DebugScene_Appearance&;

    auto
    Get_BaseMaterial() const -> UMaterialInterface*;
    auto
    Get_RenderClass() const -> ECk_DebugScene_RenderClass;
    auto
    Get_RenderClassId() const -> uint8;
    auto
    Get_Color() const -> const FLinearColor&;
    auto
    Get_Opacity() const -> float;
    auto
    IsValid() const -> bool;

  private:
    TSharedPtr<TStrongObjectPtr<UMaterialInterface>> _BaseMaterial;
    ECk_DebugScene_RenderClass _RenderClass = ECk_DebugScene_RenderClass::Opaque;
    uint8 _RenderClassId = 0;
    FLinearColor _Color = FLinearColor::White;
};

class CKDEBUGSCENE_API FCk_DebugScene_Instance
{
  public:
    auto
    Set_Mesh(TSharedPtr<FCk_DebugScene_Mesh> InMesh) -> FCk_DebugScene_Instance&;
    auto
    Set_Transform(FTransform InTransform) -> FCk_DebugScene_Instance&;
    auto
    Set_Appearance(FCk_DebugScene_Appearance InAppearance) -> FCk_DebugScene_Instance&;
    auto
    Set_PickIdentity(uint64 InPickIdentity) -> FCk_DebugScene_Instance&;

    auto
    Get_Mesh() const -> const TSharedPtr<FCk_DebugScene_Mesh>&;
    auto
    Get_Transform() const -> const FTransform&;
    auto
    Get_Appearance() const -> const FCk_DebugScene_Appearance&;
    auto
    Get_PickIdentity() const -> uint64;
    auto
    IsValid() const -> bool;

  private:
    TSharedPtr<FCk_DebugScene_Mesh> _Mesh;
    FTransform _Transform = FTransform::Identity;
    FCk_DebugScene_Appearance _Appearance;
    uint64 _PickIdentity = 0;
};

class CKDEBUGSCENE_API FCk_DebugScene_TargetConfig
{
  public:
    auto
    Set_World(UWorld* InWorld) -> FCk_DebugScene_TargetConfig&;
    auto
    Set_MaxItems(int32 InMaxItems) -> FCk_DebugScene_TargetConfig&;
    auto
    Set_MaxInstances(int32 InMaxInstances) -> FCk_DebugScene_TargetConfig&;

    auto
    Get_World() const -> UWorld*;
    auto
    Get_MaxItems() const -> int32;
    auto
    Get_MaxInstances() const -> int32;

  private:
    TWeakObjectPtr<UWorld> _World;
    int32 _MaxItems = MAX_int32;
    int32 _MaxInstances = MAX_int32;
};

struct FCk_DebugScene_Stats
{
  public:
    auto
    Get_ItemCount() const -> int32
    {
        return _ItemCount;
    }
    auto
    Get_ComponentCount() const -> int32
    {
        return _ComponentCount;
    }
    auto
    Get_BucketCount() const -> int32
    {
        return _BucketCount;
    }
    auto
    Get_InstanceCount() const -> int32
    {
        return _InstanceCount;
    }
    auto
    Get_InstancesAdded() const -> int32
    {
        return _InstancesAdded;
    }
    auto
    Get_InstancesUpdated() const -> int32
    {
        return _InstancesUpdated;
    }
    auto
    Get_InstancesRemoved() const -> int32
    {
        return _InstancesRemoved;
    }
    auto
    Get_InstancesUnchanged() const -> int32
    {
        return _InstancesUnchanged;
    }

  private:
    friend class FCk_DebugScene_Target;
    int32 _ItemCount = 0;
    int32 _ComponentCount = 0;
    int32 _BucketCount = 0;
    int32 _InstanceCount = 0;
    int32 _InstancesAdded = 0;
    int32 _InstancesUpdated = 0;
    int32 _InstancesRemoved = 0;
    int32 _InstancesUnchanged = 0;
};

struct FCk_DebugScene_Pick
{
  public:
    auto
    Get_PickIdentity() const -> uint64
    {
        return _PickIdentity;
    }
    auto
    Get_HitPoint() const -> const FVector&
    {
        return _HitPoint;
    }
    auto
    Get_Distance() const -> float
    {
        return _Distance;
    }

  private:
    friend class FCk_DebugScene_Target;
    uint64 _PickIdentity = 0;
    FVector _HitPoint = FVector::ZeroVector;
    float _Distance = 0.0f;
};

class CKDEBUGSCENE_API FCk_DebugScene_Target final
{
  public:
    explicit FCk_DebugScene_Target(const FCk_DebugScene_TargetConfig& InConfig);
    ~FCk_DebugScene_Target();

    FCk_DebugScene_Target(const FCk_DebugScene_Target&) = delete;
    auto
    operator=(const FCk_DebugScene_Target&) -> FCk_DebugScene_Target& = delete;

  public:
    auto
    Begin_Reconcile() -> void;
    auto
    Abort_Reconcile() -> void;
    auto
    Upsert_Item(uint64 InItemKey, TArrayView<const FCk_DebugScene_Instance> InInstances) -> bool;
    auto
    End_Reconcile() -> bool;
    auto
    Reconcile_One(uint64 InItemKey, TArrayView<const FCk_DebugScene_Instance> InInstances) -> void;
    auto
    TryReconcile_One(uint64 InItemKey, TArrayView<const FCk_DebugScene_Instance> InInstances) -> bool;
    auto
    Remove_Item(uint64 InItemKey) -> void;
    auto
    HideAll() -> void;
    auto
    Set_IsDesired(bool InIsDesired) -> void;
    auto
    Set_RenderVisible(bool InIsVisible) -> void;
    auto
    Get_RenderVisible() const -> bool;

    auto
    Set_WireframeMode(ECk_DebugScene_WireframeMode InMode) -> void;
    auto
    Set_RenderClassVisible(uint8 InRenderClassId, bool InIsVisible) -> void;
    auto
    Set_ItemPickable(uint64 InItemKey, bool InIsPickable) -> void;

    auto
    Set_LineChannel(FName InChannel, TArray<FCk_DebugScene_Line> InLines) -> bool;
    auto
    Clear_LineChannel(FName InChannel) -> void;
    auto
    Set_LabelChannel(FName InChannel, TArray<FCk_DebugScene_Label> InLabels) -> bool;
    auto
    Clear_LabelChannel(FName InChannel) -> void;
    auto
    Set_VectorChannel(FName InChannel, TArray<FCk_DebugScene_Vector> InVectors) -> bool;
    auto
    Clear_VectorChannel(FName InChannel) -> void;

  public:
    auto
    Get_Stats() const -> const FCk_DebugScene_Stats&;
    auto
    Reset_FrameStats() -> void;
    auto
    Get_InstanceIds(uint64 InItemKey) const -> TArray<FPrimitiveInstanceId>;
    auto
    Get_ItemInstances(uint64 InItemKey) const -> TArray<FCk_DebugScene_Instance>;
    auto
    Get_RenderClassInstanceCount(ECk_DebugScene_RenderClass InRenderClass) const -> int32;
    auto
    Get_WireframeInstanceCount() const -> int32;
    auto
    Get_ItemBounds(uint64 InItemKey) const -> TOptional<FBox>;
    auto
    Get_ContentBounds() const -> FBox;
    auto
    Get_Components() const -> TArray<UInstancedStaticMeshComponent*>;
    auto
    Get_Lines() const -> TArray<FCk_DebugScene_Line>;
    auto
    Get_RenderedLineCount() const -> int32;
    auto
    Get_Labels() const -> TArray<FCk_DebugScene_Label>;
    auto
    Get_Vectors() const -> TArray<FCk_DebugScene_Vector>;
    auto
    Get_LineCount() const -> int32;
    auto
    Get_LabelCount() const -> int32;
    auto
    Get_VectorCount() const -> int32;
    auto
    TryPick(const FVector& InOrigin, const FVector& InDirection) const -> TOptional<FCk_DebugScene_Pick>;

#if WITH_DEV_AUTOMATION_TESTS
    auto
    Set_TestFailPrepareAfterInstances(int32 InPreparedInstanceCount) -> void;
    auto
    Set_TestFailCommitAfterInstances(int32 InCommittedInstanceCount) -> void;
#endif

  private:
    struct FImpl;
    TPimplPtr<FImpl> _Impl;
};
