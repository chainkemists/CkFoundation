#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/Engine.h"
#include "Ck_Utils_Color.generated.h"

/**
 * Utility library for FLinearColor constants
 */
UCLASS()
class CKCORE_API UCk_Utils_LinearColor : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Basic Colors
	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "White"))
	static FLinearColor Get_White(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Black"))
	static FLinearColor Get_Black(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red"))
	static FLinearColor Get_Red(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green"))
	static FLinearColor Get_Green(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue"))
	static FLinearColor Get_Blue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Yellow"))
	static FLinearColor Get_Yellow(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Magenta"))
	static FLinearColor Get_Magenta(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Cyan"))
	static FLinearColor Get_Cyan(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Orange"))
	static FLinearColor Get_Orange(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Purple"))
	static FLinearColor Get_Purple(float Alpha = 1.0f);

	// Grayscale
	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gray100"))
	static FLinearColor Get_Gray100(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gray200"))
	static FLinearColor Get_Gray200(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gray300"))
	static FLinearColor Get_Gray300(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gray400"))
	static FLinearColor Get_Gray400(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gray500"))
	static FLinearColor Get_Gray500(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gray600"))
	static FLinearColor Get_Gray600(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gray700"))
	static FLinearColor Get_Gray700(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gray800"))
	static FLinearColor Get_Gray800(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gray900"))
	static FLinearColor Get_Gray900(float Alpha = 1.0f);

	// Material Design Red
	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red50"))
	static FLinearColor Get_Red50(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red100"))
	static FLinearColor Get_Red100(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red200"))
	static FLinearColor Get_Red200(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red300"))
	static FLinearColor Get_Red300(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red400"))
	static FLinearColor Get_Red400(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red500"))
	static FLinearColor Get_Red500(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red600"))
	static FLinearColor Get_Red600(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red700"))
	static FLinearColor Get_Red700(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red800"))
	static FLinearColor Get_Red800(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red900"))
	static FLinearColor Get_Red900(float Alpha = 1.0f);

	// Material Design Blue
	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue50"))
	static FLinearColor Get_Blue50(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue100"))
	static FLinearColor Get_Blue100(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue200"))
	static FLinearColor Get_Blue200(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue300"))
	static FLinearColor Get_Blue300(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue400"))
	static FLinearColor Get_Blue400(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue500"))
	static FLinearColor Get_Blue500(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue600"))
	static FLinearColor Get_Blue600(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue700"))
	static FLinearColor Get_Blue700(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue800"))
	static FLinearColor Get_Blue800(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue900"))
	static FLinearColor Get_Blue900(float Alpha = 1.0f);

	// Material Design Green
	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green50"))
	static FLinearColor Get_Green50(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green100"))
	static FLinearColor Get_Green100(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green200"))
	static FLinearColor Get_Green200(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green300"))
	static FLinearColor Get_Green300(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green400"))
	static FLinearColor Get_Green400(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green500"))
	static FLinearColor Get_Green500(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green600"))
	static FLinearColor Get_Green600(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green700"))
	static FLinearColor Get_Green700(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green800"))
	static FLinearColor Get_Green800(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green900"))
	static FLinearColor Get_Green900(float Alpha = 1.0f);

	// Named Web Colors
	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "AliceBlue"))
	static FLinearColor Get_AliceBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "AntiqueWhite"))
	static FLinearColor Get_AntiqueWhite(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Aqua"))
	static FLinearColor Get_Aqua(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Aquamarine"))
	static FLinearColor Get_Aquamarine(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Azure"))
	static FLinearColor Get_Azure(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Beige"))
	static FLinearColor Get_Beige(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Bisque"))
	static FLinearColor Get_Bisque(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "BlanchedAlmond"))
	static FLinearColor Get_BlanchedAlmond(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "BlueViolet"))
	static FLinearColor Get_BlueViolet(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Brown"))
	static FLinearColor Get_Brown(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "BurlyWood"))
	static FLinearColor Get_BurlyWood(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "CadetBlue"))
	static FLinearColor Get_CadetBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Chartreuse"))
	static FLinearColor Get_Chartreuse(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Chocolate"))
	static FLinearColor Get_Chocolate(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Coral"))
	static FLinearColor Get_Coral(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "CornflowerBlue"))
	static FLinearColor Get_CornflowerBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Cornsilk"))
	static FLinearColor Get_Cornsilk(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Crimson"))
	static FLinearColor Get_Crimson(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkBlue"))
	static FLinearColor Get_DarkBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkCyan"))
	static FLinearColor Get_DarkCyan(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkGoldenrod"))
	static FLinearColor Get_DarkGoldenrod(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkGray"))
	static FLinearColor Get_DarkGray(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkGreen"))
	static FLinearColor Get_DarkGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkKhaki"))
	static FLinearColor Get_DarkKhaki(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkMagenta"))
	static FLinearColor Get_DarkMagenta(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkOliveGreen"))
	static FLinearColor Get_DarkOliveGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkOrange"))
	static FLinearColor Get_DarkOrange(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkOrchid"))
	static FLinearColor Get_DarkOrchid(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkRed"))
	static FLinearColor Get_DarkRed(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkSalmon"))
	static FLinearColor Get_DarkSalmon(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkSeaGreen"))
	static FLinearColor Get_DarkSeaGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkSlateBlue"))
	static FLinearColor Get_DarkSlateBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkSlateGray"))
	static FLinearColor Get_DarkSlateGray(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkTurquoise"))
	static FLinearColor Get_DarkTurquoise(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkViolet"))
	static FLinearColor Get_DarkViolet(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DeepPink"))
	static FLinearColor Get_DeepPink(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DeepSkyBlue"))
	static FLinearColor Get_DeepSkyBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DimGray"))
	static FLinearColor Get_DimGray(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DodgerBlue"))
	static FLinearColor Get_DodgerBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "FireBrick"))
	static FLinearColor Get_FireBrick(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "FloralWhite"))
	static FLinearColor Get_FloralWhite(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "ForestGreen"))
	static FLinearColor Get_ForestGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Fuchsia"))
	static FLinearColor Get_Fuchsia(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gainsboro"))
	static FLinearColor Get_Gainsboro(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "GhostWhite"))
	static FLinearColor Get_GhostWhite(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gold"))
	static FLinearColor Get_Gold(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Goldenrod"))
	static FLinearColor Get_Goldenrod(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "GreenYellow"))
	static FLinearColor Get_GreenYellow(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Honeydew"))
	static FLinearColor Get_Honeydew(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "HotPink"))
	static FLinearColor Get_HotPink(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "IndianRed"))
	static FLinearColor Get_IndianRed(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Indigo"))
	static FLinearColor Get_Indigo(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Ivory"))
	static FLinearColor Get_Ivory(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Khaki"))
	static FLinearColor Get_Khaki(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Lavender"))
	static FLinearColor Get_Lavender(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LavenderBlush"))
	static FLinearColor Get_LavenderBlush(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LawnGreen"))
	static FLinearColor Get_LawnGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LemonChiffon"))
	static FLinearColor Get_LemonChiffon(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightBlue"))
	static FLinearColor Get_LightBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightCoral"))
	static FLinearColor Get_LightCoral(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightCyan"))
	static FLinearColor Get_LightCyan(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightGoldenrodYellow"))
	static FLinearColor Get_LightGoldenrodYellow(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightGray"))
	static FLinearColor Get_LightGray(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightGreen"))
	static FLinearColor Get_LightGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightPink"))
	static FLinearColor Get_LightPink(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightSalmon"))
	static FLinearColor Get_LightSalmon(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightSeaGreen"))
	static FLinearColor Get_LightSeaGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightSkyBlue"))
	static FLinearColor Get_LightSkyBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightSlateGray"))
	static FLinearColor Get_LightSlateGray(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightSteelBlue"))
	static FLinearColor Get_LightSteelBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightYellow"))
	static FLinearColor Get_LightYellow(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Lime"))
	static FLinearColor Get_Lime(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LimeGreen"))
	static FLinearColor Get_LimeGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Linen"))
	static FLinearColor Get_Linen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Maroon"))
	static FLinearColor Get_Maroon(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MediumAquamarine"))
	static FLinearColor Get_MediumAquamarine(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MediumBlue"))
	static FLinearColor Get_MediumBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MediumOrchid"))
	static FLinearColor Get_MediumOrchid(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MediumPurple"))
	static FLinearColor Get_MediumPurple(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MediumSeaGreen"))
	static FLinearColor Get_MediumSeaGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MediumSlateBlue"))
	static FLinearColor Get_MediumSlateBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MediumSpringGreen"))
	static FLinearColor Get_MediumSpringGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MediumTurquoise"))
	static FLinearColor Get_MediumTurquoise(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MediumVioletRed"))
	static FLinearColor Get_MediumVioletRed(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MidnightBlue"))
	static FLinearColor Get_MidnightBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MintCream"))
	static FLinearColor Get_MintCream(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MistyRose"))
	static FLinearColor Get_MistyRose(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Moccasin"))
	static FLinearColor Get_Moccasin(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "NavajoWhite"))
	static FLinearColor Get_NavajoWhite(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Navy"))
	static FLinearColor Get_Navy(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "OldLace"))
	static FLinearColor Get_OldLace(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Olive"))
	static FLinearColor Get_Olive(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "OliveDrab"))
	static FLinearColor Get_OliveDrab(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "OrangeRed"))
	static FLinearColor Get_OrangeRed(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Orchid"))
	static FLinearColor Get_Orchid(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "PaleGoldenrod"))
	static FLinearColor Get_PaleGoldenrod(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "PaleGreen"))
	static FLinearColor Get_PaleGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "PaleTurquoise"))
	static FLinearColor Get_PaleTurquoise(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "PaleVioletRed"))
	static FLinearColor Get_PaleVioletRed(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "PapayaWhip"))
	static FLinearColor Get_PapayaWhip(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "PeachPuff"))
	static FLinearColor Get_PeachPuff(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Peru"))
	static FLinearColor Get_Peru(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Pink"))
	static FLinearColor Get_Pink(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Plum"))
	static FLinearColor Get_Plum(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "PowderBlue"))
	static FLinearColor Get_PowderBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "RosyBrown"))
	static FLinearColor Get_RosyBrown(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "RoyalBlue"))
	static FLinearColor Get_RoyalBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "SaddleBrown"))
	static FLinearColor Get_SaddleBrown(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Salmon"))
	static FLinearColor Get_Salmon(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "SandyBrown"))
	static FLinearColor Get_SandyBrown(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "SeaGreen"))
	static FLinearColor Get_SeaGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "SeaShell"))
	static FLinearColor Get_SeaShell(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Sienna"))
	static FLinearColor Get_Sienna(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Silver"))
	static FLinearColor Get_Silver(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "SkyBlue"))
	static FLinearColor Get_SkyBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "SlateBlue"))
	static FLinearColor Get_SlateBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "SlateGray"))
	static FLinearColor Get_SlateGray(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Snow"))
	static FLinearColor Get_Snow(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "SpringGreen"))
	static FLinearColor Get_SpringGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "SteelBlue"))
	static FLinearColor Get_SteelBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Tan"))
	static FLinearColor Get_Tan(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Teal"))
	static FLinearColor Get_Teal(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Thistle"))
	static FLinearColor Get_Thistle(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Tomato"))
	static FLinearColor Get_Tomato(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Turquoise"))
	static FLinearColor Get_Turquoise(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Violet"))
	static FLinearColor Get_Violet(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Wheat"))
	static FLinearColor Get_Wheat(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "WhiteSmoke"))
	static FLinearColor Get_WhiteSmoke(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "YellowGreen"))
	static FLinearColor Get_YellowGreen(float Alpha = 1.0f);

	// Transparent
	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Transparent"))
	static FLinearColor Get_Transparent(float Alpha = 1.0f);

public:
	// Static color constants for performance
	static const FLinearColor White;
	static const FLinearColor Black;
	static const FLinearColor Red;
	static const FLinearColor Green;
	static const FLinearColor Blue;
	static const FLinearColor Yellow;
	static const FLinearColor Magenta;
	static const FLinearColor Cyan;
	static const FLinearColor Orange;
	static const FLinearColor Purple;

	// Grayscale
	static const FLinearColor Gray100;
	static const FLinearColor Gray200;
	static const FLinearColor Gray300;
	static const FLinearColor Gray400;
	static const FLinearColor Gray500;
	static const FLinearColor Gray600;
	static const FLinearColor Gray700;
	static const FLinearColor Gray800;
	static const FLinearColor Gray900;

	// Material Design Red
	static const FLinearColor Red50;
	static const FLinearColor Red100;
	static const FLinearColor Red200;
	static const FLinearColor Red300;
	static const FLinearColor Red400;
	static const FLinearColor Red500;
	static const FLinearColor Red600;
	static const FLinearColor Red700;
	static const FLinearColor Red800;
	static const FLinearColor Red900;

	// Material Design Blue
	static const FLinearColor Blue50;
	static const FLinearColor Blue100;
	static const FLinearColor Blue200;
	static const FLinearColor Blue300;
	static const FLinearColor Blue400;
	static const FLinearColor Blue500;
	static const FLinearColor Blue600;
	static const FLinearColor Blue700;
	static const FLinearColor Blue800;
	static const FLinearColor Blue900;

	// Material Design Green
	static const FLinearColor Green50;
	static const FLinearColor Green100;
	static const FLinearColor Green200;
	static const FLinearColor Green300;
	static const FLinearColor Green400;
	static const FLinearColor Green500;
	static const FLinearColor Green600;
	static const FLinearColor Green700;
	static const FLinearColor Green800;
	static const FLinearColor Green900;

	// Named Web Colors (constants - abbreviated for brevity)
	static const FLinearColor AliceBlue;
	static const FLinearColor AntiqueWhite;
	static const FLinearColor Aqua;
	static const FLinearColor Aquamarine;
	static const FLinearColor Azure;
	static const FLinearColor Beige;
	static const FLinearColor Bisque;
	static const FLinearColor BlanchedAlmond;
	static const FLinearColor BlueViolet;
	static const FLinearColor Brown;
	static const FLinearColor BurlyWood;
	static const FLinearColor CadetBlue;
	static const FLinearColor Chartreuse;
	static const FLinearColor Chocolate;
	static const FLinearColor Coral;
	static const FLinearColor CornflowerBlue;
	static const FLinearColor Cornsilk;
	static const FLinearColor Crimson;
	static const FLinearColor DarkBlue;
	static const FLinearColor DarkCyan;
	static const FLinearColor DarkGoldenrod;
	static const FLinearColor DarkGray;
	static const FLinearColor DarkGreen;
	static const FLinearColor DarkKhaki;
	static const FLinearColor DarkMagenta;
	static const FLinearColor DarkOliveGreen;
	static const FLinearColor DarkOrange;
	static const FLinearColor DarkOrchid;
	static const FLinearColor DarkRed;
	static const FLinearColor DarkSalmon;
	static const FLinearColor DarkSeaGreen;
	static const FLinearColor DarkSlateBlue;
	static const FLinearColor DarkSlateGray;
	static const FLinearColor DarkTurquoise;
	static const FLinearColor DarkViolet;
	static const FLinearColor DeepPink;
	static const FLinearColor DeepSkyBlue;
	static const FLinearColor DimGray;
	static const FLinearColor DodgerBlue;
	static const FLinearColor FireBrick;
	static const FLinearColor FloralWhite;
	static const FLinearColor ForestGreen;
	static const FLinearColor Fuchsia;
	static const FLinearColor Gainsboro;
	static const FLinearColor GhostWhite;
	static const FLinearColor Gold;
	static const FLinearColor Goldenrod;
	static const FLinearColor GreenYellow;
	static const FLinearColor Honeydew;
	static const FLinearColor HotPink;
	static const FLinearColor IndianRed;
	static const FLinearColor Indigo;
	static const FLinearColor Ivory;
	static const FLinearColor Khaki;
	static const FLinearColor Lavender;
	static const FLinearColor LavenderBlush;
	static const FLinearColor LawnGreen;
	static const FLinearColor LemonChiffon;
	static const FLinearColor LightBlue;
	static const FLinearColor LightCoral;
	static const FLinearColor LightCyan;
	static const FLinearColor LightGoldenrodYellow;
	static const FLinearColor LightGray;
	static const FLinearColor LightGreen;
	static const FLinearColor LightPink;
	static const FLinearColor LightSalmon;
	static const FLinearColor LightSeaGreen;
	static const FLinearColor LightSkyBlue;
	static const FLinearColor LightSlateGray;
	static const FLinearColor LightSteelBlue;
	static const FLinearColor LightYellow;
	static const FLinearColor Lime;
	static const FLinearColor LimeGreen;
	static const FLinearColor Linen;
	static const FLinearColor Maroon;
	static const FLinearColor MediumAquamarine;
	static const FLinearColor MediumBlue;
	static const FLinearColor MediumOrchid;
	static const FLinearColor MediumPurple;
	static const FLinearColor MediumSeaGreen;
	static const FLinearColor MediumSlateBlue;
	static const FLinearColor MediumSpringGreen;
	static const FLinearColor MediumTurquoise;
	static const FLinearColor MediumVioletRed;
	static const FLinearColor MidnightBlue;
	static const FLinearColor MintCream;
	static const FLinearColor MistyRose;
	static const FLinearColor Moccasin;
	static const FLinearColor NavajoWhite;
	static const FLinearColor Navy;
	static const FLinearColor OldLace;
	static const FLinearColor Olive;
	static const FLinearColor OliveDrab;
	static const FLinearColor OrangeRed;
	static const FLinearColor Orchid;
	static const FLinearColor PaleGoldenrod;
	static const FLinearColor PaleGreen;
	static const FLinearColor PaleTurquoise;
	static const FLinearColor PaleVioletRed;
	static const FLinearColor PapayaWhip;
	static const FLinearColor PeachPuff;
	static const FLinearColor Peru;
	static const FLinearColor Pink;
	static const FLinearColor Plum;
	static const FLinearColor PowderBlue;
	static const FLinearColor RosyBrown;
	static const FLinearColor RoyalBlue;
	static const FLinearColor SaddleBrown;
	static const FLinearColor Salmon;
	static const FLinearColor SandyBrown;
	static const FLinearColor SeaGreen;
	static const FLinearColor SeaShell;
	static const FLinearColor Sienna;
	static const FLinearColor Silver;
	static const FLinearColor SkyBlue;
	static const FLinearColor SlateBlue;
	static const FLinearColor SlateGray;
	static const FLinearColor Snow;
	static const FLinearColor SpringGreen;
	static const FLinearColor SteelBlue;
	static const FLinearColor Tan;
	static const FLinearColor Teal;
	static const FLinearColor Thistle;
	static const FLinearColor Tomato;
	static const FLinearColor Turquoise;
	static const FLinearColor Violet;
	static const FLinearColor Wheat;
	static const FLinearColor WhiteSmoke;
	static const FLinearColor YellowGreen;
	static const FLinearColor Transparent;
};

/**
 * Utility library for FColor constants (sRGB)
 */
UCLASS()
class CKCORE_API UCk_Utils_Color : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Basic Colors
	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "White"))
	static FColor Get_White(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Black"))
	static FColor Get_Black(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red"))
	static FColor Get_Red(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green"))
	static FColor Get_Green(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue"))
	static FColor Get_Blue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Yellow"))
	static FColor Get_Yellow(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Magenta"))
	static FColor Get_Magenta(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Cyan"))
	static FColor Get_Cyan(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Orange"))
	static FColor Get_Orange(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Purple"))
	static FColor Get_Purple(float Alpha = 1.0f);

	// Grayscale
	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gray100"))
	static FColor Get_Gray100(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gray200"))
	static FColor Get_Gray200(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gray300"))
	static FColor Get_Gray300(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gray400"))
	static FColor Get_Gray400(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gray500"))
	static FColor Get_Gray500(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gray600"))
	static FColor Get_Gray600(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gray700"))
	static FColor Get_Gray700(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gray800"))
	static FColor Get_Gray800(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gray900"))
	static FColor Get_Gray900(float Alpha = 1.0f);

	// Material Design Red
	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red50"))
	static FColor Get_Red50(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red100"))
	static FColor Get_Red100(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red200"))
	static FColor Get_Red200(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red300"))
	static FColor Get_Red300(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red400"))
	static FColor Get_Red400(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red500"))
	static FColor Get_Red500(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red600"))
	static FColor Get_Red600(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red700"))
	static FColor Get_Red700(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red800"))
	static FColor Get_Red800(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Red900"))
	static FColor Get_Red900(float Alpha = 1.0f);

	// Material Design Blue
	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue50"))
	static FColor Get_Blue50(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue100"))
	static FColor Get_Blue100(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue200"))
	static FColor Get_Blue200(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue300"))
	static FColor Get_Blue300(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue400"))
	static FColor Get_Blue400(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue500"))
	static FColor Get_Blue500(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue600"))
	static FColor Get_Blue600(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue700"))
	static FColor Get_Blue700(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue800"))
	static FColor Get_Blue800(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Blue900"))
	static FColor Get_Blue900(float Alpha = 1.0f);

	// Material Design Green
	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green50"))
	static FColor Get_Green50(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green100"))
	static FColor Get_Green100(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green200"))
	static FColor Get_Green200(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green300"))
	static FColor Get_Green300(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green400"))
	static FColor Get_Green400(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green500"))
	static FColor Get_Green500(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green600"))
	static FColor Get_Green600(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green700"))
	static FColor Get_Green700(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green800"))
	static FColor Get_Green800(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Green900"))
	static FColor Get_Green900(float Alpha = 1.0f);

	// Named Web Colors
	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "AliceBlue"))
	static FColor Get_AliceBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "AntiqueWhite"))
	static FColor Get_AntiqueWhite(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Aqua"))
	static FColor Get_Aqua(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Aquamarine"))
	static FColor Get_Aquamarine(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Azure"))
	static FColor Get_Azure(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Beige"))
	static FColor Get_Beige(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Bisque"))
	static FColor Get_Bisque(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "BlanchedAlmond"))
	static FColor Get_BlanchedAlmond(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "BlueViolet"))
	static FColor Get_BlueViolet(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Brown"))
	static FColor Get_Brown(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "BurlyWood"))
	static FColor Get_BurlyWood(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "CadetBlue"))
	static FColor Get_CadetBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Chartreuse"))
	static FColor Get_Chartreuse(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Chocolate"))
	static FColor Get_Chocolate(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Coral"))
	static FColor Get_Coral(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "CornflowerBlue"))
	static FColor Get_CornflowerBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Cornsilk"))
	static FColor Get_Cornsilk(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Crimson"))
	static FColor Get_Crimson(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkBlue"))
	static FColor Get_DarkBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkCyan"))
	static FColor Get_DarkCyan(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkGoldenrod"))
	static FColor Get_DarkGoldenrod(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkGray"))
	static FColor Get_DarkGray(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkGreen"))
	static FColor Get_DarkGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkKhaki"))
	static FColor Get_DarkKhaki(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkMagenta"))
	static FColor Get_DarkMagenta(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkOliveGreen"))
	static FColor Get_DarkOliveGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkOrange"))
	static FColor Get_DarkOrange(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkOrchid"))
	static FColor Get_DarkOrchid(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkRed"))
	static FColor Get_DarkRed(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkSalmon"))
	static FColor Get_DarkSalmon(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkSeaGreen"))
	static FColor Get_DarkSeaGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkSlateBlue"))
	static FColor Get_DarkSlateBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkSlateGray"))
	static FColor Get_DarkSlateGray(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkTurquoise"))
	static FColor Get_DarkTurquoise(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DarkViolet"))
	static FColor Get_DarkViolet(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DeepPink"))
	static FColor Get_DeepPink(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DeepSkyBlue"))
	static FColor Get_DeepSkyBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DimGray"))
	static FColor Get_DimGray(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "DodgerBlue"))
	static FColor Get_DodgerBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "FireBrick"))
	static FColor Get_FireBrick(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "FloralWhite"))
	static FColor Get_FloralWhite(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "ForestGreen"))
	static FColor Get_ForestGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Fuchsia"))
	static FColor Get_Fuchsia(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gainsboro"))
	static FColor Get_Gainsboro(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "GhostWhite"))
	static FColor Get_GhostWhite(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Gold"))
	static FColor Get_Gold(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Goldenrod"))
	static FColor Get_Goldenrod(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "GreenYellow"))
	static FColor Get_GreenYellow(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Honeydew"))
	static FColor Get_Honeydew(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "HotPink"))
	static FColor Get_HotPink(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "IndianRed"))
	static FColor Get_IndianRed(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Indigo"))
	static FColor Get_Indigo(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Ivory"))
	static FColor Get_Ivory(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Khaki"))
	static FColor Get_Khaki(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Lavender"))
	static FColor Get_Lavender(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LavenderBlush"))
	static FColor Get_LavenderBlush(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LawnGreen"))
	static FColor Get_LawnGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LemonChiffon"))
	static FColor Get_LemonChiffon(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightBlue"))
	static FColor Get_LightBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightCoral"))
	static FColor Get_LightCoral(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightCyan"))
	static FColor Get_LightCyan(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightGoldenrodYellow"))
	static FColor Get_LightGoldenrodYellow(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightGray"))
	static FColor Get_LightGray(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightGreen"))
	static FColor Get_LightGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightPink"))
	static FColor Get_LightPink(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightSalmon"))
	static FColor Get_LightSalmon(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightSeaGreen"))
	static FColor Get_LightSeaGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightSkyBlue"))
	static FColor Get_LightSkyBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightSlateGray"))
	static FColor Get_LightSlateGray(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightSteelBlue"))
	static FColor Get_LightSteelBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LightYellow"))
	static FColor Get_LightYellow(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Lime"))
	static FColor Get_Lime(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "LimeGreen"))
	static FColor Get_LimeGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Linen"))
	static FColor Get_Linen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Maroon"))
	static FColor Get_Maroon(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MediumAquamarine"))
	static FColor Get_MediumAquamarine(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MediumBlue"))
	static FColor Get_MediumBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MediumOrchid"))
	static FColor Get_MediumOrchid(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MediumPurple"))
	static FColor Get_MediumPurple(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MediumSeaGreen"))
	static FColor Get_MediumSeaGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MediumSlateBlue"))
	static FColor Get_MediumSlateBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MediumSpringGreen"))
	static FColor Get_MediumSpringGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MediumTurquoise"))
	static FColor Get_MediumTurquoise(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MediumVioletRed"))
	static FColor Get_MediumVioletRed(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MidnightBlue"))
	static FColor Get_MidnightBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MintCream"))
	static FColor Get_MintCream(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "MistyRose"))
	static FColor Get_MistyRose(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Moccasin"))
	static FColor Get_Moccasin(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "NavajoWhite"))
	static FColor Get_NavajoWhite(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Navy"))
	static FColor Get_Navy(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "OldLace"))
	static FColor Get_OldLace(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Olive"))
	static FColor Get_Olive(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "OliveDrab"))
	static FColor Get_OliveDrab(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "OrangeRed"))
	static FColor Get_OrangeRed(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Orchid"))
	static FColor Get_Orchid(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "PaleGoldenrod"))
	static FColor Get_PaleGoldenrod(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "PaleGreen"))
	static FColor Get_PaleGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "PaleTurquoise"))
	static FColor Get_PaleTurquoise(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "PaleVioletRed"))
	static FColor Get_PaleVioletRed(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "PapayaWhip"))
	static FColor Get_PapayaWhip(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "PeachPuff"))
	static FColor Get_PeachPuff(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Peru"))
	static FColor Get_Peru(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Pink"))
	static FColor Get_Pink(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Plum"))
	static FColor Get_Plum(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "PowderBlue"))
	static FColor Get_PowderBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "RosyBrown"))
	static FColor Get_RosyBrown(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "RoyalBlue"))
	static FColor Get_RoyalBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "SaddleBrown"))
	static FColor Get_SaddleBrown(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Salmon"))
	static FColor Get_Salmon(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "SandyBrown"))
	static FColor Get_SandyBrown(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "SeaGreen"))
	static FColor Get_SeaGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "SeaShell"))
	static FColor Get_SeaShell(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Sienna"))
	static FColor Get_Sienna(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Silver"))
	static FColor Get_Silver(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "SkyBlue"))
	static FColor Get_SkyBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "SlateBlue"))
	static FColor Get_SlateBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "SlateGray"))
	static FColor Get_SlateGray(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Snow"))
	static FColor Get_Snow(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "SpringGreen"))
	static FColor Get_SpringGreen(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "SteelBlue"))
	static FColor Get_SteelBlue(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Tan"))
	static FColor Get_Tan(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Teal"))
	static FColor Get_Teal(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Thistle"))
	static FColor Get_Thistle(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Tomato"))
	static FColor Get_Tomato(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Turquoise"))
	static FColor Get_Turquoise(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Violet"))
	static FColor Get_Violet(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Wheat"))
	static FColor Get_Wheat(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "WhiteSmoke"))
	static FColor Get_WhiteSmoke(float Alpha = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "YellowGreen"))
	static FColor Get_YellowGreen(float Alpha = 1.0f);

	// Transparent
	UFUNCTION(BlueprintPure, Category = "Ck|Color", meta = (CompactNodeTitle = "Transparent"))
	static FColor Get_Transparent(float Alpha = 1.0f);

public:
	// Static color constants for performance
	static const FColor White;
	static const FColor Black;
	static const FColor Red;
	static const FColor Green;
	static const FColor Blue;
	static const FColor Yellow;
	static const FColor Magenta;
	static const FColor Cyan;
	static const FColor Orange;
	static const FColor Purple;

	// Grayscale
	static const FColor Gray100;
	static const FColor Gray200;
	static const FColor Gray300;
	static const FColor Gray400;
	static const FColor Gray500;
	static const FColor Gray600;
	static const FColor Gray700;
	static const FColor Gray800;
	static const FColor Gray900;

	// Material Design Red
	static const FColor Red50;
	static const FColor Red100;
	static const FColor Red200;
	static const FColor Red300;
	static const FColor Red400;
	static const FColor Red500;
	static const FColor Red600;
	static const FColor Red700;
	static const FColor Red800;
	static const FColor Red900;

	// Material Design Blue
	static const FColor Blue50;
	static const FColor Blue100;
	static const FColor Blue200;
	static const FColor Blue300;
	static const FColor Blue400;
	static const FColor Blue500;
	static const FColor Blue600;
	static const FColor Blue700;
	static const FColor Blue800;
	static const FColor Blue900;

	// Material Design Green
	static const FColor Green50;
	static const FColor Green100;
	static const FColor Green200;
	static const FColor Green300;
	static const FColor Green400;
	static const FColor Green500;
	static const FColor Green600;
	static const FColor Green700;
	static const FColor Green800;
	static const FColor Green900;

	// Named Web Colors (constants - abbreviated for brevity)
	static const FColor AliceBlue;
	static const FColor AntiqueWhite;
	static const FColor Aqua;
	static const FColor Aquamarine;
	static const FColor Azure;
	static const FColor Beige;
	static const FColor Bisque;
	static const FColor BlanchedAlmond;
	static const FColor BlueViolet;
	static const FColor Brown;
	static const FColor BurlyWood;
	static const FColor CadetBlue;
	static const FColor Chartreuse;
	static const FColor Chocolate;
	static const FColor Coral;
	static const FColor CornflowerBlue;
	static const FColor Cornsilk;
	static const FColor Crimson;
	static const FColor DarkBlue;
	static const FColor DarkCyan;
	static const FColor DarkGoldenrod;
	static const FColor DarkGray;
	static const FColor DarkGreen;
	static const FColor DarkKhaki;
	static const FColor DarkMagenta;
	static const FColor DarkOliveGreen;
	static const FColor DarkOrange;
	static const FColor DarkOrchid;
	static const FColor DarkRed;
	static const FColor DarkSalmon;
	static const FColor DarkSeaGreen;
	static const FColor DarkSlateBlue;
	static const FColor DarkSlateGray;
	static const FColor DarkTurquoise;
	static const FColor DarkViolet;
	static const FColor DeepPink;
	static const FColor DeepSkyBlue;
	static const FColor DimGray;
	static const FColor DodgerBlue;
	static const FColor FireBrick;
	static const FColor FloralWhite;
	static const FColor ForestGreen;
	static const FColor Fuchsia;
	static const FColor Gainsboro;
	static const FColor GhostWhite;
	static const FColor Gold;
	static const FColor Goldenrod;
	static const FColor GreenYellow;
	static const FColor Honeydew;
	static const FColor HotPink;
	static const FColor IndianRed;
	static const FColor Indigo;
	static const FColor Ivory;
	static const FColor Khaki;
	static const FColor Lavender;
	static const FColor LavenderBlush;
	static const FColor LawnGreen;
	static const FColor LemonChiffon;
	static const FColor LightBlue;
	static const FColor LightCoral;
	static const FColor LightCyan;
	static const FColor LightGoldenrodYellow;
	static const FColor LightGray;
	static const FColor LightGreen;
	static const FColor LightPink;
	static const FColor LightSalmon;
	static const FColor LightSeaGreen;
	static const FColor LightSkyBlue;
	static const FColor LightSlateGray;
	static const FColor LightSteelBlue;
	static const FColor LightYellow;
	static const FColor Lime;
	static const FColor LimeGreen;
	static const FColor Linen;
	static const FColor Maroon;
	static const FColor MediumAquamarine;
	static const FColor MediumBlue;
	static const FColor MediumOrchid;
	static const FColor MediumPurple;
	static const FColor MediumSeaGreen;
	static const FColor MediumSlateBlue;
	static const FColor MediumSpringGreen;
	static const FColor MediumTurquoise;
	static const FColor MediumVioletRed;
	static const FColor MidnightBlue;
	static const FColor MintCream;
	static const FColor MistyRose;
	static const FColor Moccasin;
	static const FColor NavajoWhite;
	static const FColor Navy;
	static const FColor OldLace;
	static const FColor Olive;
	static const FColor OliveDrab;
	static const FColor OrangeRed;
	static const FColor Orchid;
	static const FColor PaleGoldenrod;
	static const FColor PaleGreen;
	static const FColor PaleTurquoise;
	static const FColor PaleVioletRed;
	static const FColor PapayaWhip;
	static const FColor PeachPuff;
	static const FColor Peru;
	static const FColor Pink;
	static const FColor Plum;
	static const FColor PowderBlue;
	static const FColor RosyBrown;
	static const FColor RoyalBlue;
	static const FColor SaddleBrown;
	static const FColor Salmon;
	static const FColor SandyBrown;
	static const FColor SeaGreen;
	static const FColor SeaShell;
	static const FColor Sienna;
	static const FColor Silver;
	static const FColor SkyBlue;
	static const FColor SlateBlue;
	static const FColor SlateGray;
	static const FColor Snow;
	static const FColor SpringGreen;
	static const FColor SteelBlue;
	static const FColor Tan;
	static const FColor Teal;
	static const FColor Thistle;
	static const FColor Tomato;
	static const FColor Turquoise;
	static const FColor Violet;
	static const FColor Wheat;
	static const FColor WhiteSmoke;
	static const FColor YellowGreen;
	static const FColor Transparent;
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
	using LinearColor = UCk_Utils_LinearColor;
	using Color = UCk_Utils_Color;
}

// --------------------------------------------------------------------------------------------------------------------