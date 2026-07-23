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
        // UASStruct is supplied by the optional AngelscriptCode module. Do not make foundational CkCore link against
        // that module solely for a Cast<UASStruct>: Unreal's generated runtime class path is the stable identity of
        // the same type. Some script fields need not be exported as FProperty, so runtime class identity alone is
        // insufficient: GetPropertiesSize() is the complete AngelScript value size and must also prove zero storage.
        static const auto AngelScriptStructClassPath = FString{TEXT("/Script/AngelscriptCode.ASStruct")};
        return InStruct != nullptr &&
            InStruct->GetClass() != nullptr &&
            InStruct->GetClass()->GetPathName() == AngelScriptStructClassPath &&
            InStruct->GetPropertiesSize() == 0;
    }

    auto IsApprovedGcIndependentStruct(const UScriptStruct* InStruct) -> bool
    {
        // Allowlist of native structs certified to retain no untraced UObject reference, matched by reflected path so
        // foundational CkCore need not link against the modules that declare them (same rationale as IsAngelScriptStruct).
        //
        // FCk_Entity holds only an entt::entity integer id; its int32 mirror fields exist solely for the editor debugger
        // and are compiled out under WITH_EDITORONLY_DATA. A cooked build therefore sees zero reflected fields, so the
        // field-less-struct heuristic below would otherwise reject every dynamic fragment / EntityScript spawn-param that
        // embeds an FCk_Handle (its _Entity member is an FCk_Entity). The type is provably GC-independent, not opaque.
        static const auto ApprovedPaths = TSet<FString>{TEXT("/Script/CkEcs.Ck_Entity")};
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

        // Check these before FObjectPropertyBase: weak and soft property classes derive from it but do not retain a
        // bare object address. Their serial/generation identity remains safe when the object is collected.
        if (CastField<const FWeakObjectProperty>(InProperty) != nullptr ||
            CastField<const FSoftObjectProperty>(InProperty) != nullptr)
        { return Accept(); }

        if (CastField<const FLazyObjectProperty>(InProperty) != nullptr)
        {
            return Reject(EResult::UnprovenOpaque, InPath,
                TEXT("deprecated lazy UObject references are not an approved untraced-storage contract"));
        }

        // Unreal script delegates retain their target through FWeakObjectPtr. Payload-carrying custom carriers are
        // still analyzed through their reflected struct/property graph and do not reach this case.
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

        // Custom reference collectors are proof that reflection alone does not describe ownership. An untraced sink
        // cannot invoke that collector, so retaining this value requires a traced holder.
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
