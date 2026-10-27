#include <gtest/gtest.h>
#include "Graphics/ItemRenderer.h"
#include "Graphics/Framebuffer.h"
#include "Graphics/Camera.h"
#include "Physics/Terrain.h"
#include "Physics/PhysicsTypes.h"

// Minimal dummy terrain that returns a flat height of 0
class DummyTerrain : public Physics::Terrain {
public:
    float getHeight(float x) const override { return 0.0f; }
    // Provide required methods used by ItemRenderer (if any). For now only getHeight is needed.
};

// Helper to create a simple camera with identity mapping
Graphics::Camera createTestCamera() {
    Graphics::Camera cam(200, 200);
    cam.reset({0.0f, 0.0f});
    return cam;
}

// Test that a fuel canister renders without crashing and writes some non‑zero pixels
TEST(ItemRendererTest, FuelCanRenders) {
    Graphics::Framebuffer fb(200, 200);
    fb.clear(0xFF000000); // clear to opaque black
    auto cam = createTestCamera();
    DummyTerrain terrain;
    Physics::WorldItem item;
    item.type = Physics::ItemType::FUEL_CANISTER;
    item.position = {0.0f, 0.0f};
    item.collected = false;
    float gameTime = 0.0f;

    Graphics::ItemRenderer::drawItem(fb, cam, terrain, item, gameTime);

    // Verify that at least one pixel inside the expected region is not black
    bool anyDrawn = false;
    for (int y = 0; y < fb.height(); ++y) {
        for (int x = 0; x < fb.width(); ++x) {
            if (fb.getPixelFast(x, y) != 0xFF000000) {
                anyDrawn = true;
                break;
            }
        }
        if (anyDrawn) break;
    }
    EXPECT_TRUE(anyDrawn);
}

// Test that a gold coin renders and uses the palette colours
TEST(ItemRendererTest, GoldCoinPalette) {
    Graphics::Framebuffer fb(200, 200);
    fb.clear(0xFF000000);
    auto cam = createTestCamera();
    DummyTerrain terrain;
    Physics::WorldItem item;
    item.type = Physics::ItemType::COIN_GOLD;
    item.position = {0.0f, 0.0f};
    item.collected = false;
    float gameTime = 0.0f;

    Graphics::ItemRenderer::drawItem(fb, cam, terrain, item, gameTime);

    // Verify that a pixel with the gold rim colour appears somewhere
    const uint32_t goldRim = Graphics::Palette::CoinGoldRim;
    bool foundRim = false;
    for (int y = 0; y < fb.height(); ++y) {
        for (int x = 0; x < fb.width(); ++x) {
            if (fb.getPixelFast(x, y) == goldRim) {
                foundRim = true;
                break;
            }
        }
        if (foundRim) break;
    }
    EXPECT_TRUE(foundRim);
}
