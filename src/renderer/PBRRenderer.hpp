#pragma once

#include <memory>

#include "Window.hpp"
#include "GraphicsContext.hpp"

class PBRRenderer {
public:
    PBRRenderer(std::shared_ptr<Window> window);

    ~PBRRenderer();

private:
    void processEnvironmentAndIrradianceMaps();

    void processBRDF(std::shared_ptr<RenderPass> brdfRenderPass);

    std::shared_ptr<Window> window;

    std::unique_ptr<GraphicsContext> graphicsContext;

    std::shared_ptr<FrameBasedFence> renderFence;
    std::shared_ptr<FrameBasedSemaphore> renderSemaphore;
    std::shared_ptr<FrameBasedSemaphore> presentSemaphore;

    std::shared_ptr<FrameBasedCommandBuffer> mainCommandBuffer;

    std::shared_ptr<Texture> environmentMap;
    std::shared_ptr<Texture> irradianceMap;
    std::shared_ptr<Texture> prefilterMap;
};