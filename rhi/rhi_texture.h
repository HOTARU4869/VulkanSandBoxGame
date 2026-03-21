// rhi/rhi_texture.h
#pragma once
#include <cstdint>
#include <memory>

struct RHITextureDesc {
    uint32_t width = 0;
    uint32_t height = 0;
};

class IRHITexture {
public:
    virtual ~IRHITexture() = default;
    uint32_t width = 0;
    uint32_t height = 0;
};

using RHITextureRef = std::shared_ptr<IRHITexture>;