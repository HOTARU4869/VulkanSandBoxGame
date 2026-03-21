// rhi/rhi_types.h
#pragma once
#include <cstdint>
#include <memory>

enum class RHIBackend { Vulkan, D3D12, Metal };

// š IRHITexture ‚Ì’è‹`‚Í‚±‚±‚©‚çíœ‚·‚é
// class IRHITexture { ... };  © íœ

class IRHIBuffer {
public:
    virtual ~IRHIBuffer() = default;
    uint64_t size = 0;
};

class IRHICommandList;

using RHIBufferRef = std::shared_ptr<IRHIBuffer>;
using RHICommandListRef = std::shared_ptr<IRHICommandList>;

// š RHITextureRef ‚Í‚±‚±‚Å‚Í‘O•ûéŒ¾‚¾‚¯
class IRHITexture;
using RHITextureRef = std::shared_ptr<IRHITexture>;

struct RHIBufferDesc {
    uint64_t size = 0;
    bool     isVertex = false;
    bool     isIndex = false;
    bool     isUniform = false;
    bool     hostVisible = true;
};