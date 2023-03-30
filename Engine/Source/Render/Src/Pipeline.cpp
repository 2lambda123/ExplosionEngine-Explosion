//
// Created by johnk on 2023/3/11.
//

#include <Render/Pipeline.h>

#include <utility>

namespace Render {
    class PipelineLayoutCache {
    public:
        static PipelineLayoutCache& Get(RHI::Device& device);
        ~PipelineLayoutCache();

        void Invalidate();
        PipelineLayout* GetLayout(const PipelineLayoutDesc& desc);
        PipelineLayout* GetLayout(const ComputePipelineShaderSet& shaders);
        PipelineLayout* GetLayout(const RasterPipelineShaderSet& shaders);

    private:
        explicit PipelineLayoutCache(RHI::Device& inDevice);

        RHI::Device& device;
        std::unordered_map<size_t, std::unique_ptr<PipelineLayout>> pipelineLayouts;
    };

    PipelineLayoutCache& PipelineLayoutCache::Get(RHI::Device& device)
    {
        static std::unordered_map<RHI::Device*, std::unique_ptr<PipelineLayoutCache>> map;

        auto iter = map.find(&device);
        if (iter == map.end()) {
            map[&device] = std::unique_ptr<PipelineLayoutCache>(new PipelineLayoutCache(device));
        }
        return *map[&device];
    }

    PipelineLayoutCache::PipelineLayoutCache(RHI::Device& inDevice)
        : device(inDevice)
    {
    }

    PipelineLayoutCache::~PipelineLayoutCache() = default;

    void PipelineLayoutCache::Invalidate()
    {
        pipelineLayouts.clear();
    }

    PipelineLayout* PipelineLayoutCache::GetLayout(const PipelineLayoutDesc& desc)
    {
        auto hash = desc.Hash();
        auto iter = pipelineLayouts.find(hash);
        if (iter == pipelineLayouts.end()) {
            pipelineLayouts[hash] = std::unique_ptr<PipelineLayout>(new PipelineLayout(device, desc, hash));
        }
        return pipelineLayouts[hash].get();
    }

    PipelineLayout* PipelineLayoutCache::GetLayout(const ComputePipelineShaderSet& shaders)
    {
        // TODO
        return nullptr;
    }

    PipelineLayout* PipelineLayoutCache::GetLayout(const RasterPipelineShaderSet& shaders)
    {
        // TODO
        return nullptr;
    }
}

namespace Render {
    size_t PipelineLayoutDesc::Hash() const
    {
        auto computeBindGroupLayoutHash = [](const BindGroupLayoutDesc& desc) -> size_t {
            std::vector<size_t> values = {
                Common::HashUtils::CityHash(&desc.layoutIndex, sizeof(desc.layoutIndex)),
                Common::HashUtils::CityHash(desc.entries, desc.entryNum * sizeof(RHI::BindGroupLayoutEntry))
            };
            return Common::HashUtils::CityHash(values.data(), values.size() * sizeof(size_t));
        };

        std::vector<size_t> values;
        values.reserve(bindGroupLayoutNum + 1);
        for (auto i = 0; i < bindGroupLayoutNum; i++) {
            values.emplace_back(computeBindGroupLayoutHash(bindGroupLayoutDescs[i]));
        }
        values.emplace_back(Common::HashUtils::CityHash(pipelineConstantLayoutDescs, pipelineConstantNum * sizeof(PipelineConstantLayoutDesc)));
        return Common::HashUtils::CityHash(values.data(), values.size() * sizeof(size_t));
    }

    size_t ComputePipelineStateDesc::Hash() const
    {
        std::vector<size_t> values = {
            shaders.computeShader.Hash()
        };
        return Common::HashUtils::CityHash(values.data(), values.size() * sizeof(size_t));
    }

    size_t RasterPipelineStateDesc::Hash() const
    {
        auto computeVertexAttributeHash = [](const RHI::VertexAttribute& attribute) -> size_t {
            std::vector<size_t> values = {
                Common::HashUtils::CityHash(&attribute.format, sizeof(attribute.format)),
                Common::HashUtils::CityHash(&attribute.offset, sizeof(attribute.offset)),
                Common::HashUtils::CityHash(attribute.semanticName, sizeof(attribute.semanticName)),
                Common::HashUtils::CityHash(&attribute.semanticIndex, sizeof(attribute.semanticIndex))
            };
            return Common::HashUtils::CityHash(values.data(), values.size() * sizeof(size_t));
        };
        auto computeVertexBufferLayoutHash = [computeVertexAttributeHash](const RHI::VertexBufferLayout& bufferLayout) -> size_t {
            std::vector<size_t> values;
            values.reserve(bufferLayout.attributeNum + 2);
            values.emplace_back(Common::HashUtils::CityHash(&bufferLayout.stride, sizeof(bufferLayout.stride)));
            values.emplace_back(Common::HashUtils::CityHash(&bufferLayout.stepMode, sizeof(bufferLayout.stepMode)));
            for (auto i = 0; i < bufferLayout.attributeNum; i++) {
                values.emplace_back(computeVertexAttributeHash(bufferLayout.attributes[i]));
            }
            return Common::HashUtils::CityHash(values.data(), values.size() * sizeof(size_t));
        };
        auto computeVertexStateHash = [computeVertexBufferLayoutHash](const VertexState& state) -> size_t {
            std::vector<size_t> values;
            values.reserve(state.bufferLayoutNum);
            for (auto i = 0; i < state.bufferLayoutNum; i++) {
                values.emplace_back(computeVertexBufferLayoutHash(state.bufferLayouts[i]));
            }
            return Common::HashUtils::CityHash(values.data(), values.size() * sizeof(size_t));
        };
        auto computeFragmentStateHash = [](const FragmentState& state) -> size_t {
            return Common::HashUtils::CityHash(state.colorTargets, state.colorTargetNum * sizeof(RHI::ColorTargetState));
        };

        std::vector<size_t> values = {
            shaders.vertexShader.Hash(),
            shaders.pixelShader.Hash(),
            shaders.geometryShader.Hash(),
            shaders.domainShader.Hash(),
            shaders.hullShader.Hash(),
            computeVertexStateHash(vertexState),
            Common::HashUtils::CityHash(&primitiveState, sizeof(PrimitiveState)),
            Common::HashUtils::CityHash(&depthStencilState, sizeof(DepthStencilState)),
            Common::HashUtils::CityHash(&multiSampleState, sizeof(MultiSampleState)),
            computeFragmentStateHash(fragmentState)
        };
        return Common::HashUtils::CityHash(values.data(), values.size() * sizeof(size_t));
    }

    Sampler::Sampler(RHI::Device& inDevice, const SamplerDesc& inDesc)
    {
        rhiHandle = inDevice.CreateSampler(inDesc);
    }

    Sampler::~Sampler() = default;

    RHI::Sampler* Sampler::GetRHI() const
    {
        return rhiHandle.Get();
    }

    BindGroupLayout::BindGroupLayout(BindGroupLayout::BindingMap inBindings)
        : bindings(std::move(inBindings))
    {
        // TODO
    }

    BindGroupLayout::~BindGroupLayout() = default;

    const RHI::ResourceBinding* BindGroupLayout::GetBinding(const std::string& name, RHI::ShaderStageBits shaderStage) const
    {
        auto iter = bindings.find(name);
        if (iter == bindings.end()) {
            return nullptr;
        }
        const auto& bindingPair = iter->second;
        return bindingPair.first & shaderStage ? &bindingPair.second : nullptr;
    }

    RHI::BindGroupLayout* BindGroupLayout::GetRHI() const
    {
        return rhiHandle.Get();
    }

    PipelineLayout::PipelineLayout(RHI::Device& inDevice, const PipelineLayoutDesc& inDesc, size_t inHash)
        : hash(inHash)
    {
        // TODO filter reflection data by index
        // TODO create bind group layout and pipeline layout
    }

    PipelineLayout::~PipelineLayout() = default;

    BindGroupLayout* PipelineLayout::GetBindGroupLayout(uint8_t layoutIndex) const
    {
        auto iter = bindGroupLayouts.find(layoutIndex);
        return iter == bindGroupLayouts.end() ? nullptr : iter->second.get();
    }

    RHI::PipelineLayout* PipelineLayout::GetRHI() const
    {
        return rhiHandle.Get();
    }

    size_t PipelineLayout::GetHash() const
    {
        return hash;
    }

    ComputePipelineState::ComputePipelineState(RHI::Device& inDevice, const ComputePipelineStateDesc& inDesc, size_t inHash)
        : hash(inHash)
    {
        pipelineLayout = PipelineLayoutCache::Get(inDevice).GetLayout(inDesc.shaders);

        RHI::ComputePipelineCreateInfo createInfo;
        createInfo.layout = pipelineLayout->GetRHI();
        createInfo.computeShader = inDesc.shaders.computeShader.rhiHandle;
        rhiHandle = inDevice.CreateComputePipeline(createInfo);
    }

    ComputePipelineState::~ComputePipelineState() = default;

    PipelineLayout* ComputePipelineState::GetPipelineLayout() const
    {
        return pipelineLayout;
    }

    RHI::ComputePipeline* ComputePipelineState::GetRHI() const
    {
        return rhiHandle.Get();
    }

    size_t ComputePipelineState::GetHash() const
    {
        return hash;
    }

    RasterPipelineState::RasterPipelineState(RHI::Device& inDevice, const RasterPipelineStateDesc& inDesc, size_t inHash)
        : hash(inHash)
    {
        pipelineLayout = PipelineLayoutCache::Get(inDevice).GetLayout(inDesc.shaders);

        RHI::GraphicsPipelineCreateInfo createInfo;
        createInfo.layout = pipelineLayout->GetRHI();
        createInfo.vertexShader = inDesc.shaders.vertexShader.rhiHandle;
        createInfo.pixelShader = inDesc.shaders.pixelShader.rhiHandle;
        createInfo.geometryShader = inDesc.shaders.geometryShader.rhiHandle;
        createInfo.domainShader = inDesc.shaders.domainShader.rhiHandle;
        createInfo.hullShader = inDesc.shaders.hullShader.rhiHandle;
        createInfo.vertexState = inDesc.vertexState;
        createInfo.primitiveState = inDesc.primitiveState;
        createInfo.depthStencilState = inDesc.depthStencilState;
        createInfo.multiSampleState = inDesc.multiSampleState;
        createInfo.fragmentState = inDesc.fragmentState;
        rhiHandle = inDevice.CreateGraphicsPipeline(createInfo);
    }

    RasterPipelineState::~RasterPipelineState() = default;

    PipelineLayout* RasterPipelineState::GetPipelineLayout() const
    {
        return pipelineLayout;
    }

    RHI::GraphicsPipeline* RasterPipelineState::GetRHI() const
    {
        return rhiHandle.Get();
    }

    size_t RasterPipelineState::GetHash() const
    {
        return hash;
    }

    SamplerCache& SamplerCache::Get(RHI::Device& device)
    {
        static std::unordered_map<RHI::Device*, std::unique_ptr<SamplerCache>> map;

        auto iter = map.find(&device);
        if (iter == map.end()) {
            map[&device] = std::unique_ptr<SamplerCache>(new SamplerCache(device));
        }
        return *map[&device];
    }

    SamplerCache::SamplerCache(RHI::Device& inDevice)
        : device(inDevice)
    {
    }

    SamplerCache::~SamplerCache() = default;

    Sampler* SamplerCache::FindOrCreateSampler(const SamplerDesc& desc)
    {
        size_t hash = Common::HashUtils::CityHash(&desc, sizeof(SamplerDesc));
        auto iter = samplers.find(hash);
        if (iter == samplers.end()) {
            samplers[hash] = std::unique_ptr<Sampler>(new Sampler(device, desc));
        }
        return samplers[hash].get();
    }

    PipelineCache& PipelineCache::Get(RHI::Device& device)
    {
        static std::unordered_map<RHI::Device*, std::unique_ptr<PipelineCache>> map;

        auto iter = map.find(&device);
        if (iter == map.end()) {
            map[&device] = std::unique_ptr<PipelineCache>(new PipelineCache(device));
        }
        return *map[&device];
    }

    PipelineCache::PipelineCache(RHI::Device& inDevice)
        : device(inDevice)
    {
    }

    PipelineCache::~PipelineCache() = default;

    void PipelineCache::Invalidate()
    {
        computePipelines.clear();
        rasterPipelines.clear();
    }

    ComputePipelineState* PipelineCache::GetPipeline(const ComputePipelineStateDesc& desc)
    {
        auto hash = desc.Hash();
        auto iter = computePipelines.find(hash);
        if (iter == computePipelines.end()) {
            computePipelines[hash] = std::unique_ptr<ComputePipelineState>(new ComputePipelineState(device, desc, hash));
        }
        return computePipelines[hash].get();
    }

    RasterPipelineState* PipelineCache::GetPipeline(const RasterPipelineStateDesc& desc)
    {
        auto hash = desc.Hash();
        auto iter = rasterPipelines.find(hash);
        if (iter == rasterPipelines.end()) {
            rasterPipelines[hash] = std::unique_ptr<RasterPipelineState>(new RasterPipelineState(device, desc, hash));
        }
        return rasterPipelines[hash].get();
    }
}