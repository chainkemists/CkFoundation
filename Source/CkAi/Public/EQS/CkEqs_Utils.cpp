#include "CkEqs_Utils.h"

#include "EnvironmentQuery/EnvQueryInstanceBlueprintWrapper.h"
#include "EnvironmentQuery/EnvQueryTypes.h"

// --------------------------------------------------------------------------------------------------------------------

auto 
	UCk_Utils_Eqs_UE::
	SetEqsNamedIntParam(
		UEnvQueryInstanceBlueprintWrapper* EnvQueryInstance, 
		FName	 ParamName,
		int Value)
	-> void
{
	if (!EnvQueryInstance)
		return;
	// read int bits as a float, this is jank but is what EQS expects, see FEnvQueryRequest::SetIntParam
	EnvQueryInstance->SetNamedParam(ParamName, *((float*)&Value));
}
