//
// Created by johnk on 2023/12/11.
//

#include <gtest/gtest.h>

#include <Rendering/ResourcePool.h>

using namespace Rendering;

struct ResourcePoolTest : public testing::Test {
    void SetUp() override
    {
        instance = RHI::Instance::GetByType(RHI::RHIType::dummy);

        RHI::QueueInfo queueInfo {};
        queueInfo.type = RHI::QueueType::graphics;
        queueInfo.num = 1;
        RHI::DeviceCreateInfo deviceCreateInfo {};
        deviceCreateInfo.queueCreateInfoNum = 1;
        deviceCreateInfo.queueCreateInfos = &queueInfo;
        device = instance->GetGpu(0)->RequestDevice(deviceCreateInfo);
    }

    void TearDown() override {}

    RHI::Instance* instance;
    Common::UniqueRef<RHI::Device> device;
};

TEST_F(ResourcePoolTest, BasicTest)
{
    auto& texturePool = TexturePool::Get(*device);
    PooledTextureDesc textureDesc {};
    textureDesc.dimension = RHI::TextureDimension::t2D;
    textureDesc.extent = Common::UVec3(1920, 1080, 0);
    textureDesc.format = RHI::PixelFormat::rgba8Unorm;
    textureDesc.usages = RHI::TextureUsageBits::renderAttachment | RHI::TextureUsageBits::storageBinding;
    textureDesc.mipLevels = 1;
    textureDesc.samples = 1;
    textureDesc.initialState = RHI::TextureState::undefined;
    PooledTextureRef t1 = texturePool.Allocate(textureDesc);

    textureDesc.extent = Common::UVec3(1024, 1024, 0);
    PooledTextureRef t2 = texturePool.Allocate(textureDesc);

    textureDesc.extent = Common::UVec3(1920, 1080, 0);
    PooledTextureRef t3 = texturePool.Allocate(textureDesc);
    ASSERT_NE(t1.Get(), t3.Get());

    auto* texturePtr = t1.Get();
    t1.Reset();
    PooledTextureRef t4 = texturePool.Allocate(textureDesc);
    ASSERT_EQ(texturePtr, t4.Get());
}
