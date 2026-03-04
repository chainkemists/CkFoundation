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

    // Clears all cached transient renderer data. Called on world teardown.
    static auto
    ClearCache() -> void;

private:
    struct FMeshMaterialKey
    {
        TWeakObjectPtr<UStaticMesh> Mesh;
        TArray<TPair<int32, TWeakObjectPtr<UMaterialInterface>>> MaterialSlots;

        auto operator==(const FMeshMaterialKey& Other) const -> bool;

        friend auto GetTypeHash(const FMeshMaterialKey& Key) -> uint32
        {
            uint32 Hash = ::GetTypeHash(Key.Mesh);
            for (const auto& Slot : Key.MaterialSlots)
            {
                Hash = HashCombine(Hash, ::GetTypeHash(Slot.Key));
                Hash = HashCombine(Hash, ::GetTypeHash(Slot.Value));
            }
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
        ECk_Mobility InMobility) -> UCk_IsmRenderer_Data*;
};

// --------------------------------------------------------------------------------------------------------------------
