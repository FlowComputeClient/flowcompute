#include "patch_palette.h"

#include <cmath>
#include <mutex>

namespace {
inline std::vector<std::array<float, 4>> patchColors = {{
    // 0: Strong Purple
    {0.3686274509803922, 0.20784313725490197, 0.6941176470588235, 1.0 },
    // { 0.529412f, 0.337255f, 0.572549f, 1.0f },
    // 1: Vivid Yellow (#F3C300)
    { 0.952941f, 0.764706f, 0.000000f, 1.0f },
    // 2: Very Light Blue (#A1CAF1)
    { 0.631373f, 0.792157f, 0.945098f, 1.0f },
    // 3: Vivid Orange (#F38400)
    { 0.952941f, 0.517647f, 0.000000f, 1.0f },
    // 4: Vivid Red (#BE0032)
    { 0.745098f, 0.000000f, 0.196078f, 1.0f },
    // 5: Vivid Green (#008856)
    { 0.000000f, 0.533333f, 0.337255f, 1.0f },
    // 6: Grayish Yellow (#C2B280)
    { 0.760784f, 0.698039f, 0.501961f, 1.0f },
    // 7: Strong Pink (#E68FAC)
    { 0.901961f, 0.560784f, 0.674510f, 1.0f },
    // 8: Strong Blue (#0067A5)
    { 0.000000f, 0.403922f, 0.647059f, 1.0f },
    // 9: Strong Yellowish Pink (#F99379)
    { 0.976471f, 0.576471f, 0.474510f, 1.0f },
    // 10: Strong Violet (#604E97)
    { 0.376471f, 0.305882f, 0.592157f, 1.0f },
    // 11: Vivid Orange Yellow (#F6A600)
    { 0.964706f, 0.650980f, 0.000000f, 1.0f },
    // 12: Strong Purplish Red (#B3446C)
    { 0.701961f, 0.266667f, 0.423529f, 1.0f },
    // 13: Vivid Greenish Yellow (#DCD300)
    { 0.862745f, 0.827451f, 0.000000f, 1.0f },
    // 14: Strong Reddish Brown (#882D17)
    { 0.533333f, 0.176471f, 0.090196f, 1.0f },
    // 15: Vivid Yellowish Green (#8DB600)
    { 0.552941f, 0.713725f, 0.000000f, 1.0f },
    // 16: Deep Yellowish Brown (#654522)
    { 0.396078f, 0.270588f, 0.133333f, 1.0f },
    // 17: Vivid Reddish Orange (#E25822)
    { 0.886275f, 0.345098f, 0.133333f, 1.0f },
    // 18: Dark Olive Green (#2B3D26)
    { 0.168627f, 0.239216f, 0.149020f, 1.0f },
    // 19: Medium Gray (#848482)
    { 0.517647f, 0.517647f, 0.509804f, 1.0f },
    // 20: Pure White (#FFFFFF)
    { 1.000000f, 1.000000f, 1.000000f, 1.0f },
    // 21: Off-Black (#222222) - Slightly lifted
    { 0.133333f, 0.133333f, 0.133333f, 1.0f }
}};

std::mutex paletteMutex;
}

namespace PatchPalette {

void ensureCapacity(size_t count) {
    std::lock_guard<std::mutex> lock(paletteMutex);

    if (count <= patchColors.size()) {
        return;
    }

    const double goldenRatioConjugate = 0.618033988749895;

    for (size_t i = patchColors.size(); i < count; ++i) {
        double hue        = std::fmod(0.1 + i * goldenRatioConjugate, 1.0);
        double saturation = 0.65 + 0.20 * ((i % 2 == 0) ? 1.0 : 0.0);
        double value      = 0.80 + 0.15 * ((i % 3 == 0) ? 1.0 : 0.0);

        // Push color onto vector
        QColor color = QColor::fromHsvF(hue, saturation, value);
        patchColors.push_back({
            static_cast<float>(color.redF()),
            static_cast<float>(color.greenF()),
            static_cast<float>(color.blueF()),
            static_cast<float>(color.alphaF())
        });
    }
}

const std::vector<std::array<float, 4>>& getColors() {
    return patchColors;
}

const std::array<float, 4>& getColorVec(size_t index) {
    ensureCapacity(index + 1);
    return patchColors[index];
}

QColor getColor(size_t index) {
    ensureCapacity(index + 1);
    const auto& c = patchColors[index];
    return QColor::fromRgbF(c[0], c[1], c[2], c[3]);
}
}