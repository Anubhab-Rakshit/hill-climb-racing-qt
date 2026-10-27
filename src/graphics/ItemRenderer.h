#pragma once

#include "Framebuffer.h"
#include "Camera.h"
#include "Terrain.h"

namespace Graphics {

/**
 * @brief Utility class to render individual world items (fuel canisters, coins, etc.)
 *        using the software framebuffer. All colour constants are pulled from
 *        Palette.h so that visual tweaks are centralized.
 */
class ItemRenderer {
public:
    static void drawItem(Framebuffer &fb,
                        const Camera &cam,
                        const Physics::Terrain &terrain,
                        const Physics::WorldItem &item,
                        float gameTime);
};

} // namespace Graphics
