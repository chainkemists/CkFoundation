namespace utils_physics
{
    FGameplayTagContainer Get_PhysicalMaterialTagsFromHitResult(FHitResult InHitResult)
    {
        auto Obj = Cast<UCk_PhysicalMaterialWithTags>(InHitResult.PhysMaterial);

        if (ck::Is_NOT_Valid(Obj))
        { return FGameplayTagContainer(); }

        return Obj.Get_Tags();
    }
}