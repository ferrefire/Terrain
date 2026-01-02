#include "manager.hpp"
#include "pass.hpp"
#include "pipeline.hpp"
#include "mesh.hpp"
#include "renderer.hpp"
#include "point.hpp"
#include "matrix.hpp"
#include "buffer.hpp"
#include "descriptor.hpp"
#include "input.hpp"
#include "time.hpp"
#include "loader.hpp"
#include "command.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

const int computeCascade = 8;

struct UniformData
{
	mat4 view;
	mat4 projection;
	point4D viewPosition;
	point4D lightDirection;
	point4D resolution;
	point4D terrainOffset;
	point4D heightmapOffsets[computeCascade]{};
	point4D shadowmapOffsets[3]{};
	point4D glillmapOffsets{};
};

struct LuminanceData
{
	float currentLuminance = 1;
};

UniformData data{};
std::vector<mat4> models(1);
std::vector<Buffer> frameBuffers;
std::vector<Buffer> objectBuffers;

Pass pass;
Pass postPass;

Pipeline pipeline;
meshP16 planeMesh;
meshP16 planeLodMesh;
meshP16 planeLod2Mesh;

Pipeline postPipeline;
Descriptor postDescriptor;
Pipeline luminancePipeline;
Descriptor luminanceDescriptor;
meshP16 quadMesh;
std::vector<Image> luminanceImages;
Buffer luminanceBuffer;
Buffer luminanceVariablesBuffer;
LuminanceData luminanceData{};

Pipeline transmittancePipeline;
Pipeline scatteringPipeline;
Pipeline skyPipeline;
Pipeline aerialPipeline;
Descriptor atmosphereDescriptor;
Image transmittanceImage;
Image scatteringImage;
Image skyImage;
Image aerialImage;

Pipeline glillPipeline;
Descriptor glillDescriptor;
Image glillImage;

Pipeline terrainShadowPipeline;
Descriptor terrainShadowDescriptor;
std::vector<Image> terrainShadowImages;
std::vector<Buffer> terrainShadowBuffers;
std::vector<bool> terrainShadowQueue;

Pipeline computePipeline;
Descriptor computeDescriptor;

std::vector<Image> computeImages(computeCascade);
std::vector<Image> temporaryComputeImages(2);

std::vector<Buffer> computeBuffers;
std::vector<point4D> computeDatas;
std::vector<VkSemaphore> computeSemaphores;
std::vector<Command> computeCommands;
//std::vector<Command> computeCopyCommands;

Descriptor frameDescriptor;
Descriptor materialDescriptor;
Descriptor objectDescriptor;

Image rock_diff;
Image rock_norm;
Image rock_arm;
Image grass_diff;
Image grass_norm;
Image grass_arm;
Image dry_diff;
Image dry_norm;
Image dry_arm;

int terrainRadius = 2;
int terrainLength = 2 * terrainRadius + 1;
int terrainCount = terrainLength * terrainLength;

int terrainLodRadius = 8;
int terrainLodLength = 2 * terrainLodRadius + 1;
int terrainLodCount = terrainLodLength * terrainLodLength;

int heightmapResolution = 2048;
float heightmapBaseSize = 0.075;
int computeIterations = 2;
int totalComputeIterations = int(pow(4, computeIterations));

int terrainRes = 192;
int terrainLodRes = 16;
int terrainLod2Res = 8;
float terrainResetDis = 5000.0f / float(terrainLod2Res);

int currentLod = -1;

int shadowmapResolution = 256;

void BlitFrameBuffer(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
	uint32_t renderIndex = Renderer::GetRenderIndex();

	std::vector<VkImageMemoryBarrier> preBarriers(2);

	VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = luminanceImages[renderIndex].GetImage();
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_NONE;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	preBarriers[0] = barrier;
	preBarriers[1] = barrier;

	preBarriers[1].image = pass.GetColorImage(renderIndex)->GetImage();
	preBarriers[1].oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	preBarriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	preBarriers[1].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	preBarriers[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 2, preBarriers.data());

	VkImageBlit blit{};
	blit.srcOffsets[0] = { 0, 0, 0 };
	blit.srcOffsets[1] = { CI(Manager::GetWindow().GetConfig().extent.width), CI(Manager::GetWindow().GetConfig().extent.height), 1 };
	blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	//blit.srcSubresource.mipLevel = i - 1;
	blit.srcSubresource.baseArrayLayer = 0;
	blit.srcSubresource.layerCount = 1;
	blit.dstOffsets[0] = { 0, 0, 0 };
	blit.dstOffsets[1] = { 4, 4, 1 };
	blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	//blit.dstSubresource.mipLevel = i;
	blit.dstSubresource.baseArrayLayer = 0;
	blit.dstSubresource.layerCount = 1;

	vkCmdBlitImage(commandBuffer, pass.GetColorImage(renderIndex)->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, luminanceImages[renderIndex].GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

	std::vector<VkImageMemoryBarrier> barriers(2);

	barriers[0] = barrier;
	barriers[1] = barrier;

	barriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barriers[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barriers[1].image = pass.GetColorImage(renderIndex)->GetImage();
	barriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barriers[1].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	//vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 2, barriers.data());
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 2, barriers.data());
}

void ComputeLuminance(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
	static bool firstTimeLuminance = true;
	float dt = Time::deltaTime;
	if (firstTimeLuminance) {dt = -1.0;}

	//luminanceVariablesBuffer.Update(&dt, sizeof(dt));
	//luminanceDescriptor.Bind(Renderer::GetRenderIndex(), commandBuffer, luminancePipeline);
	//luminancePipeline.Bind(commandBuffer);
	//vkCmdDispatch(commandBuffer, 1, 1, 1);

	frameDescriptor.BindDynamic(0, commandBuffer, transmittancePipeline);

	if (firstTimeLuminance || Input::GetKey(GLFW_KEY_G).pressed)
	{
		data.glillmapOffsets.x() = data.terrainOffset.x() + (Manager::GetCamera().GetPosition().x() / 10000.0f);
		data.glillmapOffsets.z() = data.terrainOffset.z() + (Manager::GetCamera().GetPosition().z() / 10000.0f);

		glillDescriptor.Bind(0, commandBuffer, glillPipeline);
		glillPipeline.Bind(commandBuffer);
		vkCmdDispatch(commandBuffer, 1024 / 8, 1024 / 8, 1);
	}

	atmosphereDescriptor.Bind(0, commandBuffer, transmittancePipeline);

	if (firstTimeLuminance)
	{
		transmittancePipeline.Bind(commandBuffer);
		vkCmdDispatch(commandBuffer, 32, 16, 1);

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);

		scatteringPipeline.Bind(commandBuffer);
		vkCmdDispatch(commandBuffer, 32, 32, 1);

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);
	}

	skyPipeline.Bind(commandBuffer);
	vkCmdDispatch(commandBuffer, 12, 8, 1);

	aerialPipeline.Bind(commandBuffer);
	vkCmdDispatch(commandBuffer, 1, 64, 32);

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);

	if (firstTimeLuminance) {firstTimeLuminance = false;}
}

void ComputeTerrainShadow(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
	for (int i = 0; i < terrainShadowQueue.size(); i++)
	{
		if (!terrainShadowQueue[i]) {continue;}

		terrainShadowQueue[i] = false;
		frameDescriptor.BindDynamic(0, commandBuffer, terrainShadowPipeline);
		terrainShadowDescriptor.Bind(i, commandBuffer, terrainShadowPipeline);
		terrainShadowPipeline.Bind(commandBuffer);
		vkCmdDispatch(commandBuffer, shadowmapResolution / 8, shadowmapResolution / 8, 1);
	}
}

void SetTerrainShadowValues(int index)
{
	if (terrainShadowQueue[index]) {return;}

	terrainShadowQueue[index] = true;

	float shadowRange = 750.0 * pow(10.0, index);
	//float shadowRange = 750.0;
	//if (index == 1) {shadowRange = 5000.0;}

	float spacing = (1.0 / float(shadowmapResolution)) * shadowRange;

	data.shadowmapOffsets[index].y() = index;
	data.shadowmapOffsets[index].x() = data.terrainOffset.x() + (Manager::GetCamera().GetPosition().x() / 10000.0f);
	data.shadowmapOffsets[index].z() = data.terrainOffset.z() + (Manager::GetCamera().GetPosition().z() / 10000.0f);
	data.shadowmapOffsets[index].x() = floor((data.shadowmapOffsets[index].x() * 10000.0) / spacing) * spacing * 0.0001;
	data.shadowmapOffsets[index].z() = floor((data.shadowmapOffsets[index].z() * 10000.0) / spacing) * spacing * 0.0001;
	data.shadowmapOffsets[index].w() = shadowRange;

	terrainShadowBuffers[index].Update(&data.shadowmapOffsets[index], sizeof(point4D));
}

void RenderPost(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
	vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

	frameDescriptor.BindDynamic(0, commandBuffer, postPipeline);
	postDescriptor.Bind(Renderer::GetRenderIndex(), commandBuffer, postPipeline);

	postPipeline.Bind(commandBuffer);

	quadMesh.Bind(commandBuffer);
	vkCmdDrawIndexed(commandBuffer, quadMesh.GetIndices().size(), 1, 0, 0, 0);
}

void Render(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
	frameDescriptor.BindDynamic(0, commandBuffer, pipeline);
	materialDescriptor.Bind(0, commandBuffer, pipeline);

	pipeline.Bind(commandBuffer);

	planeMesh.Bind(commandBuffer);
	objectDescriptor.BindDynamic(0, commandBuffer, pipeline, 0 * sizeof(mat4));
	vkCmdDrawIndexed(commandBuffer, planeMesh.GetIndices().size(), 1, 0, 0, 0);

	planeLodMesh.Bind(commandBuffer);
	vkCmdDrawIndexed(commandBuffer, planeLodMesh.GetIndices().size(), terrainCount - 1, 0, 0, 1);

	planeLod2Mesh.Bind(commandBuffer);
	vkCmdDrawIndexed(commandBuffer, planeLod2Mesh.GetIndices().size(), terrainLodCount - terrainCount, 0, 0, terrainCount);

	RenderPost(commandBuffer, frameIndex);
}

void Resize()
{
	for (int i = 0; i < Manager::GetSwapchain().GetViews().size(); i++)
	{
		postDescriptor.Update(i, 0, *pass.GetColorImage(i));
		postDescriptor.Update(i, 1, *pass.GetDepthImage(i));
	}
}

void Start()
{
	PassConfig passConfig = Pass::DefaultConfig(true);
	passConfig.useSwapchain = false;
	passConfig.colorAttachments[0].description.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	//passConfig.colorAttachments[0].description.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	passConfig.colorAttachments[0].description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	pass.Create(passConfig);

	//PassConfig postPassConfig = Pass::DefaultConfig(false);
	//postPass.Create(postPassConfig);

	PassInfo passInfo{};
	passInfo.pass = &pass;
	passInfo.useWindowExtent = true;
	Renderer::AddPass(passInfo);

	//PassInfo postPassInfo{};
	//postPassInfo.pass = &postPass;
	//postPassInfo.useWindowExtent = true;
	//Renderer::AddPass(postPassInfo);

	ShapeSettings shapeSettings{};
	shapeSettings.resolution = terrainRes;
	shapeP16 planeShape(ShapeType::Plane, shapeSettings);
	planeMesh.Create(planeShape);

	shapeSettings.resolution = terrainLodRes;
	shapeP16 planeLodShape(ShapeType::Plane, shapeSettings);
	planeLodMesh.Create(planeLodShape);

	shapeSettings.resolution = terrainLod2Res;
	shapeP16 planeLod2Shape(ShapeType::Plane, shapeSettings);
	planeLod2Mesh.Create(planeLod2Shape);

	quadMesh.Create(ShapeType::Quad);

	ImageConfig sceneLuminanceConfig = Image::DefaultConfig();
	sceneLuminanceConfig.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	sceneLuminanceConfig.viewConfig.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	sceneLuminanceConfig.width = 4;
	sceneLuminanceConfig.height = 4;
	sceneLuminanceConfig.samplerConfig.minFilter = VK_FILTER_NEAREST;
	sceneLuminanceConfig.samplerConfig.magFilter = VK_FILTER_NEAREST;

	luminanceImages.resize(Manager::GetSwapchain().GetFrameCount());
	for (Image& sceneLuminanceImage : luminanceImages) {sceneLuminanceImage.Create(sceneLuminanceConfig);}

	ImageConfig transmittanceImageConfig = Image::DefaultStorageConfig();
	transmittanceImageConfig.width = 256;
	transmittanceImageConfig.height = 64;
	transmittanceImageConfig.samplerConfig.repeatMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	transmittanceImageConfig.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	transmittanceImageConfig.viewConfig.format = VK_FORMAT_R16G16B16A16_SFLOAT;

	transmittanceImage.Create(transmittanceImageConfig);

	ImageConfig scatteringImageConfig = Image::DefaultStorageConfig();
	scatteringImageConfig.width = 32;
	scatteringImageConfig.height = 32;
	scatteringImageConfig.samplerConfig.repeatMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	scatteringImageConfig.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	scatteringImageConfig.viewConfig.format = VK_FORMAT_R16G16B16A16_SFLOAT;

	scatteringImage.Create(scatteringImageConfig);

	ImageConfig skyImageConfig = Image::DefaultStorageConfig();
	skyImageConfig.width = 192;
	skyImageConfig.height = 128;
	skyImageConfig.samplerConfig.repeatMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	skyImageConfig.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	skyImageConfig.viewConfig.format = VK_FORMAT_R16G16B16A16_SFLOAT;

	skyImage.Create(skyImageConfig);

	ImageConfig aerialImageConfig = Image::DefaultStorageConfig();
	aerialImageConfig.width = 64;
	aerialImageConfig.height = 64;
	aerialImageConfig.depth = 32;
	aerialImageConfig.type = VK_IMAGE_TYPE_3D;
	aerialImageConfig.viewConfig.type = VK_IMAGE_VIEW_TYPE_3D;
	aerialImageConfig.samplerConfig.repeatMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	aerialImageConfig.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	aerialImageConfig.viewConfig.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	//aerialImageConfig.format = VK_FORMAT_R8G8B8A8_UNORM;
	//aerialImageConfig.viewConfig.format = VK_FORMAT_R8G8B8A8_UNORM;

	aerialImage.Create(aerialImageConfig);

	ImageConfig terrainShadowImageConfig = Image::DefaultStorageConfig();
	terrainShadowImageConfig.width = shadowmapResolution;
	terrainShadowImageConfig.height = shadowmapResolution;
	terrainShadowImageConfig.samplerConfig.repeatMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	terrainShadowImageConfig.samplerConfig.minFilter = VK_FILTER_LINEAR;
	terrainShadowImageConfig.samplerConfig.magFilter = VK_FILTER_LINEAR;
	//terrainShadowImageConfig.format = VK_FORMAT_R8_UNORM;
	//terrainShadowImageConfig.viewConfig.format = VK_FORMAT_R8_UNORM;
	//terrainShadowImageConfig.format = VK_FORMAT_R16G16_UNORM;
	//terrainShadowImageConfig.viewConfig.format = VK_FORMAT_R16G16_UNORM;
	terrainShadowImageConfig.format = VK_FORMAT_R8G8_UNORM;
	terrainShadowImageConfig.viewConfig.format = VK_FORMAT_R8G8_UNORM;

	terrainShadowImages.resize(3);
	for (Image& image : terrainShadowImages) {image.Create(terrainShadowImageConfig);}

	terrainShadowQueue.resize(3);

	ImageConfig glillImageConfig = Image::DefaultStorageConfig();
	glillImageConfig.width = 1024;
	glillImageConfig.height = 1024;
	glillImageConfig.samplerConfig.repeatMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	glillImageConfig.samplerConfig.minFilter = VK_FILTER_LINEAR;
	glillImageConfig.samplerConfig.magFilter = VK_FILTER_LINEAR;
	glillImageConfig.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	glillImageConfig.viewConfig.format = VK_FORMAT_R16G16B16A16_SFLOAT;

	glillImage.Create(glillImageConfig);

	ImageConfig imageStorageConfig = Image::DefaultStorageConfig();
	imageStorageConfig.width = heightmapResolution;
	imageStorageConfig.height = heightmapResolution;
	imageStorageConfig.samplerConfig.repeatMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	imageStorageConfig.format = VK_FORMAT_R16G16B16A16_UNORM;
	imageStorageConfig.viewConfig.format = VK_FORMAT_R16G16B16A16_UNORM;
	//imageStorageConfig.format = VK_FORMAT_R8G8B8A8_UNORM;
	//imageStorageConfig.viewConfig.format = VK_FORMAT_R8G8B8A8_UNORM;

	ImageConfig imageStorageTempConfig = imageStorageConfig;

	if (computeIterations > 0) imageStorageConfig.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageStorageTempConfig.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	
	computeImages[0].Create(imageStorageConfig);
	imageStorageConfig.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageStorageConfig.viewConfig.format = VK_FORMAT_R8G8B8A8_UNORM;
	for (size_t i = 1; i < computeImages.size(); i++) {computeImages[i].Create(imageStorageConfig);}

	temporaryComputeImages[0].Create(imageStorageTempConfig);
	imageStorageTempConfig.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageStorageTempConfig.viewConfig.format = VK_FORMAT_R8G8B8A8_UNORM;
	temporaryComputeImages[1].Create(imageStorageTempConfig);

	ImageConfig imageConfig = Image::DefaultConfig();
	imageConfig.createMipmaps = true;
	imageConfig.samplerConfig.anisotropyEnabled = VK_FALSE;
	imageConfig.samplerConfig.maxAnisotropy = 0;
	imageConfig.srgb = true;
	imageConfig.compressed = true;
	imageConfig.samplerConfig.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	imageConfig.format = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
	imageConfig.viewConfig.format = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
	ImageConfig imageNormalConfig = Image::DefaultNormalConfig();
	imageNormalConfig.createMipmaps = true;
	imageNormalConfig.samplerConfig.anisotropyEnabled = VK_TRUE;
	imageNormalConfig.samplerConfig.maxAnisotropy = 8;
	imageNormalConfig.compressed = true;
	imageNormalConfig.normal = true;
	imageNormalConfig.samplerConfig.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	//imageNormalConfig.format = VK_FORMAT_R8G8_UNORM;
	//imageNormalConfig.viewConfig.format = VK_FORMAT_R8G8_UNORM;
	imageNormalConfig.format = VK_FORMAT_BC5_UNORM_BLOCK;
	imageNormalConfig.viewConfig.format = VK_FORMAT_BC5_UNORM_BLOCK;
	ImageConfig imageArmConfig = Image::DefaultNormalConfig();
	imageArmConfig.createMipmaps = true;
	imageArmConfig.samplerConfig.anisotropyEnabled = VK_TRUE;
	imageArmConfig.samplerConfig.maxAnisotropy = 4;
	imageArmConfig.compressed = true;
	imageArmConfig.samplerConfig.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	imageArmConfig.format = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
	imageArmConfig.viewConfig.format = VK_FORMAT_BC1_RGB_UNORM_BLOCK;

	std::vector<ImageLoader*> loaders = ImageLoader::LoadImages({
		{"rock_diff", ImageType::Jpg},
		{"rock_norm", ImageType::Jpg},
		{"rock_arm", ImageType::Jpg},
		{"rocky_terrain_diff", ImageType::Jpg},
		{"rocky_terrain_norm", ImageType::Jpg},
		{"rocky_terrain_arm", ImageType::Jpg},
		{"grassy_rocks_diff", ImageType::Jpg},
		{"grassy_rocks_norm", ImageType::Jpg},
		{"grassy_rocks_arm", ImageType::Jpg},
	});

	double startTime = Time::GetCurrentTime();

	rock_diff.Create(*loaders[0], imageConfig);
	rock_norm.Create(*loaders[1], imageNormalConfig);
	rock_arm.Create(*loaders[2], imageArmConfig);
	imageNormalConfig.samplerConfig.maxAnisotropy = 2;
	imageArmConfig.samplerConfig.maxAnisotropy = 2;
	grass_diff.Create(*loaders[3], imageConfig);
	grass_norm.Create(*loaders[4], imageNormalConfig);
	grass_arm.Create(*loaders[5], imageArmConfig);
	dry_diff.Create(*loaders[6], imageConfig);
	dry_norm.Create(*loaders[7], imageNormalConfig);
	dry_arm.Create(*loaders[8], imageArmConfig);

	for (size_t i = 0; i < loaders.size(); i++) {delete (loaders[i]);}
	loaders.clear();

	std::cout << "Images created in: " << (Time::GetCurrentTime() - startTime) * 1000 << "ms." << std::endl;

	data.projection = Manager::GetCamera().GetProjection();
	//data.lightDirection = point4D(point3D(0.2, 0.25, -0.4).Unitized());
	//data.lightDirection = point4D(point3D(0.377384, 0.0347139, -0.925406));
	//data.lightDirection = point4D(point3D(0.529019, 0.282315, -0.800273));
	data.lightDirection = point4D(point3D(0.613087, 0.116438, 0.781388));
	data.resolution = point4D(Manager::GetCamera().GetConfig().width, Manager::GetCamera().GetConfig().height, 
		Manager::GetCamera().GetConfig().near, Manager::GetCamera().GetConfig().far);
	data.terrainOffset = point4D(0.0);
	//data.terrainOffset.w() = 15;
	data.terrainOffset.w() = 0;
	data.glillmapOffsets = point4D(0.0);

	for (size_t i = 0; i < computeCascade; i++)
	{
		data.heightmapOffsets[i].x() = 100;
		data.heightmapOffsets[i].y() = 100;
	}

	BufferConfig frameBufferConfig{};
	frameBufferConfig.mapped = true;
	frameBufferConfig.size = sizeof(UniformData);
	frameBuffers.resize(Renderer::GetFrameCount());
	for (Buffer& buffer : frameBuffers) { buffer.Create(frameBufferConfig); }

	BufferConfig objectBufferConfig{};
	objectBufferConfig.mapped = true;
	objectBufferConfig.size = sizeof(mat4) * models.size();
	objectBuffers.resize(Renderer::GetFrameCount());
	for (Buffer& buffer : objectBuffers) { buffer.Create(objectBufferConfig); }

	std::vector<DescriptorConfig> frameDescriptorConfigs(5);
	frameDescriptorConfigs[0].type = DescriptorType::UniformBuffer;
	frameDescriptorConfigs[0].stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | 
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
	frameDescriptorConfigs[1].type = DescriptorType::CombinedSampler;
	frameDescriptorConfigs[1].stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | 
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
	frameDescriptorConfigs[1].count = computeCascade;
	frameDescriptorConfigs[2].type = DescriptorType::CombinedSampler;
	frameDescriptorConfigs[2].stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | 
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
	frameDescriptorConfigs[2].count = terrainShadowImages.size();
	frameDescriptorConfigs[3].type = DescriptorType::CombinedSampler;
	frameDescriptorConfigs[3].stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | 
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
	frameDescriptorConfigs[4].type = DescriptorType::CombinedSampler;
	frameDescriptorConfigs[4].stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | 
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
	frameDescriptor.Create(0, frameDescriptorConfigs);

	std::vector<DescriptorConfig> materialDescriptorConfigs(3);
	materialDescriptorConfigs[0].type = DescriptorType::CombinedSampler;
	materialDescriptorConfigs[0].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	materialDescriptorConfigs[0].count = 3;
	materialDescriptorConfigs[1].type = DescriptorType::CombinedSampler;
	materialDescriptorConfigs[1].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	materialDescriptorConfigs[1].count = 3;
	materialDescriptorConfigs[2].type = DescriptorType::CombinedSampler;
	materialDescriptorConfigs[2].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	materialDescriptorConfigs[2].count = 3;
	materialDescriptor.Create(1, materialDescriptorConfigs);

	std::vector<DescriptorConfig> objectDescriptorConfigs(1);
	objectDescriptorConfigs[0].type = DescriptorType::DynamicUniformBuffer;
	objectDescriptorConfigs[0].stages = VK_SHADER_STAGE_VERTEX_BIT;
	objectDescriptor.Create(2, objectDescriptorConfigs);

	frameDescriptor.GetNewSetDynamic();
	frameDescriptor.UpdateDynamic(0, 0, Utilities::Pointerize(frameBuffers));
	for (int i = 0; i < Renderer::GetFrameCount(); i++)
	{
		frameDescriptor.Update(i, 1, Utilities::Pointerize(computeImages));
		frameDescriptor.Update(i, 2, Utilities::Pointerize(terrainShadowImages));
		frameDescriptor.Update(i, 3, glillImage);
		frameDescriptor.Update(i, 4, skyImage);
	}

	materialDescriptor.GetNewSet();
	materialDescriptor.Update(0, 0, {&rock_diff, &rock_norm, &rock_arm});
	materialDescriptor.Update(0, 1, {&grass_diff, &grass_norm, &grass_arm});
	materialDescriptor.Update(0, 2, {&dry_diff, &dry_norm, &dry_arm});

	objectDescriptor.GetNewSetDynamic();
	objectDescriptor.UpdateDynamic(0, 0, Utilities::Pointerize(objectBuffers), sizeof(mat4));

	BufferConfig computeBufferConfig{};
	computeBufferConfig.mapped = true;
	computeBufferConfig.size = sizeof(point4D);
	computeBuffers.resize(computeCascade);
	computeDatas.resize(computeCascade);
	for (int i = 0; i < computeCascade; i++)
	{
		//point4D cascadeSize = point4D(heightmapBaseSize * pow(2.0, i), 0.0, 0.0, 0.0);
		point4D cascadeSize = point4D(i, 0.0, 0.0, 0.0);
		computeDatas[i] = cascadeSize;
		computeBuffers[i].Create(computeBufferConfig, &cascadeSize);
	}

	std::vector<DescriptorConfig> computeDescriptorConfigs(3);
	computeDescriptorConfigs[0].type = DescriptorType::StorageImage;
	computeDescriptorConfigs[0].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeDescriptorConfigs[1].type = DescriptorType::StorageImage;
	computeDescriptorConfigs[1].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeDescriptorConfigs[2].type = DescriptorType::UniformBuffer;
	computeDescriptorConfigs[2].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeDescriptor.Create(1, computeDescriptorConfigs);

	for (int i = 0; i < computeCascade; i++)
	{
		computeDescriptor.GetNewSet();

		if (computeIterations == 0)
		{
			computeDescriptor.Update(i, 0, computeImages[i]);
		}
		else
		{
			computeDescriptor.Update(i, 0, temporaryComputeImages[0]);
			computeDescriptor.Update(i, 1, temporaryComputeImages[1]);
		}

		computeDescriptor.Update(i, 2, computeBuffers[i]);
	}

	BufferConfig luminanceBufferConfig = Buffer::StorageConfig();
	luminanceBufferConfig.size = sizeof(LuminanceData);
	luminanceBuffer.Create(luminanceBufferConfig);

	BufferConfig luminanceVariablesBufferConfig{};
	luminanceVariablesBufferConfig.mapped = true;
	luminanceVariablesBufferConfig.size = sizeof(float);
	float dt = -1.0;
	luminanceVariablesBuffer.Create(luminanceVariablesBufferConfig, &dt);

	std::vector<DescriptorConfig> luminanceDescriptorConfigs(3);
	luminanceDescriptorConfigs[0].type = DescriptorType::CombinedSampler;
	luminanceDescriptorConfigs[0].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	luminanceDescriptorConfigs[1].type = DescriptorType::StorageBuffer;
	luminanceDescriptorConfigs[1].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	luminanceDescriptorConfigs[2].type = DescriptorType::UniformBuffer;
	luminanceDescriptorConfigs[2].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	luminanceDescriptor.Create(0, luminanceDescriptorConfigs);

	for (int i = 0; i < Manager::GetSwapchain().GetFrameCount(); i++)
	{
		luminanceDescriptor.GetNewSet();
		luminanceDescriptor.Update(i, 0, luminanceImages[i]);
		luminanceDescriptor.Update(i, 1, luminanceBuffer);
		luminanceDescriptor.Update(i, 2, luminanceVariablesBuffer);
	}

	int j = 0;
	std::vector<DescriptorConfig> postDescriptorConfigs(7);
	//postDescriptorConfigs[j].type = DescriptorType::CombinedSampler;
	//postDescriptorConfigs[j++].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	postDescriptorConfigs[j].type = DescriptorType::InputAttatchment;
	postDescriptorConfigs[j++].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	postDescriptorConfigs[j].type = DescriptorType::InputAttatchment;
	postDescriptorConfigs[j++].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	postDescriptorConfigs[j].type = DescriptorType::CombinedSampler;
	postDescriptorConfigs[j++].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	postDescriptorConfigs[j].type = DescriptorType::CombinedSampler;
	postDescriptorConfigs[j++].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	postDescriptorConfigs[j].type = DescriptorType::CombinedSampler;
	postDescriptorConfigs[j++].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	postDescriptorConfigs[j].type = DescriptorType::CombinedSampler;
	postDescriptorConfigs[j++].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	postDescriptorConfigs[j].type = DescriptorType::StorageBuffer;
	postDescriptorConfigs[j++].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	postDescriptor.Create(1, postDescriptorConfigs);

	for (int i = 0; i < Manager::GetSwapchain().GetFrameCount(); i++)
	{
		int k = 0;

		postDescriptor.GetNewSet();
		//postDescriptor.Update(i, k++, *pass.GetImage(i));
		postDescriptor.Update(i, k++, *pass.GetColorImage(i));
		postDescriptor.Update(i, k++, *pass.GetDepthImage(i));
		postDescriptor.Update(i, k++, transmittanceImage);
		postDescriptor.Update(i, k++, scatteringImage);
		postDescriptor.Update(i, k++, skyImage);
		postDescriptor.Update(i, k++, aerialImage);
		postDescriptor.Update(i, k++, luminanceBuffer);
	}

	std::vector<DescriptorConfig> atmosphereDescriptorConfigs(4);
	atmosphereDescriptorConfigs[0].type = DescriptorType::StorageImage;
	atmosphereDescriptorConfigs[0].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	atmosphereDescriptorConfigs[1].type = DescriptorType::StorageImage;
	atmosphereDescriptorConfigs[1].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	atmosphereDescriptorConfigs[2].type = DescriptorType::StorageImage;
	atmosphereDescriptorConfigs[2].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	atmosphereDescriptorConfigs[3].type = DescriptorType::StorageImage;
	atmosphereDescriptorConfigs[3].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	atmosphereDescriptor.Create(1, atmosphereDescriptorConfigs);

	atmosphereDescriptor.GetNewSet();
	atmosphereDescriptor.Update(0, 0, transmittanceImage);
	atmosphereDescriptor.Update(0, 1, scatteringImage);
	atmosphereDescriptor.Update(0, 2, skyImage);
	atmosphereDescriptor.Update(0, 3, aerialImage);

	std::vector<DescriptorConfig> glillDescriptorConfigs(1);
	glillDescriptorConfigs[0].type = DescriptorType::StorageImage;
	glillDescriptorConfigs[0].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	glillDescriptor.Create(1, glillDescriptorConfigs);

	glillDescriptor.GetNewSet();
	glillDescriptor.Update(0, 0, glillImage);

	BufferConfig terrainShadowBufferConfig{};
	terrainShadowBufferConfig.mapped = true;
	terrainShadowBufferConfig.size = sizeof(point4D);
	terrainShadowBuffers.resize(computeCascade);
	for (int i = 0; i < computeCascade; i++)
	{
		point4D temp = point4D(0.0, 0.0, 0.0, 0.0);
		terrainShadowBuffers[i].Create(terrainShadowBufferConfig, &temp);
	}

	std::vector<DescriptorConfig> terrainShadowDescriptorConfigs(2);
	terrainShadowDescriptorConfigs[0].type = DescriptorType::StorageImage;
	terrainShadowDescriptorConfigs[0].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	terrainShadowDescriptorConfigs[1].type = DescriptorType::UniformBuffer;
	terrainShadowDescriptorConfigs[1].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	//terrainShadowDescriptorConfigs[0].count = terrainShadowImages.size();
	terrainShadowDescriptor.Create(1, terrainShadowDescriptorConfigs);

	for (int i = 0; i < terrainShadowImages.size(); i++)
	{
		terrainShadowDescriptor.GetNewSet();
		terrainShadowDescriptor.Update(i, 0, terrainShadowImages[i]);
		terrainShadowDescriptor.Update(i, 1, terrainShadowBuffers[i]);
	}

	PipelineConfig pipelineConfig = Pipeline::DefaultConfig();
	pipelineConfig.shader = "terrain";
	pipelineConfig.tesselation = true;
	pipelineConfig.vertexInfo = planeMesh.GetVertexInfo();
	pipelineConfig.renderpass = pass.GetRenderpass();
	pipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), materialDescriptor.GetLayout(), objectDescriptor.GetLayout() };
	pipelineConfig.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	pipelineConfig.dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
	//pipelineConfig.rasterization.cullMode = VK_CULL_MODE_FRONT_BIT;
	//pipelineConfig.rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pipeline.Create(pipelineConfig);

	PipelineConfig postPipelineConfig = Pipeline::DefaultConfig();
	postPipelineConfig.shader = "post";
	postPipelineConfig.vertexInfo = quadMesh.GetVertexInfo();
	//postPipelineConfig.renderpass = postPass.GetRenderpass();
	postPipelineConfig.renderpass = pass.GetRenderpass();
	postPipelineConfig.subpass = 1;
	postPipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), postDescriptor.GetLayout() };
	postPipelineConfig.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	postPipelineConfig.dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
	postPipelineConfig.depthStencil.depthWriteEnable = VK_FALSE;
	postPipelineConfig.depthStencil.depthTestEnable = VK_FALSE;
	//postPipelineConfig.rasterization.cullMode = VK_CULL_MODE_FRONT_BIT;
	//postPipelineConfig.rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	postPipeline.Create(postPipelineConfig);

	PipelineConfig luminancePipelineConfig{};
	luminancePipelineConfig.shader = "luminance";
	luminancePipelineConfig.type = PipelineType::Compute;
	luminancePipelineConfig.descriptorLayouts = { luminanceDescriptor.GetLayout() };
	luminancePipeline.Create(luminancePipelineConfig);

	PipelineConfig transmittancePipelineConfig{};
	transmittancePipelineConfig.shader = "transmittance";
	transmittancePipelineConfig.type = PipelineType::Compute;
	transmittancePipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), atmosphereDescriptor.GetLayout() };
	transmittancePipeline.Create(transmittancePipelineConfig);

	PipelineConfig scatteringPipelineConfig{};
	scatteringPipelineConfig.shader = "scattering";
	scatteringPipelineConfig.type = PipelineType::Compute;
	scatteringPipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), atmosphereDescriptor.GetLayout() };
	scatteringPipeline.Create(scatteringPipelineConfig);

	PipelineConfig skyPipelineConfig{};
	skyPipelineConfig.shader = "sky";
	skyPipelineConfig.type = PipelineType::Compute;
	skyPipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), atmosphereDescriptor.GetLayout() };
	skyPipeline.Create(skyPipelineConfig);

	PipelineConfig aerialPipelineConfig{};
	aerialPipelineConfig.shader = "aerial";
	aerialPipelineConfig.type = PipelineType::Compute;
	aerialPipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), atmosphereDescriptor.GetLayout() };
	aerialPipeline.Create(aerialPipelineConfig);

	PipelineConfig terrainShadowPipelineConfig{};
	terrainShadowPipelineConfig.shader = "terrainShadow";
	terrainShadowPipelineConfig.type = PipelineType::Compute;
	terrainShadowPipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), terrainShadowDescriptor.GetLayout() };
	terrainShadowPipeline.Create(terrainShadowPipelineConfig);

	PipelineConfig glillPipelineConfig{};
	glillPipelineConfig.shader = "glill";
	glillPipelineConfig.type = PipelineType::Compute;
	glillPipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), glillDescriptor.GetLayout() };
	glillPipeline.Create(glillPipelineConfig);

	PipelineConfig computePipelineConfig{};
	computePipelineConfig.shader = "heightmap";
	computePipelineConfig.type = PipelineType::Compute;
	computePipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), computeDescriptor.GetLayout() };
	computePipeline.Create(computePipelineConfig);

	//Manager::GetCamera().Move(point3D(-5000, 2500, 5000));
	Input::TriggerMouse();
	//Manager::GetCamera().Move(point3D(-1486.45, -1815.79, -3094.54));
	//Manager::GetCamera().Move(point3D(-931.948, -2051.53, -2499.46));
	//Manager::GetCamera().Move(point3D(-24.9147, -904.676, 5320.23));
	Manager::GetCamera().Move(point3D(3884.26, -1783.41, 11323.2));
	//Manager::GetCamera().Rotate(point3D(12.8998, -149.9, 0.0));
	//Manager::GetCamera().Rotate(point3D(26.0998, -151.9, 0.0));
	//Manager::GetCamera().Rotate(point3D(4.89981, 306.799, 0.0));
	Manager::GetCamera().Rotate(point3D(-3.40032, 292.801, 0.0));
	//Manager::GetCamera().Move(point3D(7523.26, 643.268, 518.602));
	//Manager::GetCamera().Move(point3D(0, 10, 0));

	//data.shadowmapOffsets[0] = point4D(100000);
	//data.shadowmapOffsets[1] = point4D(100000);
	//data.shadowmapOffsets[2] = point4D(100000);

	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	computeSemaphores.resize(Renderer::GetFrameCount());
	for (int i = 0; i < Renderer::GetFrameCount(); i++)
	{
		if (vkCreateSemaphore(Manager::GetDevice().GetLogicalDevice(), &semaphoreCreateInfo, nullptr, &computeSemaphores[i]) != VK_SUCCESS)
			throw (std::runtime_error("Failed to create render semaphore"));
	}

	computeCommands.resize(Renderer::GetFrameCount());
	for (int i = 0; i < Renderer::GetFrameCount(); i++)
	{
		CommandConfig commandConfig{};
		commandConfig.queueIndex = Manager::GetDevice().GetQueueIndex(QueueType::Graphics);
		commandConfig.wait = false;
		commandConfig.signalSemaphores = {computeSemaphores[i]};
		computeCommands[i].Create(commandConfig, i, &Manager::GetDevice());
	}

	Manager::RegisterResizeCall(Resize);

	Renderer::RegisterCall(0, Render);
	//Renderer::RegisterCall(1, RenderPost);
	Renderer::RegisterCall(0, BlitFrameBuffer, true);
	Renderer::RegisterCall(0, ComputeLuminance, true);
	Renderer::RegisterCall(0, ComputeTerrainShadow, true);
}

void Compute(int lod)
{
	Renderer::AddFrameSemaphore(computeSemaphores[Renderer::GetCurrentFrame()], VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);

	computeBuffers[lod].Update(&computeDatas[lod], sizeof(point4D));

	computeCommands[Renderer::GetCurrentFrame()].Begin();

	frameDescriptor.BindDynamic(0, computeCommands[Renderer::GetCurrentFrame()].GetBuffer(), computePipeline);

	computeDescriptor.Bind(lod, computeCommands[Renderer::GetCurrentFrame()].GetBuffer(), computePipeline);
	computePipeline.Bind(computeCommands[Renderer::GetCurrentFrame()].GetBuffer());
	vkCmdDispatch(computeCommands[Renderer::GetCurrentFrame()].GetBuffer(), (heightmapResolution / int(pow(2, computeIterations))) / 8, 
		(heightmapResolution / int(pow(2, computeIterations))) / 8, 1);

	if (computeIterations > 0 && computeDatas[lod].w() == (totalComputeIterations - 1)) 
	{
		vkCmdPipelineBarrier(computeCommands[Renderer::GetCurrentFrame()].GetBuffer(), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);
		if (lod == 0)
		{
			temporaryComputeImages[0].CopyTo(computeImages[lod], computeCommands[Renderer::GetCurrentFrame()], false);
		}
		else
		{
			temporaryComputeImages[1].CopyTo(computeImages[lod], computeCommands[Renderer::GetCurrentFrame()], false);
		}
	}

	computeCommands[Renderer::GetCurrentFrame()].End();
	computeCommands[Renderer::GetCurrentFrame()].Submit();
}

void Frame()
{
	static double frameTime = 0;
	static double frameTimeCount = 0;

	if (Input::GetKey(GLFW_KEY_M).pressed)
	{
		Input::TriggerMouse();
	}

	if (Input::GetKey(GLFW_KEY_C).pressed)
	{
		//std::cout << Manager::GetCamera().GetPosition() + data.terrainOffset * 10000.0 << std::endl;
		//std::cout << Manager::GetCamera().GetPosition().y() << std::endl;
		//CameraConfig cameraConfig = Manager::GetCamera().GetConfig();
		//cameraConfig.speed = 2.2f;
		//cameraConfig.speed = 4000.0f;
		//Manager::GetCamera().SetConfig(cameraConfig);

		std::cout << "Camera position: " << Manager::GetCamera().GetPosition() + data.terrainOffset * 10000.0 << std::endl;
		std::cout << "Camera Rotation: " << Manager::GetCamera().GetAngles() << std::endl;
		std::cout << "Light direction: " << data.lightDirection << std::endl;
	}

	point2D angles = point3D(data.lightDirection).Angles() * -57.2957795;
	if (Input::GetKey(GLFW_KEY_RIGHT).down) {angles.y() += Time::deltaTime * -45.0f;}
	else if (Input::GetKey(GLFW_KEY_LEFT).down) {angles.y() += Time::deltaTime * 45.0f;}
	else if (Input::GetKey(GLFW_KEY_UP).down) {angles.x() += Time::deltaTime * -45.0f;}
	else if (Input::GetKey(GLFW_KEY_DOWN).down) {angles.x() += Time::deltaTime * 45.0f;}
	else {data.lightDirection.w() = 1;}

	if (data.lightDirection.w() == 0)
	{
		data.lightDirection = point3D::Rotation(angles);
		SetTerrainShadowValues(2);
		SetTerrainShadowValues(1);
		SetTerrainShadowValues(0);
	}
	data.lightDirection.w() = 0;

	if (Input::GetKey(GLFW_KEY_EQUAL).pressed) data.terrainOffset.w() += 1;
	if (Input::GetKey(GLFW_KEY_MINUS).pressed) data.terrainOffset.w() -= 1;
	data.terrainOffset.w() = std::clamp(data.terrainOffset.w(), 0.0f, 2.0f);

	if (fabs(Manager::GetCamera().GetPosition().x()) > terrainResetDis)
	{
		float camOffset = ((int(Manager::GetCamera().GetPosition().x()) / int(terrainResetDis)) * terrainResetDis);
		data.terrainOffset.x() += camOffset / 10000.0f;
		Manager::GetCamera().Move(point3D(-camOffset, 0, 0));
	}

	if (fabs(Manager::GetCamera().GetPosition().y()) > terrainResetDis)
	{
		float camOffset = ((int(Manager::GetCamera().GetPosition().y()) / int(terrainResetDis)) * terrainResetDis);
		data.terrainOffset.y() += camOffset / 10000.0f;
		Manager::GetCamera().Move(point3D(0, -camOffset, 0));
	}

	if (fabs(Manager::GetCamera().GetPosition().z()) > terrainResetDis)
	{
		float camOffset = ((int(Manager::GetCamera().GetPosition().z()) / int(terrainResetDis)) * terrainResetDis);
		data.terrainOffset.z() += camOffset / 10000.0f;
		Manager::GetCamera().Move(point3D(0, 0, -camOffset));
	}

	if (Time::newTick)
	{
		double fps = frameTime / frameTimeCount;
		glfwSetWindowTitle(Manager::GetWindow().GetData(), std::to_string(int(1.0 / fps)).c_str());

		frameTime = 0;
		frameTimeCount = 0;
	}
	frameTime += Time::deltaTime;
	frameTimeCount += 1;

	Manager::GetCamera().UpdateView();

	data.view = Manager::GetCamera().GetView();
	data.projection = Manager::GetCamera().GetProjection();
	data.viewPosition = Manager::GetCamera().GetPosition();

	data.resolution.x() = Manager::GetCamera().GetConfig().width;
	data.resolution.y() = Manager::GetCamera().GetConfig().height;

	if (Input::GetKey(GLFW_KEY_E).pressed)
	{
		data.resolution.z() = 1.0 - data.resolution.z();
	}

	if (computeIterations == 0) currentLod = -1;

	if (currentLod >= 0)
	{
		computeDatas[currentLod].w() += 1;
		if (computeDatas[currentLod].w() >= totalComputeIterations)
		{
			computeDatas[currentLod].w() = 0;
			currentLod = -1;
		}
	}

	if (currentLod == -1)
	{
		for (int i = computeCascade - 1; i >= 0; i--)
		{
			if (fabs(data.terrainOffset.x() + (Manager::GetCamera().GetPosition().x() / 10000.0f) - data.heightmapOffsets[i].x()) > (heightmapBaseSize * pow(2.0, i)) * 0.125 || 
				fabs(data.terrainOffset.z() + (Manager::GetCamera().GetPosition().z() / 10000.0f) - data.heightmapOffsets[i].y()) > (heightmapBaseSize * pow(2.0, i)) * 0.125)
			{
				currentLod = i;
				computeDatas[currentLod].y() = data.terrainOffset.x() + (Manager::GetCamera().GetPosition().x() / 10000.0f);
				computeDatas[currentLod].z() = data.terrainOffset.z() + (Manager::GetCamera().GetPosition().z() / 10000.0f);
				break;
			}
		}
	}

	if (currentLod >= 0 && (computeIterations == 0 || computeDatas[currentLod].w() == (totalComputeIterations - 1)))
	{
		data.heightmapOffsets[currentLod].x() = computeDatas[currentLod].y();
		data.heightmapOffsets[currentLod].y() = computeDatas[currentLod].z();
		
		//if (currentLod == 5) {SetTerrainShadowValues(2);}
		//if (currentLod == 1) {SetTerrainShadowValues(1);}
		//if (currentLod == 0) {SetTerrainShadowValues(0);}
		//else if (currentLod == 3) {SetTerrainShadowValues(1);}
		//else if (currentLod == 6) {SetTerrainShadowValues(2);}
		//else if (currentLod <= 5) {CTSI = 1;}
		//else {CTSI = 2;}
	}

	for (int i = 2; i >= 0; i--)
	{
		if (fabs(data.terrainOffset.x() + (Manager::GetCamera().GetPosition().x() / 10000.0f) - data.shadowmapOffsets[i].x()) > data.shadowmapOffsets[i].w() * 0.0001 * 0.125 || 
			fabs(data.terrainOffset.z() + (Manager::GetCamera().GetPosition().z() / 10000.0f) - data.shadowmapOffsets[i].z()) > data.shadowmapOffsets[i].w() * 0.0001 * 0.125)
		{
			//currentLod = i;
			//computeDatas[currentLod].y() = data.terrainOffset.x() + (Manager::GetCamera().GetPosition().x() / 10000.0f);
			//computeDatas[currentLod].z() = data.terrainOffset.z() + (Manager::GetCamera().GetPosition().z() / 10000.0f);
			//break;

			SetTerrainShadowValues(i);
		}
	}

	frameBuffers[Renderer::GetCurrentFrame()].Update(&data, sizeof(data));

	objectBuffers[Renderer::GetCurrentFrame()].Update(models.data(), sizeof(mat4) * models.size());

	if (currentLod >= 0)
	{
		Compute(currentLod);
	}
}

void End()
{
	planeMesh.Destroy();
	planeLodMesh.Destroy();
	planeLod2Mesh.Destroy();
	quadMesh.Destroy();

	for (Buffer& buffer : frameBuffers) { buffer.Destroy(); }
	frameBuffers.clear();

	for (Buffer& buffer : objectBuffers) { buffer.Destroy(); }
	objectBuffers.clear();

	for (Buffer& buffer : computeBuffers) { buffer.Destroy(); }
	computeBuffers.clear();

	for (Buffer& buffer : terrainShadowBuffers) { buffer.Destroy(); }
	terrainShadowBuffers.clear();

	luminanceBuffer.Destroy();
	luminanceVariablesBuffer.Destroy();

	for (Image& image : computeImages) { image.Destroy(); }
	computeImages.clear();

	for (Image& image : temporaryComputeImages) { image.Destroy(); }
	temporaryComputeImages.clear();

	for (Image& sceneLuminanceImage : luminanceImages) {sceneLuminanceImage.Destroy();}
	luminanceImages.clear();
	//temporaryComputeImage.Destroy();

	transmittanceImage.Destroy();
	scatteringImage.Destroy();
	skyImage.Destroy();
	aerialImage.Destroy();

	glillImage.Destroy();

	for (Image& image : terrainShadowImages) { image.Destroy(); }
	terrainShadowImages.clear();

	for (VkSemaphore& semaphore : computeSemaphores) {vkDestroySemaphore(Manager::GetDevice().GetLogicalDevice(), semaphore, nullptr);}
	computeSemaphores.clear();

	for (Command& command : computeCommands) {command.Destroy();}
	computeCommands.clear();

	//for (Command& command : computeCopyCommands) {command.Destroy();}
	//computeCopyCommands.clear();

	rock_diff.Destroy();
	rock_norm.Destroy();
	rock_arm.Destroy();
	grass_diff.Destroy();
	grass_norm.Destroy();
	grass_arm.Destroy();
	dry_diff.Destroy();
	dry_norm.Destroy();
	dry_arm.Destroy();

	pass.Destroy();
	postPass.Destroy();
	frameDescriptor.Destroy();
	materialDescriptor.Destroy();
	objectDescriptor.Destroy();
	postDescriptor.Destroy();
	luminanceDescriptor.Destroy();
	atmosphereDescriptor.Destroy();
	terrainShadowDescriptor.Destroy();
	glillDescriptor.Destroy();
	computeDescriptor.Destroy();
	pipeline.Destroy();
	postPipeline.Destroy();
	luminancePipeline.Destroy();
	transmittancePipeline.Destroy();
	scatteringPipeline.Destroy();
	skyPipeline.Destroy();
	aerialPipeline.Destroy();
	terrainShadowPipeline.Destroy();
	glillPipeline.Destroy();
	//screenPipeline.Destroy();
	computePipeline.Destroy();
}

int main(int argc, char** argv)
{
	Manager::ParseArguments(argv, argc);
	Manager::GetConfig().deviceConfig.anisotropic = true;
	Manager::GetConfig().deviceConfig.tesselation = true;
	Manager::GetConfig().framesInFlight = 1;
	//Manager::GetConfig().uncapped = true;
	//Manager::GetConfig().wireframe = true;

	Manager::Create();

	CameraConfig cameraConfig = Manager::GetCamera().GetConfig();
	cameraConfig.near = 0.1;
	cameraConfig.far = 50000.0;
	//cameraConfig.fov = 75;
	Manager::GetCamera().SetConfig(cameraConfig);

	Manager::RegisterStartCall(Start);
	Manager::RegisterFrameCall(Frame);
	Manager::RegisterEndCall(End);

	Manager::Run();

	Manager::Destroy();

	exit(EXIT_SUCCESS);
}