#include "CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Utils_Object_CopyAllProperties_Params::FCk_Utils_Object_CopyAllProperties_Params(TObjectPtr<UObject> InDestination,
    TObjectPtr<UObject> InSource)
        : _Destination(InDestination)
        , _Source(InSource)
{ }

auto UCk_Utils_Object_UE::
Request_CopyAllProperties(const FCk_Utils_Object_CopyAllProperties_Params& InParams) -> ECk_Utils_Object_CopyAllProperties_Result
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

UClass* UCk_Utils_Object_UE::Get_DefaultClass_UpToDate(UClass* InClass)
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

// --------------------------------------------------------------------------------------------------------------------


