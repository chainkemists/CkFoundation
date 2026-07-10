#pragma once

#include "CkIsmRenderer/Renderer/CkIsmRenderer_Fragment_Data.h"

#include "CkIsmRenderer_TransientFactory.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKISMRENDERER_API UCk_Utils_IsmRenderer_TransientFactory_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IsmRenderer_TransientFactory_UE);

public:
    // Creates or retrieves a cached transient UCk_IsmRenderer_Data for the given mesh.
    // Uses default materials from the mesh. Cache key: UStaticMesh*.
    UFUNCTION(BlueprintCallable, Category = "Ck|IsmRenderer|Transient")
    static UCk_IsmRenderer_Data*
    GetOrCreate_ForMesh(
        const UWorld* InWorld,
        UStaticMesh* InMesh,
        ECk_Mobility InMobility = ECk_Mobility::Static);

    // Creates or retrieves a cached transient UCk_IsmRenderer_Data for the given mesh
    // with explicit material overrides. Cache key: composite of mesh + material overrides.
    UFUNCTION(BlueprintCallable, Category = "Ck|IsmRenderer|Transient")
    static UCk_IsmRenderer_Data*
    GetOrCreate_ForMeshWithMaterials(
        const UWorld* InWorld,
        UStaticMesh* InMesh,
        const TArray<FCk_MeshMaterialOverride>& InMaterialOverrides,
        ECk_Mobility InMobility = ECk_Mobility::Static);

    // As GetOrCreate_ForMeshWithMaterials, additionally allocating per-instance custom-data floats
    // on the shared ISM component (NumCustomDataFloats). The count is part of the cache key — the
    // same mesh+materials with a different float count is a different renderer. Consumers that
    // write per-instance material data (e.g. CkVat playback state) need this overload; the plain
    // ones allocate zero floats.
    UFUNCTION(BlueprintCallable, Category = "Ck|IsmRenderer|Transient")
    static UCk_IsmRenderer_Data*
    GetOrCreate_ForMeshWithMaterialsAndCustomData(
        const UWorld* InWorld,
        UStaticMesh* InMesh,
        const TArray<FCk_MeshMaterialOverride>& InMaterialOverrides,
        int32 InNumCustomData,
        ECk_Mobility InMobility = ECk_Mobility::Static);

    // Clears all cached transient renderer data. Called on world teardown.
    static auto
    ClearCache() -> void;

private:
    struct FMeshMaterialKey
    {
        TWeakObjectPtr<UStaticMesh> Mesh;
        TArray<TPair<int32, TWeakObjectPtr<UMaterialInterface>>> MaterialSlots;
        int32 NumCustomData = 0;

        auto operator==(const FMeshMaterialKey& Other) const -> bool;

        friend auto GetTypeHash(const FMeshMaterialKey& Key) -> uint32
        {
            uint32 Hash = ::GetTypeHash(Key.Mesh);
            for (const auto& Slot : Key.MaterialSlots)
            {
                Hash = HashCombine(Hash, ::GetTypeHash(Slot.Key));
                Hash = HashCombine(Hash, ::GetTypeHash(Slot.Value));
            }
            Hash = HashCombine(Hash, ::GetTypeHash(Key.NumCustomData));
            return Hash;
        }
    };

    static TMap<TWeakObjectPtr<UStaticMesh>, TWeakObjectPtr<UCk_IsmRenderer_Data>> MeshOnlyCache;
    static TMap<FMeshMaterialKey, TWeakObjectPtr<UCk_IsmRenderer_Data>> MeshMaterialCache;

    static auto
    CreateTransient(
        const UWorld* InWorld,
        UStaticMesh* InMesh,
        const TArray<FCk_MeshMaterialOverride>& InMaterialOverrides,
        ECk_Mobility InMobility,
        int32 InNumCustomData = 0) -> UCk_IsmRenderer_Data*;
};

// --------------------------------------------------------------------------------------------------------------------
