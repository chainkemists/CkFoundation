#include "CkIsmRenderer_TransientFactory.h"

#include "CkCore/Object/CkObject_Utils.h"
#include "CkIsmRenderer/CkIsmRenderer_Log.h"

// --------------------------------------------------------------------------------------------------------------------

TMap<UCk_Utils_IsmRenderer_TransientFactory_UE::FMeshOnlyKey, TWeakObjectPtr<UCk_IsmRenderer_Data>>
    UCk_Utils_IsmRenderer_TransientFactory_UE::MeshOnlyCache;

TMap<UCk_Utils_IsmRenderer_TransientFactory_UE::FMeshMaterialKey, TWeakObjectPtr<UCk_IsmRenderer_Data>>
    UCk_Utils_IsmRenderer_TransientFactory_UE::MeshMaterialCache;

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IsmRenderer_TransientFactory_UE::FMeshOnlyKey::
    operator==(
        const FMeshOnlyKey& Other) const
    -> bool
{
    return World == Other.World &&
        Mesh == Other.Mesh &&
        Mobility == Other.Mobility;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IsmRenderer_TransientFactory_UE::FMeshMaterialKey::
    operator==(
        const FMeshMaterialKey& Other) const
    -> bool
{
    if (World != Other.World)
    { return false; }

    if (Mesh != Other.Mesh)
    { return false; }

    if (NumCustomData != Other.NumCustomData)
    { return false; }

    if (Mobility != Other.Mobility)
    { return false; }

    if (MaterialSlots.Num() != Other.MaterialSlots.Num())
    { return false; }

    for (int32 i = 0; i < MaterialSlots.Num(); ++i)
    {
        if (MaterialSlots[i].Key != Other.MaterialSlots[i].Key)
        { return false; }

        if (MaterialSlots[i].Value != Other.MaterialSlots[i].Value)
        { return false; }
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IsmRenderer_TransientFactory_UE::
    GetOrCreate_ForMesh(
        const UWorld* InWorld,
        UStaticMesh* InMesh,
        ECk_Mobility InMobility)
    -> UCk_IsmRenderer_Data*
{
    const auto WorldIsValid = ck::IsValid(InWorld);
    CK_ENSURE_IF_NOT(WorldIsValid,
        TEXT("Trying to create transient IsmRenderer_Data with an INVALID World"))
    {}
    if (NOT WorldIsValid)
    { return nullptr; }

    const auto MeshIsValid = ck::IsValid(InMesh);
    CK_ENSURE_IF_NOT(MeshIsValid,
        TEXT("Trying to create transient IsmRenderer_Data with an INVALID Mesh"))
    {}
    if (NOT MeshIsValid)
    { return nullptr; }

    auto CacheKey = FMeshOnlyKey{};
    CacheKey.World = InWorld;
    CacheKey.Mesh = InMesh;
    CacheKey.Mobility = InMobility;

    if (const auto Found = MeshOnlyCache.Find(CacheKey);
        Found && ck::IsValid(*Found))
    { return Found->Get(); }

    auto* NewData = CreateTransient(InWorld, InMesh, {}, InMobility);

    if (ck::IsValid(NewData))
    { MeshOnlyCache.Add(MoveTemp(CacheKey), NewData); }

    return NewData;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IsmRenderer_TransientFactory_UE::
    GetOrCreate_ForMeshWithMaterials(
        const UWorld* InWorld,
        UStaticMesh* InMesh,
        const TArray<FCk_MeshMaterialOverride>& InMaterialOverrides,
        ECk_Mobility InMobility)
    -> UCk_IsmRenderer_Data*
{
    const auto WorldIsValid = ck::IsValid(InWorld);
    CK_ENSURE_IF_NOT(WorldIsValid,
        TEXT("Trying to create transient IsmRenderer_Data with an INVALID World"))
    {}
    if (NOT WorldIsValid)
    { return nullptr; }

    const auto MeshIsValid = ck::IsValid(InMesh);
    CK_ENSURE_IF_NOT(MeshIsValid,
        TEXT("Trying to create transient IsmRenderer_Data with an INVALID Mesh"))
    {}
    if (NOT MeshIsValid)
    { return nullptr; }

    FMeshMaterialKey CacheKey;
    CacheKey.World = InWorld;
    CacheKey.Mesh = InMesh;
    CacheKey.Mobility = InMobility;
    CacheKey.MaterialSlots.Reserve(InMaterialOverrides.Num());

    for (const auto& Override : InMaterialOverrides)
    {
        CacheKey.MaterialSlots.Emplace(Override.Get_MaterialSlot(), Override.Get_ReplacementMaterial());
    }

    // Sort by slot for deterministic key comparison
    CacheKey.MaterialSlots.Sort([](const auto& A, const auto& B) { return A.Key < B.Key; });

    if (const auto Found = MeshMaterialCache.Find(CacheKey);
        Found && ck::IsValid(*Found))
    { return Found->Get(); }

    auto* NewData = CreateTransient(InWorld, InMesh, InMaterialOverrides, InMobility);

    if (ck::IsValid(NewData))
    { MeshMaterialCache.Add(MoveTemp(CacheKey), NewData); }

    return NewData;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IsmRenderer_TransientFactory_UE::
    GetOrCreate_ForMeshWithMaterialsAndCustomData(
        const UWorld* InWorld,
        UStaticMesh* InMesh,
        const TArray<FCk_MeshMaterialOverride>& InMaterialOverrides,
        int32 InNumCustomData,
        ECk_Mobility InMobility)
    -> UCk_IsmRenderer_Data*
{
    const auto WorldIsValid = ck::IsValid(InWorld);
    CK_ENSURE_IF_NOT(WorldIsValid,
        TEXT("Trying to create transient IsmRenderer_Data with an INVALID World"))
    {}
    if (NOT WorldIsValid)
    { return nullptr; }

    const auto MeshIsValid = ck::IsValid(InMesh);
    CK_ENSURE_IF_NOT(MeshIsValid,
        TEXT("Trying to create transient IsmRenderer_Data with an INVALID Mesh"))
    {}
    if (NOT MeshIsValid)
    { return nullptr; }

    const auto CustomDataCountIsValid = InNumCustomData >= 0;
    CK_ENSURE_IF_NOT(CustomDataCountIsValid,
        TEXT("Trying to create transient IsmRenderer_Data for Mesh [{}] with a NEGATIVE custom-data count [{}]"),
        InMesh, InNumCustomData)
    {}
    if (NOT CustomDataCountIsValid)
    { return nullptr; }

    FMeshMaterialKey CacheKey;
    CacheKey.World = InWorld;
    CacheKey.Mesh = InMesh;
    CacheKey.NumCustomData = InNumCustomData;
    CacheKey.Mobility = InMobility;
    CacheKey.MaterialSlots.Reserve(InMaterialOverrides.Num());

    for (const auto& Override : InMaterialOverrides)
    {
        CacheKey.MaterialSlots.Emplace(Override.Get_MaterialSlot(), Override.Get_ReplacementMaterial());
    }

    CacheKey.MaterialSlots.Sort([](const auto& A, const auto& B) { return A.Key < B.Key; });

    if (const auto Found = MeshMaterialCache.Find(CacheKey);
        Found && ck::IsValid(*Found))
    { return Found->Get(); }

    auto* NewData = CreateTransient(InWorld, InMesh, InMaterialOverrides, InMobility, InNumCustomData);

    if (ck::IsValid(NewData))
    { MeshMaterialCache.Add(MoveTemp(CacheKey), NewData); }

    return NewData;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IsmRenderer_TransientFactory_UE::
    CreateTransient(
        const UWorld* InWorld,
        UStaticMesh* InMesh,
        const TArray<FCk_MeshMaterialOverride>& InMaterialOverrides,
        ECk_Mobility InMobility,
        int32 InNumCustomData)
    -> UCk_IsmRenderer_Data*
{
    auto* NewData = UCk_Utils_Object_UE::Request_CreateNewObject_TransientPackage<UCk_IsmRenderer_Data>(
        [&](UCk_IsmRenderer_Data* Data)
        {
            Data->_Mobility = InMobility;
            Data->_UpdatePolicy = ECk_Ism_InstanceUpdatePolicy::Update;
            Data->_Mesh = InMesh;
            Data->_RenderPolicy = ECk_Ism_RenderPolicy::ISM;

            Data->_MaterialsInfo._MaterialOverrides = InMaterialOverrides;
            Data->_NumCustomData = InNumCustomData;
        }
    );

    const auto NewDataIsValid = ck::IsValid(NewData);
    CK_ENSURE_IF_NOT(NewDataIsValid,
        TEXT("Failed to create transient IsmRenderer_Data for Mesh [{}]"), InMesh)
    {}
    if (NOT NewDataIsValid)
    { return nullptr; }

    // Needed for GetWorld() on a transient-package object. const_cast is safe: the destination is a
    // non-owning TWeakObjectPtr.
    NewData->Set_CurrentWorld(const_cast<UWorld*>(InWorld));

    // Nothing holds a strong ref until the setup processor tick creates the renderer actor.
    NewData->AddToRoot();

    ck::ismrenderer::Verbose(TEXT("Created new Transient ISM Renderer for Mesh [{}]..."), InMesh);

    return NewData;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IsmRenderer_TransientFactory_UE::
    ClearCache(
        const UWorld* InWorld)
    -> void
{
    const auto WorldIsValid = ck::IsValid(InWorld);
    CK_ENSURE_IF_NOT(WorldIsValid,
        TEXT("Trying to clear transient IsmRenderer_Data cache for an INVALID World"))
    {}
    if (NOT WorldIsValid)
    { return; }

    ck::ismrenderer::Verbose(TEXT("Clearing Transient ISM Renderer cache for World [{}]..."), InWorld);

    for (auto It = MeshOnlyCache.CreateIterator(); It; ++It)
    {
        if (It.Key().World.Get() != InWorld)
        { continue; }

        if (It.Value().IsValid())
        { It.Value()->RemoveFromRoot(); }

        It.RemoveCurrent();
    }

    for (auto It = MeshMaterialCache.CreateIterator(); It; ++It)
    {
        if (It.Key().World.Get() != InWorld)
        { continue; }

        if (It.Value().IsValid())
        { It.Value()->RemoveFromRoot(); }

        It.RemoveCurrent();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IsmRenderer_TransientFactory_UE::
    ClearCache()
    -> void
{
    ck::ismrenderer::Verbose(TEXT("Clearing Transient ISM Renderer cache..."));

    for (auto& [Key, Value] : MeshOnlyCache)
    {
        if (Value.IsValid())
        { Value->RemoveFromRoot(); }
    }

    for (auto& [Key, Value] : MeshMaterialCache)
    {
        if (Value.IsValid())
        { Value->RemoveFromRoot(); }
    }

    MeshOnlyCache.Empty();
    MeshMaterialCache.Empty();
}

// --------------------------------------------------------------------------------------------------------------------
