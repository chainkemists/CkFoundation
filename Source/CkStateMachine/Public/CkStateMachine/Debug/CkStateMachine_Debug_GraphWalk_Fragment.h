#pragma once

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Tag/CkTag.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Marks the temp entities the graph-walk processor creates to extract a sub-SM's static
    // structure. Defined unconditionally so the Create-site checks need no #if bracketing. Carriers
    // MUST be excluded from the runtime pipeline (processors TExclude it; EntityScript BeginPlay
    // skips Enter*) or their side effects corrupt the real SM.
    CK_DEFINE_ECS_TAG(FTag_Sm_Debug_GraphWalkEntity);
}

#if CK_BUILD_SM_GRAPH_WALK

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmState_EntityScript;
class UCk_SmCondition_EntityScript;
class UCk_SmTask_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Sm_Debug_GraphWalk;
    class FProcessor_Sm_Debug_GraphWalk_Iterate;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FCk_SmDebug_StateDefinition
    {
        TSubclassOf<UCk_SmState_EntityScript> StateClass;
        TSubclassOf<UCk_SmState_EntityScript> ScriptClass;
        TSubclassOf<UCk_SmState_EntityScript> RequestedScriptClass;
        FString StateName;

        struct FConditionDef
        {
            FString ClassName;
            TSubclassOf<UCk_SmCondition_EntityScript> ScriptClass;
            ECk_SmConditionMode Mode = ECk_SmConditionMode::Polled;
        };

        struct FTransitionDef
        {
            TSubclassOf<UCk_SmState_EntityScript> TargetStateClass;
            TArray<FConditionDef> Conditions;
        };

        TArray<FTransitionDef> Transitions;

        struct FTaskDef
        {
            FString ClassName;
            TSubclassOf<UCk_SmTask_EntityScript> ScriptClass;
            ECk_SmTaskMode Mode = ECk_SmTaskMode::EnterExitOnly;
            bool HasSubStateMachine = false;
            TSubclassOf<UCk_SmState_EntityScript> SubSmInitialStateClass;
        };

        TArray<FTaskDef> Tasks;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FCk_SmDebug_SubSmDefinition
    {
        TSubclassOf<UCk_SmState_EntityScript> ParentStateClass;
        TSubclassOf<UCk_SmState_EntityScript> InitialStateClass;
        TMap<TSubclassOf<UCk_SmState_EntityScript>, FCk_SmDebug_StateDefinition> StateDefinitions;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATEMACHINE_API FFragment_Sm_Debug_GraphDefinition
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_Debug_GraphDefinition);

        friend class FProcessor_Sm_Debug_GraphWalk;
        friend class FProcessor_Sm_Debug_GraphWalk_Iterate;

    private:
        TMap<TSubclassOf<UCk_SmState_EntityScript>, FCk_SmDebug_StateDefinition> _StateDefinitions;
        TMap<TSubclassOf<UCk_SmState_EntityScript>, FCk_SmDebug_SubSmDefinition> _SubSmDefinitions;
        bool _IsComplete = false;

    public:
        CK_PROPERTY_GET(_StateDefinitions);
        CK_PROPERTY_GET(_SubSmDefinitions);
        CK_PROPERTY_GET(_IsComplete);
    };

    // --------------------------------------------------------------------------------------------------------------------
    // Walk state carried across frames.

    struct CKSTATEMACHINE_API FFragment_Sm_Debug_GraphWalk_Progress
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_Debug_GraphWalk_Progress);

        friend class FProcessor_Sm_Debug_GraphWalk;
        friend class FProcessor_Sm_Debug_GraphWalk_Iterate;

    private:
        TSet<TSubclassOf<UCk_SmState_EntityScript>> _Visited;
        TArray<TSubclassOf<UCk_SmState_EntityScript>> _PendingDiscovery;

        struct FPendingEntity
        {
            TSubclassOf<UCk_SmState_EntityScript> StateClass;
            FCk_Handle EntityHandle;
        };

        TArray<FPendingEntity> _PendingEntities;

        TMap<TSubclassOf<UCk_SmState_EntityScript>, FCk_SmDebug_StateDefinition> _StateDefinitions;
        TMap<TSubclassOf<UCk_SmState_EntityScript>, FCk_SmDebug_SubSmDefinition> _SubSmDefinitions;

        TArray<TPair<TSubclassOf<UCk_SmState_EntityScript>, TSubclassOf<UCk_SmState_EntityScript>>> _PendingSubSmWalks;
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_Sm_Debug_RequiresGraphWalk);
}

#endif // CK_BUILD_SM_GRAPH_WALK

// --------------------------------------------------------------------------------------------------------------------
