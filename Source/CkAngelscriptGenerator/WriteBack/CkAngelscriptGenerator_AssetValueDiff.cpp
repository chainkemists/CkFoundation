#include "CkAngelscriptGenerator/WriteBack/CkAngelscriptGenerator_AssetValueDiff.h"

#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Reflection/CkReflection_Utils.h"

#include "Engine/BlueprintGeneratedClass.h"
#include "UObject/Class.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptManager.h>
#include <ClassGenerator/ASClass.h>
#include <as_context.h>
#include <as_module.h>
#include <as_scriptfunction.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ck::angelscriptgenerator::write_back
{
    namespace ck_angelscript_generator_asset_value_diff
    {
        constexpr auto NeverAuthoredFlags =
            CPF_Transient |
            CPF_DuplicateTransient |
            CPF_NonPIEDuplicateTransient |
            CPF_Parm;

        auto Make_Unresolved(
            const FString&                 InPath,
            const FString&                 InDetail,
            ECk_AccessorResolve_FailReason InReason,
            const FString&                 InMessage) -> FCk_WriteBackUnresolved
        {
            auto Out = FCk_WriteBackUnresolved{};
            Out.PropertyPath = InPath;
            Out.Detail       = InDetail;
            Out.FailReason   = InReason;
            Out.Message      = InMessage;
            return Out;
        }

        auto Get_IsObjectish(
            const FProperty* InProperty) -> bool
        {
            return CastField<FObjectPropertyBase>(InProperty) != nullptr
                || CastField<FInterfaceProperty>(InProperty)  != nullptr;
        }

        // Deep comparison is for INSTANCED subobjects and nothing else.
        //
        // An instanced property gives the live asset and the scratch baseline each their own
        // instance, so pointer identity would report it changed forever, put an inexpressible
        // property in the patch set, and abort every write. Deep comparison reads those as equal.
        //
        // For a plain asset reference it is actively WRONG: FObjectPropertyBase::StaticIdentical
        // deep-compares ANY two objects that merely share a class and an FName
        // (PropertyBaseObject.cpp:137-146) — it is PPF_DeepCompareInstances, not PPF_DeepComparison,
        // that restricts the test to instances. So re-pointing a property from
        // `/Engine/EngineMeshes/Cube` to `/Engine/BasicShapes/Cube` could read as unchanged, drop
        // out of the patch set, and be reset from the file by the reload the write triggers —
        // destroying the edit in the gesture that claimed to save it.
        auto Get_ComparisonFlags(
            const FProperty* InProperty) -> uint32
        {
            constexpr auto InstancedFlags =
                CPF_InstancedReference |
                CPF_ContainsInstancedReference |
                CPF_PersistentInstance |
                CPF_ExportObject;

            return InProperty->HasAnyPropertyFlags(InstancedFlags) ? PPF_DeepComparison : PPF_None;
        }

        auto Get_IsContainer(
            const FProperty* InProperty) -> bool
        {
            return InProperty->IsA<FArrayProperty>()
                || InProperty->IsA<FSetProperty>()
                || InProperty->IsA<FMapProperty>();
        }

        // AngelScript names a class by its prefixed C++ spelling — `AActor`, `UCk_Thing_Data` —
        // which is a legal TSubclassOf expression on its own.
        auto Get_BareClassExpression(
            const UClass* InClass) -> FString
        {
            return FString::Printf(TEXT("%s%s"), InClass->GetPrefixCPP(), *InClass->GetName());
        }

        auto Get_IsLiteralAsset(
            const UObject* InObject) -> bool
        {
#if WITH_ANGELSCRIPT_CK
            return ck::IsValid(InObject, ck::IsValid_Policy_NullptrOnly{})
                && InObject->GetOuter() == FAngelscriptManager::Get().AssetsPackage;
#else
            return false;
#endif
        }

        // Object-ish leaves MUST come through here before anything consults
        // UCk_Utils_Reflection_UE::Get_PropertyDefaultValueLiteral, which returns the literal
        // `nullptr` for every object property unconditionally (CkReflection_Utils.cpp:664-674) and
        // would silently rewrite `_Skeleton = assets::load::SK_Mannequin();` to
        // `_Skeleton = nullptr;`.
        auto Try_ResolveObjectLeaf(
            const FProperty*                 InProperty,
            const void*                      InValuePtr,
            const FString&                   InPath,
            const FCk_AssetValueDiffContext& InContext,
            FString&                         OutExpression,
            FCk_WriteBackUnresolved&         OutUnresolved) -> bool
        {
            const auto Resolve = [&](const FString& InObjectPath, ECk_ScriptAccessorKind InKind) -> bool
            {
                const auto Resolved = FCkAsAccessorResolver::Resolve(
                    *InContext.Accessors,
                    InContext.AnyProviderRegistered,
                    InObjectPath,
                    InKind,
                    InContext.TargetBlockIsEditorOnly);

                if (Resolved.Success)
                {
                    OutExpression = Resolved.Expression;
                    return true;
                }

                OutUnresolved = Make_Unresolved(InPath, InObjectPath, Resolved.FailReason, Resolved.ErrorMessage);
                return false;
            };

            // Deliberate override to null reads as `nullptr`, which assigns to every object-ish
            // AngelScript type (object handles, TSubclassOf, TSoftObjectPtr, TSoftClassPtr).
            if (const auto* SoftClassProp = CastField<FSoftClassProperty>(InProperty))
            {
                const auto& Value = SoftClassProp->GetPropertyValue(InValuePtr);
                if (Value.IsNull())
                { OutExpression = TEXT("nullptr"); return true; }

                return Resolve(Value.ToSoftObjectPath().ToString(), ECk_ScriptAccessorKind::SoftClass);
            }

            if (const auto* SoftObjectProp = CastField<FSoftObjectProperty>(InProperty))
            {
                const auto& Value = SoftObjectProp->GetPropertyValue(InValuePtr);
                if (Value.IsNull())
                { OutExpression = TEXT("nullptr"); return true; }

                return Resolve(Value.ToSoftObjectPath().ToString(), ECk_ScriptAccessorKind::SoftObject);
            }

            if (const auto* ClassProp = CastField<FClassProperty>(InProperty))
            {
                auto* Value = Cast<UClass>(ClassProp->GetObjectPropertyValue(InValuePtr));
                if (Value == nullptr)
                { OutExpression = TEXT("nullptr"); return true; }

                // A generated Blueprint class has no AngelScript spelling; every other class does.
                if (Cast<UBlueprintGeneratedClass>(Value) == nullptr)
                { OutExpression = Get_BareClassExpression(Value); return true; }

                return Resolve(Value->GetPathName(), ECk_ScriptAccessorKind::HardClass);
            }

            // Weak / lazy / interface are out of v1 scope. They must abort loudly rather than fall
            // through to a `nullptr` that would destroy the existing reference.
            if (CastField<FWeakObjectProperty>(InProperty) != nullptr
                || CastField<FLazyObjectProperty>(InProperty) != nullptr
                || CastField<FInterfaceProperty>(InProperty) != nullptr)
            {
                OutUnresolved = Make_Unresolved(InPath, InProperty->GetCPPType(),
                    ECk_AccessorResolve_FailReason::UnsupportedPropertyKind,
                    FString::Printf(TEXT("'%s' is a weak/lazy/interface reference; write-back cannot express one yet."),
                        *InPath));
                return false;
            }

            if (const auto* ObjectProp = CastField<FObjectProperty>(InProperty))
            {
                auto* Value = ObjectProp->GetObjectPropertyValue(InValuePtr);
                if (Value == nullptr)
                { OutExpression = TEXT("nullptr"); return true; }

                if (auto* AsClass = Cast<UClass>(Value);
                    AsClass != nullptr)
                {
                    if (Cast<UBlueprintGeneratedClass>(AsClass) == nullptr)
                    { OutExpression = Get_BareClassExpression(AsClass); return true; }

                    return Resolve(AsClass->GetPathName(), ECk_ScriptAccessorKind::HardClass);
                }

                if (Get_IsLiteralAsset(Value))
                {
                    const auto LiteralName = Value->GetName();
                    if (InContext.SameFileLiteralAssetNames.Contains(LiteralName))
                    { OutExpression = LiteralName; return true; }

                    OutUnresolved = Make_Unresolved(InPath, LiteralName,
                        ECk_AccessorResolve_FailReason::CrossFileLiteralAsset,
                        FString::Printf(
                            TEXT("'%s' points at the literal asset '%s', declared in another .as file. Bare names ")
                            TEXT("are not visible across files — add a namespace wrapper there (the ")
                            TEXT("`iskm_assets::RendererData_Demo()` idiom) and assign that instead."),
                            *InPath, *LiteralName));
                    return false;
                }

                return Resolve(Value->GetPathName(), ECk_ScriptAccessorKind::HardObject);
            }

            OutUnresolved = Make_Unresolved(InPath, InProperty->GetCPPType(),
                ECk_AccessorResolve_FailReason::UnsupportedPropertyKind,
                FString::Printf(TEXT("'%s' is an object reference of a kind write-back does not handle."), *InPath));
            return false;
        }

        // True when the file text produces a reference here but the live object holds none.
        auto Get_IsClearedReference(
            const FProperty* InProperty,
            const void*      InLiveContainer,
            const void*      InBaselineContainer) -> bool
        {
            if (NOT Get_IsObjectish(InProperty) || InBaselineContainer == nullptr)
            { return false; }

            const auto* LiveValue     = InProperty->ContainerPtrToValuePtr<void>(InLiveContainer);
            const auto* BaselineValue = InProperty->ContainerPtrToValuePtr<void>(InBaselineContainer);

            // Soft references answer at the PATH, never the resolved pointer:
            // FSoftObjectProperty::GetObjectPropertyValue is a weak resolve (PropertySoftObjectPtr.cpp:273),
            // so a reference that is set but not loaded would otherwise read as cleared.
            if (const auto* SoftProperty = CastField<FSoftObjectProperty>(InProperty))
            {
                return SoftProperty->GetPropertyValue(LiveValue).IsNull()
                    && NOT SoftProperty->GetPropertyValue(BaselineValue).IsNull();
            }

            const auto* ObjectProperty = CastField<FObjectPropertyBase>(InProperty);
            if (ObjectProperty == nullptr)
            { return false; }

            return ObjectProperty->GetObjectPropertyValue(LiveValue) == nullptr
                && ObjectProperty->GetObjectPropertyValue(BaselineValue) != nullptr;
        }

        auto Try_ResolveLeafExpression(
            const FProperty*                 InProperty,
            const void*                      InContainerPtr,
            const FString&                   InPath,
            const FCk_AssetValueDiffContext& InContext,
            FString&                         OutExpression,
            bool&                            OutIsLossyText,
            FCk_WriteBackUnresolved&         OutUnresolved) -> bool
        {
            if (Get_IsObjectish(InProperty))
            {
                const auto* ValuePtr = InProperty->ContainerPtrToValuePtr<void>(InContainerPtr);
                return Try_ResolveObjectLeaf(InProperty, ValuePtr, InPath, InContext, OutExpression, OutUnresolved);
            }

            if (Get_IsContainer(InProperty))
            {
                OutUnresolved = Make_Unresolved(InPath, InProperty->GetCPPType(),
                    ECk_AccessorResolve_FailReason::UnsupportedPropertyKind,
                    FString::Printf(
                        TEXT("'%s' is a container. Write-back cannot express TArray/TSet/TMap yet; edit it in the ")
                        TEXT(".as body instead."), *InPath));
                return false;
            }

            // A struct emitted whole carries its FText fields with it, so the dialog's lossy note has
            // to fire for those too — not just for a top-level FText.
            if (CastField<FTextProperty>(InProperty) != nullptr)
            { OutIsLossyText = true; }
            else if (const auto* StructProperty = CastField<FStructProperty>(InProperty))
            {
                for (TFieldIterator<FProperty> FieldIt(StructProperty->Struct, EFieldIteratorFlags::IncludeSuper);
                     FieldIt; ++FieldIt)
                {
                    if (CastField<FTextProperty>(*FieldIt) != nullptr)
                    { OutIsLossyText = true; break; }
                }
            }

            const auto Literal = UCk_Utils_Reflection_UE::Get_PropertyDefaultValueLiteral(InProperty, InContainerPtr);
            if (NOT Literal.IsSet())
            {
                OutUnresolved = Make_Unresolved(InPath, InProperty->GetCPPType(),
                    ECk_AccessorResolve_FailReason::UnsupportedPropertyKind,
                    FString::Printf(TEXT("'%s' has no AngelScript literal representation for its current value."),
                        *InPath));
                return false;
            }

            OutExpression = UCk_Utils_Reflection_UE::Get_AngelscriptDefaultExpression(*Literal);
            if (OutExpression.IsEmpty())
            {
                OutUnresolved = Make_Unresolved(InPath, InProperty->GetCPPType(),
                    ECk_AccessorResolve_FailReason::UnsupportedPropertyKind,
                    FString::Printf(TEXT("'%s' produced an empty AngelScript expression."), *InPath));
                return false;
            }

            return true;
        }

        // Walks a struct as a (live, baseline) PAIR, emitting one assignment per changed leaf.
        //
        // Deliberately NOT UCk_Utils_Reflection_UE::Get_StructFieldOverrides: that one diffs against a
        // zero-initialized InitializeStruct buffer, so a class whose CDO customises a struct field
        // reads as edited and churns lines the user never touched.
        auto Collect_ChangedStructLeaves(
            const UScriptStruct*             InStruct,
            const void*                      InLivePtr,
            const void*                      InBaselinePtr,
            const FString&                   InRootProperty,
            const FString&                   InSubPath,
            const FCk_AssetValueDiffContext& InContext,
            TArray<FCk_AssetBlockAssignment>& OutAssignments,
            bool&                            OutIsLossyText,
            TArray<FCk_WriteBackUnresolved>& OutUnresolved,
            TArray<FString>&                 OutClearedReferences) -> void
        {
            for (TFieldIterator<FProperty> FieldIt(InStruct, EFieldIteratorFlags::IncludeSuper); FieldIt; ++FieldIt)
            {
                const auto* Field = *FieldIt;
                if (Field->HasAnyPropertyFlags(NeverAuthoredFlags))
                { continue; }

                const auto* LiveField     = Field->ContainerPtrToValuePtr<void>(InLivePtr);
                const auto* BaselineField = Field->ContainerPtrToValuePtr<void>(InBaselinePtr);

                if (Field->Identical(LiveField, BaselineField, Get_ComparisonFlags(Field)))
                { continue; }

                const auto FieldSubPath = InSubPath + TEXT(".") + Field->GetName();
                const auto FieldPath    = InRootProperty + FieldSubPath;

                // Descend only into nested structs that carry an object field: those cannot use the
                // positional constructor in AngelScript, so each leaf needs its own assignment.
                if (const auto* StructField = CastField<FStructProperty>(Field);
                    StructField != nullptr
                    && UCk_Utils_Reflection_UE::Has_UObjectPointerField(StructField->Struct))
                {
                    Collect_ChangedStructLeaves(StructField->Struct, LiveField, BaselineField,
                        InRootProperty, FieldSubPath, InContext, OutAssignments, OutIsLossyText,
                        OutUnresolved, OutClearedReferences);
                    continue;
                }

                if (Get_IsClearedReference(Field, InLivePtr, InBaselinePtr))
                { OutClearedReferences.Add(FieldPath); }

                auto Expression = FString{};
                auto Unresolved = FCk_WriteBackUnresolved{};
                if (NOT Try_ResolveLeafExpression(Field, InLivePtr, FieldPath, InContext,
                        Expression, OutIsLossyText, Unresolved))
                {
                    OutUnresolved.Add(MoveTemp(Unresolved));
                    continue;
                }

                OutAssignments.Add(FCk_AssetBlockAssignment{FieldSubPath, MoveTemp(Expression)});
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetValueDiff::
        Get_IsWriteBackCandidate(
            const FProperty* InProperty)
        -> bool
    {
        if (ck::Is_NOT_Valid(InProperty))
        { return false; }

        if (InProperty->HasAnyPropertyFlags(ck_angelscript_generator_asset_value_diff::NeverAuthoredFlags))
        { return false; }

        // The button saves what the user changed IN THE DETAILS PANEL, so a property they cannot
        // edit there cannot be part of that. Excluding them is not a convenience — a whole-value
        // compare of such a property reports phantom differences that no expression could ever
        // express, which would abort every write on the asset:
        //   - `UCurveFloat::FloatCurve` is a bare UPROPERTY() with no Edit flag at all; the
        //     FRichCurve inside it carries a `transient` key-handle map that is generated lazily and
        //     per-instance, so live and baseline differ whenever one of them has been iterated.
        //   - `UCurveBase::AssetImportData` is VisibleAnywhere (CPF_Edit | CPF_EditConst) editor
        //     import metadata, and each instance owns a distinct subobject.
        // Both were measured diverging on all 32 UCurveFloat literal assets in this project.
        return InProperty->HasAnyPropertyFlags(CPF_Edit)
            && NOT InProperty->HasAnyPropertyFlags(CPF_EditConst);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetValueDiff::
        Build_ScratchBaseline(
            UClass*        InClass,
            const FString& InAssetName)
        -> TStrongObjectPtr<UObject>
    {
#if WITH_ANGELSCRIPT_CK
        const auto ClassIsValid = ck::IsValid(InClass, ck::IsValid_Policy_NullptrOnly{});
        CK_ENSURE_IF_NOT(ClassIsValid, TEXT("Cannot build a write-back baseline without a class"))
        { return {}; }

        const auto Module = FAngelscriptManager::Get().GetModuleContainingLiteralAsset(InAssetName);
        if (NOT Module.IsValid() || Module->ScriptModule == nullptr)
        {
            ck::angelscriptgenerator::Warning(
                TEXT("[WriteBack] No AngelScript module declares literal asset [{}] — cannot build a baseline"),
                InAssetName);
            return {};
        }

        const auto InitFunctionName = FString::Printf(TEXT("__Init_%s"), *InAssetName);

        auto* InitFunction = static_cast<asCScriptFunction*>(nullptr);
        const auto Count = static_cast<int32>(Module->ScriptModule->globalFunctionList.GetLength());
        for (auto Index = 0; Index < Count; ++Index)
        {
            auto* Function = Module->ScriptModule->globalFunctionList[Index];
            if (InitFunctionName == FString{StringCast<TCHAR>(Function->name.AddressOf()).Get()})
            { InitFunction = Function; break; }
        }

        if (InitFunction == nullptr)
        {
            ck::angelscriptgenerator::Warning(
                TEXT("[WriteBack] Could not resolve [{}] in module [{}] — cannot build a baseline"),
                InitFunctionName, Module->ModuleName);
            return {};
        }

        // A fresh instance already carries the CDO's values, so the file text is the only thing that
        // moves it — no separate reset step is needed (and resetting would orphan freshly created
        // instanced subobjects).
        auto Scratch = TStrongObjectPtr<UObject>{
            NewObject<UObject>(GetTransientPackage(), InClass, NAME_None, RF_Transient)};

        if (ck::Is_NOT_Valid(Scratch.Get()))
        { return {}; }

        auto Context = FAngelscriptContext{};
        Context->Prepare(InitFunction);
        Context->SetArgObject(0, Scratch.Get());

        if (const auto ExecutionResult = Context->Execute();
            ExecutionResult != asEXECUTION_FINISHED)
        {
            ck::angelscriptgenerator::Warning(
                TEXT("[WriteBack] [{}] failed to execute against the scratch instance: asExecutionResult=[{}]"),
                InitFunctionName, static_cast<int32>(ExecutionResult));
            return {};
        }

        return Scratch;
#else
        return {};
#endif
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetValueDiff::
        Get_DiffersFromClassDefaults(
            const UObject* InLive)
        -> bool
    {
        if (ck::Is_NOT_Valid(InLive, ck::IsValid_Policy_NullptrOnly{}))
        { return false; }

        const auto* Class = InLive->GetClass();
        const auto* Defaults = Class->GetDefaultObject(/*bCreateIfNeeded=*/false);
        if (ck::Is_NOT_Valid(Defaults, ck::IsValid_Policy_NullptrOnly{}) || Defaults == InLive)
        { return false; }

        for (TFieldIterator<FProperty> PropIt(Class, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
        {
            const auto* Property = *PropIt;
            if (NOT Get_IsWriteBackCandidate(Property))
            { continue; }

            if (NOT Property->Identical_InContainer(InLive, Defaults, 0,
                ck_angelscript_generator_asset_value_diff::Get_ComparisonFlags(Property)))
            { return true; }
        }

        return false;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetValueDiff::
        Compute(
            const UObject*                   InLive,
            const UObject*                   InBaseline,
            const UObject*                   InClassDefaults,
            const FCk_AssetValueDiffContext& InContext)
        -> FCk_AssetValueDiffResult
    {
        namespace detail = ck_angelscript_generator_asset_value_diff;

        auto Result = FCk_AssetValueDiffResult{};

        const auto InputsAreValid = ck::IsValid(InLive, ck::IsValid_Policy_NullptrOnly{})
                                 && ck::IsValid(InBaseline, ck::IsValid_Policy_NullptrOnly{})
                                 && InContext.Accessors != nullptr;
        CK_ENSURE_IF_NOT(InputsAreValid, TEXT("Write-back diff needs a live object, a baseline, and an accessor index"))
        { return Result; }

        const auto* Class = InLive->GetClass();

        for (TFieldIterator<FProperty> PropIt(Class, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
        {
            const auto* Property = *PropIt;
            if (NOT Get_IsWriteBackCandidate(Property))
            { continue; }

            // THE patch-set predicate: what the user changed relative to what the file text produces.
            //
            // PPF_DeepComparison is load-bearing, not incidental. Without it an instanced subobject
            // compares by pointer, so the live asset and the scratch baseline — which each own a
            // distinct instance — would differ forever, putting an inexpressible property in the
            // patch set and aborting every write. With it, same-class + same-name + same-values
            // reads as unchanged (FObjectProperty::Identical, PropertyObject.cpp).
            if (Property->Identical_InContainer(InLive, InBaseline, 0, detail::Get_ComparisonFlags(Property)))
            { continue; }

            const auto PropertyName = Property->GetName();

            if (detail::Get_IsClearedReference(Property, InLive, InBaseline))
            { Result.ClearedObjectReferences.Add(PropertyName); }

            // Reverted to the class default since load, so the line has nothing left to say.
            if (ck::IsValid(InClassDefaults, ck::IsValid_Policy_NullptrOnly{})
                && NOT detail::Get_IsContainer(Property)
                && Property->Identical_InContainer(InLive, InClassDefaults, 0, detail::Get_ComparisonFlags(Property)))
            {
                Result.Entries.Add(FCk_AssetBlockPatchEntry::Make_Delete(PropertyName));
                continue;
            }

            if (const auto* StructProperty = CastField<FStructProperty>(Property);
                StructProperty != nullptr
                && UCk_Utils_Reflection_UE::Has_UObjectPointerField(StructProperty->Struct))
            {
                auto Assignments = TArray<FCk_AssetBlockAssignment>{};

                detail::Collect_ChangedStructLeaves(
                    StructProperty->Struct,
                    StructProperty->ContainerPtrToValuePtr<void>(InLive),
                    StructProperty->ContainerPtrToValuePtr<void>(InBaseline),
                    PropertyName,
                    FString{},
                    InContext,
                    Assignments,
                    Result.HasLossyText,
                    Result.Unresolved,
                    Result.ClearedObjectReferences);

                if (NOT Assignments.IsEmpty())
                { Result.Entries.Add(FCk_AssetBlockPatchEntry::Make_Assign(PropertyName, Assignments)); }

                continue;
            }

            auto Expression = FString{};
            auto Unresolved = FCk_WriteBackUnresolved{};
            if (NOT detail::Try_ResolveLeafExpression(Property, InLive, PropertyName, InContext,
                    Expression, Result.HasLossyText, Unresolved))
            {
                Result.Unresolved.Add(MoveTemp(Unresolved));
                continue;
            }

            Result.Entries.Add(FCk_AssetBlockPatchEntry::Make_Assign(
                PropertyName, {FCk_AssetBlockAssignment{FString{}, MoveTemp(Expression)}}));
        }

        Result.Success = Result.Unresolved.IsEmpty();
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
