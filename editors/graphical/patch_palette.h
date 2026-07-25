#ifndef EDITORS_GRAPHICAL_PATCH_COLORS_H_
#define EDITORS_GRAPHICAL_PATCH_COLORS_H_

#include <QColor>

#include <array>
#include <vector>

namespace PatchPalette {

// Create colors as needed
void ensureCapacity(size_t count);

// Access colors
const std::vector<std::array<float, 4>>& getColors();

// Get color vector at specific index (used for rendering)
const std::array<float, 4>& getColorVec(size_t index);

// Get color at specific index
QColor getColor(size_t index);
}

#endif  // EDITORS_GRAPHICAL_PATCH_COLORS_H_
