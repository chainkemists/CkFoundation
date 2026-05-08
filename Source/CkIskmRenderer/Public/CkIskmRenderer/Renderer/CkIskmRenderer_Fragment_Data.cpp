#include "CkIskmRenderer/Renderer/CkIskmRenderer_Fragment_Data.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"

#if WITH_EDITOR
    #include "Misc/DataValidation.h"
#endif

auto
    UCk_IskmRenderer_Data::
    Find_SubmeshIndex_ByName(FName InName) const
    -> int32
{
    return _Submeshes.IndexOfByPredicate([&](const FCk_IskmRenderer_MeshDesc& E)
    {
        return E.Get_Name() == InName;
    });
}

#if WITH_EDITOR
auto
    UCk_IskmRenderer_Data::
    PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent) -> void
{
    Super::PostEditChangeProperty(InPropertyChangedEvent);
}

auto
    UCk_IskmRenderer_Data::
    IsDataValid(FDataValidationContext& InContext) const
    -> EDataValidationResult
{
    auto Result = Super::IsDataValid(InContext);

    if (ck::Is_NOT_Valid(_AnimCollection))
    {
        InContext.AddError(FText::FromString(TEXT("Renderer PDA has no AnimCollection.")));
        Result = EDataValidationResult::Invalid;
        return Result;
    }

    auto* Skeleton = _AnimCollection->Get_Skeleton().Get();
    for (auto Index = 0; Index < _Submeshes.Num(); ++Index)
    {
        const auto& Def = _Submeshes[Index];
        const auto* Mesh = Def.Get_Mesh().Get();
        if (ck::Is_NOT_Valid(Mesh))
        {
            InContext.AddError(FText::FromString(FString::Printf(TEXT("Submesh [%d] (%s) has no Mesh."), Index, *Def.Get_Name().ToString())));
            Result = EDataValidationResult::Invalid;
            continue;
        }
        if (Mesh->GetSkeleton() != Skeleton)
        {
            InContext.AddError(FText::FromString(FString::Printf(TEXT("Submesh [%d] (%s) skeleton mismatch with AnimCollection."), Index, *Def.Get_Name().ToString())));
            Result = EDataValidationResult::Invalid;
        }
    }

    return Result;
}
#endif
