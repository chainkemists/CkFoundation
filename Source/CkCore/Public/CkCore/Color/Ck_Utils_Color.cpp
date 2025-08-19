#include "Ck_Utils_Color.h"

// UCk_Utils_LinearColor Static Constants
const FLinearColor UCk_Utils_LinearColor::White = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Black = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Red = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Green = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Blue = FLinearColor(0.0f, 0.0f, 1.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Yellow = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Magenta = FLinearColor(1.0f, 0.0f, 1.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Cyan = FLinearColor(0.0f, 1.0f, 1.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Orange = FLinearColor(1.0f, 0.647f, 0.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Purple = FLinearColor(0.5f, 0.0f, 0.5f, 1.0f);

// Grayscale
const FLinearColor UCk_Utils_LinearColor::Gray100 = FLinearColor(0.956f, 0.956f, 0.956f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Gray200 = FLinearColor(0.898f, 0.898f, 0.898f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Gray300 = FLinearColor(0.827f, 0.827f, 0.827f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Gray400 = FLinearColor(0.741f, 0.741f, 0.741f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Gray500 = FLinearColor(0.620f, 0.620f, 0.620f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Gray600 = FLinearColor(0.459f, 0.459f, 0.459f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Gray700 = FLinearColor(0.388f, 0.388f, 0.388f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Gray800 = FLinearColor(0.278f, 0.278f, 0.278f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Gray900 = FLinearColor(0.129f, 0.129f, 0.129f, 1.0f);

// Material Design Red
const FLinearColor UCk_Utils_LinearColor::Red50 = FLinearColor(1.0f, 0.922f, 0.933f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Red100 = FLinearColor(1.0f, 0.804f, 0.824f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Red200 = FLinearColor(0.937f, 0.604f, 0.604f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Red300 = FLinearColor(0.898f, 0.451f, 0.451f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Red400 = FLinearColor(0.937f, 0.325f, 0.314f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Red500 = FLinearColor(0.957f, 0.263f, 0.212f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Red600 = FLinearColor(0.898f, 0.224f, 0.208f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Red700 = FLinearColor(0.827f, 0.184f, 0.184f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Red800 = FLinearColor(0.776f, 0.157f, 0.157f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Red900 = FLinearColor(0.718f, 0.110f, 0.110f, 1.0f);

// Material Design Blue
const FLinearColor UCk_Utils_LinearColor::Blue50 = FLinearColor(0.890f, 0.949f, 1.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Blue100 = FLinearColor(0.733f, 0.871f, 0.984f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Blue200 = FLinearColor(0.565f, 0.792f, 0.976f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Blue300 = FLinearColor(0.392f, 0.710f, 0.965f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Blue400 = FLinearColor(0.259f, 0.635f, 0.957f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Blue500 = FLinearColor(0.129f, 0.588f, 0.953f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Blue600 = FLinearColor(0.118f, 0.533f, 0.898f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Blue700 = FLinearColor(0.098f, 0.463f, 0.824f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Blue800 = FLinearColor(0.078f, 0.408f, 0.753f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Blue900 = FLinearColor(0.051f, 0.278f, 0.631f, 1.0f);

// Material Design Green
const FLinearColor UCk_Utils_LinearColor::Green50 = FLinearColor(0.910f, 0.969f, 0.914f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Green100 = FLinearColor(0.784f, 0.902f, 0.788f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Green200 = FLinearColor(0.647f, 0.839f, 0.655f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Green300 = FLinearColor(0.506f, 0.776f, 0.518f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Green400 = FLinearColor(0.400f, 0.733f, 0.416f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Green500 = FLinearColor(0.298f, 0.686f, 0.314f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Green600 = FLinearColor(0.263f, 0.627f, 0.278f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Green700 = FLinearColor(0.220f, 0.557f, 0.235f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Green800 = FLinearColor(0.180f, 0.490f, 0.196f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Green900 = FLinearColor(0.110f, 0.369f, 0.125f, 1.0f);

// Named Web Colors
const FLinearColor UCk_Utils_LinearColor::AliceBlue = FLinearColor(0.941f, 0.973f, 1.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::AntiqueWhite = FLinearColor(0.980f, 0.922f, 0.843f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Aqua = FLinearColor(0.0f, 1.0f, 1.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Aquamarine = FLinearColor(0.498f, 1.0f, 0.831f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Azure = FLinearColor(0.941f, 1.0f, 1.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Beige = FLinearColor(0.961f, 0.961f, 0.863f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Bisque = FLinearColor(1.0f, 0.894f, 0.769f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::BlanchedAlmond = FLinearColor(1.0f, 0.922f, 0.804f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::BlueViolet = FLinearColor(0.541f, 0.169f, 0.886f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Brown = FLinearColor(0.647f, 0.165f, 0.165f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::BurlyWood = FLinearColor(0.871f, 0.722f, 0.529f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::CadetBlue = FLinearColor(0.373f, 0.620f, 0.627f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Chartreuse = FLinearColor(0.498f, 1.0f, 0.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Chocolate = FLinearColor(0.824f, 0.412f, 0.118f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Coral = FLinearColor(1.0f, 0.498f, 0.314f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::CornflowerBlue = FLinearColor(0.392f, 0.584f, 0.929f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Cornsilk = FLinearColor(1.0f, 0.973f, 0.863f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Crimson = FLinearColor(0.863f, 0.078f, 0.235f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DarkBlue = FLinearColor(0.0f, 0.0f, 0.545f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DarkCyan = FLinearColor(0.0f, 0.545f, 0.545f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DarkGoldenrod = FLinearColor(0.722f, 0.525f, 0.043f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DarkGray = FLinearColor(0.663f, 0.663f, 0.663f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DarkGreen = FLinearColor(0.0f, 0.392f, 0.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DarkKhaki = FLinearColor(0.741f, 0.718f, 0.420f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DarkMagenta = FLinearColor(0.545f, 0.0f, 0.545f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DarkOliveGreen = FLinearColor(0.333f, 0.420f, 0.184f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DarkOrange = FLinearColor(1.0f, 0.549f, 0.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DarkOrchid = FLinearColor(0.600f, 0.196f, 0.800f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DarkRed = FLinearColor(0.545f, 0.0f, 0.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DarkSalmon = FLinearColor(0.914f, 0.588f, 0.478f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DarkSeaGreen = FLinearColor(0.561f, 0.737f, 0.561f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DarkSlateBlue = FLinearColor(0.282f, 0.239f, 0.545f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DarkSlateGray = FLinearColor(0.184f, 0.310f, 0.310f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DarkTurquoise = FLinearColor(0.0f, 0.808f, 0.820f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DarkViolet = FLinearColor(0.580f, 0.0f, 0.827f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DeepPink = FLinearColor(1.0f, 0.078f, 0.576f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DeepSkyBlue = FLinearColor(0.0f, 0.749f, 1.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DimGray = FLinearColor(0.412f, 0.412f, 0.412f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::DodgerBlue = FLinearColor(0.118f, 0.565f, 1.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::FireBrick = FLinearColor(0.698f, 0.133f, 0.133f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::FloralWhite = FLinearColor(1.0f, 0.980f, 0.941f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::ForestGreen = FLinearColor(0.133f, 0.545f, 0.133f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Fuchsia = FLinearColor(1.0f, 0.0f, 1.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Gainsboro = FLinearColor(0.863f, 0.863f, 0.863f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::GhostWhite = FLinearColor(0.973f, 0.973f, 1.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Gold = FLinearColor(1.0f, 0.843f, 0.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Goldenrod = FLinearColor(0.855f, 0.647f, 0.125f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::GreenYellow = FLinearColor(0.678f, 1.0f, 0.184f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Honeydew = FLinearColor(0.941f, 1.0f, 0.941f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::HotPink = FLinearColor(1.0f, 0.412f, 0.706f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::IndianRed = FLinearColor(0.804f, 0.361f, 0.361f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Indigo = FLinearColor(0.294f, 0.0f, 0.510f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Ivory = FLinearColor(1.0f, 1.0f, 0.941f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Khaki = FLinearColor(0.941f, 0.902f, 0.549f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Lavender = FLinearColor(0.902f, 0.902f, 0.980f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::LavenderBlush = FLinearColor(1.0f, 0.941f, 0.961f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::LawnGreen = FLinearColor(0.486f, 0.988f, 0.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::LemonChiffon = FLinearColor(1.0f, 0.980f, 0.804f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::LightBlue = FLinearColor(0.678f, 0.847f, 0.902f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::LightCoral = FLinearColor(0.941f, 0.502f, 0.502f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::LightCyan = FLinearColor(0.878f, 1.0f, 1.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::LightGoldenrodYellow = FLinearColor(0.980f, 0.980f, 0.824f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::LightGray = FLinearColor(0.827f, 0.827f, 0.827f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::LightGreen = FLinearColor(0.565f, 0.933f, 0.565f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::LightPink = FLinearColor(1.0f, 0.714f, 0.757f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::LightSalmon = FLinearColor(1.0f, 0.627f, 0.478f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::LightSeaGreen = FLinearColor(0.125f, 0.698f, 0.667f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::LightSkyBlue = FLinearColor(0.529f, 0.808f, 0.980f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::LightSlateGray = FLinearColor(0.467f, 0.533f, 0.600f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::LightSteelBlue = FLinearColor(0.690f, 0.769f, 0.871f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::LightYellow = FLinearColor(1.0f, 1.0f, 0.878f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Lime = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::LimeGreen = FLinearColor(0.196f, 0.804f, 0.196f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Linen = FLinearColor(0.980f, 0.941f, 0.902f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Maroon = FLinearColor(0.502f, 0.0f, 0.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::MediumAquamarine = FLinearColor(0.400f, 0.804f, 0.667f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::MediumBlue = FLinearColor(0.0f, 0.0f, 0.804f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::MediumOrchid = FLinearColor(0.729f, 0.333f, 0.827f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::MediumPurple = FLinearColor(0.576f, 0.439f, 0.859f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::MediumSeaGreen = FLinearColor(0.235f, 0.702f, 0.443f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::MediumSlateBlue = FLinearColor(0.482f, 0.408f, 0.933f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::MediumSpringGreen = FLinearColor(0.0f, 0.980f, 0.604f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::MediumTurquoise = FLinearColor(0.282f, 0.820f, 0.800f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::MediumVioletRed = FLinearColor(0.780f, 0.082f, 0.522f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::MidnightBlue = FLinearColor(0.098f, 0.098f, 0.439f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::MintCream = FLinearColor(0.961f, 1.0f, 0.980f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::MistyRose = FLinearColor(1.0f, 0.894f, 0.882f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Moccasin = FLinearColor(1.0f, 0.894f, 0.710f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::NavajoWhite = FLinearColor(1.0f, 0.871f, 0.678f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Navy = FLinearColor(0.0f, 0.0f, 0.502f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::OldLace = FLinearColor(0.992f, 0.961f, 0.902f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Olive = FLinearColor(0.502f, 0.502f, 0.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::OliveDrab = FLinearColor(0.420f, 0.557f, 0.137f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::OrangeRed = FLinearColor(1.0f, 0.271f, 0.0f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Orchid = FLinearColor(0.855f, 0.439f, 0.839f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::PaleGoldenrod = FLinearColor(0.933f, 0.910f, 0.667f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::PaleGreen = FLinearColor(0.596f, 0.984f, 0.596f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::PaleTurquoise = FLinearColor(0.686f, 0.933f, 0.933f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::PaleVioletRed = FLinearColor(0.859f, 0.439f, 0.576f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::PapayaWhip = FLinearColor(1.0f, 0.937f, 0.835f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::PeachPuff = FLinearColor(1.0f, 0.855f, 0.725f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Peru = FLinearColor(0.804f, 0.522f, 0.247f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Pink = FLinearColor(1.0f, 0.753f, 0.796f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Plum = FLinearColor(0.867f, 0.627f, 0.867f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::PowderBlue = FLinearColor(0.690f, 0.878f, 0.902f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::RosyBrown = FLinearColor(0.737f, 0.561f, 0.561f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::RoyalBlue = FLinearColor(0.255f, 0.412f, 0.882f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::SaddleBrown = FLinearColor(0.545f, 0.271f, 0.075f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Salmon = FLinearColor(0.980f, 0.502f, 0.447f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::SandyBrown = FLinearColor(0.957f, 0.643f, 0.376f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::SeaGreen = FLinearColor(0.180f, 0.545f, 0.341f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::SeaShell = FLinearColor(1.0f, 0.961f, 0.933f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Sienna = FLinearColor(0.627f, 0.322f, 0.176f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Silver = FLinearColor(0.753f, 0.753f, 0.753f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::SkyBlue = FLinearColor(0.529f, 0.808f, 0.922f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::SlateBlue = FLinearColor(0.416f, 0.353f, 0.804f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::SlateGray = FLinearColor(0.439f, 0.502f, 0.565f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Snow = FLinearColor(1.0f, 0.980f, 0.980f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::SpringGreen = FLinearColor(0.0f, 1.0f, 0.498f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::SteelBlue = FLinearColor(0.275f, 0.510f, 0.706f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Tan = FLinearColor(0.824f, 0.706f, 0.549f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Teal = FLinearColor(0.0f, 0.502f, 0.502f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Thistle = FLinearColor(0.847f, 0.749f, 0.847f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Tomato = FLinearColor(1.0f, 0.388f, 0.278f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Turquoise = FLinearColor(0.251f, 0.878f, 0.816f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Violet = FLinearColor(0.933f, 0.510f, 0.933f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Wheat = FLinearColor(0.961f, 0.871f, 0.702f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::WhiteSmoke = FLinearColor(0.961f, 0.961f, 0.961f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::YellowGreen = FLinearColor(0.604f, 0.804f, 0.196f, 1.0f);
const FLinearColor UCk_Utils_LinearColor::Transparent = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

// UCk_Utils_LinearColor Function Implementations
FLinearColor UCk_Utils_LinearColor::Get_White(float Alpha)
{
    return White * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Black(float Alpha)
{
    return Black * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Red(float Alpha)
{
    return Red * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Green(float Alpha)
{
    return Green * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Blue(float Alpha)
{
    return Blue * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Yellow(float Alpha)
{
    return Yellow * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Magenta(float Alpha)
{
    return Magenta * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Cyan(float Alpha)
{
    return Cyan * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Orange(float Alpha)
{
    return Orange * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Purple(float Alpha)
{
    return Purple * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

// Grayscale
FLinearColor UCk_Utils_LinearColor::Get_Gray100(float Alpha)
{
    return Gray100 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Gray200(float Alpha)
{
    return Gray200 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Gray300(float Alpha)
{
    return Gray300 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Gray400(float Alpha)
{
    return Gray400 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Gray500(float Alpha)
{
    return Gray500 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Gray600(float Alpha)
{
    return Gray600 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Gray700(float Alpha)
{
    return Gray700 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Gray800(float Alpha)
{
    return Gray800 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Gray900(float Alpha)
{
    return Gray900 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

// Material Design Red Functions
FLinearColor UCk_Utils_LinearColor::Get_Red50(float Alpha)
{
    return Red50 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Red100(float Alpha)
{
    return Red100 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Red200(float Alpha)
{
    return Red200 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Red300(float Alpha)
{
    return Red300 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Red400(float Alpha)
{
    return Red400 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Red500(float Alpha)
{
    return Red500 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Red600(float Alpha)
{
    return Red600 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Red700(float Alpha)
{
    return Red700 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Red800(float Alpha)
{
    return Red800 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Red900(float Alpha)
{
    return Red900 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

// Material Design Blue Functions
FLinearColor UCk_Utils_LinearColor::Get_Blue50(float Alpha)
{
    return Blue50 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Blue100(float Alpha)
{
    return Blue100 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Blue200(float Alpha)
{
    return Blue200 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Blue300(float Alpha)
{
    return Blue300 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Blue400(float Alpha)
{
    return Blue400 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Blue500(float Alpha)
{
    return Blue500 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Blue600(float Alpha)
{
    return Blue600 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Blue700(float Alpha)
{
    return Blue700 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Blue800(float Alpha)
{
    return Blue800 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Blue900(float Alpha)
{
    return Blue900 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

// Material Design Green Functions
FLinearColor UCk_Utils_LinearColor::Get_Green50(float Alpha)
{
    return Green50 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Green100(float Alpha)
{
    return Green100 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Green200(float Alpha)
{
    return Green200 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Green300(float Alpha)
{
    return Green300 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Green400(float Alpha)
{
    return Green400 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Green500(float Alpha)
{
    return Green500 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Green600(float Alpha)
{
    return Green600 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Green700(float Alpha)
{
    return Green700 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Green800(float Alpha)
{
    return Green800 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Green900(float Alpha)
{
    return Green900 * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

// Named Web Colors Functions (sample implementation - pattern for all others)
FLinearColor UCk_Utils_LinearColor::Get_AliceBlue(float Alpha)
{
    return AliceBlue * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_AntiqueWhite(float Alpha)
{
    return AntiqueWhite * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Aqua(float Alpha)
{
    return Aqua * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Aquamarine(float Alpha)
{
    return Aquamarine * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Azure(float Alpha)
{
    return Azure * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Beige(float Alpha)
{
    return Beige * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Bisque(float Alpha)
{
    return Bisque * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_BlanchedAlmond(float Alpha)
{
    return BlanchedAlmond * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_BlueViolet(float Alpha)
{
    return BlueViolet * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Brown(float Alpha)
{
    return Brown * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_BurlyWood(float Alpha)
{
    return BurlyWood * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_CadetBlue(float Alpha)
{
    return CadetBlue * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Chartreuse(float Alpha)
{
    return Chartreuse * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Chocolate(float Alpha)
{
    return Chocolate * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Coral(float Alpha)
{
    return Coral * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_CornflowerBlue(float Alpha)
{
    return CornflowerBlue * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Cornsilk(float Alpha)
{
    return Cornsilk * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Crimson(float Alpha)
{
    return Crimson * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DarkBlue(float Alpha)
{
    return DarkBlue * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DarkCyan(float Alpha)
{
    return DarkCyan * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DarkGoldenrod(float Alpha)
{
    return DarkGoldenrod * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DarkGray(float Alpha)
{
    return DarkGray * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DarkGreen(float Alpha)
{
    return DarkGreen * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DarkKhaki(float Alpha)
{
    return DarkKhaki * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DarkMagenta(float Alpha)
{
    return DarkMagenta * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DarkOliveGreen(float Alpha)
{
    return DarkOliveGreen * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DarkOrange(float Alpha)
{
    return DarkOrange * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DarkOrchid(float Alpha)
{
    return DarkOrchid * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DarkRed(float Alpha)
{
    return DarkRed * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DarkSalmon(float Alpha)
{
    return DarkSalmon * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DarkSeaGreen(float Alpha)
{
    return DarkSeaGreen * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DarkSlateBlue(float Alpha)
{
    return DarkSlateBlue * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DarkSlateGray(float Alpha)
{
    return DarkSlateGray * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DarkTurquoise(float Alpha)
{
    return DarkTurquoise * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DarkViolet(float Alpha)
{
    return DarkViolet * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DeepPink(float Alpha)
{
    return DeepPink * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DeepSkyBlue(float Alpha)
{
    return DeepSkyBlue * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DimGray(float Alpha)
{
    return DimGray * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_DodgerBlue(float Alpha)
{
    return DodgerBlue * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_FireBrick(float Alpha)
{
    return FireBrick * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_FloralWhite(float Alpha)
{
    return FloralWhite * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_ForestGreen(float Alpha)
{
    return ForestGreen * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Fuchsia(float Alpha)
{
    return Fuchsia * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Gainsboro(float Alpha)
{
    return Gainsboro * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_GhostWhite(float Alpha)
{
    return GhostWhite * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Gold(float Alpha)
{
    return Gold * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Goldenrod(float Alpha)
{
    return Goldenrod * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_GreenYellow(float Alpha)
{
    return GreenYellow * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Honeydew(float Alpha)
{
    return Honeydew * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_HotPink(float Alpha)
{
    return HotPink * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_IndianRed(float Alpha)
{
    return IndianRed * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Indigo(float Alpha)
{
    return Indigo * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Ivory(float Alpha)
{
    return Ivory * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Khaki(float Alpha)
{
    return Khaki * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Lavender(float Alpha)
{
    return Lavender * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_LavenderBlush(float Alpha)
{
    return LavenderBlush * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_LawnGreen(float Alpha)
{
    return LawnGreen * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_LemonChiffon(float Alpha)
{
    return LemonChiffon * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_LightBlue(float Alpha)
{
    return LightBlue * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_LightCoral(float Alpha)
{
    return LightCoral * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_LightCyan(float Alpha)
{
    return LightCyan * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_LightGoldenrodYellow(float Alpha)
{
    return LightGoldenrodYellow * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_LightGray(float Alpha)
{
    return LightGray * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_LightGreen(float Alpha)
{
    return LightGreen * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_LightPink(float Alpha)
{
    return LightPink * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_LightSalmon(float Alpha)
{
    return LightSalmon * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_LightSeaGreen(float Alpha)
{
    return LightSeaGreen * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_LightSkyBlue(float Alpha)
{
    return LightSkyBlue * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_LightSlateGray(float Alpha)
{
    return LightSlateGray * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_LightSteelBlue(float Alpha)
{
    return LightSteelBlue * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_LightYellow(float Alpha)
{
    return LightYellow * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Lime(float Alpha)
{
    return Lime * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_LimeGreen(float Alpha)
{
    return LimeGreen * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Linen(float Alpha)
{
    return Linen * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Maroon(float Alpha)
{
    return Maroon * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_MediumAquamarine(float Alpha)
{
    return MediumAquamarine * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_MediumBlue(float Alpha)
{
    return MediumBlue * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_MediumOrchid(float Alpha)
{
    return MediumOrchid * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_MediumPurple(float Alpha)
{
    return MediumPurple * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_MediumSeaGreen(float Alpha)
{
    return MediumSeaGreen * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_MediumSlateBlue(float Alpha)
{
    return MediumSlateBlue * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_MediumSpringGreen(float Alpha)
{
    return MediumSpringGreen * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_MediumTurquoise(float Alpha)
{
    return MediumTurquoise * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_MediumVioletRed(float Alpha)
{
    return MediumVioletRed * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_MidnightBlue(float Alpha)
{
    return MidnightBlue * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_MintCream(float Alpha)
{
    return MintCream * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_MistyRose(float Alpha)
{
    return MistyRose * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Moccasin(float Alpha)
{
    return Moccasin * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_NavajoWhite(float Alpha)
{
    return NavajoWhite * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Navy(float Alpha)
{
    return Navy * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_OldLace(float Alpha)
{
    return OldLace * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Olive(float Alpha)
{
    return Olive * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_OliveDrab(float Alpha)
{
    return OliveDrab * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_OrangeRed(float Alpha)
{
    return OrangeRed * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Orchid(float Alpha)
{
    return Orchid * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_PaleGoldenrod(float Alpha)
{
    return PaleGoldenrod * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_PaleGreen(float Alpha)
{
    return PaleGreen * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_PaleTurquoise(float Alpha)
{
    return PaleTurquoise * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_PaleVioletRed(float Alpha)
{
    return PaleVioletRed * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_PapayaWhip(float Alpha)
{
    return PapayaWhip * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_PeachPuff(float Alpha)
{
    return PeachPuff * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Peru(float Alpha)
{
    return Peru * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Pink(float Alpha)
{
    return Pink * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Plum(float Alpha)
{
    return Plum * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_PowderBlue(float Alpha)
{
    return PowderBlue * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_RosyBrown(float Alpha)
{
    return RosyBrown * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_RoyalBlue(float Alpha)
{
    return RoyalBlue * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_SaddleBrown(float Alpha)
{
    return SaddleBrown * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Salmon(float Alpha)
{
    return Salmon * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_SandyBrown(float Alpha)
{
    return SandyBrown * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_SeaGreen(float Alpha)
{
    return SeaGreen * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_SeaShell(float Alpha)
{
    return SeaShell * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Sienna(float Alpha)
{
    return Sienna * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Silver(float Alpha)
{
    return Silver * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_SkyBlue(float Alpha)
{
    return SkyBlue * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_SlateBlue(float Alpha)
{
    return SlateBlue * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_SlateGray(float Alpha)
{
    return SlateGray * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Snow(float Alpha)
{
    return Snow * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_SpringGreen(float Alpha)
{
    return SpringGreen * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_SteelBlue(float Alpha)
{
    return SteelBlue * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Tan(float Alpha)
{
    return Tan * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Teal(float Alpha)
{
    return Teal * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Thistle(float Alpha)
{
    return Thistle * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Tomato(float Alpha)
{
    return Tomato * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Turquoise(float Alpha)
{
    return Turquoise * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Violet(float Alpha)
{
    return Violet * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Wheat(float Alpha)
{
    return Wheat * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_WhiteSmoke(float Alpha)
{
    return WhiteSmoke * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_YellowGreen(float Alpha)
{
    return YellowGreen * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

FLinearColor UCk_Utils_LinearColor::Get_Transparent(float Alpha)
{
    return Transparent * FLinearColor(1.0f, 1.0f, 1.0f, Alpha);
}

// ========================================
// UCk_Utils_Color (FColor) Implementation
// ========================================

// UCk_Utils_Color Static Constants
const FColor UCk_Utils_Color::White = FColor(255, 255, 255, 255);
const FColor UCk_Utils_Color::Black = FColor(0, 0, 0, 255);
const FColor UCk_Utils_Color::Red = FColor(255, 0, 0, 255);
const FColor UCk_Utils_Color::Green = FColor(0, 255, 0, 255);
const FColor UCk_Utils_Color::Blue = FColor(0, 0, 255, 255);
const FColor UCk_Utils_Color::Yellow = FColor(255, 255, 0, 255);
const FColor UCk_Utils_Color::Magenta = FColor(255, 0, 255, 255);
const FColor UCk_Utils_Color::Cyan = FColor(0, 255, 255, 255);
const FColor UCk_Utils_Color::Orange = FColor(255, 165, 0, 255);
const FColor UCk_Utils_Color::Purple = FColor(128, 0, 128, 255);

// Grayscale
const FColor UCk_Utils_Color::Gray100 = FColor(244, 244, 244, 255);
const FColor UCk_Utils_Color::Gray200 = FColor(229, 229, 229, 255);
const FColor UCk_Utils_Color::Gray300 = FColor(211, 211, 211, 255);
const FColor UCk_Utils_Color::Gray400 = FColor(189, 189, 189, 255);
const FColor UCk_Utils_Color::Gray500 = FColor(158, 158, 158, 255);
const FColor UCk_Utils_Color::Gray600 = FColor(117, 117, 117, 255);
const FColor UCk_Utils_Color::Gray700 = FColor(99, 99, 99, 255);
const FColor UCk_Utils_Color::Gray800 = FColor(71, 71, 71, 255);
const FColor UCk_Utils_Color::Gray900 = FColor(33, 33, 33, 255);

// Material Design Red
const FColor UCk_Utils_Color::Red50 = FColor(255, 235, 238, 255);
const FColor UCk_Utils_Color::Red100 = FColor(255, 205, 210, 255);
const FColor UCk_Utils_Color::Red200 = FColor(239, 154, 154, 255);
const FColor UCk_Utils_Color::Red300 = FColor(229, 115, 115, 255);
const FColor UCk_Utils_Color::Red400 = FColor(239, 83, 80, 255);
const FColor UCk_Utils_Color::Red500 = FColor(244, 67, 54, 255);
const FColor UCk_Utils_Color::Red600 = FColor(229, 57, 53, 255);
const FColor UCk_Utils_Color::Red700 = FColor(211, 47, 47, 255);
const FColor UCk_Utils_Color::Red800 = FColor(198, 40, 40, 255);
const FColor UCk_Utils_Color::Red900 = FColor(183, 28, 28, 255);

// Material Design Blue
const FColor UCk_Utils_Color::Blue50 = FColor(227, 242, 253, 255);
const FColor UCk_Utils_Color::Blue100 = FColor(187, 222, 251, 255);
const FColor UCk_Utils_Color::Blue200 = FColor(144, 202, 249, 255);
const FColor UCk_Utils_Color::Blue300 = FColor(100, 181, 246, 255);
const FColor UCk_Utils_Color::Blue400 = FColor(66, 165, 245, 255);
const FColor UCk_Utils_Color::Blue500 = FColor(33, 150, 243, 255);
const FColor UCk_Utils_Color::Blue600 = FColor(30, 136, 229, 255);
const FColor UCk_Utils_Color::Blue700 = FColor(25, 118, 210, 255);
const FColor UCk_Utils_Color::Blue800 = FColor(20, 104, 192, 255);
const FColor UCk_Utils_Color::Blue900 = FColor(13, 71, 161, 255);

// Material Design Green
const FColor UCk_Utils_Color::Green50 = FColor(232, 245, 233, 255);
const FColor UCk_Utils_Color::Green100 = FColor(200, 230, 201, 255);
const FColor UCk_Utils_Color::Green200 = FColor(165, 214, 167, 255);
const FColor UCk_Utils_Color::Green300 = FColor(129, 199, 132, 255);
const FColor UCk_Utils_Color::Green400 = FColor(102, 187, 106, 255);
const FColor UCk_Utils_Color::Green500 = FColor(76, 175, 80, 255);
const FColor UCk_Utils_Color::Green600 = FColor(67, 160, 71, 255);
const FColor UCk_Utils_Color::Green700 = FColor(56, 142, 60, 255);
const FColor UCk_Utils_Color::Green800 = FColor(46, 125, 50, 255);
const FColor UCk_Utils_Color::Green900 = FColor(27, 94, 32, 255);

// Named Web Colors
const FColor UCk_Utils_Color::AliceBlue = FColor(240, 248, 255, 255);
const FColor UCk_Utils_Color::AntiqueWhite = FColor(250, 235, 215, 255);
const FColor UCk_Utils_Color::Aqua = FColor(0, 255, 255, 255);
const FColor UCk_Utils_Color::Aquamarine = FColor(127, 255, 212, 255);
const FColor UCk_Utils_Color::Azure = FColor(240, 255, 255, 255);
const FColor UCk_Utils_Color::Beige = FColor(245, 245, 220, 255);
const FColor UCk_Utils_Color::Bisque = FColor(255, 228, 196, 255);
const FColor UCk_Utils_Color::BlanchedAlmond = FColor(255, 235, 205, 255);
const FColor UCk_Utils_Color::BlueViolet = FColor(138, 43, 226, 255);
const FColor UCk_Utils_Color::Brown = FColor(165, 42, 42, 255);
const FColor UCk_Utils_Color::BurlyWood = FColor(222, 184, 135, 255);
const FColor UCk_Utils_Color::CadetBlue = FColor(95, 158, 160, 255);
const FColor UCk_Utils_Color::Chartreuse = FColor(127, 255, 0, 255);
const FColor UCk_Utils_Color::Chocolate = FColor(210, 105, 30, 255);
const FColor UCk_Utils_Color::Coral = FColor(255, 127, 80, 255);
const FColor UCk_Utils_Color::CornflowerBlue = FColor(100, 149, 237, 255);
const FColor UCk_Utils_Color::Cornsilk = FColor(255, 248, 220, 255);
const FColor UCk_Utils_Color::Crimson = FColor(220, 20, 60, 255);
const FColor UCk_Utils_Color::DarkBlue = FColor(0, 0, 139, 255);
const FColor UCk_Utils_Color::DarkCyan = FColor(0, 139, 139, 255);
const FColor UCk_Utils_Color::DarkGoldenrod = FColor(184, 134, 11, 255);
const FColor UCk_Utils_Color::DarkGray = FColor(169, 169, 169, 255);
const FColor UCk_Utils_Color::DarkGreen = FColor(0, 100, 0, 255);
const FColor UCk_Utils_Color::DarkKhaki = FColor(189, 183, 107, 255);
const FColor UCk_Utils_Color::DarkMagenta = FColor(139, 0, 139, 255);
const FColor UCk_Utils_Color::DarkOliveGreen = FColor(85, 107, 47, 255);
const FColor UCk_Utils_Color::DarkOrange = FColor(255, 140, 0, 255);
const FColor UCk_Utils_Color::DarkOrchid = FColor(153, 50, 204, 255);
const FColor UCk_Utils_Color::DarkRed = FColor(139, 0, 0, 255);
const FColor UCk_Utils_Color::DarkSalmon = FColor(233, 150, 122, 255);
const FColor UCk_Utils_Color::DarkSeaGreen = FColor(143, 188, 143, 255);
const FColor UCk_Utils_Color::DarkSlateBlue = FColor(72, 61, 139, 255);
const FColor UCk_Utils_Color::DarkSlateGray = FColor(47, 79, 79, 255);
const FColor UCk_Utils_Color::DarkTurquoise = FColor(0, 206, 209, 255);
const FColor UCk_Utils_Color::DarkViolet = FColor(148, 0, 211, 255);
const FColor UCk_Utils_Color::DeepPink = FColor(255, 20, 147, 255);
const FColor UCk_Utils_Color::DeepSkyBlue = FColor(0, 191, 255, 255);
const FColor UCk_Utils_Color::DimGray = FColor(105, 105, 105, 255);
const FColor UCk_Utils_Color::DodgerBlue = FColor(30, 144, 255, 255);
const FColor UCk_Utils_Color::FireBrick = FColor(178, 34, 34, 255);
const FColor UCk_Utils_Color::FloralWhite = FColor(255, 250, 240, 255);
const FColor UCk_Utils_Color::ForestGreen = FColor(34, 139, 34, 255);
const FColor UCk_Utils_Color::Fuchsia = FColor(255, 0, 255, 255);
const FColor UCk_Utils_Color::Gainsboro = FColor(220, 220, 220, 255);
const FColor UCk_Utils_Color::GhostWhite = FColor(248, 248, 255, 255);
const FColor UCk_Utils_Color::Gold = FColor(255, 215, 0, 255);
const FColor UCk_Utils_Color::Goldenrod = FColor(218, 165, 32, 255);
const FColor UCk_Utils_Color::GreenYellow = FColor(173, 255, 47, 255);
const FColor UCk_Utils_Color::Honeydew = FColor(240, 255, 240, 255);
const FColor UCk_Utils_Color::HotPink = FColor(255, 105, 180, 255);
const FColor UCk_Utils_Color::IndianRed = FColor(205, 92, 92, 255);
const FColor UCk_Utils_Color::Indigo = FColor(75, 0, 130, 255);
const FColor UCk_Utils_Color::Ivory = FColor(255, 255, 240, 255);
const FColor UCk_Utils_Color::Khaki = FColor(240, 230, 140, 255);
const FColor UCk_Utils_Color::Lavender = FColor(230, 230, 250, 255);
const FColor UCk_Utils_Color::LavenderBlush = FColor(255, 240, 245, 255);
const FColor UCk_Utils_Color::LawnGreen = FColor(124, 252, 0, 255);
const FColor UCk_Utils_Color::LemonChiffon = FColor(255, 250, 205, 255);
const FColor UCk_Utils_Color::LightBlue = FColor(173, 216, 230, 255);
const FColor UCk_Utils_Color::LightCoral = FColor(240, 128, 128, 255);
const FColor UCk_Utils_Color::LightCyan = FColor(224, 255, 255, 255);
const FColor UCk_Utils_Color::LightGoldenrodYellow = FColor(250, 250, 210, 255);
const FColor UCk_Utils_Color::LightGray = FColor(211, 211, 211, 255);
const FColor UCk_Utils_Color::LightGreen = FColor(144, 238, 144, 255);
const FColor UCk_Utils_Color::LightPink = FColor(255, 182, 193, 255);
const FColor UCk_Utils_Color::LightSalmon = FColor(255, 160, 122, 255);
const FColor UCk_Utils_Color::LightSeaGreen = FColor(32, 178, 170, 255);
const FColor UCk_Utils_Color::LightSkyBlue = FColor(135, 206, 250, 255);
const FColor UCk_Utils_Color::LightSlateGray = FColor(119, 136, 153, 255);
const FColor UCk_Utils_Color::LightSteelBlue = FColor(176, 196, 222, 255);
const FColor UCk_Utils_Color::LightYellow = FColor(255, 255, 224, 255);
const FColor UCk_Utils_Color::Lime = FColor(0, 255, 0, 255);
const FColor UCk_Utils_Color::LimeGreen = FColor(50, 205, 50, 255);
const FColor UCk_Utils_Color::Linen = FColor(250, 240, 230, 255);
const FColor UCk_Utils_Color::Maroon = FColor(128, 0, 0, 255);
const FColor UCk_Utils_Color::MediumAquamarine = FColor(102, 205, 170, 255);
const FColor UCk_Utils_Color::MediumBlue = FColor(0, 0, 205, 255);
const FColor UCk_Utils_Color::MediumOrchid = FColor(186, 85, 211, 255);
const FColor UCk_Utils_Color::MediumPurple = FColor(147, 112, 219, 255);
const FColor UCk_Utils_Color::MediumSeaGreen = FColor(60, 179, 113, 255);
const FColor UCk_Utils_Color::MediumSlateBlue = FColor(123, 104, 238, 255);
const FColor UCk_Utils_Color::MediumSpringGreen = FColor(0, 250, 154, 255);
const FColor UCk_Utils_Color::MediumTurquoise = FColor(72, 209, 204, 255);
const FColor UCk_Utils_Color::MediumVioletRed = FColor(199, 21, 133, 255);
const FColor UCk_Utils_Color::MidnightBlue = FColor(25, 25, 112, 255);
const FColor UCk_Utils_Color::MintCream = FColor(245, 255, 250, 255);
const FColor UCk_Utils_Color::MistyRose = FColor(255, 228, 225, 255);
const FColor UCk_Utils_Color::Moccasin = FColor(255, 228, 181, 255);
const FColor UCk_Utils_Color::NavajoWhite = FColor(255, 222, 173, 255);
const FColor UCk_Utils_Color::Navy = FColor(0, 0, 128, 255);
const FColor UCk_Utils_Color::OldLace = FColor(253, 245, 230, 255);
const FColor UCk_Utils_Color::Olive = FColor(128, 128, 0, 255);
const FColor UCk_Utils_Color::OliveDrab = FColor(107, 142, 35, 255);
const FColor UCk_Utils_Color::OrangeRed = FColor(255, 69, 0, 255);
const FColor UCk_Utils_Color::Orchid = FColor(218, 112, 214, 255);
const FColor UCk_Utils_Color::PaleGoldenrod = FColor(238, 232, 170, 255);
const FColor UCk_Utils_Color::PaleGreen = FColor(152, 251, 152, 255);
const FColor UCk_Utils_Color::PaleTurquoise = FColor(175, 238, 238, 255);
const FColor UCk_Utils_Color::PaleVioletRed = FColor(219, 112, 147, 255);
const FColor UCk_Utils_Color::PapayaWhip = FColor(255, 239, 213, 255);
const FColor UCk_Utils_Color::PeachPuff = FColor(255, 218, 185, 255);
const FColor UCk_Utils_Color::Peru = FColor(205, 133, 63, 255);
const FColor UCk_Utils_Color::Pink = FColor(255, 192, 203, 255);
const FColor UCk_Utils_Color::Plum = FColor(221, 160, 221, 255);
const FColor UCk_Utils_Color::PowderBlue = FColor(176, 224, 230, 255);
const FColor UCk_Utils_Color::RosyBrown = FColor(188, 143, 143, 255);
const FColor UCk_Utils_Color::RoyalBlue = FColor(65, 105, 225, 255);
const FColor UCk_Utils_Color::SaddleBrown = FColor(139, 69, 19, 255);
const FColor UCk_Utils_Color::Salmon = FColor(250, 128, 114, 255);
const FColor UCk_Utils_Color::SandyBrown = FColor(244, 164, 96, 255);
const FColor UCk_Utils_Color::SeaGreen = FColor(46, 139, 87, 255);
const FColor UCk_Utils_Color::SeaShell = FColor(255, 245, 238, 255);
const FColor UCk_Utils_Color::Sienna = FColor(160, 82, 45, 255);
const FColor UCk_Utils_Color::Silver = FColor(192, 192, 192, 255);
const FColor UCk_Utils_Color::SkyBlue = FColor(135, 206, 235, 255);
const FColor UCk_Utils_Color::SlateBlue = FColor(106, 90, 205, 255);
const FColor UCk_Utils_Color::SlateGray = FColor(112, 128, 144, 255);
const FColor UCk_Utils_Color::Snow = FColor(255, 250, 250, 255);
const FColor UCk_Utils_Color::SpringGreen = FColor(0, 255, 127, 255);
const FColor UCk_Utils_Color::SteelBlue = FColor(70, 130, 180, 255);
const FColor UCk_Utils_Color::Tan = FColor(210, 180, 140, 255);
const FColor UCk_Utils_Color::Teal = FColor(0, 128, 128, 255);
const FColor UCk_Utils_Color::Thistle = FColor(216, 191, 216, 255);
const FColor UCk_Utils_Color::Tomato = FColor(255, 99, 71, 255);
const FColor UCk_Utils_Color::Turquoise = FColor(64, 224, 208, 255);
const FColor UCk_Utils_Color::Violet = FColor(238, 130, 238, 255);
const FColor UCk_Utils_Color::Wheat = FColor(245, 222, 179, 255);
const FColor UCk_Utils_Color::WhiteSmoke = FColor(245, 245, 245, 255);
const FColor UCk_Utils_Color::YellowGreen = FColor(154, 205, 50, 255);
const FColor UCk_Utils_Color::Transparent = FColor(0, 0, 0, 0);

// UCk_Utils_Color Function Implementations
FColor UCk_Utils_Color::Get_White(float Alpha)
{
    return UCk_Utils_LinearColor::Get_White(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Black(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Black(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Red(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Red(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Green(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Green(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Blue(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Blue(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Yellow(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Yellow(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Magenta(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Magenta(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Cyan(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Cyan(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Orange(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Orange(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Purple(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Purple(Alpha).ToFColor(true);
}

// Grayscale
FColor UCk_Utils_Color::Get_Gray100(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Gray100(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Gray200(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Gray200(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Gray300(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Gray300(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Gray400(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Gray400(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Gray500(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Gray500(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Gray600(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Gray600(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Gray700(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Gray700(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Gray800(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Gray800(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Gray900(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Gray900(Alpha).ToFColor(true);
}

// Material Design Red Functions
FColor UCk_Utils_Color::Get_Red50(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Red50(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Red100(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Red100(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Red200(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Red200(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Red300(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Red300(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Red400(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Red400(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Red500(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Red500(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Red600(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Red600(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Red700(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Red700(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Red800(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Red800(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Red900(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Red900(Alpha).ToFColor(true);
}

// Material Design Blue Functions
FColor UCk_Utils_Color::Get_Blue50(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Blue50(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Blue100(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Blue100(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Blue200(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Blue200(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Blue300(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Blue300(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Blue400(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Blue400(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Blue500(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Blue500(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Blue600(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Blue600(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Blue700(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Blue700(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Blue800(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Blue800(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Blue900(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Blue900(Alpha).ToFColor(true);
}

// Material Design Green Functions
FColor UCk_Utils_Color::Get_Green50(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Green50(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Green100(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Green100(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Green200(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Green200(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Green300(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Green300(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Green400(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Green400(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Green500(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Green500(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Green600(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Green600(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Green700(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Green700(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Green800(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Green800(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Green900(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Green900(Alpha).ToFColor(true);
}

// Named Web Colors Functions (sample implementation - pattern for all others)
FColor UCk_Utils_Color::Get_AliceBlue(float Alpha)
{
    return UCk_Utils_LinearColor::Get_AliceBlue(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_AntiqueWhite(float Alpha)
{
    return UCk_Utils_LinearColor::Get_AntiqueWhite(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Aqua(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Aqua(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Aquamarine(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Aquamarine(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Azure(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Azure(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Beige(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Beige(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Bisque(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Bisque(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_BlanchedAlmond(float Alpha)
{
    return UCk_Utils_LinearColor::Get_BlanchedAlmond(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_BlueViolet(float Alpha)
{
    return UCk_Utils_LinearColor::Get_BlueViolet(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Brown(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Brown(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_BurlyWood(float Alpha)
{
    return UCk_Utils_LinearColor::Get_BurlyWood(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_CadetBlue(float Alpha)
{
    return UCk_Utils_LinearColor::Get_CadetBlue(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Chartreuse(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Chartreuse(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Chocolate(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Chocolate(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Coral(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Coral(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_CornflowerBlue(float Alpha)
{
    return UCk_Utils_LinearColor::Get_CornflowerBlue(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Cornsilk(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Cornsilk(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Crimson(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Crimson(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DarkBlue(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DarkBlue(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DarkCyan(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DarkCyan(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DarkGoldenrod(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DarkGoldenrod(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DarkGray(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DarkGray(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DarkGreen(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DarkGreen(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DarkKhaki(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DarkKhaki(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DarkMagenta(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DarkMagenta(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DarkOliveGreen(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DarkOliveGreen(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DarkOrange(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DarkOrange(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DarkOrchid(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DarkOrchid(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DarkRed(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DarkRed(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DarkSalmon(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DarkSalmon(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DarkSeaGreen(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DarkSeaGreen(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DarkSlateBlue(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DarkSlateBlue(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DarkSlateGray(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DarkSlateGray(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DarkTurquoise(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DarkTurquoise(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DarkViolet(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DarkViolet(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DeepPink(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DeepPink(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DeepSkyBlue(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DeepSkyBlue(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DimGray(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DimGray(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_DodgerBlue(float Alpha)
{
    return UCk_Utils_LinearColor::Get_DodgerBlue(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_FireBrick(float Alpha)
{
    return UCk_Utils_LinearColor::Get_FireBrick(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_FloralWhite(float Alpha)
{
    return UCk_Utils_LinearColor::Get_FloralWhite(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_ForestGreen(float Alpha)
{
    return UCk_Utils_LinearColor::Get_ForestGreen(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Fuchsia(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Fuchsia(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Gainsboro(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Gainsboro(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_GhostWhite(float Alpha)
{
    return UCk_Utils_LinearColor::Get_GhostWhite(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Gold(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Gold(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Goldenrod(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Goldenrod(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_GreenYellow(float Alpha)
{
    return UCk_Utils_LinearColor::Get_GreenYellow(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Honeydew(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Honeydew(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_HotPink(float Alpha)
{
    return UCk_Utils_LinearColor::Get_HotPink(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_IndianRed(float Alpha)
{
    return UCk_Utils_LinearColor::Get_IndianRed(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Indigo(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Indigo(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Ivory(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Ivory(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Khaki(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Khaki(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Lavender(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Lavender(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_LavenderBlush(float Alpha)
{
    return UCk_Utils_LinearColor::Get_LavenderBlush(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_LawnGreen(float Alpha)
{
    return UCk_Utils_LinearColor::Get_LawnGreen(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_LemonChiffon(float Alpha)
{
    return UCk_Utils_LinearColor::Get_LemonChiffon(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_LightBlue(float Alpha)
{
    return UCk_Utils_LinearColor::Get_LightBlue(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_LightCoral(float Alpha)
{
    return UCk_Utils_LinearColor::Get_LightCoral(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_LightCyan(float Alpha)
{
    return UCk_Utils_LinearColor::Get_LightCyan(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_LightGoldenrodYellow(float Alpha)
{
    return UCk_Utils_LinearColor::Get_LightGoldenrodYellow(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_LightGray(float Alpha)
{
    return UCk_Utils_LinearColor::Get_LightGray(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_LightGreen(float Alpha)
{
    return UCk_Utils_LinearColor::Get_LightGreen(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_LightPink(float Alpha)
{
    return UCk_Utils_LinearColor::Get_LightPink(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_LightSalmon(float Alpha)
{
    return UCk_Utils_LinearColor::Get_LightSalmon(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_LightSeaGreen(float Alpha)
{
    return UCk_Utils_LinearColor::Get_LightSeaGreen(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_LightSkyBlue(float Alpha)
{
    return UCk_Utils_LinearColor::Get_LightSkyBlue(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_LightSlateGray(float Alpha)
{
    return UCk_Utils_LinearColor::Get_LightSlateGray(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_LightSteelBlue(float Alpha)
{
    return UCk_Utils_LinearColor::Get_LightSteelBlue(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_LightYellow(float Alpha)
{
    return UCk_Utils_LinearColor::Get_LightYellow(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Lime(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Lime(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_LimeGreen(float Alpha)
{
    return UCk_Utils_LinearColor::Get_LimeGreen(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Linen(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Linen(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Maroon(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Maroon(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_MediumAquamarine(float Alpha)
{
    return UCk_Utils_LinearColor::Get_MediumAquamarine(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_MediumBlue(float Alpha)
{
    return UCk_Utils_LinearColor::Get_MediumBlue(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_MediumOrchid(float Alpha)
{
    return UCk_Utils_LinearColor::Get_MediumOrchid(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_MediumPurple(float Alpha)
{
    return UCk_Utils_LinearColor::Get_MediumPurple(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_MediumSeaGreen(float Alpha)
{
    return UCk_Utils_LinearColor::Get_MediumSeaGreen(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_MediumSlateBlue(float Alpha)
{
    return UCk_Utils_LinearColor::Get_MediumSlateBlue(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_MediumSpringGreen(float Alpha)
{
    return UCk_Utils_LinearColor::Get_MediumSpringGreen(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_MediumTurquoise(float Alpha)
{
    return UCk_Utils_LinearColor::Get_MediumTurquoise(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_MediumVioletRed(float Alpha)
{
    return UCk_Utils_LinearColor::Get_MediumVioletRed(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_MidnightBlue(float Alpha)
{
    return UCk_Utils_LinearColor::Get_MidnightBlue(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_MintCream(float Alpha)
{
    return UCk_Utils_LinearColor::Get_MintCream(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_MistyRose(float Alpha)
{
    return UCk_Utils_LinearColor::Get_MistyRose(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Moccasin(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Moccasin(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_NavajoWhite(float Alpha)
{
    return UCk_Utils_LinearColor::Get_NavajoWhite(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Navy(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Navy(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_OldLace(float Alpha)
{
    return UCk_Utils_LinearColor::Get_OldLace(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Olive(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Olive(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_OliveDrab(float Alpha)
{
    return UCk_Utils_LinearColor::Get_OliveDrab(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_OrangeRed(float Alpha)
{
    return UCk_Utils_LinearColor::Get_OrangeRed(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Orchid(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Orchid(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_PaleGoldenrod(float Alpha)
{
    return UCk_Utils_LinearColor::Get_PaleGoldenrod(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_PaleGreen(float Alpha)
{
    return UCk_Utils_LinearColor::Get_PaleGreen(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_PaleTurquoise(float Alpha)
{
    return UCk_Utils_LinearColor::Get_PaleTurquoise(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_PaleVioletRed(float Alpha)
{
    return UCk_Utils_LinearColor::Get_PaleVioletRed(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_PapayaWhip(float Alpha)
{
    return UCk_Utils_LinearColor::Get_PapayaWhip(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_PeachPuff(float Alpha)
{
    return UCk_Utils_LinearColor::Get_PeachPuff(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Peru(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Peru(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Pink(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Pink(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Plum(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Plum(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_PowderBlue(float Alpha)
{
    return UCk_Utils_LinearColor::Get_PowderBlue(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_RosyBrown(float Alpha)
{
    return UCk_Utils_LinearColor::Get_RosyBrown(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_RoyalBlue(float Alpha)
{
    return UCk_Utils_LinearColor::Get_RoyalBlue(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_SaddleBrown(float Alpha)
{
    return UCk_Utils_LinearColor::Get_SaddleBrown(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Salmon(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Salmon(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_SandyBrown(float Alpha)
{
    return UCk_Utils_LinearColor::Get_SandyBrown(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_SeaGreen(float Alpha)
{
    return UCk_Utils_LinearColor::Get_SeaGreen(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_SeaShell(float Alpha)
{
    return UCk_Utils_LinearColor::Get_SeaShell(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Sienna(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Sienna(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Silver(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Silver(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_SkyBlue(float Alpha)
{
    return UCk_Utils_LinearColor::Get_SkyBlue(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_SlateBlue(float Alpha)
{
    return UCk_Utils_LinearColor::Get_SlateBlue(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_SlateGray(float Alpha)
{
    return UCk_Utils_LinearColor::Get_SlateGray(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Snow(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Snow(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_SpringGreen(float Alpha)
{
    return UCk_Utils_LinearColor::Get_SpringGreen(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_SteelBlue(float Alpha)
{
    return UCk_Utils_LinearColor::Get_SteelBlue(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Tan(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Tan(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Teal(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Teal(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Thistle(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Thistle(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Tomato(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Tomato(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Turquoise(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Turquoise(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Violet(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Violet(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Wheat(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Wheat(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_WhiteSmoke(float Alpha)
{
    return UCk_Utils_LinearColor::Get_WhiteSmoke(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_YellowGreen(float Alpha)
{
    return UCk_Utils_LinearColor::Get_YellowGreen(Alpha).ToFColor(true);
}

FColor UCk_Utils_Color::Get_Transparent(float Alpha)
{
    return UCk_Utils_LinearColor::Get_Transparent(Alpha).ToFColor(true);
}
