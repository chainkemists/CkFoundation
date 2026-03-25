#pragma once

#include "CkStateMachine_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Tag/CkTag.h"

#if CK_BUILD_SM_GRAPH_WALK

// --------------------------------------------------------------------------------------------------------------------

class UCk_SmState_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Sm_Debug_GraphWalk;
    class FProcessor_Sm_Debug_GraphWalk_Iterate;

    // ================================================================================================================
    // STATE DEFINITION — Per-state graph structure discovered by the walker
    // ================================================================================================================

    struct CKSTATEMACHINE_API FCk_SmDebug_StateDefinition
    {
        TSubclassOf<UCk_SmState_EntityScript> StateClass;
        FString StateName;

        struct FTransitionDef
        {
            TSubclassOf<UCk_SmState_EntityScript> TargetStateClass;
            int32 Order = 0;
        };

        TArray<FTransitionDef> Transitions;

        struct FTaskDef
        {
            FString ClassName;
            ECk_SmTaskMode Mode = ECk_SmTaskMode::EnterExitOnly;
            bool HasSubStateMachine = false;
            TSubclassOf<UCk_SmState_EntityScript> SubSmInitialStateClass;
        };

        TArray<FTaskDef> Tasks;
    };

    // ================================================================================================================
    // SUB-SM DEFINITION — Full sub-SM graph keyed by parent state class
    // ================================================================================================================

    struct CKSTATEMACHINE_API FCk_SmDebug_SubSmDefinition
    {
        TSubclassOf<UCk_SmState_EntityScript> ParentStateClass;
        TSubclassOf<UCk_SmState_EntityScript> InitialStateClass;
        TMap<TSubclassOf<UCk_SmState_EntityScript>, FCk_SmDebug_StateDefinition> StateDefinitions;
    };

    // ================================================================================================================
    // GRAPH DEFINITION FRAGMENT — Stored on the SM entity
    // ================================================================================================================

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

    // ================================================================================================================
    // GRAPH WALK PROGRESS — Tracks multi-frame walk state
    // ================================================================================================================

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

    // ================================================================================================================
    // ONE-SHOT TAG — Triggers the graph walk
    // ================================================================================================================

    CK_DEFINE_ECS_TAG(FTag_Sm_Debug_RequiresGraphWalk);
}

#endif // CK_BUILD_SM_GRAPH_WALK

// --------------------------------------------------------------------------------------------------------------------
