#include "CkDebugScene_Target.h"

#include "CkDebugScene/CkDebugScene_Materials.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Diagnostics/CkDiagnosticVisibility.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Components/LineBatchComponent.h>
#include <Engine/StaticMesh.h>
#include <Engine/World.h>
#include <Materials/MaterialInstanceDynamic.h>
#include <Materials/MaterialInterface.h>
#include <UObject/StrongObjectPtr.h>

namespace ck_debug_scene_target
{
const auto ColorParameter = FName{TEXT("Color")};
constexpr auto ConvertToSrgb = true;
constexpr auto DoesNotAffectNavigation = false;
constexpr auto DoesNotCastShadow = false;
constexpr auto IsHiddenInGame = false;
constexpr auto IsVisible = true;
constexpr auto IsNotVisible = false;
constexpr auto IsWorldSpace = false;
constexpr auto IsPersistentBucket = false;
constexpr auto IsTransientBucket = true;

auto
IsFinite(const FLinearColor& InColor) -> bool
{
    return FMath::IsFinite(InColor.R) && FMath::IsFinite(InColor.G) && FMath::IsFinite(InColor.B) &&
           FMath::IsFinite(InColor.A);
}

auto
IsFinite(const FVector& InVector) -> bool
{
    return FMath::IsFinite(InVector.X) && FMath::IsFinite(InVector.Y) && FMath::IsFinite(InVector.Z);
}

auto
RayHitsBox(const FBox& InBounds, const FVector& InOrigin, const FVector& InDirection, double InMaxDistance) -> bool
{
    double Near = 0.0;
    double Far = InMaxDistance;
    for (const auto Axis : {0, 1, 2})
    {
        const auto Origin = static_cast<double>(InOrigin[Axis]);
        const auto Direction = static_cast<double>(InDirection[Axis]);
        const auto Minimum = static_cast<double>(InBounds.Min[Axis]);
        const auto Maximum = static_cast<double>(InBounds.Max[Axis]);
        if (FMath::IsNearlyZero(Direction))
        {
            if (Origin < Minimum || Origin > Maximum)
            {
                return false;
            }
            continue;
        }
        auto T0 = (Minimum - Origin) / Direction;
        auto T1 = (Maximum - Origin) / Direction;
        if (T0 > T1)
        {
            Swap(T0, T1);
        }
        Near = FMath::Max(Near, T0);
        Far = FMath::Min(Far, T1);
        if (Near > Far)
        {
            return false;
        }
    }
    return Far >= 0.0;
}

struct FBucketKey
{
    TSharedPtr<FCk_DebugScene_Mesh> _Mesh;
    TWeakObjectPtr<UMaterialInterface> _Material;
    ECk_DebugScene_RenderClass _RenderClass = ECk_DebugScene_RenderClass::Opaque;
    uint8 _RenderClassId = 0;
    FColor _Color;
    ECk_DebugScene_DepthPriority _DepthPriority = ECk_DebugScene_DepthPriority::World;
    int32 _TranslucencySortPriority = 0;

    auto
    operator==(const FBucketKey& InOther) const -> bool
    {
        return _Mesh == InOther._Mesh && _Material == InOther._Material && _RenderClass == InOther._RenderClass &&
               _RenderClassId == InOther._RenderClassId && _Color == InOther._Color &&
               _DepthPriority == InOther._DepthPriority &&
               _TranslucencySortPriority == InOther._TranslucencySortPriority;
    }

    friend auto
    GetTypeHash(const FBucketKey& InKey)
    -> uint32
    {
        auto Hash = GetTypeHash(InKey._Mesh.Get());
        Hash = HashCombine(Hash, GetTypeHash(InKey._Material));
        Hash = HashCombine(Hash, GetTypeHash(InKey._RenderClass));
        Hash = HashCombine(Hash, GetTypeHash(InKey._RenderClassId));
        Hash = HashCombine(Hash, GetTypeHash(InKey._Color));
        Hash = HashCombine(Hash, GetTypeHash(InKey._DepthPriority));
        return HashCombine(Hash, GetTypeHash(InKey._TranslucencySortPriority));
    }
};

struct FBucket
{
    TStrongObjectPtr<UInstancedStaticMeshComponent> _Component;
    TStrongObjectPtr<UMaterialInstanceDynamic> _BaseMid;
    TStrongObjectPtr<UMaterialInstanceDynamic> _WireMid;
    int32 _SlotCount = 0;
};

struct FSlot
{
    FBucketKey _Bucket;
    FPrimitiveInstanceId _InstanceId;
    FCk_DebugScene_Instance _Submission;
};

struct FItem
{
    TArray<FSlot> _Slots;
    bool _IsPickable = true;
    FBox _Bounds = FBox{ForceInit};
};

auto
MakeBucketKey(const FCk_DebugScene_Instance& InInstance) -> FBucketKey
{
    const auto& Appearance = InInstance.Get_Appearance();
    return FBucketKey{InInstance.Get_Mesh(), Appearance.Get_BaseMaterial(), Appearance.Get_RenderClass(),
                      Appearance.Get_RenderClassId(), Appearance.Get_Color().ToFColor(ConvertToSrgb),
                      Appearance.Get_DepthPriority(), Appearance.Get_TranslucencySortPriority()};
}

auto
GetInstanceBounds(const FCk_DebugScene_Instance& InInstance) -> FBox
{
    return InInstance.Get_Mesh()->Get_LocalBounds().TransformBy(InInstance.Get_Transform());
}
} // namespace ck_debug_scene_target

struct FCk_DebugScene_Target::FImpl
{
    TWeakObjectPtr<UWorld> _World;
    int32 _MaxItems = MAX_int32;
    int32 _MaxInstances = MAX_int32;
    bool _IsDesired = true;
    bool _RenderVisible = true;
    bool _Reconciling = false;
    bool _FrameInputValid = true;
    ECk_DebugScene_WireframeMode _WireframeMode = ECk_DebugScene_WireframeMode::None;
    TSet<uint64> _SeenItems;
    TMap<uint64, TArray<FCk_DebugScene_Instance>> _StagedItems;
    TSet<uint8> _HiddenRenderClasses;
    TMap<uint64, ck_debug_scene_target::FItem> _Items;
    TMap<ck_debug_scene_target::FBucketKey, ck_debug_scene_target::FBucket> _Buckets;
    TStrongObjectPtr<ULineBatchComponent> _Lines;
    TMap<FName, TArray<FCk_DebugScene_Line>> _LineChannels;
    TMap<FName, TArray<FCk_DebugScene_Label>> _LabelChannels;
    TMap<FName, TArray<FCk_DebugScene_Vector>> _VectorChannels;
    TMap<FName, TArray<FCk_DebugScene_Line>> _StagedLineChannels;
    TMap<FName, TArray<FCk_DebugScene_Label>> _StagedLabelChannels;
    TMap<FName, TArray<FCk_DebugScene_Vector>> _StagedVectorChannels;
    FDelegateHandle _WorldCleanupHandle;
    FCk_DebugScene_Stats _Stats;
#if WITH_DEV_AUTOMATION_TESTS
    int32 _TestFailPrepareAfterInstances = INDEX_NONE;
    int32 _TestFailCommitAfterInstances = INDEX_NONE;
#endif
};

auto
    FCk_DebugScene_Appearance::
    Set_BaseMaterial(UMaterialInterface* InMaterial)
    -> FCk_DebugScene_Appearance&
{
    _BaseMaterial = MakeShared<TStrongObjectPtr<UMaterialInterface>>(InMaterial);
    return *this;
}

auto
    FCk_DebugScene_Appearance::
    Set_RenderClass(ECk_DebugScene_RenderClass InRenderClass)
    -> FCk_DebugScene_Appearance&
{
    _RenderClass = InRenderClass;
    return *this;
}

auto
    FCk_DebugScene_Appearance::
    Set_RenderClassId(uint8 InRenderClassId)
    -> FCk_DebugScene_Appearance&
{
    _RenderClassId = InRenderClassId;
    return *this;
}

auto
    FCk_DebugScene_Appearance::
    Set_Color(FLinearColor InColor)
    -> FCk_DebugScene_Appearance&
{
    _Color = InColor;
    return *this;
}

auto
    FCk_DebugScene_Appearance::
    Set_Opacity(float InOpacity)
    -> FCk_DebugScene_Appearance&
{
    _Color.A = FMath::Clamp(InOpacity, 0.0f, 1.0f);
    return *this;
}

auto
    FCk_DebugScene_Appearance::
    Set_DepthPriority(ECk_DebugScene_DepthPriority InDepthPriority)
    -> FCk_DebugScene_Appearance&
{
    _DepthPriority = InDepthPriority;
    return *this;
}

auto
    FCk_DebugScene_Appearance::
    Set_TranslucencySortPriority(int32 InTranslucencySortPriority)
    -> FCk_DebugScene_Appearance&
{
    _TranslucencySortPriority = InTranslucencySortPriority;
    return *this;
}

auto
    FCk_DebugScene_Appearance::
    Get_BaseMaterial() const
    -> UMaterialInterface*
{
    return _BaseMaterial.IsValid() ? _BaseMaterial->Get() : nullptr;
}
auto
    FCk_DebugScene_Appearance::
    Get_RenderClass() const
    -> ECk_DebugScene_RenderClass
{
    return _RenderClass;
}
auto
    FCk_DebugScene_Appearance::
    Get_RenderClassId() const
    -> uint8
{
    return _RenderClassId;
}
auto
    FCk_DebugScene_Appearance::
    Get_Color() const
    -> const FLinearColor&
{
    return _Color;
}
auto
    FCk_DebugScene_Appearance::
    Get_Opacity() const
    -> float
{
    return _Color.A;
}

auto
    FCk_DebugScene_Appearance::
    Get_DepthPriority() const
    -> ECk_DebugScene_DepthPriority
{
    return _DepthPriority;
}

auto
    FCk_DebugScene_Appearance::
    Get_TranslucencySortPriority() const
    -> int32
{
    return _TranslucencySortPriority;
}
auto
FCk_DebugScene_Appearance::
    IsValid() const
    -> bool
{
    return ck::debug_scene::materials::Is_IsmCompatible(Get_BaseMaterial()) &&
           ck_debug_scene_target::IsFinite(_Color);
}

auto
    FCk_DebugScene_Instance::
    Set_Mesh(TSharedPtr<FCk_DebugScene_Mesh> InMesh)
    -> FCk_DebugScene_Instance&
{
    _Mesh = MoveTemp(InMesh);
    return *this;
}
auto
    FCk_DebugScene_Instance::
    Set_Transform(FTransform InTransform)
    -> FCk_DebugScene_Instance&
{
    _Transform = MoveTemp(InTransform);
    return *this;
}
auto
    FCk_DebugScene_Instance::
    Set_Appearance(FCk_DebugScene_Appearance InAppearance)
    -> FCk_DebugScene_Instance&
{
    _Appearance = MoveTemp(InAppearance);
    return *this;
}
auto
    FCk_DebugScene_Instance::
    Set_PickIdentity(uint64 InPickIdentity)
    -> FCk_DebugScene_Instance&
{
    _PickIdentity = InPickIdentity;
    return *this;
}
auto
    FCk_DebugScene_Instance::
    Get_Mesh() const
    -> const TSharedPtr<FCk_DebugScene_Mesh>&
{
    return _Mesh;
}
auto
    FCk_DebugScene_Instance::
    Get_Transform() const
    -> const FTransform&
{
    return _Transform;
}
auto
    FCk_DebugScene_Instance::
    Get_Appearance() const
    -> const FCk_DebugScene_Appearance&
{
    return _Appearance;
}
auto
    FCk_DebugScene_Instance::
    Get_PickIdentity() const
    -> uint64
{
    return _PickIdentity;
}
auto
    FCk_DebugScene_Instance::
    IsValid() const
    -> bool
{
    return _Mesh.IsValid() && ck::IsValid(_Mesh->Get_StaticMesh()) && _Appearance.IsValid() &&
           NOT _Transform.ContainsNaN();
}

auto
    FCk_DebugScene_TargetConfig::
    Set_World(UWorld* InWorld)
    -> FCk_DebugScene_TargetConfig&
{
    _World = InWorld;
    return *this;
}
auto
    FCk_DebugScene_TargetConfig::
    Set_MaxItems(int32 InMaxItems)
    -> FCk_DebugScene_TargetConfig&
{
    _MaxItems = InMaxItems;
    return *this;
}
auto
    FCk_DebugScene_TargetConfig::
    Set_MaxInstances(int32 InMaxInstances)
    -> FCk_DebugScene_TargetConfig&
{
    _MaxInstances = InMaxInstances;
    return *this;
}
auto
    FCk_DebugScene_TargetConfig::
    Get_World() const
    -> UWorld*
{
    return _World.Get();
}
auto
    FCk_DebugScene_TargetConfig::
    Get_MaxItems() const
    -> int32
{
    return _MaxItems;
}
auto
    FCk_DebugScene_TargetConfig::
    Get_MaxInstances() const
    -> int32
{
    return _MaxInstances;
}

FCk_DebugScene_Target::
    FCk_DebugScene_Target(
        const FCk_DebugScene_TargetConfig& InConfig)
    : _Impl(MakePimpl<FImpl>())
{
    _Impl->_World = InConfig.Get_World();
    _Impl->_MaxItems = InConfig.Get_MaxItems();
    _Impl->_MaxInstances = InConfig.Get_MaxInstances();

    const auto ConfigIsValid = ck::IsValid(_Impl->_World.Get()) && _Impl->_MaxItems >= 0 && _Impl->_MaxInstances >= 0;
    CK_ENSURE_IF_NOT(ConfigIsValid, TEXT("CkDebugScene rejected an invalid target configuration"))
    {
        return;
    }

    _Impl->_WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddLambda(
        [this](UWorld* InWorld, bool, bool) -> void
        {
            if (InWorld == _Impl->_World.Get())
            {
                Abort_Reconcile();
                HideAll();
                _Impl->_IsDesired = false;
                _Impl->_World.Reset();
            }
        });
}

FCk_DebugScene_Target::
    ~FCk_DebugScene_Target()
{
    HideAll();
    FWorldDelegates::OnWorldCleanup.Remove(_Impl->_WorldCleanupHandle);
}

namespace ck_debug_scene_target
{
auto
DestroyBucket(FBucket& InOutBucket) -> void
{
    if (auto* Component = InOutBucket._Component.Get(); ck::IsValid(Component))
    {
        Component->DestroyComponent();
    }

    InOutBucket._WireMid.Reset();
    InOutBucket._BaseMid.Reset();
    InOutBucket._Component.Reset();
    InOutBucket._SlotCount = 0;
}

auto
IsWireframeEnabled(ECk_DebugScene_WireframeMode InMode, ECk_DebugScene_RenderClass InRenderClass) -> bool
{
    return InMode == ECk_DebugScene_WireframeMode::All || (InMode == ECk_DebugScene_WireframeMode::TransparentOnly &&
                                                           InRenderClass == ECk_DebugScene_RenderClass::Transparent);
}

auto
ApplyWireframeBucket(ECk_DebugScene_WireframeMode InMode, const FBucketKey& InKey, FBucket& InOutBucket) -> void
{
    auto* Component = InOutBucket._Component.Get();
    if (ck::Is_NOT_Valid(Component))
    {
        return;
    }
    Component->SetMaterial(0, InOutBucket._BaseMid.Get());
    if (NOT IsWireframeEnabled(InMode, InKey._RenderClass))
    {
        Component->SetOverlayMaterial(nullptr);
        return;
    }
    if (ck::Is_NOT_Valid(InOutBucket._WireMid.Get()))
    {
        auto* Wireframe = ck::debug_scene::materials::TryGet_Wireframe();
        const auto WireframeIsValid = ck::IsValid(Wireframe);
        CK_ENSURE_IF_NOT(WireframeIsValid, TEXT("CkDebugScene could not load the engine wireframe material"))
        {
            Component->SetOverlayMaterial(nullptr);
            return;
        }
        InOutBucket._WireMid.Reset(UMaterialInstanceDynamic::Create(Wireframe, Component));
    }
    if (ck::IsValid(InOutBucket._WireMid.Get()))
    {
        auto WireColor = FLinearColor{InKey._Color};
        WireColor.A = 1.0f;
        InOutBucket._WireMid->SetVectorParameterValue(ColorParameter, WireColor);
        if (InMode == ECk_DebugScene_WireframeMode::All)
        {
            Component->SetMaterial(0, InOutBucket._WireMid.Get());
            Component->SetOverlayMaterial(nullptr);
        }
        else
        {
            Component->SetOverlayMaterial(InOutBucket._WireMid.Get());
        }
    }
}

auto
TryCreateBucket(UWorld* InWorld, const FBucketKey& InKey, const FCk_DebugScene_Instance& InSubmission,
                bool InInitialVisibility, FBucket& OutBucket) -> bool
{
    auto* Component = NewObject<UInstancedStaticMeshComponent>(
        InWorld, MakeUniqueObjectName(InWorld, UInstancedStaticMeshComponent::StaticClass(), TEXT("CkDebugSceneIsm")));
    const auto ComponentWasCreated = ck::IsValid(Component);
    CK_ENSURE_IF_NOT(ComponentWasCreated, TEXT("CkDebugScene failed to create an instanced mesh component"))
    {
        return false;
    }

    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetCanEverAffectNavigation(DoesNotAffectNavigation);
    Component->SetCastShadow(DoesNotCastShadow);
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetWorldLocation(FVector::ZeroVector);
    Component->SetHiddenInGame(IsHiddenInGame);
    Component->SetVisibility(InInitialVisibility);
    Component->SetDepthPriorityGroup(InKey._DepthPriority == ECk_DebugScene_DepthPriority::Foreground
                                         ? SDPG_Foreground
                                         : SDPG_World);
    Component->SetTranslucentSortPriority(InKey._TranslucencySortPriority);
    Component->SetStaticMesh(InSubmission.Get_Mesh()->Get_StaticMesh());
    Component->RegisterComponentWithWorld(InWorld);

    const auto ComponentRegistered = Component->IsRegistered();
    CK_ENSURE_IF_NOT(ComponentRegistered, TEXT("CkDebugScene failed to register an instanced mesh component"))
    {
        Component->DestroyComponent();
        return false;
    }

    auto* Mid = UMaterialInstanceDynamic::Create(InSubmission.Get_Appearance().Get_BaseMaterial(), Component);
    const auto MidWasCreated = ck::IsValid(Mid);
    CK_ENSURE_IF_NOT(MidWasCreated, TEXT("CkDebugScene failed to create an instance material"))
    {
        Component->DestroyComponent();
        return false;
    }

    Mid->SetVectorParameterValue(ColorParameter, InSubmission.Get_Appearance().Get_Color());
    Component->SetMaterial(0, Mid);
    OutBucket._Component.Reset(Component);
    OutBucket._BaseMid.Reset(Mid);
    return true;
}

auto
DestroyStagedBuckets(TMap<FBucketKey, FBucket>& InOutBuckets) -> void
{
    for (auto& [Key, Bucket] : InOutBuckets)
    {
        DestroyBucket(Bucket);
    }
    InOutBuckets.Reset();
}
} // namespace ck_debug_scene_target

auto
    FCk_DebugScene_Target::
    Begin_Reconcile()
    -> void
{
    _Impl->_Reconciling = true;
    _Impl->_FrameInputValid = true;
    _Impl->_SeenItems.Reset();
    _Impl->_StagedItems.Reset();
    _Impl->_StagedLineChannels = _Impl->_LineChannels;
    _Impl->_StagedLabelChannels = _Impl->_LabelChannels;
    _Impl->_StagedVectorChannels = _Impl->_VectorChannels;

    const auto IsRuntimeVisible = NOT ck::diagnostic_visibility::Is_HiddenForStreamerMode();
    for (auto& [Key, Bucket] : _Impl->_Buckets)
    {
        if (ck::IsValid(Bucket._Component.Get()))
        {
            Bucket._Component->SetVisibility(_Impl->_IsDesired && _Impl->_RenderVisible && IsRuntimeVisible &&
                                             NOT _Impl->_HiddenRenderClasses.Contains(Key._RenderClassId));
        }
    }
    if (auto* Lines = _Impl->_Lines.Get(); ck::IsValid(Lines))
    {
        Lines->SetVisibility(_Impl->_IsDesired && _Impl->_RenderVisible && IsRuntimeVisible);
    }
}

auto
    FCk_DebugScene_Target::
    Abort_Reconcile()
    -> void
{
    if (NOT _Impl->_Reconciling)
    {
        return;
    }
    _Impl->_SeenItems.Reset();
    _Impl->_StagedItems.Reset();
    _Impl->_StagedLineChannels.Reset();
    _Impl->_StagedLabelChannels.Reset();
    _Impl->_StagedVectorChannels.Reset();
    _Impl->_Reconciling = false;
}

auto
    FCk_DebugScene_Target::
    Upsert_Item(uint64 InItemKey, TArrayView<const FCk_DebugScene_Instance> InInstances)
    -> bool
{
    if (NOT _Impl->_Reconciling)
    {
        return TryReconcile_One(InItemKey, InInstances);
    }

    auto IsValidSubmission = NOT InInstances.IsEmpty();
    for (const auto& Instance : InInstances)
    {
        IsValidSubmission &= Instance.IsValid();
    }

    CK_ENSURE_IF_NOT(IsValidSubmission, TEXT("CkDebugScene rejected invalid item submission"))
    {
        _Impl->_FrameInputValid = false;
        return false;
    }

    _Impl->_SeenItems.Add(InItemKey);
    _Impl->_StagedItems.Add(InItemKey, TArray<FCk_DebugScene_Instance>{InInstances});
    return true;
}

auto
    FCk_DebugScene_Target::
    End_Reconcile()
    -> bool
{
    if (NOT _Impl->_Reconciling)
    {
        return false;
    }

    auto TotalInstances = 0;
    for (const auto& [ItemKey, Instances] : _Impl->_StagedItems)
    {
        TotalInstances += Instances.Num();
    }

    const auto FitsFrameCapacity = _Impl->_FrameInputValid && _Impl->_StagedItems.Num() <= _Impl->_MaxItems &&
                                   TotalInstances <= _Impl->_MaxInstances;
    CK_ENSURE_IF_NOT(FitsFrameCapacity, TEXT("CkDebugScene rejected an over-capacity reconcile frame"))
    {
        Abort_Reconcile();
        return false;
    }

    struct FDeferredUpdate
    {
        ck_debug_scene_target::FSlot _OldSlot;
        FCk_DebugScene_Instance _Submission;
    };
    struct FDeferredAddition
    {
        uint64 _ItemKey;
        int32 _SlotIndex;
        ck_debug_scene_target::FBucketKey _Key;
        FCk_DebugScene_Instance _Submission;
    };
    auto PreparedBuckets = TMap<ck_debug_scene_target::FBucketKey, ck_debug_scene_target::FBucket>{};
    auto PreparedItems = TMap<uint64, ck_debug_scene_target::FItem>{};
    auto DeferredUpdates = TArray<FDeferredUpdate>{};
    auto DeferredAdditions = TArray<FDeferredAddition>{};
    auto DeferredReleases = TArray<ck_debug_scene_target::FSlot>{};
    auto UnchangedInstances = 0;
    auto DidPrepare = true;
    auto PreparedInstanceCount = 0;
    for (const auto& [ItemKey, Instances] : _Impl->_StagedItems)
    {
        auto PreparedItem = ck_debug_scene_target::FItem{};
        if (const auto* ExistingItem = _Impl->_Items.Find(ItemKey))
        {
            PreparedItem._IsPickable = ExistingItem->_IsPickable;
        }

        const auto* ExistingItem = _Impl->_Items.Find(ItemKey);
        auto SlotsMatch = ExistingItem != nullptr && ExistingItem->_Slots.Num() == Instances.Num();
        if (SlotsMatch)
        {
            for (auto Index = 0; Index < Instances.Num(); ++Index)
            {
                const auto& OldSlot = ExistingItem->_Slots[Index];
                const auto& Submission = Instances[Index];
                const auto* Bucket = _Impl->_Buckets.Find(OldSlot._Bucket);
                SlotsMatch &= OldSlot._Bucket == ck_debug_scene_target::MakeBucketKey(Submission) &&
                              Bucket != nullptr && ck::IsValid(Bucket->_Component.Get()) &&
                              Bucket->_Component->IsValidId(OldSlot._InstanceId);
                if (NOT SlotsMatch)
                {
                    break;
                }
            }
        }

        if (SlotsMatch)
        {
            for (auto Index = 0; Index < Instances.Num(); ++Index)
            {
                const auto& OldSlot = ExistingItem->_Slots[Index];
                const auto& Submission = Instances[Index];
                auto NewSlot = OldSlot;
                NewSlot._Submission = Submission;
                PreparedItem._Slots.Emplace(MoveTemp(NewSlot));
                PreparedItem._Bounds += ck_debug_scene_target::GetInstanceBounds(Submission);
                if (OldSlot._Submission.Get_Transform().Equals(Submission.Get_Transform()) &&
                    OldSlot._Submission.Get_PickIdentity() == Submission.Get_PickIdentity())
                {
                    ++UnchangedInstances;
                }
                else
                {
                    DeferredUpdates.Emplace(FDeferredUpdate{OldSlot, Submission});
                }
            }
        }
        else
        {
            if (ExistingItem != nullptr)
            {
                DeferredReleases.Append(ExistingItem->_Slots);
            }
            for (const auto& Submission : Instances)
            {
                const auto Key = ck_debug_scene_target::MakeBucketKey(Submission);
                if (_Impl->_Buckets.Find(Key) == nullptr && PreparedBuckets.Find(Key) == nullptr)
                {
                    auto NewBucket = ck_debug_scene_target::FBucket{};
                    DidPrepare =
                        ck_debug_scene_target::TryCreateBucket(
                            _Impl->_World.Get(), Key, Submission, ck_debug_scene_target::IsPersistentBucket, NewBucket);
                    if (NOT DidPrepare)
                    {
                        break;
                    }
                    PreparedBuckets.Add(Key, MoveTemp(NewBucket));
                }
#if WITH_DEV_AUTOMATION_TESTS
                if (_Impl->_TestFailPrepareAfterInstances != INDEX_NONE &&
                    PreparedInstanceCount >= _Impl->_TestFailPrepareAfterInstances)
                {
                    _Impl->_TestFailPrepareAfterInstances = INDEX_NONE;
                    DidPrepare = false;
                    break;
                }
#endif
                PreparedItem._Slots.Emplace(
                    ck_debug_scene_target::FSlot{Key, FPrimitiveInstanceId{INDEX_NONE}, Submission});
                DeferredAdditions.Emplace(FDeferredAddition{ItemKey, PreparedItem._Slots.Num() - 1, Key, Submission});
                PreparedItem._Bounds += ck_debug_scene_target::GetInstanceBounds(Submission);
                ++PreparedInstanceCount;
            }
        }

        if (NOT DidPrepare)
        {
            break;
        }
        PreparedItems.Add(ItemKey, MoveTemp(PreparedItem));
    }

    if (NOT DidPrepare)
    {
        CK_ENSURE_IF_NOT(DidPrepare, TEXT("CkDebugScene failed to prepare a reconcile frame"))
        {
        }
        ck_debug_scene_target::DestroyStagedBuckets(PreparedBuckets);
        Abort_Reconcile();
        return false;
    }

    auto FrameCreatedLines = TStrongObjectPtr<ULineBatchComponent>{};
    if (NOT _Impl->_StagedLineChannels.IsEmpty() && ck::Is_NOT_Valid(_Impl->_Lines.Get()))
    {
        auto* World = _Impl->_World.Get();
        auto* Lines = NewObject<ULineBatchComponent>(
            World, MakeUniqueObjectName(World, ULineBatchComponent::StaticClass(), TEXT("CkDebugSceneLines")));
        const auto LinesWereCreated = ck::IsValid(Lines);
        CK_ENSURE_IF_NOT(LinesWereCreated, TEXT("CkDebugScene failed to prepare a line batch component"))
        {
            ck_debug_scene_target::DestroyStagedBuckets(PreparedBuckets);
            Abort_Reconcile();
            return false;
        }
        Lines->DefaultLifeTime = 0.0f;
        Lines->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Lines->SetCanEverAffectNavigation(ck_debug_scene_target::DoesNotAffectNavigation);
        Lines->SetCastShadow(ck_debug_scene_target::DoesNotCastShadow);
        Lines->SetMobility(EComponentMobility::Movable);
        Lines->SetWorldLocation(FVector::ZeroVector);
        Lines->SetVisibility(ck_debug_scene_target::IsNotVisible);
        Lines->RegisterComponentWithWorld(World);
        if (NOT Lines->IsRegistered())
        {
            Lines->DestroyComponent();
            ck_debug_scene_target::DestroyStagedBuckets(PreparedBuckets);
            Abort_Reconcile();
            return false;
        }
        FrameCreatedLines.Reset(Lines);
    }

    for (const auto& [OldItemKey, OldItem] : _Impl->_Items)
    {
        if (NOT _Impl->_StagedItems.Contains(OldItemKey))
        {
            DeferredReleases.Append(OldItem._Slots);
        }
    }

    auto PreparedBucketKeys = TArray<ck_debug_scene_target::FBucketKey>{};
    for (auto& [Key, Bucket] : PreparedBuckets)
    {
        PreparedBucketKeys.Emplace(Key);
        _Impl->_Buckets.Add(Key, MoveTemp(Bucket));
        ck_debug_scene_target::ApplyWireframeBucket(_Impl->_WireframeMode, Key, _Impl->_Buckets.FindChecked(Key));
    }

    struct FCommittedAddition
    {
        ck_debug_scene_target::FBucketKey _Key;
        FPrimitiveInstanceId _InstanceId;
    };
    auto CommittedAdditions = TArray<FCommittedAddition>{};
    auto DidCommitAdditions = true;
    for (const auto& Addition : DeferredAdditions)
    {
#if WITH_DEV_AUTOMATION_TESTS
        if (_Impl->_TestFailCommitAfterInstances != INDEX_NONE &&
            CommittedAdditions.Num() >= _Impl->_TestFailCommitAfterInstances)
        {
            _Impl->_TestFailCommitAfterInstances = INDEX_NONE;
            DidCommitAdditions = false;
            break;
        }
#endif
        auto& Bucket = _Impl->_Buckets.FindChecked(Addition._Key);
        auto* Component = Bucket._Component.Get();
        const auto InstanceId = Component->AddInstanceById(
            Addition._Submission.Get_Transform(), ck_debug_scene_target::IsWorldSpace);
        const auto InstanceWasAdded = Component->IsValidId(InstanceId);
        CK_ENSURE_IF_NOT(InstanceWasAdded, TEXT("CkDebugScene failed to commit a prepared instance"))
        {
            DidCommitAdditions = false;
            break;
        }
        Bucket._SlotCount++;
        CommittedAdditions.Emplace(FCommittedAddition{Addition._Key, InstanceId});
        PreparedItems.FindChecked(Addition._ItemKey)._Slots[Addition._SlotIndex]._InstanceId = InstanceId;
    }

    if (NOT DidCommitAdditions)
    {
        CK_ENSURE_IF_NOT(DidCommitAdditions, TEXT("CkDebugScene failed to commit a prepared reconcile frame"))
        {
        }
        for (const auto& Addition : CommittedAdditions)
        {
            auto& Bucket = _Impl->_Buckets.FindChecked(Addition._Key);
            if (ck::IsValid(Bucket._Component.Get()) && Bucket._Component->IsValidId(Addition._InstanceId))
            {
                Bucket._Component->RemoveInstanceById(Addition._InstanceId);
                Bucket._SlotCount = FMath::Max(0, Bucket._SlotCount - 1);
            }
        }
        for (const auto& Key : PreparedBucketKeys)
        {
            auto& Bucket = _Impl->_Buckets.FindChecked(Key);
            ck_debug_scene_target::DestroyBucket(Bucket);
            _Impl->_Buckets.Remove(Key);
        }
        if (FrameCreatedLines.IsValid())
        {
            FrameCreatedLines->DestroyComponent();
            FrameCreatedLines.Reset();
        }
        Abort_Reconcile();
        return false;
    }

    for (const auto& Update : DeferredUpdates)
    {
        auto* Component = _Impl->_Buckets.FindChecked(Update._OldSlot._Bucket)._Component.Get();
        Component->UpdateInstanceTransformById(
            Update._OldSlot._InstanceId, Update._Submission.Get_Transform(), ck_debug_scene_target::IsWorldSpace);
    }
    for (const auto& Slot : DeferredReleases)
    {
        auto* Bucket = _Impl->_Buckets.Find(Slot._Bucket);
        if (Bucket != nullptr && ck::IsValid(Bucket->_Component.Get()) &&
            Bucket->_Component->IsValidId(Slot._InstanceId))
        {
            Bucket->_Component->RemoveInstanceById(Slot._InstanceId);
            Bucket->_SlotCount = FMath::Max(0, Bucket->_SlotCount - 1);
        }
    }
    _Impl->_Items = MoveTemp(PreparedItems);
    if (FrameCreatedLines.IsValid())
    {
        _Impl->_Lines = MoveTemp(FrameCreatedLines);
        _Impl->_Lines->SetVisibility(_Impl->_IsDesired && _Impl->_RenderVisible &&
                                     NOT ck::diagnostic_visibility::Is_HiddenForStreamerMode());
    }
    _Impl->_LineChannels = MoveTemp(_Impl->_StagedLineChannels);
    _Impl->_LabelChannels = MoveTemp(_Impl->_StagedLabelChannels);
    _Impl->_VectorChannels = MoveTemp(_Impl->_StagedVectorChannels);
    if (auto* Lines = _Impl->_Lines.Get(); ck::IsValid(Lines))
    {
        Lines->Flush();
        for (const auto& [Channel, ChannelLines] : _Impl->_LineChannels)
        {
            auto Batched = TArray<FBatchedLine>{};
            Batched.Reserve(ChannelLines.Num());
            for (const auto& Line : ChannelLines)
            {
                Batched.Emplace(Line._From, Line._To, Line._Color, 0.0f, Line._Thickness, SDPG_World);
            }
            Lines->DrawLines(Batched);
        }
    }
    auto EmptyKeys = TArray<ck_debug_scene_target::FBucketKey>{};
    for (const auto& [Key, Bucket] : _Impl->_Buckets)
    {
        if (Bucket._SlotCount == 0)
        {
            EmptyKeys.Emplace(Key);
        }
    }
    for (const auto& Key : EmptyKeys)
    {
        auto& Bucket = _Impl->_Buckets.FindChecked(Key);
        ck_debug_scene_target::DestroyBucket(Bucket);
        _Impl->_Buckets.Remove(Key);
    }
    Reset_FrameStats();
    _Impl->_Stats._InstancesAdded = DeferredAdditions.Num();
    _Impl->_Stats._InstancesUpdated = DeferredUpdates.Num();
    _Impl->_Stats._InstancesRemoved = DeferredReleases.Num();
    _Impl->_Stats._InstancesUnchanged = UnchangedInstances;
    _Impl->_Stats._ItemCount = _Impl->_Items.Num();
    _Impl->_Stats._BucketCount = _Impl->_Buckets.Num();
    _Impl->_Stats._ComponentCount = Get_Components().Num();
    _Impl->_Stats._InstanceCount = TotalInstances;

    for (auto& [Key, Bucket] : _Impl->_Buckets)
    {
        if (ck::IsValid(Bucket._Component.Get()))
        {
            Bucket._Component->SetVisibility(_Impl->_IsDesired && _Impl->_RenderVisible &&
                                              NOT ck::diagnostic_visibility::Is_HiddenForStreamerMode() &&
                                              NOT _Impl->_HiddenRenderClasses.Contains(Key._RenderClassId));
        }
    }
    Set_WireframeMode(_Impl->_WireframeMode);

    _Impl->_SeenItems.Reset();
    _Impl->_StagedItems.Reset();
    _Impl->_Reconciling = false;
    return true;
}

auto
    FCk_DebugScene_Target::
    Reconcile_One(uint64 InItemKey, TArrayView<const FCk_DebugScene_Instance> InInstances)
    -> void
{
    TryReconcile_One(InItemKey, InInstances);
}

auto
    FCk_DebugScene_Target::
    TryReconcile_One(uint64 InItemKey, TArrayView<const FCk_DebugScene_Instance> InInstances)
    -> bool
{
    auto* World = _Impl->_World.Get();
    auto InputIsValid = ck::IsValid(World) && NOT InInstances.IsEmpty() && InInstances.Num() <= _Impl->_MaxInstances;
    for (const auto& Instance : InInstances)
    {
        InputIsValid &= Instance.IsValid();
    }

    auto* Item = _Impl->_Items.Find(InItemKey);
    const auto IsNewItem = Item == nullptr;
    const auto FitsItemCapacity = NOT IsNewItem || _Impl->_Items.Num() < _Impl->_MaxItems;
    InputIsValid &= FitsItemCapacity;

    const auto ReplacedInstanceCount = Item != nullptr ? Item->_Slots.Num() : 0;
    InputIsValid &= _Impl->_Stats._InstanceCount - ReplacedInstanceCount + InInstances.Num() <= _Impl->_MaxInstances;

    CK_ENSURE_IF_NOT(InputIsValid, TEXT("CkDebugScene rejected invalid item submission"))
    {
        return false;
    }

    auto NewKeys = TArray<ck_debug_scene_target::FBucketKey, TInlineAllocator<4>>{};
    NewKeys.Reserve(InInstances.Num());
    for (const auto& Instance : InInstances)
    {
        NewKeys.Emplace(ck_debug_scene_target::MakeBucketKey(Instance));
    }

    auto SlotsStillMatch = Item != nullptr && Item->_Slots.Num() == InInstances.Num();
    if (SlotsStillMatch)
    {
        for (auto Index = 0; Index < InInstances.Num(); ++Index)
        {
            const auto* Bucket = _Impl->_Buckets.Find(Item->_Slots[Index]._Bucket);
            SlotsStillMatch &= Item->_Slots[Index]._Bucket == NewKeys[Index] && Bucket != nullptr &&
                               ck::IsValid(Bucket->_Component.Get()) &&
                               Bucket->_Component->IsValidId(Item->_Slots[Index]._InstanceId);
            if (NOT SlotsStillMatch)
            {
                break;
            }
        }
    }

    if (SlotsStillMatch)
    {
        Item->_Bounds = FBox{ForceInit};
        for (auto Index = 0; Index < InInstances.Num(); ++Index)
        {
            auto& Slot = Item->_Slots[Index];
            const auto& Submission = InInstances[Index];
            auto* Component = _Impl->_Buckets.FindChecked(Slot._Bucket)._Component.Get();
            if (Slot._Submission.Get_Transform().Equals(Submission.Get_Transform()) &&
                Slot._Submission.Get_PickIdentity() == Submission.Get_PickIdentity())
            {
                ++_Impl->_Stats._InstancesUnchanged;
            }
            else
            {
                Component->UpdateInstanceTransformById(
                    Slot._InstanceId, Submission.Get_Transform(), ck_debug_scene_target::IsWorldSpace);
                ++_Impl->_Stats._InstancesUpdated;
            }

            Slot._Submission = Submission;
            Item->_Bounds += ck_debug_scene_target::GetInstanceBounds(Submission);
        }
    }
    else
    {
        struct FStagedSlot
        {
            ck_debug_scene_target::FBucketKey _Key;
            FPrimitiveInstanceId _InstanceId;
            FCk_DebugScene_Instance _Submission;
            bool _UsesExistingBucket = false;
        };

        auto StagedBuckets = TMap<ck_debug_scene_target::FBucketKey, ck_debug_scene_target::FBucket>{};
        auto StagedSlots = TArray<FStagedSlot, TInlineAllocator<4>>{};
        StagedSlots.Reserve(InInstances.Num());
        auto StagedBounds = FBox{ForceInit};
        auto DidStage = true;

        for (auto Index = 0; Index < InInstances.Num(); ++Index)
        {
            const auto& Submission = InInstances[Index];
            const auto& Key = NewKeys[Index];
            auto* ExistingBucket = _Impl->_Buckets.Find(Key);
            auto* StagedBucket = StagedBuckets.Find(Key);

            if (ExistingBucket == nullptr && StagedBucket == nullptr)
            {
                auto NewBucket = ck_debug_scene_target::FBucket{};
                DidStage = ck_debug_scene_target::TryCreateBucket(
                    World, Key, Submission, ck_debug_scene_target::IsTransientBucket, NewBucket);
                if (NOT DidStage)
                {
                    break;
                }
                StagedBucket = &StagedBuckets.Add(Key, MoveTemp(NewBucket));
            }

            auto* Bucket = ExistingBucket != nullptr ? ExistingBucket : StagedBucket;
            auto* Component = Bucket != nullptr ? Bucket->_Component.Get() : nullptr;
            DidStage &= ck::IsValid(Component);
            if (NOT DidStage)
            {
                break;
            }

            const auto IsVisible = NOT _Impl->_HiddenRenderClasses.Contains(Key._RenderClassId) && _Impl->_IsDesired &&
                                   _Impl->_RenderVisible &&
                                   NOT ck::diagnostic_visibility::Is_HiddenForStreamerMode();
            Component->SetVisibility(IsVisible);
            const auto InstanceId = Component->AddInstanceById(
                Submission.Get_Transform(), ck_debug_scene_target::IsWorldSpace);
            DidStage &= Component->IsValidId(InstanceId);
            if (NOT DidStage)
            {
                break;
            }

            StagedSlots.Emplace(FStagedSlot{Key, InstanceId, Submission, ExistingBucket != nullptr});
            StagedBounds += ck_debug_scene_target::GetInstanceBounds(Submission);
        }

        if (NOT DidStage)
        {
            CK_ENSURE_IF_NOT(DidStage, TEXT("CkDebugScene failed to stage an item representation"))
            {
            }
            for (const auto& Slot : StagedSlots)
            {
                auto* Bucket =
                    Slot._UsesExistingBucket ? _Impl->_Buckets.Find(Slot._Key) : StagedBuckets.Find(Slot._Key);
                if (Bucket != nullptr && ck::IsValid(Bucket->_Component.Get()) &&
                    Bucket->_Component->IsValidId(Slot._InstanceId))
                {
                    Bucket->_Component->RemoveInstanceById(Slot._InstanceId);
                }
            }
            ck_debug_scene_target::DestroyStagedBuckets(StagedBuckets);
            return false;
        }

        const auto WasPickable = Item != nullptr ? Item->_IsPickable : true;
        for (const auto& Slot : StagedSlots)
        {
            if (Slot._UsesExistingBucket)
            {
                ++_Impl->_Buckets.FindChecked(Slot._Key)._SlotCount;
            }
        }
        if (Item != nullptr)
        {
            Remove_Item(InItemKey);
        }
        for (auto& [Key, Bucket] : StagedBuckets)
        {
            _Impl->_Buckets.Add(Key, MoveTemp(Bucket));
            ck_debug_scene_target::ApplyWireframeBucket(_Impl->_WireframeMode, Key, _Impl->_Buckets.FindChecked(Key));
        }

        auto& RebuiltItem = _Impl->_Items.FindOrAdd(InItemKey);
        RebuiltItem._IsPickable = WasPickable;
        RebuiltItem._Bounds = StagedBounds;
        for (const auto& Slot : StagedSlots)
        {
            RebuiltItem._Slots.Emplace(ck_debug_scene_target::FSlot{Slot._Key, Slot._InstanceId, Slot._Submission});
            if (NOT Slot._UsesExistingBucket)
            {
                ++_Impl->_Buckets.FindChecked(Slot._Key)._SlotCount;
            }
            ++_Impl->_Stats._InstancesAdded;
        }
    }

    _Impl->_Stats._ItemCount = _Impl->_Items.Num();
    _Impl->_Stats._BucketCount = _Impl->_Buckets.Num();
    _Impl->_Stats._ComponentCount = _Impl->_Buckets.Num();
    _Impl->_Stats._InstanceCount = 0;
    for (const auto& [Key, Bucket] : _Impl->_Buckets)
    {
        _Impl->_Stats._InstanceCount += Bucket._SlotCount;
    }
    return true;
}

auto
    FCk_DebugScene_Target::
    Remove_Item(uint64 InItemKey)
    -> void
{
    auto* Item = _Impl->_Items.Find(InItemKey);
    if (Item == nullptr)
    {
        return;
    }

    for (const auto& Slot : Item->_Slots)
    {
        auto* Bucket = _Impl->_Buckets.Find(Slot._Bucket);
        if (Bucket == nullptr || ck::Is_NOT_Valid(Bucket->_Component.Get()) ||
            NOT Bucket->_Component->IsValidId(Slot._InstanceId))
        {
            continue;
        }

        Bucket->_Component->RemoveInstanceById(Slot._InstanceId);
        Bucket->_SlotCount = FMath::Max(0, Bucket->_SlotCount - 1);
        ++_Impl->_Stats._InstancesRemoved;
    }

    _Impl->_Items.Remove(InItemKey);
    auto EmptyKeys = TArray<ck_debug_scene_target::FBucketKey>{};
    for (const auto& [Key, Bucket] : _Impl->_Buckets)
    {
        if (Bucket._SlotCount == 0)
        {
            EmptyKeys.Emplace(Key);
        }
    }
    for (const auto& Key : EmptyKeys)
    {
        auto& Bucket = _Impl->_Buckets.FindChecked(Key);
        ck_debug_scene_target::DestroyBucket(Bucket);
        _Impl->_Buckets.Remove(Key);
    }

    _Impl->_Stats._ItemCount = _Impl->_Items.Num();
    _Impl->_Stats._BucketCount = _Impl->_Buckets.Num();
    _Impl->_Stats._ComponentCount = Get_Components().Num();
    _Impl->_Stats._InstanceCount = 0;
    for (const auto& [Key, Bucket] : _Impl->_Buckets)
    {
        _Impl->_Stats._InstanceCount += Bucket._SlotCount;
    }
}

auto
    FCk_DebugScene_Target::
    HideAll()
    -> void
{
    for (auto& [Key, Bucket] : _Impl->_Buckets)
    {
        ck_debug_scene_target::DestroyBucket(Bucket);
    }
    _Impl->_Buckets.Reset();
    _Impl->_Items.Reset();

    if (auto* Lines = _Impl->_Lines.Get(); ck::IsValid(Lines))
    {
        Lines->DestroyComponent();
    }
    _Impl->_Lines.Reset();
    _Impl->_LineChannels.Reset();
    _Impl->_LabelChannels.Reset();
    _Impl->_VectorChannels.Reset();
    _Impl->_Stats._ItemCount = 0;
    _Impl->_Stats._BucketCount = 0;
    _Impl->_Stats._ComponentCount = 0;
    _Impl->_Stats._InstanceCount = 0;
}

auto
    FCk_DebugScene_Target::
    Set_IsDesired(bool InIsDesired)
    -> void
{
    _Impl->_IsDesired = InIsDesired;
    if (NOT InIsDesired)
    {
        HideAll();
    }
}

auto
    FCk_DebugScene_Target::
    Set_RenderVisible(bool InIsVisible)
    -> void
{
    _Impl->_RenderVisible = InIsVisible;
    for (auto& [Key, Bucket] : _Impl->_Buckets)
    {
        if (ck::IsValid(Bucket._Component.Get()))
        {
            Bucket._Component->SetVisibility(_Impl->_IsDesired && _Impl->_RenderVisible &&
                                             NOT ck::diagnostic_visibility::Is_HiddenForStreamerMode() &&
                                             NOT _Impl->_HiddenRenderClasses.Contains(Key._RenderClassId));
        }
    }
    if (auto* Lines = _Impl->_Lines.Get(); ck::IsValid(Lines))
    {
        Lines->SetVisibility(_Impl->_IsDesired && _Impl->_RenderVisible &&
                             NOT ck::diagnostic_visibility::Is_HiddenForStreamerMode());
    }
}

auto
    FCk_DebugScene_Target::
    Get_RenderVisible() const
    -> bool
{
    return _Impl->_RenderVisible;
}

auto
    FCk_DebugScene_Target::
    Set_WireframeMode(ECk_DebugScene_WireframeMode InMode)
    -> void
{
    if (_Impl->_WireframeMode == InMode)
    {
        return;
    }
    _Impl->_WireframeMode = InMode;

    for (auto& [Key, Bucket] : _Impl->_Buckets)
    {
        ck_debug_scene_target::ApplyWireframeBucket(InMode, Key, Bucket);
    }
}

auto
    FCk_DebugScene_Target::
    Set_RenderClassVisible(uint8 InRenderClassId, bool InIsVisible)
    -> void
{
    if (InIsVisible)
    {
        _Impl->_HiddenRenderClasses.Remove(InRenderClassId);
    }
    else
    {
        _Impl->_HiddenRenderClasses.Add(InRenderClassId);
    }

    for (auto& [Key, Bucket] : _Impl->_Buckets)
    {
        if (Key._RenderClassId == InRenderClassId && ck::IsValid(Bucket._Component.Get()))
        {
            Bucket._Component->SetVisibility(InIsVisible && _Impl->_IsDesired && _Impl->_RenderVisible &&
                                             NOT ck::diagnostic_visibility::Is_HiddenForStreamerMode());
        }
    }
}

auto
    FCk_DebugScene_Target::
    Set_ItemPickable(uint64 InItemKey, bool InIsPickable)
    -> void
{
    if (auto* Item = _Impl->_Items.Find(InItemKey))
    {
        Item->_IsPickable = InIsPickable;
    }
}

auto
    FCk_DebugScene_Target::
    Set_LineChannel(FName InChannel, TArray<FCk_DebugScene_Line> InLines)
    -> bool
{
    auto InputIsValid = NOT InChannel.IsNone() && ck::IsValid(_Impl->_World.Get());
    for (const auto& Line : InLines)
    {
        InputIsValid &= ck_debug_scene_target::IsFinite(Line._From) && ck_debug_scene_target::IsFinite(Line._To) &&
                        ck_debug_scene_target::IsFinite(Line._Color) && FMath::IsFinite(Line._Thickness) &&
                        Line._Thickness >= 0.0f;
    }

    CK_ENSURE_IF_NOT(InputIsValid, TEXT("CkDebugScene rejected invalid line channel input"))
    {
        return false;
    }

    if (_Impl->_Reconciling)
    {
        _Impl->_StagedLineChannels.Add(InChannel, MoveTemp(InLines));
        return true;
    }

    auto* Lines = _Impl->_Lines.Get();
    if (ck::Is_NOT_Valid(Lines))
    {
        auto* World = _Impl->_World.Get();
        Lines = NewObject<ULineBatchComponent>(
            World, MakeUniqueObjectName(World, ULineBatchComponent::StaticClass(), TEXT("CkDebugSceneLines")));
        const auto LinesWereCreated = ck::IsValid(Lines);
        CK_ENSURE_IF_NOT(LinesWereCreated, TEXT("CkDebugScene failed to create a line batch component"))
        {
            return false;
        }

        Lines->DefaultLifeTime = 0.0f;
        Lines->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Lines->SetCanEverAffectNavigation(ck_debug_scene_target::DoesNotAffectNavigation);
        Lines->SetCastShadow(ck_debug_scene_target::DoesNotCastShadow);
        Lines->SetMobility(EComponentMobility::Movable);
        Lines->SetWorldLocation(FVector::ZeroVector);
        Lines->SetHiddenInGame(ck_debug_scene_target::IsHiddenInGame);
        Lines->SetVisibility(_Impl->_IsDesired && _Impl->_RenderVisible &&
                             NOT ck::diagnostic_visibility::Is_HiddenForStreamerMode());
        Lines->RegisterComponentWithWorld(World);
        _Impl->_Lines.Reset(Lines);
    }

    _Impl->_LineChannels.Add(InChannel, MoveTemp(InLines));

    Lines->Flush();
    for (const auto& [Channel, ChannelLines] : _Impl->_LineChannels)
    {
        auto Batched = TArray<FBatchedLine>{};
        Batched.Reserve(ChannelLines.Num());
        for (const auto& Line : ChannelLines)
        {
            Batched.Emplace(Line._From, Line._To, Line._Color, 0.0f, Line._Thickness, SDPG_World);
        }
        Lines->DrawLines(Batched);
    }
    Lines->SetVisibility(_Impl->_IsDesired && _Impl->_RenderVisible &&
                         NOT ck::diagnostic_visibility::Is_HiddenForStreamerMode());
    return true;
}

auto
    FCk_DebugScene_Target::
    Clear_LineChannel(FName InChannel)
    -> void
{
    if (_Impl->_Reconciling)
    {
        _Impl->_StagedLineChannels.Remove(InChannel);
        return;
    }
    if (_Impl->_LineChannels.Remove(InChannel) == 0)
    {
        return;
    }
    if (auto* Lines = _Impl->_Lines.Get(); ck::IsValid(Lines))
    {
        Lines->Flush();
        for (const auto& [Channel, ChannelLines] : _Impl->_LineChannels)
        {
            auto Batched = TArray<FBatchedLine>{};
            Batched.Reserve(ChannelLines.Num());
            for (const auto& Line : ChannelLines)
            {
                Batched.Emplace(Line._From, Line._To, Line._Color, 0.0f, Line._Thickness, SDPG_World);
            }
            Lines->DrawLines(Batched);
        }
    }
}

auto
    FCk_DebugScene_Target::
    Set_LabelChannel(FName InChannel, TArray<FCk_DebugScene_Label> InLabels)
    -> bool
{
    auto InputIsValid = NOT InChannel.IsNone();
    for (const auto& Label : InLabels)
    {
        InputIsValid &= ck_debug_scene_target::IsFinite(Label._WorldPosition) &&
                        ck_debug_scene_target::IsFinite(Label._Color) && FMath::IsFinite(Label._Scale) &&
                        Label._Scale > 0.0f;
    }

    CK_ENSURE_IF_NOT(InputIsValid, TEXT("CkDebugScene rejected invalid label channel input"))
    {
        return false;
    }

    if (_Impl->_Reconciling)
    {
        _Impl->_StagedLabelChannels.Add(InChannel, MoveTemp(InLabels));
        return true;
    }

    _Impl->_LabelChannels.Add(InChannel, MoveTemp(InLabels));
    return true;
}

auto
    FCk_DebugScene_Target::
    Clear_LabelChannel(FName InChannel)
    -> void
{
    if (_Impl->_Reconciling)
    {
        _Impl->_StagedLabelChannels.Remove(InChannel);
        return;
    }
    _Impl->_LabelChannels.Remove(InChannel);
}

auto
    FCk_DebugScene_Target::
    Set_VectorChannel(FName InChannel, TArray<FCk_DebugScene_Vector> InVectors)
    -> bool
{
    auto InputIsValid = NOT InChannel.IsNone();
    for (const auto& Vector : InVectors)
    {
        InputIsValid &= ck_debug_scene_target::IsFinite(Vector._Origin) &&
                        ck_debug_scene_target::IsFinite(Vector._NormalizedDirection) &&
                        Vector._NormalizedDirection.IsNormalized() && ck_debug_scene_target::IsFinite(Vector._Color) &&
                        FMath::IsFinite(Vector._Length) && Vector._Length > 0.0f &&
                        FMath::IsFinite(Vector._ArrowHeadScale) && Vector._ArrowHeadScale > 0.0f;
    }

    CK_ENSURE_IF_NOT(InputIsValid, TEXT("CkDebugScene rejected invalid vector channel input"))
    {
        return false;
    }

    if (_Impl->_Reconciling)
    {
        _Impl->_StagedVectorChannels.Add(InChannel, MoveTemp(InVectors));
        return true;
    }

    _Impl->_VectorChannels.Add(InChannel, MoveTemp(InVectors));
    return true;
}

auto
    FCk_DebugScene_Target::
    Clear_VectorChannel(FName InChannel)
    -> void
{
    if (_Impl->_Reconciling)
    {
        _Impl->_StagedVectorChannels.Remove(InChannel);
        return;
    }
    _Impl->_VectorChannels.Remove(InChannel);
}

auto
    FCk_DebugScene_Target::
    Get_Stats() const
    -> const FCk_DebugScene_Stats&
{
    return _Impl->_Stats;
}

auto
    FCk_DebugScene_Target::
    Reset_FrameStats()
    -> void
{
    _Impl->_Stats._InstancesAdded = 0;
    _Impl->_Stats._InstancesUpdated = 0;
    _Impl->_Stats._InstancesRemoved = 0;
    _Impl->_Stats._InstancesUnchanged = 0;
}

auto
    FCk_DebugScene_Target::
    Get_InstanceIds(uint64 InItemKey) const
    -> TArray<FPrimitiveInstanceId>
{
    const auto* Item = _Impl->_Items.Find(InItemKey);
    if (Item == nullptr)
    {
        return {};
    }

    auto Result = TArray<FPrimitiveInstanceId>{};
    Result.Reserve(Item->_Slots.Num());
    for (const auto& Slot : Item->_Slots)
    {
        Result.Emplace(Slot._InstanceId);
    }
    return Result;
}

auto
    FCk_DebugScene_Target::
    Get_ItemInstances(uint64 InItemKey) const
    -> TArray<FCk_DebugScene_Instance>
{
    const auto* Item = _Impl->_Items.Find(InItemKey);
    if (Item == nullptr)
    {
        return {};
    }

    auto Result = TArray<FCk_DebugScene_Instance>{};
    Result.Reserve(Item->_Slots.Num());
    for (const auto& Slot : Item->_Slots)
    {
        Result.Emplace(Slot._Submission);
    }
    return Result;
}

auto
    FCk_DebugScene_Target::
    Get_RenderClassInstanceCount(ECk_DebugScene_RenderClass InRenderClass) const
    -> int32
{
    auto Count = 0;
    for (const auto& [Key, Bucket] : _Impl->_Buckets)
    {
        if (Key._RenderClass == InRenderClass)
        {
            Count += Bucket._SlotCount;
        }
    }
    return Count;
}

auto
    FCk_DebugScene_Target::
    Get_WireframeInstanceCount() const
    -> int32
{
    auto Count = 0;
    for (const auto& [Key, Bucket] : _Impl->_Buckets)
    {
        if (ck_debug_scene_target::IsWireframeEnabled(_Impl->_WireframeMode, Key._RenderClass))
        {
            Count += Bucket._SlotCount;
        }
    }
    return Count;
}

auto
    FCk_DebugScene_Target::
    Get_ItemBounds(uint64 InItemKey) const
    -> TOptional<FBox>
{
    const auto* Item = _Impl->_Items.Find(InItemKey);
    return Item != nullptr && Item->_Bounds.IsValid ? TOptional<FBox>{Item->_Bounds} : TOptional<FBox>{};
}

auto
    FCk_DebugScene_Target::
    Get_ContentBounds() const
    -> FBox
{
    auto Bounds = FBox{ForceInit};
    for (const auto& [ItemKey, Item] : _Impl->_Items)
    {
        for (const auto& Slot : Item._Slots)
        {
            if (NOT _Impl->_HiddenRenderClasses.Contains(Slot._Bucket._RenderClassId))
            {
                Bounds += ck_debug_scene_target::GetInstanceBounds(Slot._Submission);
            }
        }
    }
    return Bounds;
}

auto
    FCk_DebugScene_Target::
    Get_Components() const
    -> TArray<UInstancedStaticMeshComponent*>
{
    auto Result = TArray<UInstancedStaticMeshComponent*>{};
    Result.Reserve(_Impl->_Buckets.Num());
    for (const auto& [Key, Bucket] : _Impl->_Buckets)
    {
        if (ck::IsValid(Bucket._Component.Get()))
        {
            Result.Emplace(Bucket._Component.Get());
        }
    }
    return Result;
}

auto
    FCk_DebugScene_Target::
    Get_Lines() const
    -> TArray<FCk_DebugScene_Line>
{
    auto Result = TArray<FCk_DebugScene_Line>{};
    for (const auto& [Channel, Lines] : _Impl->_LineChannels)
    {
        Result.Append(Lines);
    }
    return Result;
}

auto
    FCk_DebugScene_Target::
    Get_RenderedLineCount() const
    -> int32
{
    const auto* Lines = _Impl->_Lines.Get();
    return ck::IsValid(Lines) ? Lines->BatchedLines.Num() : 0;
}

auto
    FCk_DebugScene_Target::
    Get_Labels() const
    -> TArray<FCk_DebugScene_Label>
{
    auto Result = TArray<FCk_DebugScene_Label>{};
    for (const auto& [Channel, Labels] : _Impl->_LabelChannels)
    {
        Result.Append(Labels);
    }
    return Result;
}

auto
    FCk_DebugScene_Target::
    Get_Vectors() const
    -> TArray<FCk_DebugScene_Vector>
{
    auto Result = TArray<FCk_DebugScene_Vector>{};
    for (const auto& [Channel, Vectors] : _Impl->_VectorChannels)
    {
        Result.Append(Vectors);
    }
    return Result;
}

auto
    FCk_DebugScene_Target::
    Get_LineCount() const
    -> int32
{
    auto Count = 0;
    for (const auto& [Channel, Lines] : _Impl->_LineChannels)
    {
        Count += Lines.Num();
    }
    return Count;
}

auto
    FCk_DebugScene_Target::
    Get_LabelCount() const
    -> int32
{
    auto Count = 0;
    for (const auto& [Channel, Labels] : _Impl->_LabelChannels)
    {
        Count += Labels.Num();
    }
    return Count;
}

auto
    FCk_DebugScene_Target::
    Get_VectorCount() const
    -> int32
{
    auto Count = 0;
    for (const auto& [Channel, Vectors] : _Impl->_VectorChannels)
    {
        Count += Vectors.Num();
    }
    return Count;
}

#if WITH_DEV_AUTOMATION_TESTS
auto
    FCk_DebugScene_Target::
    Set_TestFailPrepareAfterInstances(int32 InPreparedInstanceCount)
    -> void
{
    _Impl->_TestFailPrepareAfterInstances = InPreparedInstanceCount;
}

auto
    FCk_DebugScene_Target::
    Set_TestFailCommitAfterInstances(int32 InCommittedInstanceCount)
    -> void
{
    _Impl->_TestFailCommitAfterInstances = InCommittedInstanceCount;
}
#endif

auto
    FCk_DebugScene_Target::
    TryPick(const FVector& InOrigin, const FVector& InDirection) const
    -> TOptional<FCk_DebugScene_Pick>
{
    const auto DirectionIsUsable = ck_debug_scene_target::IsFinite(InOrigin) &&
                                   ck_debug_scene_target::IsFinite(InDirection) && NOT InDirection.IsNearlyZero();
    CK_ENSURE_IF_NOT(DirectionIsUsable, TEXT("CkDebugScene rejected a degenerate pick ray"))
    {
        return {};
    }
    if (NOT _Impl->_RenderVisible || ck::diagnostic_visibility::Is_HiddenForStreamerMode())
    {
        return {};
    }

    auto NearestDistance = TNumericLimits<double>::Max();
    auto Result = TOptional<FCk_DebugScene_Pick>{};

    for (const auto& [ItemKey, Item] : _Impl->_Items)
    {
        if (NOT Item._IsPickable)
        {
            continue;
        }
        if (NOT Item._Bounds.IsValid ||
            NOT ck_debug_scene_target::RayHitsBox(Item._Bounds, InOrigin, InDirection, NearestDistance))
        {
            continue;
        }

        for (const auto& Slot : Item._Slots)
        {
            if (_Impl->_HiddenRenderClasses.Contains(Slot._Bucket._RenderClassId))
            {
                continue;
            }
            if (NOT ck_debug_scene_target::RayHitsBox(ck_debug_scene_target::GetInstanceBounds(Slot._Submission),
                                                      InOrigin, InDirection, NearestDistance))
            {
                continue;
            }

            const auto& Transform = Slot._Submission.Get_Transform();
            const auto LocalOrigin = Transform.InverseTransformPosition(InOrigin);
            const auto LocalDirection = Transform.InverseTransformVector(InDirection);
            auto HitDistance = NearestDistance;
            if (NOT Slot._Submission.Get_Mesh()->TryIntersect_Ray(LocalOrigin, LocalDirection, NearestDistance,
                                                                  HitDistance))
            {
                continue;
            }

            NearestDistance = HitDistance;
            Result = FCk_DebugScene_Pick{};
            Result->_PickIdentity = Slot._Submission.Get_PickIdentity();
            Result->_HitPoint = InOrigin + InDirection * HitDistance;
            Result->_Distance = static_cast<float>(HitDistance * InDirection.Size());
        }
    }

    return Result;
}
