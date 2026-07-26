#pragma once

#include "CkCamera/Camera/CkCamera_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkAttribute/CkAttribute_Fragment_Data.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"
#include "CkAttribute/VectorAttribute/CkVectorAttribute_Fragment_Data.h"
#include "CkAttribute/RotatorAttribute/CkRotatorAttribute_Fragment_Data.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_CameraLayer_EntityScript;

namespace ck
{
    struct CKCAMERA_API FFragment_CameraLayer_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_CameraLayer_Params);

    public:
        friend class FProcessor_Camera_HandleRequests;

    private:
        TSubclassOf<UCk_CameraLayer_EntityScript> _LayerClass;
        int32                                     _Priority = 0;
        FCk_Camera_Target                         _CameraTarget;

        bool _IsDefault = false;

    public:
        CK_PROPERTY_GET(_LayerClass);
        CK_PROPERTY(_Priority);
        CK_PROPERTY(_CameraTarget);
        CK_PROPERTY(_IsDefault);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_CameraLayer_Params, _LayerClass);
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKCAMERA_API FFragment_CameraLayer_Blend
    {
    public:
        CK_GENERATED_BODY(FFragment_CameraLayer_Blend);

    public:
        friend class FProcessor_Camera_HandleRequests;
        friend class FProcessor_CameraLayer_Lifecycle;
        friend class FProcessor_CameraLayer_Blend;

    private:
        float _Alpha       = 0.0f;
        float _TargetAlpha = 1.0f;
        float _BlendRate   = 1000.0f; // ~instant unless overridden

        bool _FiredBlendedIn  = false;
        bool _FiredBlendedOut = false;

    public:
        CK_PROPERTY(_Alpha);
        CK_PROPERTY(_TargetAlpha);
        CK_PROPERTY(_BlendRate);
        CK_PROPERTY(_FiredBlendedIn);
        CK_PROPERTY(_FiredBlendedOut);
    };

    // ----------------------------------------------------------------------------------------------------------------

    // _Target is the layer-authored value at FULL blend; the blend processor scales it to the modifier's live delta.
    template <typename T_ModHandle, typename T_AttrHandle, typename T_Delta>
    struct TCk_CameraLayer_AcquiredModifier
    {
        T_ModHandle                     _Modifier;
        T_AttrHandle                    _Attribute;
        T_Delta                         _Target    = T_Delta{};
        ECk_AttributeModifier_Operation _Op        = ECk_AttributeModifier_Operation::Add;
        ECk_MinMaxCurrent               _Component = ECk_MinMaxCurrent::Current;
    };

    using FCk_CameraLayer_FloatModifier   = TCk_CameraLayer_AcquiredModifier<FCk_Handle_FloatAttributeModifier,   FCk_Handle_FloatAttribute,   float>;
    using FCk_CameraLayer_VectorModifier  = TCk_CameraLayer_AcquiredModifier<FCk_Handle_VectorAttributeModifier,  FCk_Handle_VectorAttribute,  FVector>;
    using FCk_CameraLayer_RotatorModifier = TCk_CameraLayer_AcquiredModifier<FCk_Handle_RotatorAttributeModifier, FCk_Handle_RotatorAttribute, FRotator>;
    using FCk_CameraLayer_IntegerModifier = TCk_CameraLayer_AcquiredModifier<FCk_Handle_IntegerAttributeModifier, FCk_Handle_IntegerAttribute, int32>;

    // These modifiers live under the tuner ATTRIBUTE, not under the layer, so destroying the layer does NOT destroy
    // them — teardown must go through UCk_Utils_CameraLayer_UE::Remove_AllAcquiredModifiers.
    struct CKCAMERA_API FFragment_CameraLayer_AcquiredModifiers
    {
    public:
        CK_GENERATED_BODY(FFragment_CameraLayer_AcquiredModifiers);

    public:
        TArray<FCk_CameraLayer_FloatModifier>   _Float;
        TArray<FCk_CameraLayer_VectorModifier>  _Vector;
        TArray<FCk_CameraLayer_RotatorModifier> _Rotator;
        TArray<FCk_CameraLayer_IntegerModifier> _Integer;
    };

    // ----------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_CameraLayer_Active);

    // ----------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ENTITY_HOLDER_AND_UTILS_TRANSIENT(TUtils_CameraLayer_OwningCamera, FFragment_CameraLayer_OwningCamera, FCk_Handle_Camera);

    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfCameraLayers, FCk_Handle_CameraLayer);

    struct CKCAMERA_API FUtils_RecordOfCameraLayers : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfCameraLayers> {};
}

// --------------------------------------------------------------------------------------------------------------------
