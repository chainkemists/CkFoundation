#include "CkUsf/Stylize/CkUsf_CrossHatchPreset.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkUsf_CrossHatchPreset::
    Get_AsParams() const
    -> FCk_Usf_CrossHatch_Params
{
    auto Params = FCk_Usf_CrossHatch_Params{};

    Params.Set_Enabled(_Enabled)
          .Set_StyleStrength(_StyleStrength)
          .Set_UseWorldSpaceNormals(_UseWorldSpaceNormals)
          .Set_AngleOffset(_AngleOffset)
          .Set_NormalAlignment(_NormalAlignment)
          .Set_Spacing(_Spacing)
          .Set_LayerCount(_LayerCount)
          .Set_LayerAngleStep(_LayerAngleStep)
          .Set_StrokePattern(_StrokePattern)
          .Set_StrokeThickness(_StrokeThickness)
          .Set_StrokeIrregularity(_StrokeIrregularity)
          .Set_DarknessBias(_DarknessBias)
          .Set_DarknessContrast(_DarknessContrast)
          .Set_BackgroundMode(_BackgroundMode)
          .Set_PaperColor(_PaperColor)
          .Set_InkColor(_InkColor)
          .Set_Saturation(_Saturation)
          .Set_AffectSky(_AffectSky)
          .Set_SkyDistance(_SkyDistance)
          .Set_Mask(_Mask)
          .Set_DebugMode(_DebugMode);

    return Params;
}

// --------------------------------------------------------------------------------------------------------------------
