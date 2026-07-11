#include "CkObject_Utils.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "CkCore/CkCoreLog.h"
#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"
#include "CkCore/ObjectPooling/CkObjectPooling_Subsystem.h"
#include "Interfaces/IPluginManager.h"

#include <Engine/World.h>

#include "HAL/IConsoleManager.h"
#include "UObject/GarbageCollection.h"
#include "UObject/Package.h"
#include "UObject/UObjectArray.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectHash.h"

#include <Engine/Blueprint.h>
#include <Engine/BlueprintGeneratedClass.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Object_UE::
    Get_CleanClassName(
        const UClass* InClass)
    -> FString
{
    if (ck::Is_NOT_Valid(InClass))
    { return TEXT("(unknown)"); }

    auto Name = InClass->GetName();
    Name.RemoveFromStart(TEXT("BP_"));
    Name.RemoveFromEnd(TEXT("_C"));
    return Name;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Object_UE::
    Get_TagFromClassName(
        const UClass* InClass,
        const FString& InComment)
    -> FGameplayTag
{
    if (ck::Is_NOT_Valid(InClass))
    { return {}; }

    auto ClassName = InClass->GetName();

    if (ClassName.EndsWith(TEXT("_C")))
    { ClassName = ClassName.LeftChop(2); }

    ClassName = ClassName.Replace(TEXT("_"), TEXT("."));

    return UCk_Utils_GameplayTag_UE::ResolveGameplayTag(*ClassName, InComment);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Object_UE::
    Request_CallFunctionByName(
        UObject* InObject,
        FName InFunctionName,
        bool InEnsureFunctionExists)
    -> ECk_SucceededFailed
{
    return DoRequest_CallFunctionByName(InObject, InFunctionName, InEnsureFunctionExists);
}

auto
    UCk_Utils_Object_UE::
    ForEach_ObjectsWithOuter(
        const UObject* InOuterObject,
        TFunction<void(UObject*)> InFunc)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOuterObject),
        TEXT("Invalid Outer Object when trying to loop over all objects with this specific outer"))
    { return; }

    auto SubObjects = TArray<UObject*>{};

    constexpr auto IncludeNestedObjects = true;
    GetObjectsWithOuter(InOuterObject, SubObjects, IncludeNestedObjects);

    for (const auto SubObject : SubObjects)
    {
        InFunc(SubObject);
    }
}

auto
    UCk_Utils_Object_UE::
    Get_GeneratedUniqueName(
        UObject* InThis,
        UClass* InObj,
        FName InBaseName)
    -> FName
{
    CK_ENSURE_IF_NOT(ck::IsValid(InThis),
        TEXT("Invalid Outer supplied to Process_GenerateUniqueName"))
    { return NAME_None; }

    CK_ENSURE_IF_NOT(ck::IsValid(InObj), TEXT("Invalid Object Class supplied to Process_GenerateUniqueName"))
    { return NAME_None; }

    return MakeUniqueObjectName(InThis, InObj->StaticClass(), InBaseName);
}

auto
    UCk_Utils_Object_UE::
    Request_TrySetOuter(
        const FCk_Utils_Object_SetOuter_Params& InParams)
    -> ECk_SucceededFailed
{
    const auto& Object = InParams.Get_Object();
    CK_ENSURE_IF_NOT(ck::IsValid(Object), TEXT("Object is not valid!"))
    { return ECk_SucceededFailed::Failed; }

    const auto& Outer = InParams.Get_Outer();
    CK_ENSURE_IF_NOT(ck::IsValid(Outer), TEXT("Outer is not valid!"))
    { return ECk_SucceededFailed::Failed; }

    const auto& RenameFlag = [&]()
    {
        switch (InParams.Get_RenameFlags())
        {
            case ECk_Utils_Object_RenameFlags::None:                  return REN_None;
            case ECk_Utils_Object_RenameFlags::ForceNoResetLoaders:   return REN_ForceNoResetLoaders;
            case ECk_Utils_Object_RenameFlags::DoNotDirty:            return REN_DoNotDirty;
            case ECk_Utils_Object_RenameFlags::DontCreateRedirectors: return REN_DontCreateRedirectors;
            case ECk_Utils_Object_RenameFlags::ForceGlobalUnique:     return REN_ForceGlobalUnique;
            case ECk_Utils_Object_RenameFlags::SkipGeneratedClass:    return REN_SkipGeneratedClasses;
            default:                                                  return REN_None;
        }
    }();

    if (const auto& Result = Object->Rename(nullptr, Outer, RenameFlag);
        NOT Result)
    { return ECk_SucceededFailed::Failed; }

    return ECk_SucceededFailed::Succeeded;
}

auto
    UCk_Utils_Object_UE::
    Request_CopyAllProperties(
        const FCk_Utils_Object_CopyAllProperties_Params& InParams)
    -> ECk_Utils_Object_CopyAllProperties_Result
{
    using ResultType = ECk_Utils_Object_CopyAllProperties_Result;

    const auto& Source = InParams.Get_Source();
    CK_ENSURE_IF_NOT(ck::IsValid(Source), TEXT("The Source in Params is not valid"))
    { return ResultType::Failed; }

    const auto& Destination = InParams.Get_Destination();
    CK_ENSURE_IF_NOT(ck::IsValid(Destination), TEXT("The Destination in Params is not valid"))
    { return ResultType::Failed; }

    const auto& SourceClass = Get_DefaultClass_UpToDate(Source->GetClass());

    CK_ENSURE_IF_NOT(ck::IsValid(SourceClass), TEXT("Could not get the up-to-date default class of Source object [{}]"), Source)
    { return ResultType::Failed; }

    CK_ENSURE_IF_NOT(Destination->IsA(SourceClass), TEXT("The Destination [{}] and Source [{}] are not the same type"), Destination, Source)
    { return ResultType::Failed; }

    auto Result = ResultType::AllPropertiesIdentical;

    for (auto* Property = Destination->GetClass()->PropertyLink; ck::IsValid(Property); Property = Property->PropertyLinkNext)
    {
        if (Result == ResultType::AllPropertiesIdentical && NOT Property->Identical_InContainer(Destination, Source, PPF_None))
        {
            Result = ResultType::Succeeded;
        }

        Property->CopyCompleteValue_InContainer(Destination, Source);
    }

    return Result;
}

auto
    UCk_Utils_Object_UE::
    Request_ResetAllPropertiesToDefault(
        UObject* InObject)
    -> ECk_SucceededFailed
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Invalid Object supplied to Request_ResetAllPropertiesToDefault"))
    { return {}; }

    if (Get_IsDefaultObject(InObject))
    { return ECk_SucceededFailed::Succeeded; }

    const auto& ObjectCDO = DoGet_ClassDefaultObject(InObject->GetClass());

    const auto CopyAllPropertiesParams = FCk_Utils_Object_CopyAllProperties_Params{}
                                            .Set_Destination(InObject)
                                            .Set_Source(ObjectCDO);

    const auto& Result = Request_CopyAllProperties(CopyAllPropertiesParams);

    return Result == ECk_Utils_Object_CopyAllProperties_Result::Failed
                        ? ECk_SucceededFailed::Failed
                        : ECk_SucceededFailed::Succeeded;
}

auto
    UCk_Utils_Object_UE::
    Request_CreateNewObject_TransientPackage_UE(
        TSubclassOf<UObject> InObject)
    -> UObject*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Invalid Class supplied to Request_CreateNewObject_TransientPackage_UE"))
    { return {}; }

    return Request_CreateNewObject_TransientPackage<UObject>(InObject, [](auto InObj){});
}

auto
    UCk_Utils_Object_UE::
    Request_CreateNewObject_Pooled(
        UObject* InOuter,
        TSubclassOf<UObject> InClass,
        UObject* InTemplateArchetype,
        const FCk_ObjectPooling_PoolParams& InPoolParams)
    -> UObject*
{
    return DoRequest_AcquirePooled(InOuter, InClass, InTemplateArchetype, InPoolParams);
}

auto
    UCk_Utils_Object_UE::
    TryReleaseToPool(
        UObject* InObject)
    -> ECk_SucceededFailed
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Invalid Object supplied to TryReleaseToPool"))
    { return ECk_SucceededFailed::Failed; }

    const auto& World = InObject->GetWorld();

    CK_ENSURE_IF_NOT(ck::IsValid(World),
        TEXT("Could not resolve a World from [{}] — only ObjectPooling-managed objects (outered to their "
             "world) can be released to a pool"), InObject)
    { return ECk_SucceededFailed::Failed; }

    auto* Subsystem = World->GetSubsystem<UCk_ObjectPooling_Subsystem_UE>();

    CK_ENSURE_IF_NOT(ck::IsValid(Subsystem),
        TEXT("World [{}] has no ObjectPooling subsystem — cannot release [{}]"), World, InObject)
    { return ECk_SucceededFailed::Failed; }

    return Subsystem->TryReleaseToPool(InObject);
}

auto
    UCk_Utils_Object_UE::
    Get_ObjectPoolStats(
        const UObject* InWorldContextObject,
        TSubclassOf<UObject> InClass,
        UObject* InArchetype)
    -> FCk_ObjectPooling_PoolStats
{
    CK_ENSURE_IF_NOT(ck::IsValid(InClass), TEXT("Invalid Class supplied to Get_ObjectPoolStats"))
    { return {}; }

    const auto& World = ck::IsValid(InWorldContextObject) ? InWorldContextObject->GetWorld() : nullptr;

    if (ck::Is_NOT_Valid(World))
    { return {}; }

    const auto* Subsystem = World->GetSubsystem<UCk_ObjectPooling_Subsystem_UE>();
    
    if (ck::Is_NOT_Valid(Subsystem))
    { return {}; }

    auto* Archetype = ck::IsValid(InArchetype)
        ? InArchetype
        : InClass->GetDefaultObject();

    return Subsystem->Get_PoolStats(FCk_ObjectPooling_PoolKey{InClass.Get(), Archetype});
}

auto
    UCk_Utils_Object_UE::
    Get_IsPoolTrackedObject(
        const UObject* InObject)
    -> bool
{
    if (ck::Is_NOT_Valid(InObject))
    { return false; }

    const auto& World = InObject->GetWorld();

    if (ck::Is_NOT_Valid(World))
    { return false; }

    const auto* Subsystem = World->GetSubsystem<UCk_ObjectPooling_Subsystem_UE>();

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Get_IsTrackedObject(InObject);
}

auto
    UCk_Utils_Object_UE::
    DoRequest_AcquirePooled(
        UObject* InOuter,
        TSubclassOf<UObject> InClass,
        UObject* InTemplateArchetype,
        const FCk_ObjectPooling_PoolParams& InPoolParams)
    -> UObject*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InClass), TEXT("Invalid Class supplied to the pooled Request_CreateNewObject"))
    { return {}; }

    const auto& World = ck::IsValid(InOuter) ? InOuter->GetWorld() : nullptr;

    auto* Subsystem = ck::IsValid(World)
        ? World->GetSubsystem<UCk_ObjectPooling_Subsystem_UE>()
        : nullptr;

    CK_ENSURE_IF_NOT(ck::IsValid(Subsystem),
        TEXT("Pooled Request_CreateNewObject for [{}] could not resolve an ObjectPooling subsystem from "
             "Outer [{}] — falling back to a plain (non-pooled, CALLER-owned) create. The caller must "
             "hold a strong reference in this case"), InClass, InOuter)
    { return NewObject<UObject>(InOuter, InClass, NAME_None, RF_NoFlags, InTemplateArchetype); }

    return Subsystem->AcquireFromPool(InClass, InTemplateArchetype, InPoolParams, InOuter);
}

auto
    UCk_Utils_Object_UE::
    Get_DefaultClass_UpToDate(
        UClass* InClass)
    -> UClass*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InClass), TEXT("InClass is INVALID."))
    { return {}; }

    if (NOT InClass->HasAnyClassFlags(CLASS_NewerVersionExists))
    { return InClass; }

    const auto& AuthoritativeClass = InClass->GetAuthoritativeClass();

    CK_ENSURE_IF_NOT(ck::IsValid(AuthoritativeClass),
        TEXT("Unable to get the most up to date class. The class [{}] somehow does NOT have an authoratative class."),
        InClass)
    { return InClass; }

    return AuthoritativeClass;
}

auto
    UCk_Utils_Object_UE::
    Get_IsDefaultObject(
        const UObject* InObject)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Invalid Object supplied to Get_IsDefaultObject"))
    { return {}; }

    return InObject == DoGet_ClassDefaultObject(InObject->GetClass()) && InObject->HasAnyFlags(RF_ClassDefaultObject);
}

auto
    UCk_Utils_Object_UE::
    Get_IsArchetypeObject(
        const UObject* InObject)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Invalid Object supplied to Get_IsArchetypeObject"))
    { return {}; }

    return InObject == DoGet_ClassDefaultObject(InObject->GetClass()) && InObject->HasAnyFlags(RF_ArchetypeObject);
}

auto
    UCk_Utils_Object_UE::
    Get_ObjectNativeParentClass(
        const UObject* InObject)
    -> UClass*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Invalid Object supplied to Get_ObjectNativeParentClass"))
    { return {}; }

    const auto& NativeParentClass = [&]() -> UClass*
    {
        auto ObjectClass = InObject->GetClass();
        auto BlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(ObjectClass);

        while (ck::IsValid(BlueprintGeneratedClass, ck::IsValid_Policy_NullptrOnly{}))
        {
            ObjectClass = ObjectClass->GetSuperClass();
            BlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(ObjectClass);
        }

        return ObjectClass;
    }();

    return NativeParentClass;
}

auto
    UCk_Utils_Object_UE::
    Get_BlueprintGeneratedClass(
        const UObject* InBlueprintObject)
    -> UClass*
{
    const auto& Blueprint = Cast<UBlueprint>(InBlueprintObject);

    CK_ENSURE_IF_NOT(ck::IsValid(Blueprint),
        TEXT("Object [{}] supplied to Get_BlueprintGeneratedClass is Invalid OR is NOT of type UBlueprint!"),
        InBlueprintObject)
    { return {}; }

    return Blueprint->GeneratedClass;
}

auto
    UCk_Utils_Object_UE::
    Get_ClassGeneratedByBlueprint(
        const UClass* InBlueprintGeneratedClass)
    -> UObject*
{
#if WITH_EDITOR
    CK_ENSURE_IF_NOT(ck::IsValid(InBlueprintGeneratedClass),
        TEXT("Class [{}] supplied to Get_ClassGeneratedByBlueprint is Invalid"),
        InBlueprintGeneratedClass)
    { return {}; }

    const auto& Bpgc = Cast<UBlueprintGeneratedClass>(InBlueprintGeneratedClass);

    if(ck::Is_NOT_Valid(Bpgc))
    { return {}; }

    return Bpgc->ClassGeneratedBy;
#else
    return nullptr;
#endif
}

auto
    UCk_Utils_Object_UE::
    Get_DerivedClasses(
        const UClass* InBaseClass,
        bool InRecursive)
    -> TArray<UClass*>
{
    auto Result = TArray<UClass*>{};
    GetDerivedClasses(InBaseClass, Result, InRecursive);
    return Result;
}

auto
    UCk_Utils_Object_UE::
    Cast_ObjectToInterface(
        UObject* InInterfaceObject)
    -> TScriptInterface<UInterface>
{
    return Cast<UInterface>(InInterfaceObject);
}

auto
    UCk_Utils_Object_UE::
    Cast_ClassToInterface(
        // ReSharper disable once CppPassValueParameterByConstReference
        TSubclassOf<UClass> InInterfaceClass)
    -> TSubclassOf<UInterface>
{
    return InInterfaceClass.Get();
}

auto
    UCk_Utils_Object_UE::
    DoGet_ClassDefaultObject(
        TSubclassOf<UObject> InObject)
    -> UObject*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Invalid Class supplied to DoGet_ClassDefaultObject"))
    { return {}; }

    if (InObject.Get()->GetOuter() == nullptr)
    { return {}; }

    return InObject->GetDefaultObject();
}

auto
    UCk_Utils_Object_UE::
    DoGet_ClassDefaultObject_UpToDate(
        TSubclassOf<UObject> InObject)
    -> UObject*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Invalid Class supplied to DoGet_ClassDefaultObject_UpToDate"))
    { return {}; }

    const auto& DefaultClass = Get_DefaultClass_UpToDate(InObject->GetClass());

    CK_ENSURE_IF_NOT(ck::IsValid(DefaultClass), TEXT("Could not get the Default Class (Up-To-Date) of Object [{}]"), InObject)
    { return {}; }

    CK_ENSURE_IF_NOT(NOT DefaultClass->bLayoutChanging,
        TEXT("Default Class (Up-To-Date) of Object [{}] has its layout changing! Cannot retrieve its CDO!\n"
             "Are we trying to get the CDO of an object still being compiled ?"), InObject)
    { return {}; }

    constexpr auto CreateIfNeeded = true;
    return DefaultClass->GetDefaultObject(CreateIfNeeded);
}

auto
    UCk_Utils_Object_UE::
    DoRequest_CallFunctionByName(
        UObject* InObject,
        FName InFunctionName,
        bool InEnsureFunctionExists,
        void* InFunctionParams)
    -> ECk_SucceededFailed
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Object is not valid!"))
    { return ECk_SucceededFailed::Failed; }

    if (auto* FoundFunction = InObject->FindFunction(InFunctionName);
        ck::IsValid(FoundFunction))
    {
        InObject->ProcessEvent(FoundFunction, InFunctionParams);
        return ECk_SucceededFailed::Succeeded;
    }

    if (InEnsureFunctionExists)
    {
        CK_TRIGGER_ENSURE(TEXT("Couldn't find function: [{}] in object [{}]"), InFunctionName, InObject);
    }
    else
    {
        ck::core::VeryVerbose(TEXT("Couldn't find function: [{}] in object [{}]"), InFunctionName, InObject);
    }

    return ECk_SucceededFailed::Failed;
}

// --------------------------------------------------------------------------------------------------------------------
// [Ck][Diag] AngelScript runtime-asset GC diagnostic.
// Dumps every object under /Script/AngelscriptAssets with its (and its outer's) GC-relevant
// internal-object-flag picture, forces a full GC, then dumps again. Diagnostic-only — no
// behavior change. Drives the root-cause investigation of why `asset ... of` sub-objects
// (item traits, PlayerMappableKeySettings) are reclaimed on the first sweep even though their
// rooted owner references them via a UPROPERTY. Grep the log for [ASGCDIAG].
// --------------------------------------------------------------------------------------------------------------------
namespace ck_asgcdiag
{
    static auto DescribeGcFlags(UObject* InObject) -> FString
    {
        const auto* Item = GUObjectArray.ObjectToObjectItem(InObject);
        const auto ClusterRootIndex = Item != nullptr ? Item->GetOwnerIndex() : 0;

        return FString::Printf(
            TEXT("Rooted=%d Native=%d LoaderImport=%d ClusterRoot=%d Garbage=%d Unreachable=%d Disregard=%d ClusterRootIdx=%d"),
            InObject->IsRooted() ? 1 : 0,
            InObject->HasAnyInternalFlags(EInternalObjectFlags::Native) ? 1 : 0,
            InObject->HasAnyInternalFlags(EInternalObjectFlags::LoaderImport) ? 1 : 0,
            InObject->HasAnyInternalFlags(EInternalObjectFlags::ClusterRoot) ? 1 : 0,
            InObject->HasAnyInternalFlags(EInternalObjectFlags::Garbage) ? 1 : 0,
            InObject->HasAnyInternalFlags(EInternalObjectFlags::Unreachable) ? 1 : 0,
            GUObjectArray.IsDisregardForGC(InObject) ? 1 : 0,
            ClusterRootIndex);
    }

    static auto DumpAngelscriptAssets(const TCHAR* InLabel) -> void
    {
        auto* AssetsPackage = FindObject<UPackage>(nullptr, TEXT("/Script/AngelscriptAssets"));
        if (AssetsPackage == nullptr)
        {
            UE_LOG(LogTemp, Warning, TEXT("[ASGCDIAG][%s] pkg /Script/AngelscriptAssets NOT FOUND"), InLabel);
            return;
        }

        auto Total = 0;
        auto NotRooted = 0;
        auto Garbage = 0;
        // Among the NotRooted (the at-risk sub-objects), tally what their OWNER looks like — this
        // single summary instantly discriminates the leading hypotheses.
        auto NotRooted_OwnerClusterRoot = 0;
        auto NotRooted_OwnerNative = 0;
        auto NotRooted_OwnerLoaderImport = 0;
        auto NotRooted_OwnerDisregard = 0;
        auto NotRooted_SelfClusterMember = 0;

        constexpr auto bIncludeNestedObjects = true;
        ForEachObjectWithOuter(AssetsPackage, [&](UObject* O)
        {
            const auto bRooted = O->IsRooted();
            const auto bGarbage = O->HasAnyInternalFlags(EInternalObjectFlags::Garbage | EInternalObjectFlags::Unreachable);
            auto* Outer = O->GetOuter();

            UE_LOG(LogTemp, Warning, TEXT("[ASGCDIAG][%s] OBJ %s | class=%s | %s || OUTER %s | %s"),
                InLabel,
                *O->GetName(),
                *GetNameSafe(O->GetClass()),
                *DescribeGcFlags(O),
                *GetNameSafe(Outer),
                Outer != nullptr ? *DescribeGcFlags(Outer) : TEXT("(none)"));

            ++Total;
            if (bGarbage) { ++Garbage; }
            if (NOT bRooted)
            {
                ++NotRooted;

                const auto* SelfItem = GUObjectArray.ObjectToObjectItem(O);
                if (SelfItem != nullptr && SelfItem->GetOwnerIndex() > 0) { ++NotRooted_SelfClusterMember; }

                if (Outer != nullptr)
                {
                    if (Outer->HasAnyInternalFlags(EInternalObjectFlags::ClusterRoot))   { ++NotRooted_OwnerClusterRoot; }
                    if (Outer->HasAnyInternalFlags(EInternalObjectFlags::Native))        { ++NotRooted_OwnerNative; }
                    if (Outer->HasAnyInternalFlags(EInternalObjectFlags::LoaderImport))  { ++NotRooted_OwnerLoaderImport; }
                    if (GUObjectArray.IsDisregardForGC(Outer))                           { ++NotRooted_OwnerDisregard; }
                }
            }
        }, bIncludeNestedObjects);

        UE_LOG(LogTemp, Warning,
            TEXT("[ASGCDIAG][%s] === Total %d | NotRooted %d | Garbage %d || NotRooted-owner: ClusterRoot %d Native %d LoaderImport %d Disregard %d | self-ClusterMember %d ==="),
            InLabel, Total, NotRooted, Garbage,
            NotRooted_OwnerClusterRoot, NotRooted_OwnerNative, NotRooted_OwnerLoaderImport, NotRooted_OwnerDisregard,
            NotRooted_SelfClusterMember);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Phase-1 scope probe — the CDO side of the same bug.
    //
    // The literal-asset dump above covers Phase 2 (our minted sub-objects under disregard owners in
    // /Script/AngelscriptAssets). Phase 1 lives in /Script/Angelscript: each AS class CDO is a
    // disregard object, and CkDeferredAssetInit re-runs `default X = assets::load::...` on it, leaving
    // the disregard CDO pointing at a normal-pool cooked asset. Those are the refs the GC verifier
    // (GarbageCollectionVerification.cpp:110) flags. We enumerate them directly here: for every CDO,
    // FindReferences with NO outer limit (LimitOuter=nullptr) so EXTERNAL asset refs are captured —
    // unlike Phase 2's owner-scoped, direct-outer finder. A ref is a violation iff its referencing CDO
    // is disregard AND the ref is not rooted, not disregard, not garbage, not a cluster member.
    // ----------------------------------------------------------------------------------------------------------------
    static auto DumpAngelscriptCDOs(const TCHAR* InLabel) -> void
    {
        auto* AsPackage = FindObject<UPackage>(nullptr, TEXT("/Script/Angelscript"));
        if (AsPackage == nullptr)
        {
            UE_LOG(LogTemp, Warning, TEXT("[ASGCDIAG-CDO][%s] pkg /Script/Angelscript NOT FOUND"), InLabel);
            return;
        }

        auto CdoTotal           = 0;
        auto CdoDisregard       = 0;
        auto ViolatingRefs      = 0;
        auto ViolatorBytes      = uint64{0};
        auto DistinctViolators  = TSet<FString>{};

        constexpr auto bIncludeNestedObjects = false;
        ForEachObjectWithOuter(AsPackage, [&](UObject* O)
        {
            auto* AsUClass = Cast<UClass>(O);
            if (AsUClass == nullptr)
            { return; }

            constexpr auto bCreateIfNeeded = false;
            auto* CDO = AsUClass->GetDefaultObject(bCreateIfNeeded);
            if (CDO == nullptr)
            { return; }

            ++CdoTotal;
            const auto bCdoDisregard = GUObjectArray.IsDisregardForGC(CDO);
            if (bCdoDisregard) { ++CdoDisregard; }

            // Only disregard CDOs can trip the verifier — skip the rest (e.g. classes whose CDO landed
            // in the normal pool would be traced normally).
            if (NOT bCdoDisregard)
            { return; }

            auto OutRefs = TArray<UObject*>{};
            constexpr auto bRequireDirectOuter = false;
            auto Finder = FReferenceFinder{OutRefs, nullptr, bRequireDirectOuter};
            Finder.FindReferences(CDO);

            for (auto* Ref : OutRefs)
            {
                if (Ref == nullptr)
                { continue; }

                const auto bRooted    = Ref->IsRooted();
                const auto bDisregard = GUObjectArray.IsDisregardForGC(Ref);
                const auto bGarbage   = Ref->HasAnyInternalFlags(EInternalObjectFlags::Garbage | EInternalObjectFlags::Unreachable);
                const auto* RefItem   = GUObjectArray.ObjectToObjectItem(Ref);
                const auto bCluster   = RefItem != nullptr && (RefItem->GetOwnerIndex() > 0 || RefItem->HasAnyFlags(EInternalObjectFlags::ClusterRoot));

                const auto bViolation = NOT bRooted && NOT bDisregard && NOT bGarbage && NOT bCluster;
                if (NOT bViolation)
                { continue; }

                ++ViolatingRefs;
                DistinctViolators.Add(FString::Printf(TEXT("%s:%s"), *GetNameSafe(Ref->GetClass()), *GetNameSafe(Ref)));
                ViolatorBytes += static_cast<uint64>(Ref->GetResourceSizeBytes(EResourceSizeMode::EstimatedTotal));

                UE_LOG(LogTemp, Warning, TEXT("[ASGCDIAG-CDO][%s] VIOLATION CDO %s -> %s | class=%s | %s"),
                    InLabel, *CDO->GetName(), *Ref->GetName(), *GetNameSafe(Ref->GetClass()), *DescribeGcFlags(Ref));
            }
        }, bIncludeNestedObjects);

        UE_LOG(LogTemp, Warning,
            TEXT("[ASGCDIAG-CDO][%s] === CDOs %d | Disregard-CDOs %d | Phase1-violating-refs %d | distinct-assets %d | ~%llu KB ==="),
            InLabel, CdoTotal, CdoDisregard, ViolatingRefs, DistinctViolators.Num(), ViolatorBytes / 1024);
    }

    static FAutoConsoleCommand GCmd_DumpAngelscriptAssets(
        TEXT("Ck.Diag.DumpAngelscriptAssets"),
        TEXT("[Ck][Diag] dump AS asset+subobject+CDO GC state (with owner flags), force a full GC, dump again"),
        FConsoleCommandDelegate::CreateLambda([]()
        {
            DumpAngelscriptAssets(TEXT("BEFORE-GC"));
            DumpAngelscriptCDOs(TEXT("BEFORE-GC"));
            constexpr auto bFullPurge = true;
            CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, bFullPurge);
            DumpAngelscriptAssets(TEXT("AFTER-GC"));
            DumpAngelscriptCDOs(TEXT("AFTER-GC"));
        }));

    // The authoritative oracle: turn the engine's own disregard-for-GC verifier on, then force a
    // full-purge GC. It logs every "Disregard for GC object X referencing Y which is not part of root
    // set" as a Warning, then Fatals (GarbageCollectionVerification.cpp:155). All warnings flush to the
    // log before the crash, so this enumerates the COMPLETE violation list (Phase 1 + Phase 2). Use to
    // cross-validate the structured dumps above. Expect a Fatal on an unfixed build.
    static FAutoConsoleCommand GCmd_VerifyGCAssumptions(
        TEXT("Ck.Diag.VerifyGCAssumptions"),
        TEXT("[Ck][Diag] enable gc.VerifyAssumptions(+OnFullPurge), force a full-purge GC (enumerates violations as Warnings, then Fatals)"),
        FConsoleCommandDelegate::CreateLambda([]()
        {
            if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("gc.VerifyAssumptions")))
            { CVar->Set(1); }
            if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("gc.VerifyAssumptionsOnFullPurge")))
            { CVar->Set(1); }
            constexpr auto bFullPurge = true;
            CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, bFullPurge);
        }));
}

// --------------------------------------------------------------------------------------------------------------------