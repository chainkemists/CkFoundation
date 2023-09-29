#include "CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Utils_Object_CopyAllProperties_Params::
    FCk_Utils_Object_CopyAllProperties_Params(
        UObject* InDestination,
        UObject* InSource)
    : _Destination(InDestination)
    , _Source(InSource)
{ }

// --------------------------------------------------------------------------------------------------------------------

FCk_Utils_Object_SetOuter_Params::
    FCk_Utils_Object_SetOuter_Params(
        UObject* InObject,
        UObject* InOuter,
        ECk_Utils_Object_RenameFlags InFlag)
    : _Object(InObject)
    , _Outer(InOuter)
    , _RenameFlags(InFlag)
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Object_UE::
    Request_TrySetOuter(
        const FCk_Utils_Object_SetOuter_Params& InParams)
    -> bool
{
    const auto& Object = InParams.Get_Object();
    CK_ENSURE_IF_NOT(ck::IsValid(Object), TEXT("Object is not valid!"))
    { return false; }

    const auto& Outer = InParams.Get_Outer();
    CK_ENSURE_IF_NOT(ck::IsValid(Outer), TEXT("Outer is not valid!"))
    { return false; }

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
    { return false; }

    return true;
}

auto
    UCk_Utils_Object_UE::
    Request_CopyAllProperties(
        const FCk_Utils_Object_CopyAllProperties_Params& InParams)
    -> ECk_Utils_Object_CopyAllProperties_Result
{
    using result_type = ECk_Utils_Object_CopyAllProperties_Result;

    const auto& Source = InParams.Get_Source();
    CK_ENSURE_IF_NOT(ck::IsValid(Source), TEXT("The Source in Params is not valid"))
    { return result_type::Failed; }

    const auto& Destination = InParams.Get_Destination();
    CK_ENSURE_IF_NOT(ck::IsValid(Destination), TEXT("The Destination in Params is not valid"))
    { return result_type::Failed; }

    const auto& SourceClass = Get_DefaultClass_UpToDate(Source->GetClass());

    CK_ENSURE_IF_NOT(ck::IsValid(SourceClass), TEXT("Could not get the up-to-date default class of Source object [{}]"), Source)
    { return result_type::Failed; }

    CK_ENSURE_IF_NOT(Destination->IsA(SourceClass), TEXT("The Destination [{}] and Source [{}] are not the same type"), Destination, Source)
    { return result_type::Failed; }

    auto Result = result_type::AllPropertiesIdentical;

    for (auto* Property = Destination->GetClass()->PropertyLink; ck::IsValid(Property); Property = Property->PropertyLinkNext)
    {
        if (Result == result_type::AllPropertiesIdentical && NOT Property->Identical_InContainer(Destination, Source, PPF_None))
        {
            Result = result_type::Succeeded;
        }

        Property->CopyCompleteValue_InContainer(Destination, Source);
    }

    return Result;
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

    return InObject == InObject->GetClass()->GetDefaultObject();
}

auto
    UCk_Utils_Object_UE::
    Get_ClassDefaultObject(
        TSubclassOf<UObject> InObject)
    -> UObject*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Invalid Class supplied to Get_ClassDefaultObject"))
    { return {}; }

    return InObject->GetDefaultObject();
}

// --------------------------------------------------------------------------------------------------------------------


