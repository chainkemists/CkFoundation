#include "CkIsmRenderer_TransientFactory.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

TMap<TWeakObjectPtr<UStaticMesh>, TWeakObjectPtr<UCk_IsmRenderer_Data>>
    UCk_Utils_IsmRenderer_TransientFactory_UE::MeshOnlyCache;

TMap<UCk_Utils_IsmRenderer_TransientFactory_UE::FMeshMaterialKey, TWeakObjectPtr<UCk_IsmRenderer_Data>>
    UCk_Utils_IsmRenderer_TransientFactory_UE::MeshMaterialCache;

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IsmRenderer_TransientFactory_UE::FMeshMaterialKey::
    operator==(const FMeshMaterialKey& Other) const
    -> bool
{
    if (Mesh != Other.Mesh)
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
    CK_ENSURE_IF_NOT(ck::IsValid(InWorld),
        TEXT("Trying to create transient IsmRenderer_Data with an INVALID World"))
    { return nullptr; }

    CK_ENSURE_IF_NOT(ck::IsValid(InMesh),
        TEXT("Trying to create transient IsmRenderer_Data with an INVALID Mesh"))
    { return nullptr; }

    if (const auto Found = MeshOnlyCache.Find(InMesh);
        Found && ck::IsValid(*Found))
    { return Found->Get(); }

    auto* NewData = CreateTransient(InWorld, InMesh, {}, InMobility);

    if (ck::IsValid(NewData))
    { MeshOnlyCache.Add(InMesh, NewData); }

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
    CK_ENSURE_IF_NOT(ck::IsValid(InWorld),
        TEXT("Trying to create transient IsmRenderer_Data with an INVALID World"))
    { return nullptr; }

    CK_ENSURE_IF_NOT(ck::IsValid(InMesh),
        TEXT("Trying to create transient IsmRenderer_Data with an INVALID Mesh"))
    { return nullptr; }

    // Build the cache key
    FMeshMaterialKey CacheKey;
    CacheKey.Mesh = InMesh;
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
    CreateTransient(
        const UWorld* InWorld,
        UStaticMesh* InMesh,
        const TArray<FCk_MeshMaterialOverride>& InMaterialOverrides,
        ECk_Mobility InMobility)
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
        }
    );

    CK_ENSURE_IF_NOT(ck::IsValid(NewData),
        TEXT("Failed to create transient IsmRenderer_Data for Mesh [{}]"), InMesh)
    { return nullptr; }

    // Set _CurrentWorld so GetWorld() works for transient package objects.
    // const_cast is safe: _CurrentWorld is mutable and TWeakObjectPtr is a non-owning reference.
    NewData->Set_CurrentWorld(const_cast<UWorld*>(InWorld));

    return NewData;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IsmRenderer_TransientFactory_UE::
    ClearCache()
    -> void
{
    MeshOnlyCache.Empty();
    MeshMaterialCache.Empty();
}

// --------------------------------------------------------------------------------------------------------------------
