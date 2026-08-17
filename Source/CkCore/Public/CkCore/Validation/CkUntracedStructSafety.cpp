#include "CkCore/Validation/CkUntracedStructSafety.h"

#include "CkCore/Validation/CkIsValid.h"

#include <UObject/PropertyOptional.h>
#include <UObject/UnrealType.h>

namespace ck_untraced_struct_safety
{
    using EResult = ck::ECk_UntracedStructSafety;
    using FResult = ck::FCk_UntracedStructSafetyResult;

    auto IsAngelScriptStruct(const UScriptStruct* InStruct) -> bool
    {
        // Class identity alone is insufficient HERE — script fields need not be exported as FProperty, and this
        // predicate is only reached from the field-less branch below, so the complete AngelScript value size must
        // also prove zero storage.
        return ck::Is_AngelScriptDeclaredStruct(InStruct) && InStruct->GetPropertiesSize() == 0;
    }

    auto IsApprovedGcIndependentStruct(const UScriptStruct* InStruct) -> bool
    {
        // Native structs certified to retain no untraced UObject reference, matched by reflected path (same
        // link-avoidance rationale as Is_AngelScriptDeclaredStruct). FCk_Entity is provably GC-independent yet has
        // zero reflected fields in a cooked build, so the field-less-struct heuristic below would otherwise reject
        // every dynamic fragment / spawn-param embedding an FCk_Handle. The three posture markers are
        // empty-by-design and AngelScript fragments carry them as a FIELD (script structs cannot inherit) —
        // without approval a marker would fail schema validation on every fragment that declares its posture,
        // and Produce refuses the whole entity's payload on an unsafe schema.
        static const auto ApprovedPaths = TSet<FString>{
            TEXT("/Script/CkEcs.Ck_Entity"),
            TEXT("/Script/CkEcs.Ck_Snapshot_Durable"),
            TEXT("/Script/CkEcs.Ck_Snapshot_Session"),
            TEXT("/Script/CkDynamic.Ck_DynamicFragment_SnapshotTransient")};
        return InStruct != nullptr && ApprovedPaths.Contains(InStruct->GetPathName());
    }

    auto Accept() -> FResult
    { return {.Safety = EResult::GcIndependent}; }

    auto Reject(EResult InSafety, FString InPath, FString InReason) -> FResult
    {
        return {
            .Safety = InSafety,
            .FailurePath = MoveTemp(InPath),
            .FailureReason = MoveTemp(InReason)};
    }

    auto AppendPath(const FString& InParent, const FString& InChild) -> FString
    { return InParent.IsEmpty() ? InChild : FString::Printf(TEXT("%s.%s"), *InParent, *InChild); }

    auto AnalyzeStruct(
        const UScriptStruct* InStruct,
        const FString& InPath,
        TSet<const UScriptStruct*>& InOutActiveStructs) -> FResult;

    auto AnalyzeProperty(
        const FProperty* InProperty,
        const FString& InPath,
        TSet<const UScriptStruct*>& InOutActiveStructs) -> FResult
    {
        if (InProperty == nullptr)
        { return Reject(EResult::UnprovenOpaque, InPath, TEXT("property metadata is missing")); }

        // Must precede FObjectPropertyBase: weak and soft property classes derive from it but retain a
        // serial/generation identity rather than a bare object address, so they survive collection.
        if (CastField<const FWeakObjectProperty>(InProperty) != nullptr ||
            CastField<const FSoftObjectProperty>(InProperty) != nullptr)
        { return Accept(); }

        if (CastField<const FLazyObjectProperty>(InProperty) != nullptr)
        {
            return Reject(EResult::UnprovenOpaque, InPath,
                TEXT("deprecated lazy UObject references are not an approved untraced-storage contract"));
        }

        // Unreal script delegates retain their target through FWeakObjectPtr.
        if (CastField<const FDelegateProperty>(InProperty) != nullptr ||
            CastField<const FMulticastDelegateProperty>(InProperty) != nullptr)
        { return Accept(); }

        if (CastField<const FObjectPropertyBase>(InProperty) != nullptr)
        {
            return Reject(EResult::RequiresGcTracing, InPath,
                FString::Printf(TEXT("raw/strong UObject leaf [%s] requires a GC-traced owner"),
                    *InProperty->GetClass()->GetName()));
        }

        if (CastField<const FInterfaceProperty>(InProperty) != nullptr)
        {
            return Reject(EResult::RequiresGcTracing, InPath,
                TEXT("UObject interface leaf retains an object address and requires a GC-traced owner"));
        }

        if (const auto* ArrayProperty = CastField<const FArrayProperty>(InProperty))
        { return AnalyzeProperty(ArrayProperty->Inner, InPath + TEXT("[]"), InOutActiveStructs); }

        if (const auto* SetProperty = CastField<const FSetProperty>(InProperty))
        { return AnalyzeProperty(SetProperty->ElementProp, InPath + TEXT("{}"), InOutActiveStructs); }

        if (const auto* MapProperty = CastField<const FMapProperty>(InProperty))
        {
            auto Result = AnalyzeProperty(MapProperty->KeyProp, InPath + TEXT("{Key}"), InOutActiveStructs);
            if (NOT Result.IsGcIndependent())
            { return Result; }

            return AnalyzeProperty(MapProperty->ValueProp, InPath + TEXT("{Value}"), InOutActiveStructs);
        }

        if (const auto* OptionalProperty = CastField<const FOptionalProperty>(InProperty))
        { return AnalyzeProperty(OptionalProperty->GetValueProperty(), InPath + TEXT("?"), InOutActiveStructs); }

        if (const auto* StructProperty = CastField<const FStructProperty>(InProperty))
        { return AnalyzeStruct(StructProperty->Struct, InPath, InOutActiveStructs); }

        return Accept();
    }

    auto AnalyzeStruct(
        const UScriptStruct* InStruct,
        const FString& InPath,
        TSet<const UScriptStruct*>& InOutActiveStructs) -> FResult
    {
        if (ck::Is_NOT_Valid(InStruct))
        { return Reject(EResult::UnprovenOpaque, InPath, TEXT("nested struct type is invalid")); }

        // An untraced sink cannot invoke a custom reference collector, so retaining this value needs a traced holder.
        if (InStruct->StructFlags & STRUCT_AddStructReferencedObjects)
        {
            return Reject(EResult::RequiresGcTracing, InPath,
                FString::Printf(TEXT("opaque struct [%s] owns references through AddStructReferencedObjects"),
                    *InStruct->GetPathName()));
        }

        if (InOutActiveStructs.Contains(InStruct))
        { return Accept(); }

        InOutActiveStructs.Add(InStruct);
        bool HasReflectedProperty = false;
        for (TFieldIterator<FProperty> PropertyIt(InStruct, EFieldIterationFlags::IncludeSuper); PropertyIt; ++PropertyIt)
        {
            HasReflectedProperty = true;
            const auto* Property = *PropertyIt;
            auto Result = AnalyzeProperty(Property, AppendPath(InPath, Property->GetName()), InOutActiveStructs);
            if (NOT Result.IsGcIndependent())
            {
                InOutActiveStructs.Remove(InStruct);
                return Result;
            }
        }
        InOutActiveStructs.Remove(InStruct);

        if (NOT HasReflectedProperty)
        {
            if (IsAngelScriptStruct(InStruct))
            { return Accept(); }

            if (IsApprovedGcIndependentStruct(InStruct))
            { return Accept(); }

            return Reject(EResult::UnprovenOpaque, InPath,
                FString::Printf(TEXT("native/opaque struct [%s] has no reflected fields and no approved GC contract"),
                    *InStruct->GetPathName()));
        }

        return Accept();
    }
}

auto
    ck::
    Is_AngelScriptDeclaredStruct(
        const UScriptStruct* InStruct)
    -> bool
{
    // Path-matched rather than Cast<UASStruct> so foundational CkCore need not link the optional AngelscriptCode
    // module. UASStruct derives UScriptStruct, so every script struct's CLASS is exactly this one type.
    static const auto AngelScriptStructClassPath = FString{TEXT("/Script/AngelscriptCode.ASStruct")};

    return InStruct != nullptr &&
        InStruct->GetClass() != nullptr &&
        InStruct->GetClass()->GetPathName() == AngelScriptStructClassPath;
}

auto
    ck::
    Analyze_UntracedStructSafety(
        const UScriptStruct* InStructType)
    -> FCk_UntracedStructSafetyResult
{
    if (ck::Is_NOT_Valid(InStructType))
    {
        return ck_untraced_struct_safety::Reject(
            ECk_UntracedStructSafety::UnprovenOpaque,
            TEXT("<invalid>"),
            TEXT("struct type is invalid"));
    }

    auto ActiveStructs = TSet<const UScriptStruct*>{};
    return ck_untraced_struct_safety::AnalyzeStruct(InStructType, InStructType->GetName(), ActiveStructs);
}
