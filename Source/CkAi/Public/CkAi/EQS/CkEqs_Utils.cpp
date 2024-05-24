#include "CkEqs_Utils.h"

#include "CkCore/Validation/CkIsValid.h"
#include <EnvironmentQuery/EnvQueryInstanceBlueprintWrapper.h>

// --------------------------------------------------------------------------------------------------------------------

auto 
	UCk_Utils_Eqs_UE::
	SetEqsNamedIntParam(
		UEnvQueryInstanceBlueprintWrapper* InEnvQueryInstance, 
		FName InParamName,
		int32 InValue)
	-> void
{
	if (ck::Is_NOT_Valid(InEnvQueryInstance))
	{ return; }
	
	// read int bits as a float, this is jank but is what EQS expects, see FEnvQueryRequest::SetIntParam
	InEnvQueryInstance->SetNamedParam(InParamName, *reinterpret_cast<float*>(&InValue));
}

// --------------------------------------------------------------------------------------------------------------------