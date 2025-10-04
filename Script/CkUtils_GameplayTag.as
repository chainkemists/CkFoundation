class UCk_GameplayTags : UCk_DataAsset_PDA
{
    UPROPERTY()
    TArray<FName> GameplayTags;
}

namespace ck
{
    UFUNCTION()
    void ResolveAllGameplayTags()
    {
        TArray<FAssetData> Assets;
        AssetRegistry::GetAssetsByClass(FTopLevelAssetPath(UCk_GameplayTags), Assets, true);

        for (auto Asset : Assets)
        {
            auto Tags = Cast<UCk_GameplayTags>(Asset.GetAsset());
            for (auto Tag : Tags.GameplayTags)
            {
                Print(f"Resolving tag: {Tag}");
                utils_gameplay_tag::ResolveGameplayTag(Tag, "Added via Angelscript auto resolve");
            }
        }
    }
}