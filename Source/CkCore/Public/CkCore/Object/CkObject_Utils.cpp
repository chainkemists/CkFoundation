#include "CkObject_Utils.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "CkCore/CkCoreLog.h"
#include "Interfaces/IPluginManager.h"

#include <Engine/Blueprint.h>
#include <Engine/BlueprintGeneratedClass.h>

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
// ASSET LOADING IMPLEMENTATION
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Object_UE::
    LoadAssetsByName(
        const FString& AssetName,
        ECk_Utils_Object_AssetSearchScope SearchScope,
        ECk_Utils_Object_AssetSearchStrategy SearchStrategy)
    -> FCk_Utils_Object_AssetSearchResult_Array
{
    return DoLoadAssetsByName(AssetName, nullptr, SearchScope, SearchStrategy);
}

auto
    UCk_Utils_Object_UE::
    LoadAssetsByName_FilterByClass(
        const FString& AssetName,
        TSubclassOf<UObject> AssetClass,
        ECk_Utils_Object_AssetSearchScope SearchScope,
        ECk_Utils_Object_AssetSearchStrategy SearchStrategy)
    -> FCk_Utils_Object_AssetSearchResult_Array
{
    CK_ENSURE_IF_NOT(ck::IsValid(AssetClass), TEXT("Asset class cannot be null"))
    {
        return FCk_Utils_Object_AssetSearchResult_Array{};
    }

    return DoLoadAssetsByName(AssetName, AssetClass.Get(), SearchScope, SearchStrategy);
}

auto
    UCk_Utils_Object_UE::
    LoadAssetByName(
        const FString& AssetName,
        ECk_Utils_Object_AssetSearchScope SearchScope,
        ECk_Utils_Object_AssetSearchStrategy SearchStrategy)
    -> FCk_Utils_Object_AssetSearchResult_Single
{
    const auto& ArrayResult = LoadAssetsByName(AssetName, SearchScope, SearchStrategy);

    auto SingleResult = FCk_Utils_Object_AssetSearchResult_Single{};

    if (ArrayResult.Get_Results().Num() > 0)
    {
        const auto& FirstResult = ArrayResult.Get_Results()[0];
        SingleResult._Asset = FirstResult.Get_Asset();
        SingleResult._AssetName = FirstResult.Get_AssetName();
        SingleResult._AssetPath = FirstResult.Get_AssetPath();
        SingleResult._MatchType = FirstResult.Get_MatchType();
        SingleResult._UniqueAsset = ArrayResult.Get_Results().Num() == 1 ? ECk_Unique::Unique : ECk_Unique::NotUnique;
    }
    else
    {
        SingleResult._UniqueAsset = ECk_Unique::DoesNotExist;
    }

    return SingleResult;
}

auto
    UCk_Utils_Object_UE::
    LoadAssetByName_FilterByClass(
        const FString& AssetName,
        TSubclassOf<UObject> AssetClass,
        ECk_Utils_Object_AssetSearchScope SearchScope,
        ECk_Utils_Object_AssetSearchStrategy SearchStrategy)
    -> FCk_Utils_Object_AssetSearchResult_Single
{
    const auto& ArrayResult = LoadAssetsByName_FilterByClass(AssetName, AssetClass, SearchScope, SearchStrategy);

    auto SingleResult = FCk_Utils_Object_AssetSearchResult_Single{};

    if (ArrayResult.Get_Results().Num() > 0)
    {
        const auto& FirstResult = ArrayResult.Get_Results()[0];
        SingleResult._Asset = FirstResult.Get_Asset();
        SingleResult._AssetName = FirstResult.Get_AssetName();
        SingleResult._AssetPath = FirstResult.Get_AssetPath();
        SingleResult._MatchType = FirstResult.Get_MatchType();
        SingleResult._UniqueAsset = ArrayResult.Get_Results().Num() == 1 ? ECk_Unique::Unique : ECk_Unique::NotUnique;
    }
    else
    {
        SingleResult._UniqueAsset = ECk_Unique::DoesNotExist;
    }

    return SingleResult;
}

auto
    UCk_Utils_Object_UE::
    FindAssetsByName(
        const FString& AssetName,
        ECk_Utils_Object_AssetSearchScope SearchScope)
    -> TArray<FString>
{
    auto Results = TArray<FString>{};

    if (AssetName.IsEmpty())
    {
        ck::core::Warning(TEXT("FindAssetsByName: Asset name cannot be empty"));
        return Results;
    }

    // Get the asset registry
    auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    auto& AssetRegistry = AssetRegistryModule.Get();

    // Get search paths
    const auto& SearchPaths = Get_SearchPaths(SearchScope);

    // Create filter to search assets
    auto Filter = FARFilter{};
    Filter.bRecursivePaths = true;

    // Convert FName array to FString array for this legacy function
    auto SearchPathsAsStrings = TArray<FString>{};
    for (const auto& Path : SearchPaths)
    {
        SearchPathsAsStrings.Add(Path.ToString());
    }

    Filter.PackagePaths = SearchPaths;

    // Get assets
    auto AssetDataList = TArray<FAssetData>{};
    AssetRegistry.GetAssets(Filter, AssetDataList);

    // Look for matches
    for (const auto& AssetData : AssetDataList)
    {
        const auto& CurrentAssetName = AssetData.AssetName.ToString();
        if (CurrentAssetName.Contains(AssetName, ESearchCase::IgnoreCase))
        {
            const auto& AssetInfo = FString::Printf(TEXT("%s (%s) - %s"),
                *CurrentAssetName,
                *AssetData.GetSoftObjectPath().ToString(),
                *AssetData.GetSoftObjectPath().ToString());
            Results.Add(AssetInfo);
        }
    }

    return Results;
}

auto
    UCk_Utils_Object_UE::
    DoLoadAssetsByName(
        const FString& AssetName,
        UClass* AssetClass,
        ECk_Utils_Object_AssetSearchScope SearchScope,
        ECk_Utils_Object_AssetSearchStrategy SearchStrategy)
    -> FCk_Utils_Object_AssetSearchResult_Array
{
    if (AssetName.IsEmpty())
    {
        ck::core::Error(TEXT("DoLoadAssetsByName: Asset name cannot be empty"));
        return FCk_Utils_Object_AssetSearchResult_Array{};
    }

    // OPTIMIZATION: Use fast exact lookups for ExactOnly and ExactThenFuzzy strategies
    if (SearchStrategy == ECk_Utils_Object_AssetSearchStrategy::ExactOnly)
    {
        return DoFastExactLookup(AssetName, AssetClass, SearchScope);
    }

    if (SearchStrategy == ECk_Utils_Object_AssetSearchStrategy::ExactThenFuzzy)
    {
        // Try fast exact lookup first
        auto ExactResults = DoFastExactLookup(AssetName, AssetClass, SearchScope);
        if (ExactResults.Get_Results().Num() > 0)
        {
            return ExactResults; // Found exact matches - stop here!
        }

        // No exact matches found - fall back to fuzzy search
        return DoFuzzySearch(AssetName, AssetClass, SearchScope);
    }

    // For FuzzyOnly and Both strategies, use the original full-scan approach
    return DoFullAssetScan(AssetName, AssetClass, SearchScope, SearchStrategy);
}

auto
    UCk_Utils_Object_UE::
    DoFastExactLookup(
        const FString& AssetName,
        UClass* AssetClass,
        ECk_Utils_Object_AssetSearchScope SearchScope)
    -> FCk_Utils_Object_AssetSearchResult_Array
{
    auto Result = FCk_Utils_Object_AssetSearchResult_Array{};

    // Get the asset registry
    auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    auto& AssetRegistry = AssetRegistryModule.Get();

    auto FoundAssets = TArray<FAssetData>{};

    // Try direct path lookup first (if it looks like a path)
    if (AssetName.StartsWith(TEXT("/")))
    {
        ck::core::VeryVerbose(TEXT("DoFastExactLookup: Attempting direct path lookup for: '{}'"), *AssetName);

        const auto& AssetPath = FSoftObjectPath(AssetName);
        const auto& AssetData = AssetRegistry.GetAssetByObjectPath(AssetPath);

        ck::core::VeryVerbose(TEXT("DoFastExactLookup: Direct path lookup - Valid: {}"), AssetData.IsValid());

        if (AssetData.IsValid())
        {
            // For exact paths, skip search scope validation - user knows where the asset is
            // Only validate class filter if specified
            auto ClassMatches = true;
            if (ck::IsValid(AssetClass, ck::IsValid_Policy_NullptrOnly{}))
            {
                if (NOT AssetData.GetClass()->IsChildOf(AssetClass))
                {
                    ck::core::Warning(TEXT("DoFastExactLookup: Asset '{}' found but wrong class"), *AssetName);
                    ClassMatches = false;
                }
            }

            if (ClassMatches)
            {
                // FAST PATH: Load immediately and return
                auto LoadedAsset = AssetData.GetAsset();
                if (ck::IsValid(LoadedAsset, ck::IsValid_Policy_NullptrOnly{}))
                {
                    auto SingleResult = FCk_Utils_Object_AssetSearchResult_Single{};
                    SingleResult._Asset = LoadedAsset;
                    SingleResult._AssetName = AssetData.AssetName.ToString();
                    SingleResult._AssetPath = AssetData.GetSoftObjectPath().ToString();
                    SingleResult._MatchType = ECk_Utils_Object_AssetMatchType::ExactMatch;
                    SingleResult._UniqueAsset = ECk_Unique::Unique;

                    Result._Results.Add(SingleResult);
                    Result._ExactMatchCount = 1;
                    Result._FuzzyMatchCount = 0;

                    ck::core::VeryVerbose(TEXT("DoFastExactLookup: FAST PATH - Successfully loaded asset '{}' from direct path '{}'"),
                        *LoadedAsset->GetName(), *AssetName);

                    return Result; // IMMEDIATE RETURN - No further processing needed!
                }
            }
        }
    }

    // If no path-based match, try filtering all assets by name
    if (FoundAssets.Num() == 0)
    {
        // Create a filter to find assets by exact name
        auto NameFilter = FARFilter{};
        NameFilter.bRecursivePaths = true;
        NameFilter.PackagePaths = Get_SearchPaths(SearchScope);

        if (ck::IsValid(AssetClass, ck::IsValid_Policy_NullptrOnly{}))
        {
            NameFilter.ClassPaths.Add(AssetClass->GetClassPathName());
        }

        auto AllAssets = TArray<FAssetData>{};
        AssetRegistry.GetAssets(NameFilter, AllAssets);

        // Filter by exact name match
        for (const auto& AssetData : AllAssets)
        {
            if (AssetData.AssetName.ToString().Equals(AssetName, ESearchCase::IgnoreCase))
            {
                FoundAssets.Add(AssetData);
            }
        }

        ck::core::VeryVerbose(TEXT("DoFastExactLookup: Found {} assets by name lookup for '{}'"),
            FoundAssets.Num(), *AssetName);
    }

    // Filter results by search scope and class
    const auto& SearchPaths = Get_SearchPaths(SearchScope);
    auto FilteredAssets = TArray<FAssetData>{};

    for (const auto& AssetData : FoundAssets)
    {
        // Check if asset is in allowed search paths
        auto AssetInScope = false;
        const auto& AssetPath = AssetData.PackageName.ToString();

        for (const auto& SearchPath : SearchPaths)
        {
            if (AssetPath.StartsWith(SearchPath.ToString()))
            {
                AssetInScope = true;
                break;
            }
        }

        if (NOT AssetInScope)
        {
            continue;
        }

        // Check class filter
        if (ck::IsValid(AssetClass, ck::IsValid_Policy_NullptrOnly{}))
        {
            if (NOT AssetData.GetClass()->IsChildOf(AssetClass))
            {
                continue;
            }
        }

        FilteredAssets.Add(AssetData);
    }

    // Load the filtered assets and build results
    for (const auto& AssetData : FilteredAssets)
    {
        auto LoadedAsset = AssetData.GetAsset();

        if (ck::IsValid(LoadedAsset, ck::IsValid_Policy_NullptrOnly{}))
        {
            auto SingleResult = FCk_Utils_Object_AssetSearchResult_Single{};
            SingleResult._Asset = LoadedAsset;
            SingleResult._AssetName = AssetData.AssetName.ToString();
            SingleResult._AssetPath = AssetData.GetSoftObjectPath().ToString();
            SingleResult._MatchType = ECk_Utils_Object_AssetMatchType::ExactMatch;
            SingleResult._UniqueAsset = ECk_Unique::Unique; // Will be overridden by caller

            Result._Results.Add(SingleResult);

            ck::core::VeryVerbose(TEXT("DoFastExactLookup: Successfully loaded asset '{}' from path '{}'"),
                *LoadedAsset->GetName(), *AssetData.GetSoftObjectPath().ToString());
        }
        else
        {
            ck::core::Error(TEXT("DoFastExactLookup: Failed to load asset from path '{}'"),
                *AssetData.GetSoftObjectPath().ToString());
        }
    }

    // Set counts - all matches are exact
    Result._ExactMatchCount = Result._Results.Num();
    Result._FuzzyMatchCount = 0;

    return Result;
}

auto
    UCk_Utils_Object_UE::
    DoFuzzySearch(
        const FString& AssetName,
        UClass* AssetClass,
        ECk_Utils_Object_AssetSearchScope SearchScope)
    -> FCk_Utils_Object_AssetSearchResult_Array
{
    // For fuzzy search, we need to scan all assets (can't optimize this easily)
    return DoFullAssetScan(AssetName, AssetClass, SearchScope, ECk_Utils_Object_AssetSearchStrategy::FuzzyOnly);
}

auto
    UCk_Utils_Object_UE::
    DoFullAssetScan(
        const FString& AssetName,
        UClass* AssetClass,
        ECk_Utils_Object_AssetSearchScope SearchScope,
        ECk_Utils_Object_AssetSearchStrategy SearchStrategy)
    -> FCk_Utils_Object_AssetSearchResult_Array
{
    auto Result = FCk_Utils_Object_AssetSearchResult_Array{};

    // Get the asset registry
    auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    auto& AssetRegistry = AssetRegistryModule.Get();

    // Ensure asset registry is loaded, especially for plugin content
    if (NOT AssetRegistry.IsLoadingAssets())
    {
        AssetRegistry.SearchAllAssets(true);
    }

    // Get search paths
    const auto& SearchPaths = Get_SearchPaths(SearchScope);

    // Create filter to search assets
    auto Filter = FARFilter{};
    Filter.bRecursivePaths = true;
    Filter.PackagePaths = SearchPaths;

    if (ck::IsValid(AssetClass, ck::IsValid_Policy_NullptrOnly{}))
    {
        Filter.ClassPaths.Add(AssetClass->GetClassPathName());
    }

    // Get assets
    auto AssetDataList = TArray<FAssetData>{};
    AssetRegistry.GetAssets(Filter, AssetDataList);

    // Debug logging for plugin search issues
    if (EnumHasAnyFlags(SearchScope, ECk_Utils_Object_AssetSearchScope::Plugins))
    {
        ck::core::VeryVerbose(TEXT("DoFullAssetScan: Searching for '{}' in {} total assets across search paths"),
            *AssetName, AssetDataList.Num());

        for (const auto& Path : SearchPaths)
        {
            auto PathFilter = FARFilter{};
            PathFilter.bRecursivePaths = true;
            PathFilter.PackagePaths.Add(Path);

            auto PathAssets = TArray<FAssetData>{};
            AssetRegistry.GetAssets(PathFilter, PathAssets);

            ck::core::VeryVerbose(TEXT("DoFullAssetScan: Found {} assets in path '{}'"),
                PathAssets.Num(), *Path.ToString());
        }
    }

    auto ExactMatches = TArray<FAssetData>{};
    auto FuzzyMatches = TArray<FAssetData>{};

    // Separate exact and fuzzy matches
    for (const auto& AssetData : AssetDataList)
    {
        const auto& CurrentAssetName = AssetData.AssetName.ToString();
        if (CurrentAssetName.Equals(AssetName, ESearchCase::IgnoreCase))
        {
            ExactMatches.Add(AssetData);
        }
        else if (CurrentAssetName.Contains(AssetName, ESearchCase::IgnoreCase))
        {
            FuzzyMatches.Add(AssetData);
        }
    }

    // Apply search strategy
    auto AssetsToLoad = TArray<FAssetData>{};
    auto ExactMatchesToLoad = TArray<FAssetData>{};
    auto FuzzyMatchesToLoad = TArray<FAssetData>{};

    switch (SearchStrategy)
    {
        case ECk_Utils_Object_AssetSearchStrategy::ExactOnly:
            AssetsToLoad = ExactMatches;
            ExactMatchesToLoad = ExactMatches;
            break;

        case ECk_Utils_Object_AssetSearchStrategy::FuzzyOnly:
            AssetsToLoad = FuzzyMatches;
            FuzzyMatchesToLoad = FuzzyMatches;
            break;

        case ECk_Utils_Object_AssetSearchStrategy::ExactThenFuzzy:
            if (ExactMatches.Num() > 0)
            {
                AssetsToLoad = ExactMatches;
                ExactMatchesToLoad = ExactMatches;
            }
            else
            {
                AssetsToLoad = FuzzyMatches;
                FuzzyMatchesToLoad = FuzzyMatches;
            }
            break;

        case ECk_Utils_Object_AssetSearchStrategy::Both:
            AssetsToLoad = ExactMatches;
            AssetsToLoad.Append(FuzzyMatches);
            ExactMatchesToLoad = ExactMatches;
            FuzzyMatchesToLoad = FuzzyMatches;
            break;
    }

    // Handle no results
    if (AssetsToLoad.Num() == 0)
    {
        const auto& ClassFilter = ck::IsValid(AssetClass, ck::IsValid_Policy_NullptrOnly{})
            ? FString::Printf(TEXT(" of class %s"), *AssetClass->GetName())
            : FString{};

        ck::core::Warning(TEXT("DoFullAssetScan: No assets{} found matching '{}'"), *ClassFilter, *AssetName);
        return FCk_Utils_Object_AssetSearchResult_Array{};
    }

    // Load assets and populate results
    for (const auto& AssetData : AssetsToLoad)
    {
        auto LoadedAsset = AssetData.GetAsset();

        if (ck::IsValid(LoadedAsset, ck::IsValid_Policy_NullptrOnly{}))
        {
            auto SingleResult = FCk_Utils_Object_AssetSearchResult_Single{};
            SingleResult._Asset = LoadedAsset;
            SingleResult._AssetName = AssetData.AssetName.ToString();
            SingleResult._AssetPath = AssetData.GetSoftObjectPath().ToString();

            // Determine match type
            const auto& IsExactMatch = ExactMatchesToLoad.ContainsByPredicate([&AssetData](const FAssetData& Data)
            {
                return Data.GetSoftObjectPath() == AssetData.GetSoftObjectPath();
            });

            SingleResult._MatchType = IsExactMatch
                ? ECk_Utils_Object_AssetMatchType::ExactMatch
                : ECk_Utils_Object_AssetMatchType::FuzzyMatch;

            // UniqueAsset will be set by the calling function
            SingleResult._UniqueAsset = ECk_Unique::Unique; // Default, will be overridden

            Result._Results.Add(SingleResult);

            ck::core::VeryVerbose(TEXT("DoFullAssetScan: Successfully loaded asset '{}' from path '{}'"),
                *LoadedAsset->GetName(), *AssetData.GetSoftObjectPath().ToString());
        }
        else
        {
            ck::core::Error(TEXT("DoFullAssetScan: Failed to load asset from path '{}'"),
                *AssetData.GetSoftObjectPath().ToString());
        }
    }

    // Set counts
    Result._ExactMatchCount = ExactMatchesToLoad.Num();
    Result._FuzzyMatchCount = FuzzyMatchesToLoad.Num();

    // Log multiple matches if necessary
    if (Result._Results.Num() > 1)
    {
        const auto& ClassFilter = ck::IsValid(AssetClass, ck::IsValid_Policy_NullptrOnly{})
            ? FString::Printf(TEXT(" %s"), *AssetClass->GetName())
            : FString{};

        auto ErrorMsg = FString::Printf(TEXT("DoFullAssetScan: Multiple%s assets found matching '%s':"),
            *ClassFilter, *AssetName);

        for (const auto& SingleResult : Result._Results)
        {
            ErrorMsg += FString::Printf(TEXT("\n  - %s (%s)"),
                *SingleResult.Get_AssetName(),
                *SingleResult.Get_AssetPath());
        }

        ck::core::Warning(TEXT("{}"), *ErrorMsg);
    }

    return Result;
}

auto
    UCk_Utils_Object_UE::
    Get_SearchPaths(
        ECk_Utils_Object_AssetSearchScope SearchScope)
    -> TArray<FName>
{
    auto SearchPaths = TArray<FName>{};

    if (EnumHasAnyFlags(SearchScope, ECk_Utils_Object_AssetSearchScope::Game))
    {
        SearchPaths.Add(FName(TEXT("/Game")));
    }

    if (EnumHasAnyFlags(SearchScope, ECk_Utils_Object_AssetSearchScope::Plugins))
    {
        // Try both the general plugins path and more specific ones
        SearchPaths.Add(FName(TEXT("/Plugins")));

        // Also try common plugin content paths
        SearchPaths.Add(FName(TEXT("/PluginContent")));

        // Get all mounted plugin paths dynamically
        auto& PluginManager = IPluginManager::Get();
        auto EnabledPlugins = PluginManager.GetEnabledPlugins();

        for (const auto& Plugin : EnabledPlugins)
        {
            if (Plugin->CanContainContent())
            {
                const auto& PluginContentPath = FString::Printf(TEXT("/%s"), *Plugin->GetName());
                SearchPaths.AddUnique(FName(*PluginContentPath));
            }
        }
    }

    if (EnumHasAnyFlags(SearchScope, ECk_Utils_Object_AssetSearchScope::Engine))
    {
        SearchPaths.Add(FName(TEXT("/Engine")));
    }

    // Fallback to Game if no scope specified
    if (SearchPaths.Num() == 0)
    {
        SearchPaths.Add(FName(TEXT("/Game")));
    }

    return SearchPaths;
}

// --------------------------------------------------------------------------------------------------------------------