#include "CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

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
    -> bool
{
    const auto& Object = InParams.Get_Object();
    CK_ENSURE_IF_NOT(ck::IsValid(Object), TEXT("Object is not valid!"))
    { return {}; }

    const auto& Outer = InParams.Get_Outer();
    CK_ENSURE_IF_NOT(ck::IsValid(Outer), TEXT("Outer is not valid!"))
    { return {}; }

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
    { return {}; }

    return true;
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
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Invalid Object supplied to Request_ResetAllPropertiesToDefault"))
    { return {}; }

    if (Get_IsDefaultObject(InObject))
    { return true; }

    const auto& ObjectCDO = DoGet_ClassDefaultObject(InObject->GetClass());

    const auto CopyAllPropertiesParams = FCk_Utils_Object_CopyAllProperties_Params{}
                                            .Set_Destination(InObject)
                                            .Set_Source(ObjectCDO);

    const auto& Result = Request_CopyAllProperties(CopyAllPropertiesParams);

    return Result == ECk_Utils_Object_CopyAllProperties_Result::Failed ? false : true;
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
        UObject* InObject)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Invalid Object supplied to Get_IsDefaultObject"))
    { return {}; }

    return InObject == DoGet_ClassDefaultObject(InObject->GetClass()) && InObject->HasAnyFlags(RF_ClassDefaultObject);
}

auto
    UCk_Utils_Object_UE::
    Get_ObjectNativeParentClass(
        UObject* InObject)
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
        UObject* InBlueprintObject)
    -> UClass*
{
    const auto& Blueprint = Cast<UBlueprint>(InBlueprintObject);

    CK_ENSURE_IF_NOT(ck::IsValid(InBlueprintObject),
        TEXT("Object [{}] supplied to Get_BlueprintGeneratedClass is Invalid OR is NOT of type UBlueprint!"),
        InBlueprintObject)
    { return {}; }

    return Blueprint->GeneratedClass;
}

auto
    UCk_Utils_Object_UE::
    Get_ClassGeneratedByBlueprint(
        UClass* InBlueprintGeneratedClass)
    -> UObject*
{
#if WITH_EDITOR
    const auto& Bpgc = Cast<UBlueprintGeneratedClass>(InBlueprintGeneratedClass);

    CK_ENSURE_IF_NOT(ck::IsValid(Bpgc),
        TEXT("Class [{}] supplied to Get_ClassGeneratedByBlueprint is Invalid OR is NOT of type UBlueprintGeneratedClass!"),
        InBlueprintGeneratedClass)
    { return {}; }

    return Bpgc->ClassGeneratedBy;
#else
    return nullptr;
#endif
}

auto
    UCk_Utils_Object_UE::
    DoGet_ClassDefaultObject(
        TSubclassOf<UObject> InObject)
    -> UObject*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Invalid Class supplied to DoGet_ClassDefaultObject"))
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

    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Could not get the Default Class Up-To-Date of Object [{}]"), InObject)
    { return {}; }

    return DefaultClass->GetDefaultObject();
}

// --------------------------------------------------------------------------------------------------------------------


