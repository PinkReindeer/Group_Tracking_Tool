#include "IconsFontAwesome6.h"

#include "Layer.h"

#include "Application.h"

static std::unique_ptr<TrackingTool::Layer> s_DeferredLayerCleanup;

void TrackingTool::Layer::ClearDeferredCleanup()
{
    s_DeferredLayerCleanup.reset();
}

void TrackingTool::Layer::QueueTransition(std::unique_ptr<Layer> toLayer)
{
    auto& layerStack = Application::Get().m_LayerStack;

    for (auto& layer : layerStack)
    {
        if (layer.get() == this)
        {
            s_DeferredLayerCleanup = std::move(layer);

            layer = std::move(toLayer);
            return;
        }
    }
}