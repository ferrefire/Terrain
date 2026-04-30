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
#include "ui.hpp"
#include "tree.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

const int computeCascade = 8;
const int shadowCascades = 4;
const int heightmapResolution = 4096;

struct alignas(16) UniformData
{
	mat4 view;
	mat4 projection;
	mat4 shadowMatrices[shadowCascades];
	point4D viewPosition;
	point4D viewDirection;
	point4D lightDirection;
	point4D resolution;
	point4D terrainOffset;
	point4D heightmapOffsets[computeCascade]{};
	point4D shadowmapOffsets[3]{};
	point4D glillmapOffsets[3]{};
};

struct alignas(16) LuminanceData
{
	float currentLuminance = 1;
	uint32_t padding[3];
};

struct alignas(16) GlillData
{
	point4D offset{};
	point4D settings{};
	float interPower = 1.5;
	float globalSamplePower = 1.0;
	float treeDensityStrength = 1.0;
	float treeDensityPower = 1.0;
	//uint32_t padding[2];
};

struct alignas(16) AtmosphereData
{
	float miePhaseFunction = 0.8;
	float offsetRadius = 0.01;
	float rayleighScatteringStrength = 1.0;
	float mieScatteringStrength = 1.0;
	float mieExtinctionStrength = 1.0;
	float absorptionExtinctionStrength = 1.0;
	float mistStrength = 48.0;
	float skyStrength = 8.0;
	float rayleighScaleHeight = 8.0;
	float mieScaleHeight = 1.2;
	float cameraScale = 0.001;
	float absorptionDensityHeight = 25.0;
	float absorption1 = 1.0 / 15.0;
	float absorption2 = -2.0 / 3.0;
	float absorption3 = -1.0 / 15.0;
	float absorption4 = 8.0 / 3.0;
	float calculateInShadow = 0.0;
	float skyPower = 1.0;
	float defaultSkyPower = 500.0;
	float skyDilute = 1.0;
	point4D skyColor;
	//uint32_t padding[2];
};

struct alignas(16) TerrainComputeData
{
	float seed = 0.303586;
	//float erodeFactor = 1.0;
	//float erodeFactor = 4.0;
	//float erodeFactor = 2.0;
	//float erodeFactor = 3.0;
	float erodeFactor = 2.5;
	//float steepness = 2.0;
	//float steepness = 1.75;
	//float steepness = 1.5;
	//float steepness = 1.25;
	float steepness = 1.0;
	//float steepness = 1.0;
	//float steepness = 2.0;
	int32_t resolution = heightmapResolution;
	//uint32_t padding[1];
};

struct alignas(16) TerrainShaderData
{
	uint32_t debugMode = 0;
	float rockSteepness = 0.10;
	//float rockTransition = 0.075;
	float rockTransition = 0.025;
	//float snowHeight = 1500.0;
	float snowHeight = 1250.0;
	float snowBlend = 1500.0;
	float snowSteepness = 0.3;
	uint32_t snowEnabled = 1;
	//uint32_t padding[3];
};

struct alignas(16) AerialData
{
	float mistStrength = 10.0;
	float mistHeight = 0.01;
	float mistHeightPower = 1.0;
	float mistBuildupPower = 1.0;
	float sliceOffset = 0.5;
	float maximumHeight = 750.0;
	float decreaseHeight = 0.0;
	float decreasePower = 1.0;
	float blendDistance = 10.0;
	float defaultOcclusion = 0.5;
	float phaseStrength = 1.0;
	float sunStrength = 1.0;
	float sampleCountMult = 1.0;
	uint32_t lodOcclusion = 0.0;
	uint32_t blendOcclusion = 0.0;
	uint32_t useOcclusion = 1.0;
	uint32_t mistEnabled = 1;
	uint32_t shadowsEnabled = 1;
	uint32_t sunMist = 0;
	uint32_t padding[2];
};

struct alignas(16) ScatteringData
{
	float scatteringStrength = 1.0;
	float lightStrength = 1.0;
	float transmittanceStrength = 0.0;
	uint32_t padding[1];
};

struct alignas(16) SkyData
{
	float rayleighStrength = 1.0;
	uint32_t padding[3];
};

struct alignas(16) PostData
{
	uint32_t useLinearDepth = 0;
	uint32_t aerialBlendMode = 0;
	//uint32_t toneMapping = 1;
	uint32_t toneMapping = 0;
	float exposure = 1.0;
	//uint32_t padding[2];
};

struct alignas(16) RetrieveData
{
	float viewTerrainHeight;
	uint32_t padding[3];
};

struct alignas(16) TreeData
{
	point4D position{};
	point4D rotation{};
	point4D terrainValues{};
	point4D info{};
	//uint32_t padding[3];
};

struct alignas(16) TreeComputeConfig
{
	float treeSpacing = 35.0;
	//float treeSpacing = 30.0;
	//float treeOffset = 50.0;
	float treeOffset = 15.0;
	//float maxSteepness = 0.015;
	float maxSteepness = 0.0065;
	uint32_t overdrawCulling = 1;

	float noiseScale = 0.0013;
	int32_t noiseOctaves = 2;
	float noiseCutoff = 0.5;
	float noiseCutoffRandomness = 0.1;

	uint32_t occlusionCulling = 1;
	int32_t cullIterations = 10;
	uint32_t cullExponent = 2;
	float cullStartDistance = 100.0;

	int32_t radiuses[8] = {4, 10, 25, 63, 256};
	int32_t shadowRadiuses[8] = {6, 15, 38, 94, 256};
	int32_t squaredLengths[8] = {0, 0, 0, 0, 0};
	int32_t squaredShadowLengths[8] = {0, 0, 0, 0, 0};
	float squaredDistances[8] = {0, 0, 0, 0, 0};
	int32_t leafCounts[8] = {0, 0, 0, 0, 0};
	float cascadeTolerances[4] = {0.0, 0.0, 0.02, 0.0};

	//int32_t radiuses[4] = {4, 10, 25, 256};
	//int32_t shadowRadiuses[4] = {6, 15, 38, 256};
	//int32_t squaredLengths[4] = {0, 0, 0, 0};
	//int32_t squaredShadowLengths[4] = {0, 0, 0, 0};
	//float squaredDistances[4] = {0, 0, 0, 0};
	//int32_t leafCounts[4] = {0, 0, 0, 0};

	point4D treesCenter;

	int32_t overdrawCullMinimum = 15;
	int32_t overdrawCullMaximum = 100;
	//float overdrawCullHeightBase = 25.0;
	float overdrawCullHeightBase = 15.0;
	//float overdrawCullHeightOffset = 10.0;
	float overdrawCullHeightOffset = 0.0;
	uint32_t overdrawCullHeightOnly = 1;
	uint32_t overdrawLodCull = 1;
	int32_t overdrawLodCullMinimum = 10;
	int32_t overdrawLodCullMaximum = 100;
	uint32_t overdrawMisses = 1;
	uint32_t overdrawLodCullHeavy = 1;

	float cullHeight = 30.0;

	float minTreeScale = 0.8;
	//float minTreeScale = 1.0;
	//float maxTreeScale = 1.75;
	float maxTreeScale = 2.0;
	float treeScalePower = 1.0;

	int32_t originalFirstIndex = 0;
	int32_t originalIndexCount = 0;
	uint32_t highQualityLodLeaves = 0;
	uint32_t overdrawTreeForceGround = 0;

	float cascadeCullBias = 1.3;

	//uint32_t padding[2];
};

struct alignas(16) TreeShaderConfig
{
	int32_t weightPower = 72;
	//float uvScale = 0.25;
	float uvScale = 0.2;
	//float glillNormalMix = 0.45;
	float glillNormalMix = 0.0;
	float normalStrength = 1.0;
	
	int32_t textureLod = 2;
	float ambientStrength = 0.25;

	uint32_t sampleOcclusion = 0;
	float defaultOcclusion = 0.0;

	uint32_t debugMode = 0;
	
	//uint32_t padding[1];
};

struct alignas(16) ShadowData
{
	uint32_t enabled = 1;
	float blend0Dis = 0.0001;
	float blend1Dis = 0.01;
	uint32_t test = 0;

	//uint32_t padding[1];
};

struct alignas(16) LeafShaderConfig
{
	float localNormalBlend = 0.1;
	float worldNormalHeight = 0.0;
	//float worldNormalHeight = 15.0;
	//float worldNormalHeight = 8.0;
	float translucencyBlend = 0.0;
	float shadowTranslucencyDim = 0.99;
	//float translucencyBias = 0.5;
	float translucencyBias = 0.0;
	//float translucencyRange = 0.425;
	float translucencyRange = 0.25;

	//float lod0Size = 1.0;
	float lod0Size = 1.25;
	//float lod1Size = 1.75;
	float lod1Size = 2.0;
	float lod2Size = 5.0;
	//float lod3Size = 12.0;
	//float lod3Size = 16.0;
	//float lod3Size = 14.0;
	//float lod4Size = 24.0;
	float lod3Size = 12.5;
	float lod4Size = 20.0;

	uint32_t lodInterMod = 1;
	float lodInterPow = 2.0;

	uint32_t scaleWithTree = 1;
	uint32_t sampleOcclusion = 0;
	uint32_t flipLocalNormal = 1;
	float defaultOcclusion = 0.4;
	
	uint32_t debugMode = 0;
	float flatLocalNormalBlend = 0.0;
	//float qualityNormalBlendLodStart = 2.0;
	float qualityNormalBlendLodStart = 3.0;
	float qualityNormalBlendLodPower = 1.0;
	float qualitySmoothness = 0.625;
	//float qualityNormalBlendLodPower = 1.5;

	float colorMult = 1.0;

	//uint32_t padding[1];
};

UniformData data{};
std::vector<mat4> models(1);
std::vector<Buffer> frameBuffers;
std::vector<Buffer> objectBuffers;

Pass pass;
Pass postPass;

Pipeline pipeline;
Pipeline prePipeline;
meshP16 planeMesh;
meshP16 planeLodMesh;
meshP16 planeLod2Mesh;
TerrainShaderData terrainShaderData{};
Buffer terrainShaderBuffer;

Pipeline postPipeline;
Descriptor postDescriptor;
Pipeline luminancePipeline;
Descriptor luminanceDescriptor;
PostData postData{};
Buffer postBuffer;
meshP16 quadMesh;
std::vector<Image> luminanceImages;
Buffer luminanceBuffer;
Buffer luminanceVariablesBuffer;
LuminanceData luminanceData{};

ShadowData shadowData{};
Buffer shadowDataBuffer;
Pass shadowPass;
//Image shadowMap;

AtmosphereData atmosphereData{};
Buffer atmosphereBuffer;
Descriptor atmosphereDescriptor;

Pipeline transmittancePipeline;
Image transmittanceImage;

Pipeline scatteringPipeline;
Image scatteringImage;
ScatteringData scatteringData{};
Buffer scatteringBuffer;
Descriptor scatteringDescriptor;

Pipeline skyPipeline;
Image skyImage;
SkyData skyData{};
Buffer skyBuffer;
Descriptor skyDescriptor;

Pipeline aerialPipeline;
Image aerialImage;
AerialData aerialData{};
Buffer aerialBuffer;
Descriptor aerialDescriptor;

Pipeline glillPipeline;
Descriptor glillDescriptor;
std::vector<Image> glillImages;
std::vector<GlillData> glillDatas;
std::vector<Buffer> glillBuffers;
std::vector<bool> glillQueue;

Pipeline terrainShadowPipeline;
Descriptor terrainShadowDescriptor;
std::vector<Image> terrainShadowImages;
std::vector<Buffer> terrainShadowBuffers;
std::vector<bool> terrainShadowQueue;

Pipeline computePipeline;
Descriptor computeDescriptor;
TerrainComputeData terrainComputeData{};
Buffer terrainComputeBuffer;

TreeComputeConfig treeComputeConfig{};
Pipeline treeSetupComputePipeline;
Pipeline treeRenderComputePipeline;
Descriptor treeComputeDescriptor;
Buffer treeSetupDataBuffer;
Buffer treeRenderDataBuffer;
Buffer treeShadowRenderDataBuffer;
Buffer treeDrawBuffer;
Buffer treeShadowDrawBuffer;
Buffer treeComputeConfigBuffer;

TreeShaderConfig treeShaderConfig{};
Buffer treeShaderConfigBuffer;
meshPN32 treeMesh;
Descriptor treeDescriptor;
Pipeline treePipeline;
Pipeline treeShadowPipeline;
Image bark_diff;
Image bark_norm;
Image bark_arm;

LeafShaderConfig leafShaderConfig{};
meshPN16 leafMesh;
Descriptor leafDescriptor;
Pipeline leafPipeline;
Pipeline leafShadowPipeline;
Buffer leafDrawBuffer;
Buffer leafShadowDrawBuffer;
Buffer leafPositionsBuffer;
Buffer leafLodPositionsBuffer;
Buffer leafShaderConfigBuffer;

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

RetrieveData retrieveData{};
Pipeline retrievePipeline;
Buffer retrieveBuffer;
Descriptor retrieveDescriptor;

int terrainRadius = 2;
int terrainLength = 2 * terrainRadius + 1;
int terrainCount = terrainLength * terrainLength;

int terrainLodRadius = 8;
int terrainLodLength = 2 * terrainLodRadius + 1;
int terrainLodCount = terrainLodLength * terrainLodLength;

//int heightmapResolution = 4096;
float heightmapBaseSize = 0.075;
int computeIterations = 2;
int totalComputeIterations = int(pow(4, computeIterations));

int terrainRes = 192;
int terrainLodRes = 16;
int terrainLod2Res = 8;
float terrainResetDis = 5000.0f / float(terrainLod2Res);

int currentLod = -1;

int shadowmapResolution = 256;
int glillResolutions[3] = {512, 1024, 1024};

float globalGlillSamplePower = 1.0;
float globalTreeDensityStrength = 0.4;
float globalTreeDensityPower = 1.0;

Point<int, 3> aerialRes = Point<int, 3>(64, 64, 32);

int treeComputeBase = 512;
int treeCount = treeComputeBase * treeComputeBase;

uint32_t terrainEnabled = 1;

uint32_t treesEnabled = 1;
TreeConfig treeMeshConfig{};

uint32_t leavesEnabled = 1;

//uint32_t shadowsEnabled = 1;
float shadowDistance = 35.0;
float shadowDepthMultiplier = 16.0;

static bool allMapsComputed = false;

static bool shouldComputeTrees = true;

/*void BlitFrameBuffer(VkCommandBuffer commandBuffer, uint32_t frameIndex)
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
}*/

static bool recompileAtmosphere = true;

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

	retrieveDescriptor.Bind(0, commandBuffer, retrievePipeline);
	retrievePipeline.Bind(commandBuffer);
	vkCmdDispatch(commandBuffer, 1, 1, 1);

	atmosphereDescriptor.Bind(0, commandBuffer, transmittancePipeline);

	if (recompileAtmosphere)
	{
		recompileAtmosphere = false;

		transmittancePipeline.Bind(commandBuffer);
		vkCmdDispatch(commandBuffer, 32, 16, 1);

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);

		scatteringDescriptor.Bind(0, commandBuffer, scatteringPipeline);
		scatteringPipeline.Bind(commandBuffer);
		vkCmdDispatch(commandBuffer, 32, 32, 1);

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);
	}

	skyDescriptor.Bind(0, commandBuffer, skyPipeline);
	skyPipeline.Bind(commandBuffer);
	vkCmdDispatch(commandBuffer, 12, 8, 1);

	aerialDescriptor.Bind(0, commandBuffer, aerialPipeline);
	aerialPipeline.Bind(commandBuffer);
	vkCmdDispatch(commandBuffer, 1, aerialRes.y(), aerialRes.z());

	//vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);

	if (firstTimeLuminance) {firstTimeLuminance = false;}
}

void ComputeTerrainGlill(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
	for (int i = glillQueue.size() - 1; i >= 0; i--)
	{
		if (glillQueue[i])
		{
			glillQueue[i] = false;

			frameDescriptor.BindDynamic(0, commandBuffer, glillPipeline);
			glillDescriptor.Bind(i, commandBuffer, glillPipeline);
			glillPipeline.Bind(commandBuffer);
			vkCmdDispatch(commandBuffer, glillResolutions[i] / 8, glillResolutions[i] / 8, 1);

			//vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);
		}
	}
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

		VkImageMemoryBarrier2 barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		barrier.srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;

		barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;

		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.image = terrainShadowImages[i].GetImage();

		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkDependencyInfo dependency{};
		dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependency.imageMemoryBarrierCount = 1;
		dependency.pImageMemoryBarriers = &barrier;

		vkCmdPipelineBarrier2(commandBuffer, &dependency);

		//vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);
	}
}

void ComputeTrees(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
	if (treesEnabled && allMapsComputed)
	{
		frameDescriptor.BindDynamic(0, commandBuffer, treeRenderComputePipeline);
		treeComputeDescriptor.Bind(0, commandBuffer, treeRenderComputePipeline);
		//treeComputeDescriptor.BindDynamic(0, commandBuffer, treeComputePipeline);

		if (shouldComputeTrees)
		{
			shouldComputeTrees = false;

			treeSetupComputePipeline.Bind(commandBuffer);
			vkCmdDispatch(commandBuffer, treeComputeBase / 8, treeComputeBase / 8, 1);

			VkMemoryBarrier2 barriers[1] = {};

			// indirect command buffer visibility
			barriers[0].sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
			barriers[0].srcStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			barriers[0].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
			barriers[0].dstStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			barriers[0].dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;

			VkDependencyInfo depInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
			depInfo.memoryBarrierCount = 1;
			depInfo.pMemoryBarriers = barriers;

			vkCmdPipelineBarrier2(commandBuffer, &depInfo);
		}

		treeRenderComputePipeline.Bind(commandBuffer);
		vkCmdDispatch(commandBuffer, treeComputeBase / 8, treeComputeBase / 8, 1);

		//vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, nullptr, 0, nullptr, 0, nullptr);

		VkMemoryBarrier2 barriers[2] = {};

		// indirect command buffer visibility
		barriers[0].sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
		barriers[0].srcStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		barriers[0].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
		barriers[0].dstStageMask  = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
		barriers[0].dstAccessMask = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;

		// instance data visibility to vertex input/shader
		barriers[1].sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
		barriers[1].srcStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		barriers[1].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
		barriers[1].dstStageMask  = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
		barriers[1].dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;

		VkDependencyInfo depInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
		depInfo.memoryBarrierCount = 2;
		depInfo.pMemoryBarriers = barriers;

		vkCmdPipelineBarrier2(commandBuffer, &depInfo);
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

	//frameDescriptor.BindDynamic(0, commandBuffer, postPipeline);
	postDescriptor.BindDynamic(0, commandBuffer, postPipeline);

	postPipeline.Bind(commandBuffer);

	quadMesh.Bind(commandBuffer);
	vkCmdDrawIndexed(commandBuffer, quadMesh.GetIndices().size(), 1, 0, 0, 0);
}

void Render(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
	frameDescriptor.BindDynamic(0, commandBuffer, pipeline);

	//uint32_t treeRenderCount = (*reinterpret_cast<uint32_t*>(treeCountBuffer.GetAddress()));

	if (leavesEnabled)
	{
		leafDescriptor.Bind(0, commandBuffer, leafPipeline);
		leafPipeline.Bind(commandBuffer);
		leafMesh.Bind(commandBuffer);
		vkCmdDrawIndexedIndirect(commandBuffer, leafDrawBuffer.GetBuffer(), 0, 5, sizeof(VkDrawIndexedIndirectCommand));
	}

	if (treesEnabled)
	{
		treeDescriptor.Bind(0, commandBuffer, treePipeline);
		treePipeline.Bind(commandBuffer);
		treeMesh.Bind(commandBuffer);
		vkCmdDrawIndexedIndirect(commandBuffer, treeDrawBuffer.GetBuffer(), 0, 4, sizeof(VkDrawIndexedIndirectCommand));
	}

	if (terrainEnabled)
	{
		materialDescriptor.Bind(0, commandBuffer, pipeline);

		pipeline.Bind(commandBuffer);
	
		planeMesh.Bind(commandBuffer);
		objectDescriptor.BindDynamic(0, commandBuffer, pipeline, 0 * sizeof(mat4));
		vkCmdDrawIndexed(commandBuffer, planeMesh.GetIndices().size(), 1, 0, 0, 0);
	
		planeLodMesh.Bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, planeLodMesh.GetIndices().size(), terrainCount - 1, 0, 0, 1);
	
		planeLod2Mesh.Bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, planeLod2Mesh.GetIndices().size(), terrainLodCount - terrainCount, 0, 0, terrainCount);	
	}

	RenderPost(commandBuffer, frameIndex); // Remoce and add it as a renderer call!
}

void RenderPre(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
	frameDescriptor.BindDynamic(0, commandBuffer, prePipeline);
	materialDescriptor.Bind(0, commandBuffer, prePipeline);

	prePipeline.Bind(commandBuffer);

	planeMesh.Bind(commandBuffer);
	objectDescriptor.BindDynamic(0, commandBuffer, prePipeline, 0 * sizeof(mat4));
	vkCmdDrawIndexed(commandBuffer, planeMesh.GetIndices().size(), 1, 0, 0, 0);

	vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderShadows(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
	if (shadowData.enabled && leavesEnabled)
	{
		frameDescriptor.BindDynamic(0, commandBuffer, leafShadowPipeline);
		leafDescriptor.Bind(0, commandBuffer, leafShadowPipeline);
		leafShadowPipeline.Bind(commandBuffer);
		leafMesh.Bind(commandBuffer);
		uint32_t pc = 0;
		vkCmdPushConstants(commandBuffer, leafShadowPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
		vkCmdDrawIndexedIndirect(commandBuffer, leafShadowDrawBuffer.GetBuffer(), 0 * sizeof(VkDrawIndexedIndirectCommand), 1, sizeof(VkDrawIndexedIndirectCommand));
	}

	if (shadowData.enabled && treesEnabled)
	{
		frameDescriptor.BindDynamic(0, commandBuffer, treeShadowPipeline);
		treeDescriptor.Bind(0, commandBuffer, treeShadowPipeline);
		treeShadowPipeline.Bind(commandBuffer);
		treeMesh.Bind(commandBuffer);
		uint32_t pc = 0;
		vkCmdPushConstants(commandBuffer, treeShadowPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
		vkCmdDrawIndexedIndirect(commandBuffer, treeShadowDrawBuffer.GetBuffer(), 0 * sizeof(VkDrawIndexedIndirectCommand), 1, sizeof(VkDrawIndexedIndirectCommand));
	}
}

void RenderShadows2(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
	if (shadowData.enabled && leavesEnabled)
	{
		frameDescriptor.BindDynamic(0, commandBuffer, leafShadowPipeline);
		leafDescriptor.Bind(0, commandBuffer, leafShadowPipeline);
		leafShadowPipeline.Bind(commandBuffer);
		leafMesh.Bind(commandBuffer);
		uint32_t pc = 1;
		vkCmdPushConstants(commandBuffer, leafShadowPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
		vkCmdDrawIndexedIndirect(commandBuffer, leafShadowDrawBuffer.GetBuffer(), 1 * sizeof(VkDrawIndexedIndirectCommand), 1, sizeof(VkDrawIndexedIndirectCommand));
	}

	if (shadowData.enabled && treesEnabled)
	{
		frameDescriptor.BindDynamic(0, commandBuffer, treeShadowPipeline);
		treeDescriptor.Bind(0, commandBuffer, treeShadowPipeline);
		treeShadowPipeline.Bind(commandBuffer);
		treeMesh.Bind(commandBuffer);
		uint32_t pc = 1;
		vkCmdPushConstants(commandBuffer, treeShadowPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
		vkCmdDrawIndexedIndirect(commandBuffer, treeShadowDrawBuffer.GetBuffer(), 1 * sizeof(VkDrawIndexedIndirectCommand), 1, sizeof(VkDrawIndexedIndirectCommand));
	}
}

void RenderShadows3(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
	if (shadowData.enabled && leavesEnabled)
	{
		frameDescriptor.BindDynamic(0, commandBuffer, leafShadowPipeline);
		leafDescriptor.Bind(0, commandBuffer, leafShadowPipeline);
		leafShadowPipeline.Bind(commandBuffer);
		leafMesh.Bind(commandBuffer);
		uint32_t pc = 2;
		vkCmdPushConstants(commandBuffer, leafShadowPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
		vkCmdDrawIndexedIndirect(commandBuffer, leafShadowDrawBuffer.GetBuffer(), 2 * sizeof(VkDrawIndexedIndirectCommand), 1, sizeof(VkDrawIndexedIndirectCommand));
	}

	if (shadowData.enabled && treesEnabled)
	{
		frameDescriptor.BindDynamic(0, commandBuffer, treeShadowPipeline);
		treeDescriptor.Bind(0, commandBuffer, treeShadowPipeline);
		treeShadowPipeline.Bind(commandBuffer);
		treeMesh.Bind(commandBuffer);
		uint32_t pc = 2;
		vkCmdPushConstants(commandBuffer, treeShadowPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
		vkCmdDrawIndexedIndirect(commandBuffer, treeShadowDrawBuffer.GetBuffer(), 2 * sizeof(VkDrawIndexedIndirectCommand), 1, sizeof(VkDrawIndexedIndirectCommand));
	}
}

void RenderShadows4(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
	if (shadowData.enabled && leavesEnabled)
	{
		frameDescriptor.BindDynamic(0, commandBuffer, leafShadowPipeline);
		leafDescriptor.Bind(0, commandBuffer, leafShadowPipeline);
		leafShadowPipeline.Bind(commandBuffer);
		leafMesh.Bind(commandBuffer);
		uint32_t pc = 3;
		vkCmdPushConstants(commandBuffer, leafShadowPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
		vkCmdDrawIndexedIndirect(commandBuffer, leafShadowDrawBuffer.GetBuffer(), 3 * sizeof(VkDrawIndexedIndirectCommand), 1, sizeof(VkDrawIndexedIndirectCommand));
	}
}

void Resize()
{
	//for (int i = 0; i < Manager::GetSwapchain().GetViews().size(); i++)
	for (int i = 0; i < Renderer::GetFrameCount(); i++)
	{
		//postDescriptor.Update(i, 0, *pass.GetColorImage(i));
		//postDescriptor.Update(i, 1, *pass.GetDepthImage(i));

		postDescriptor.Update(i, 0, *pass.GetAttachmentImage(0, i));
		postDescriptor.Update(i, 1, *pass.GetAttachmentImage(2, i));
	}
}

void UpdateAtmosphereData()
{
	recompileAtmosphere = true;
	atmosphereBuffer.Update(&atmosphereData, sizeof(atmosphereData));
}

void UpdateAerialData()
{
	aerialBuffer.Update(&aerialData, sizeof(aerialData));
}

void UpdateScatteringData()
{
	recompileAtmosphere = true;
	scatteringBuffer.Update(&scatteringData, sizeof(scatteringData));
}

void UpdateSkyData()
{
	skyBuffer.Update(&skyData, sizeof(skyData));
}

void UpdateGlillData()
{
	for (int i = 0; i < glillDatas.size(); i++)
	{
		data.glillmapOffsets[i].x() = 10000000;
		data.glillmapOffsets[i].z() = 10000000;
	}
}

void UpdateTerrainComputeData()
{
	allMapsComputed = false;
	shouldComputeTrees = true;

	terrainComputeBuffer.Update(&terrainComputeData, sizeof(terrainComputeData));

	for (int i = 0; i < computeCascade; i++)
	{
		data.heightmapOffsets[i].x() = 10000000;
		data.heightmapOffsets[i].y() = 10000000;
	}

	for (int i = 0; i < terrainShadowQueue.size(); i++)
	{
		data.shadowmapOffsets[i].x() = 10000000;
		data.shadowmapOffsets[i].z() = 10000000;
	}

	UpdateGlillData();
}

void UpdateTerrainShaderData()
{
	terrainShaderBuffer.Update(&terrainShaderData, sizeof(terrainShaderData));
}

void UpdateLeafShaderData()
{
	leafShaderConfigBuffer.Update(&leafShaderConfig, sizeof(leafShaderConfig));
}

float cameraSensitivity = 0.1;

void UpdateCameraSensitivity()
{
	CameraConfig cameraConfig = Manager::GetCamera().GetConfig();
	cameraConfig.sensitivity = cameraSensitivity;
	Manager::GetCamera().SetConfig(cameraConfig);
}

void UpdatePostData()
{
	postBuffer.Update(&postData, sizeof(postData));
}

void RecaluclateTreeComputeConfig()
{
	for (int i = 0; i < 5; i++)
	{
		treeComputeConfig.squaredLengths[i] = (treeComputeConfig.radiuses[i] * 2 + 1) * (treeComputeConfig.radiuses[i] * 2 + 1);
		treeComputeConfig.squaredShadowLengths[i] = (treeComputeConfig.shadowRadiuses[i] * 2 + 1) * (treeComputeConfig.shadowRadiuses[i] * 2 + 1);
		treeComputeConfig.squaredDistances[i] = (treeComputeConfig.radiuses[i] * treeComputeConfig.treeSpacing) * (treeComputeConfig.radiuses[i] * treeComputeConfig.treeSpacing);

		//std::cout << "sl" << i << ": " << treeComputeConfig.squaredLengths[i] << std::endl;
		//std::cout << "ssl" << i << ": " << treeComputeConfig.squaredShadowLengths[i] << std::endl;
		//std::cout << "sd" << i << ": " << treeComputeConfig.squaredDistances[i] << std::endl << std::endl;
	}
}

void UpdateTreeComputeData()
{
	RecaluclateTreeComputeConfig();

	treeComputeConfigBuffer.Update(&treeComputeConfig, sizeof(TreeComputeConfig));
	shouldComputeTrees = true;
}

void UpdateTreeShaderData()
{
	treeShaderConfigBuffer.Update(&treeShaderConfig, sizeof(treeShaderConfig));
}

void UpdateShadowData()
{
	shadowDataBuffer.Update(&shadowData, sizeof(shadowData));
}

std::vector<VkDrawIndexedIndirectCommand> leafDrawCommands;
struct LeafLodPosition
{
	point4D position;
	point4D normal;
};
std::vector<LeafLodPosition> leafLodPositions;

void CreateLeafMesh(bool update)
{
	leafDrawCommands.resize(5);

	leafMesh.Destroy();

	ShapeSettings leafSettings{};
	leafSettings.lod = 0;
	shapePN16 leafShape(ShapeType::Leaf, leafSettings);

	leafDrawCommands[0].indexCount = leafShape.GetIndices().size();
	leafDrawCommands[0].instanceCount = 0;
	leafDrawCommands[0].firstIndex = 0;
	leafDrawCommands[0].vertexOffset = 0;
	leafDrawCommands[0].firstInstance = 0;

	leafDrawCommands[1].indexCount = leafShape.GetIndices().size();
	leafDrawCommands[1].firstIndex = 0;
	leafDrawCommands[1].instanceCount = 0;
	leafDrawCommands[1].vertexOffset = 0;
	leafDrawCommands[1].firstInstance = 0;

	//ShapeSettings leafSettings{};
	leafSettings.lod = 1;
	shapePN16 leafShape1(ShapeType::Leaf, leafSettings);

	leafLodPositions.resize(leafShape.GetVertices().size());

	leafDrawCommands[2].indexCount = leafShape1.GetIndices().size();
	leafDrawCommands[2].firstIndex = leafShape.GetIndices().size();
	leafDrawCommands[2].vertexOffset = 0;
	leafDrawCommands[2].instanceCount = 0;
	leafDrawCommands[2].firstInstance = 0;

	leafDrawCommands[3].indexCount = leafShape1.GetIndices().size();
	leafDrawCommands[3].firstIndex = leafShape.GetIndices().size();
	leafDrawCommands[3].vertexOffset = 0;
	leafDrawCommands[3].instanceCount = 0;
	leafDrawCommands[3].firstInstance = 0;

	leafShape.Join(leafShape1);
	leafShape.Rotate(180.0, Axis::z);
	leafShape.Scale(2.0);

	leafShape1.Rotate(180.0, Axis::z);
	leafShape1.Scale(2.0);

	leafShape.CalculateNormals();
	leafShape1.CalculateNormals();

	leafLodPositions[0].position = leafShape1.GetVertices()[0].position;
	leafLodPositions[0].normal = leafShape1.GetVertices()[0].normal;
	leafLodPositions[1].position = leafShape1.GetVertices()[1].position;
	leafLodPositions[1].normal = leafShape1.GetVertices()[1].normal;
	leafLodPositions[2].position = leafShape1.GetVertices()[2].position;
	leafLodPositions[2].normal = leafShape1.GetVertices()[2].normal;
	leafLodPositions[3].position = leafShape1.GetVertices()[2].position;
	leafLodPositions[3].normal = leafShape1.GetVertices()[2].normal;
	leafLodPositions[4].position = leafShape1.GetVertices()[3].position;
	leafLodPositions[4].normal = leafShape1.GetVertices()[3].normal;
	leafLodPositions[5].position = leafShape1.GetVertices()[4].position;
	leafLodPositions[5].normal = leafShape1.GetVertices()[4].normal;
	leafLodPositions[6].position = leafShape1.GetVertices()[4].position;
	leafLodPositions[6].normal = leafShape1.GetVertices()[4].normal;
	leafLodPositions[7].position = leafShape1.GetVertices()[5].position;
	leafLodPositions[7].normal = leafShape1.GetVertices()[5].normal;
	leafLodPositions[8].position = leafShape1.GetVertices()[6].position;
	leafLodPositions[8].normal = leafShape1.GetVertices()[6].normal;
	leafLodPositions[9].position = leafShape1.GetVertices()[6].position;
	leafLodPositions[9].normal = leafShape1.GetVertices()[6].normal;

	leafSettings.lod = 2;
	leafSettings.scalarized = false;
	shapePN16 leafShape2(ShapeType::Leaf, leafSettings);

	leafDrawCommands[4].indexCount = leafShape2.GetIndices().size();
	leafDrawCommands[4].firstIndex = leafShape.GetIndices().size();
	leafDrawCommands[4].vertexOffset = 0;
	leafDrawCommands[4].instanceCount = 0;
	leafDrawCommands[4].firstInstance = 0;

	leafShape.Join(leafShape2);
	leafShape.CalculateNormals();
	leafMesh.Create(leafShape);
}

void Start()
{
	aerialData.mistHeight = 0.075;
	aerialData.mistHeightPower = 0.35;
	aerialData.sliceOffset = 0.0;
	aerialData.lodOcclusion = 1;
	aerialData.defaultOcclusion = 0.75;
	aerialData.mistStrength = 8.0;
	aerialData.mistBuildupPower = 2.0;

	globalGlillSamplePower = 1.0;
	
	scatteringData.scatteringStrength = 1.0;
	
	atmosphereData.skyPower = 2.0;
	atmosphereData.skyDilute = 128.0;
	atmosphereData.skyStrength = 24.0;
	atmosphereData.mistStrength = 24.0;

	postData.aerialBlendMode = 2.0;

	skyData.rayleighStrength = 0.25;

	pass.AddAttachment(Pass::DefaultHDRAttachment());
	pass.AddAttachment(Pass::DefaultSwapAttachment());
	pass.AddAttachment(Pass::DefaultDepthAttachment(true));

	SubpassConfig subpass0{};
	subpass0.AddColorReference(0);
	subpass0.AddDepthReference(2);

	SubpassConfig subpass1{};
	subpass1.AddColorReference(1);
	subpass1.AddInputReference(0);
	subpass1.AddInputReference(2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

	VkSubpassDependency dependency = 
	{
    	.srcSubpass = 0,
    	.dstSubpass = 1,
    	.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
    	.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    	.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    	.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
		.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
	};
	subpass1.AddDependency(dependency);

	pass.AddSubpass(subpass0);
	pass.AddSubpass(subpass1);

	pass.Create();

	PassInfo passInfo{};
	passInfo.pass = &pass;
	passInfo.useWindowExtent = true;
	//Renderer::AddPass(passInfo);

	shadowPass.AddAttachment(Pass::DefaultShadowAttachment());

	SubpassConfig shadowSubpass0{};
	shadowSubpass0.AddDepthReference(0);

	VkSubpassDependency shadowDependency0 = 
	{
    	.srcSubpass = 0,
    	.dstSubpass = VK_SUBPASS_EXTERNAL,
    	.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
    	.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    	.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    	.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
	};

	shadowSubpass0.AddDependency(shadowDependency0);

	shadowPass.AddSubpass(shadowSubpass0);

	shadowPass.Create({2048, 2048}, shadowCascades);

	PassInfo shadowPass0Info{};
	shadowPass0Info.pass = &shadowPass;
	shadowPass0Info.useWindowExtent = false;
	shadowPass0Info.viewport.x = 0.0f;
	shadowPass0Info.viewport.y = 0.0f;
	shadowPass0Info.viewport.width = 2048;
	shadowPass0Info.viewport.height = 2048;
	shadowPass0Info.viewport.minDepth = 0.0f;
	shadowPass0Info.viewport.maxDepth = 1.0f;
	shadowPass0Info.scissor.offset = {0, 0};
	shadowPass0Info.scissor.extent.width = 2048;
	shadowPass0Info.scissor.extent.height = 2048;
	shadowPass0Info.renderIndex = 0;

	PassInfo shadowPass1Info = shadowPass0Info;
	shadowPass1Info.renderIndex = 1;

	PassInfo shadowPass2Info = shadowPass0Info;
	shadowPass2Info.renderIndex = 2;

	PassInfo shadowPass3Info = shadowPass0Info;
	shadowPass3Info.renderIndex = 3;

	Renderer::AddPass(shadowPass0Info);
	Renderer::AddPass(shadowPass1Info);
	Renderer::AddPass(shadowPass2Info);
	if (shadowCascades > 3) {Renderer::AddPass(shadowPass3Info);}

	Renderer::AddPass(passInfo);

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

	std::vector<VkDrawIndexedIndirectCommand> drawCommands;
	drawCommands.resize(5);

	//std::vector<VkDrawIndexedIndirectCommand> leafDrawCommands;
	//leafDrawCommands.resize(4);

	//std::vector<VkDrawIndexedIndirectCommand> leafShadowDrawCommands;
	//leafShadowDrawCommands.resize(4);

	//shapePN16 cubeShape0(ShapeType::Cube);
	//treeMeshConfig.seed = 6282;
	//treeMeshConfig.seed = 9206;
	treeMeshConfig.seed = 15;
	//treeMeshConfig.seed = 15628;
	//treeMeshConfig.seed = 1563;
	shapePN32 treeShape = Tree::GenerateTree(treeMeshConfig);
	treeShape.Scale(point3D(4.0));

	drawCommands[0].indexCount = treeShape.GetIndices().size();
	drawCommands[0].instanceCount = 0;
	drawCommands[0].firstIndex = 0;
	drawCommands[0].vertexOffset = 0;
	drawCommands[0].firstInstance = 0;

	
	//leafShape.Rotate(180.0, Axis::z);
	//leafShape.Scale(2.0);
	
	std::vector<LeafData> leafPositions = Tree::GetLeafPositions();
	treeComputeConfig.leafCounts[0] = leafPositions.size();
	treeComputeConfig.leafCounts[1] = treeComputeConfig.leafCounts[0] / 3;
	treeComputeConfig.leafCounts[2] = treeComputeConfig.leafCounts[1] / 6;
	treeComputeConfig.leafCounts[3] = treeComputeConfig.leafCounts[2] / 9;
	treeComputeConfig.leafCounts[4] = treeComputeConfig.leafCounts[3] / 3;
	for (LeafData &p : leafPositions) {p.leafPosition *= 4.0;}

	CreateLeafMesh(false);

	/*shapeP16 leafShape(ShapeType::Leaf);
	leafDrawCommands[0].indexCount = leafShape.GetIndices().size();
	leafDrawCommands[0].instanceCount = 0;
	leafDrawCommands[0].firstIndex = 0;
	leafDrawCommands[0].vertexOffset = 0;
	leafDrawCommands[0].firstInstance = 0;
	leafDrawCommands[1].indexCount = leafShape.GetIndices().size();
	leafDrawCommands[1].instanceCount = 0;
	leafDrawCommands[1].firstIndex = 0;
	leafDrawCommands[1].vertexOffset = 0;
	leafDrawCommands[1].firstInstance = 0;

	ShapeSettings leafSettings{};
	//leafSettings.lod = 1;
	//shapeP16 leafShape1(ShapeType::Leaf, leafSettings);

	//leafDrawCommands[2].indexCount = leafShape1.GetIndices().size();
	//leafDrawCommands[2].firstIndex = leafShape.GetIndices().size();
	leafDrawCommands[2].indexCount = leafShape.GetIndices().size();
	leafDrawCommands[2].firstIndex = 0;
	leafDrawCommands[2].vertexOffset = 0;
	leafDrawCommands[2].instanceCount = 0;
	leafDrawCommands[2].firstInstance = 0;

	//leafShape.Join(leafShape1);
	leafShape.Rotate(180.0, Axis::z);
	leafShape.Scale(2.0);
	
	leafSettings.lod = 2;
	leafSettings.scalarized = false;
	shapeP16 leafShape2(ShapeType::Leaf, leafSettings);

	leafDrawCommands[3].indexCount = leafShape2.GetIndices().size();
	leafDrawCommands[3].firstIndex = leafShape.GetIndices().size();
	leafDrawCommands[3].vertexOffset = 0;
	leafDrawCommands[3].instanceCount = 0;
	leafDrawCommands[3].firstInstance = 0;

	leafShape.Join(leafShape2);
	leafMesh.Create(leafShape);*/

	treeMeshConfig.horizontalResolution = {3, 4};
	treeMeshConfig.verticalResolution = {1, 4};
	treeMeshConfig.minimumThickness = 0.075;
	shapePN32 treeShape1 = Tree::GenerateTree(treeMeshConfig);
	treeShape1.Scale(point3D(4.0));

	drawCommands[1].indexCount = treeShape1.GetIndices().size();
	drawCommands[1].instanceCount = 0;
	drawCommands[1].firstIndex = treeShape.GetIndices().size();
	drawCommands[1].vertexOffset = 0;
	drawCommands[1].firstInstance = 0;

	treeShape.Join(treeShape1);

	treeMeshConfig.horizontalResolution = {3, 3};
	treeMeshConfig.verticalResolution = {1, 1};
	treeMeshConfig.minimumThickness = 0.135;
	shapePN32 treeShape2 = Tree::GenerateTree(treeMeshConfig);
	treeShape2.Scale(point3D(6.0, 4.0, 6.0));

	drawCommands[2].indexCount = treeShape2.GetIndices().size();
	drawCommands[2].instanceCount = 0;
	drawCommands[2].firstIndex = treeShape.GetIndices().size();
	drawCommands[2].vertexOffset = 0;
	drawCommands[2].firstInstance = 0;

	treeShape.Join(treeShape2);

	shapePN32 treeShape3(ShapeType::Cube);
	//treeShape3.Scale(point3D(6.0, 300.0, 6.0));
	treeShape3.Scale(point3D(2.0, 20.0, 2.0));
	treeShape3.Move(point3D(0.0, 10.0, 0.0));

	drawCommands[3].indexCount = treeShape3.GetIndices().size();
	drawCommands[3].instanceCount = 0;
	drawCommands[3].firstIndex = treeShape.GetIndices().size();
	drawCommands[3].vertexOffset = 0;
	drawCommands[3].firstInstance = 0;

	treeShape.Join(treeShape3);

	drawCommands[4].indexCount = 0;
	drawCommands[4].instanceCount = 0;
	drawCommands[4].firstIndex = 0;
	drawCommands[4].vertexOffset = 0;
	drawCommands[4].firstInstance = 0;

	treeShape.CalculateNormals();
	treeMesh.Create(treeShape);

	ImageConfig sceneLuminanceConfig = Image::DefaultConfig();
	sceneLuminanceConfig.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	sceneLuminanceConfig.viewConfig.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	sceneLuminanceConfig.width = 4;
	sceneLuminanceConfig.height = 4;
	sceneLuminanceConfig.samplerConfig.minFilter = VK_FILTER_NEAREST;
	sceneLuminanceConfig.samplerConfig.magFilter = VK_FILTER_NEAREST;

	luminanceImages.resize(Renderer::GetFrameCount());
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
	aerialImageConfig.width = aerialRes.x();
	aerialImageConfig.height = aerialRes.y();
	aerialImageConfig.depth = aerialRes.z();
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
	//glillImageConfig.width = glillResolution;
	//glillImageConfig.height = glillResolution;
	glillImageConfig.samplerConfig.repeatMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	glillImageConfig.samplerConfig.minFilter = VK_FILTER_LINEAR;
	glillImageConfig.samplerConfig.magFilter = VK_FILTER_LINEAR;
	glillImageConfig.format = VK_FORMAT_R8_UNORM;
	glillImageConfig.viewConfig.format = VK_FORMAT_R8_UNORM;

	int index = 0;
	glillImages.resize(3);
	for (Image& image : glillImages)
	{
		glillImageConfig.width = glillResolutions[index];
		glillImageConfig.height = glillResolutions[index];
		image.Create(glillImageConfig);
		index++;
	}
	glillQueue.resize(glillImages.size());

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
	imageConfig.samplerConfig.anisotropyEnabled = VK_TRUE;
	imageConfig.samplerConfig.maxAnisotropy = 2;
	imageConfig.srgb = true;
	imageConfig.compressed = true;
	imageConfig.samplerConfig.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	imageConfig.format = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
	imageConfig.viewConfig.format = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
	ImageConfig imageNormalConfig = Image::DefaultNormalConfig();
	imageNormalConfig.createMipmaps = true;
	imageNormalConfig.samplerConfig.anisotropyEnabled = VK_TRUE;
	imageNormalConfig.samplerConfig.maxAnisotropy = 16;
	imageNormalConfig.compressed = true;
	imageNormalConfig.normal = true;
	imageNormalConfig.samplerConfig.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	//imageNormalConfig.format = VK_FORMAT_R8G8_UNORM;
	//imageNormalConfig.viewConfig.format = VK_FORMAT_R8G8_UNORM;
	imageNormalConfig.format = VK_FORMAT_BC5_UNORM_BLOCK;
	imageNormalConfig.viewConfig.format = VK_FORMAT_BC5_UNORM_BLOCK;
	ImageConfig imageArmConfig = Image::DefaultNormalConfig();
	imageArmConfig.createMipmaps = true;
	imageArmConfig.samplerConfig.anisotropyEnabled = VK_TRUE;
	imageArmConfig.samplerConfig.maxAnisotropy = 4;
	imageArmConfig.compressed = true;
	imageArmConfig.samplerConfig.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
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
		{"bark_diff", ImageType::Jpg},
		{"bark_norm", ImageType::Jpg},
		{"bark_arm", ImageType::Jpg},
	});

	double startTime = Time::GetCurrentTime();

	rock_diff.Create(*loaders[0], imageConfig);
	rock_norm.Create(*loaders[1], imageNormalConfig);
	rock_arm.Create(*loaders[2], imageArmConfig);
	imageNormalConfig.samplerConfig.maxAnisotropy = 2;
	imageArmConfig.samplerConfig.maxAnisotropy = 2;
	bark_diff.Create(*loaders[9], imageConfig);
	bark_norm.Create(*loaders[10], imageNormalConfig);
	bark_arm.Create(*loaders[11], imageArmConfig);
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
	//data.lightDirection = point4D(point3D(0.613087, 0.116438, 0.781388));
	//data.lightDirection = point4D(point3D(0.582976, 0.328745, 0.743011));
	data.lightDirection = point4D(point3D(0.139343, 0.53843, 0.83107));
	//data.lightDirection = point4D(point3D(0.749465, 0.596829, 0.286527));
	//data.lightDirection = point4D(point3D(0.310526, 0.408194, 0.858459));
	data.resolution = point4D(Manager::GetCamera().GetConfig().width, Manager::GetCamera().GetConfig().height, 
		Manager::GetCamera().GetConfig().near, Manager::GetCamera().GetConfig().far);
	data.terrainOffset = point4D(0.0);
	//data.terrainOffset.w() = 15;
	data.terrainOffset.w() = 0;
	data.glillmapOffsets[0] = point4D(100000.0, 0.0, 100000.0, 2500.0);
	data.glillmapOffsets[1] = point4D(100000.0, 0.0, 100000.0, 15000.0);
	data.glillmapOffsets[2] = point4D(100000.0, 0.0, 100000.0, 75000.0);

	for (size_t i = 0; i < computeCascade; i++)
	{
		data.heightmapOffsets[i].x() = 100;
		data.heightmapOffsets[i].y() = 100;
	}

	BufferConfig retrieveBufferConfig = Buffer::MappedStorageConfig();
	retrieveBufferConfig.size = sizeof(RetrieveData);
	retrieveBuffer.Create(retrieveBufferConfig);

	BufferConfig frameBufferConfig{};
	frameBufferConfig.mapped = true;
	frameBufferConfig.size = sizeof(UniformData);
	frameBuffers.resize(Renderer::GetFrameCount());
	for (Buffer& buffer : frameBuffers) { buffer.Create(frameBufferConfig); }

	BufferConfig atmosphereBufferConfig = Buffer::MappedStorageConfig();
	atmosphereBufferConfig.mapped = true;
	atmosphereBufferConfig.size = sizeof(AtmosphereData);
	atmosphereBuffer.Create(atmosphereBufferConfig, &atmosphereData);

	BufferConfig objectBufferConfig{};
	objectBufferConfig.mapped = true;
	objectBufferConfig.size = sizeof(mat4) * models.size();
	objectBuffers.resize(Renderer::GetFrameCount());
	for (Buffer& buffer : objectBuffers) { buffer.Create(objectBufferConfig); }

	BufferConfig terrainShaderBufferConfig{};
	terrainShaderBufferConfig.mapped = true;
	terrainShaderBufferConfig.size = sizeof(TerrainShaderData);
	terrainShaderBuffer.Create(terrainShaderBufferConfig, &terrainShaderData);

	BufferConfig shadowDataBufferConfig{};
	shadowDataBufferConfig.mapped = true;
	shadowDataBufferConfig.size = sizeof(TerrainShaderData);
	shadowDataBuffer.Create(shadowDataBufferConfig, &shadowData);

	std::vector<DescriptorConfig> frameDescriptorConfigs(8);
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
	frameDescriptorConfigs[3].count = glillImages.size();
	frameDescriptorConfigs[4].type = DescriptorType::CombinedSampler;
	frameDescriptorConfigs[4].stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | 
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
	frameDescriptorConfigs[5].type = DescriptorType::StorageBuffer;
	frameDescriptorConfigs[5].stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | 
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
	frameDescriptorConfigs[6].type = DescriptorType::CombinedSampler;
	frameDescriptorConfigs[6].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	frameDescriptorConfigs[6].count = shadowCascades;
	frameDescriptorConfigs[7].type = DescriptorType::UniformBuffer;
	frameDescriptorConfigs[7].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	frameDescriptor.Create(0, frameDescriptorConfigs);

	std::vector<DescriptorConfig> materialDescriptorConfigs(4);
	materialDescriptorConfigs[0].type = DescriptorType::CombinedSampler;
	materialDescriptorConfigs[0].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	materialDescriptorConfigs[0].count = 3;
	materialDescriptorConfigs[1].type = DescriptorType::CombinedSampler;
	materialDescriptorConfigs[1].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	materialDescriptorConfigs[1].count = 3;
	materialDescriptorConfigs[2].type = DescriptorType::CombinedSampler;
	materialDescriptorConfigs[2].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	materialDescriptorConfigs[2].count = 3;
	materialDescriptorConfigs[3].type = DescriptorType::UniformBuffer;
	materialDescriptorConfigs[3].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
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
		frameDescriptor.Update(i, 3, Utilities::Pointerize(glillImages));
		frameDescriptor.Update(i, 4, skyImage);
		frameDescriptor.Update(i, 5, atmosphereBuffer);
		//frameDescriptor.Update(i, 6, {shadowPass.GetAttachmentImage(0, 0), shadowPass.GetAttachmentImage(0, 1), shadowPass.GetAttachmentImage(0, 2)});
		frameDescriptor.Update(i, 6, {shadowPass.GetAttachmentImage(0, 0), shadowPass.GetAttachmentImage(0, 1), shadowPass.GetAttachmentImage(0, 2), shadowPass.GetAttachmentImage(0, 3)});
		frameDescriptor.Update(i, 7, shadowDataBuffer);
	}

	materialDescriptor.GetNewSet();
	materialDescriptor.Update(0, 0, {&rock_diff, &rock_norm, &rock_arm});
	materialDescriptor.Update(0, 1, {&grass_diff, &grass_norm, &grass_arm});
	materialDescriptor.Update(0, 2, {&dry_diff, &dry_norm, &dry_arm});
	materialDescriptor.Update(0, 3, terrainShaderBuffer);

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

	BufferConfig terrainComputeBufferConfig{};
	terrainComputeBufferConfig.mapped = true;
	terrainComputeBufferConfig.size = sizeof(TerrainComputeData);
	terrainComputeBuffer.Create(terrainComputeBufferConfig, &terrainComputeData);

	std::vector<DescriptorConfig> computeDescriptorConfigs(4);
	computeDescriptorConfigs[0].type = DescriptorType::StorageImage;
	computeDescriptorConfigs[0].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeDescriptorConfigs[1].type = DescriptorType::StorageImage;
	computeDescriptorConfigs[1].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeDescriptorConfigs[2].type = DescriptorType::UniformBuffer;
	computeDescriptorConfigs[2].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	computeDescriptorConfigs[3].type = DescriptorType::UniformBuffer;
	computeDescriptorConfigs[3].stages = VK_SHADER_STAGE_COMPUTE_BIT;
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
		computeDescriptor.Update(i, 3, terrainComputeBuffer);
	}

	//treeDataBuffers.resize(Renderer::GetFrameCount());

	BufferConfig treeSetupDataBufferConfig = Buffer::StorageConfig();
	treeSetupDataBufferConfig.size = sizeof(TreeData) * treeCount;
	treeSetupDataBuffer.Create(treeSetupDataBufferConfig);

	BufferConfig treeRenderDataBufferConfig = Buffer::StorageConfig();
	treeRenderDataBufferConfig.size = sizeof(TreeData) * treeCount;
	treeRenderDataBuffer.Create(treeRenderDataBufferConfig);

	BufferConfig treeShadowRenderDataBufferConfig = Buffer::StorageConfig();
	treeShadowRenderDataBufferConfig.size = sizeof(TreeData) * treeCount;
	treeShadowRenderDataBuffer.Create(treeShadowRenderDataBufferConfig);

	RecaluclateTreeComputeConfig();
	BufferConfig treeComputeConfigBufferConfig = Buffer::MappedStorageConfig();
	//treeComputeConfigBufferConfig.mapped = true;
	treeComputeConfigBufferConfig.size = sizeof(TreeComputeConfig);
	treeComputeConfigBuffer.Create(treeComputeConfigBufferConfig, &treeComputeConfig);

	BufferConfig treeShaderConfigBufferConfig{};
	treeShaderConfigBufferConfig.mapped = true;
	treeShaderConfigBufferConfig.size = sizeof(TreeShaderConfig);
	treeShaderConfigBuffer.Create(treeShaderConfigBufferConfig, &treeShaderConfig);

	BufferConfig treeDrawBufferConfig = Buffer::DrawCommandConfig();
	treeDrawBufferConfig.size = sizeof(VkDrawIndexedIndirectCommand) * drawCommands.size();
	treeDrawBuffer.Create(treeDrawBufferConfig, drawCommands.data());

	BufferConfig treeShadowDrawBufferConfig = Buffer::DrawCommandConfig();
	treeShadowDrawBufferConfig.size = sizeof(VkDrawIndexedIndirectCommand) * drawCommands.size();
	treeShadowDrawBuffer.Create(treeShadowDrawBufferConfig, drawCommands.data());

	BufferConfig leafDrawBufferConfig = Buffer::DrawCommandConfig();
	leafDrawBufferConfig.size = sizeof(VkDrawIndexedIndirectCommand) * leafDrawCommands.size();
	leafDrawBuffer.Create(leafDrawBufferConfig, leafDrawCommands.data());

	leafDrawCommands[1] = leafDrawCommands[2];
	leafDrawCommands[2] = leafDrawCommands[3];
	leafDrawCommands[3] = leafDrawCommands[4];
	//leafDrawCommands[4].firstIndex = 0;
	//leafDrawCommands[4].indexCount = 0;

	BufferConfig leafShadowDrawBufferConfig = Buffer::DrawCommandConfig();
	leafShadowDrawBufferConfig.size = sizeof(VkDrawIndexedIndirectCommand) * leafDrawCommands.size();
	leafShadowDrawBuffer.Create(leafShadowDrawBufferConfig, leafDrawCommands.data());

	BufferConfig leafPositionsBufferConfig = Buffer::StorageConfig();
	leafPositionsBufferConfig.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	leafPositionsBufferConfig.size = sizeof(LeafData) * leafPositions.size();
	leafPositionsBuffer.Create(leafPositionsBufferConfig, leafPositions.data());

	BufferConfig leafLodPositionsBufferConfig = Buffer::StorageConfig();
	leafLodPositionsBufferConfig.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	leafLodPositionsBufferConfig.size = sizeof(LeafLodPosition) * leafLodPositions.size();
	leafLodPositionsBuffer.Create(leafLodPositionsBufferConfig, leafLodPositions.data());

	BufferConfig leafShaderConfigBufferConfig{};
	leafShaderConfigBufferConfig.mapped = true;
	leafShaderConfigBufferConfig.size = sizeof(LeafShaderConfig);
	leafShaderConfigBuffer.Create(leafShaderConfigBufferConfig, &leafShaderConfig);

	std::vector<DescriptorConfig> treeComputeDescriptorConfigs(8);
	treeComputeDescriptorConfigs[0].type = DescriptorType::StorageBuffer;
	treeComputeDescriptorConfigs[0].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	treeComputeDescriptorConfigs[1].type = DescriptorType::StorageBuffer;
	treeComputeDescriptorConfigs[1].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	treeComputeDescriptorConfigs[2].type = DescriptorType::StorageBuffer;
	treeComputeDescriptorConfigs[2].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	treeComputeDescriptorConfigs[3].type = DescriptorType::StorageBuffer;
	treeComputeDescriptorConfigs[3].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	treeComputeDescriptorConfigs[4].type = DescriptorType::StorageBuffer;
	treeComputeDescriptorConfigs[4].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	treeComputeDescriptorConfigs[5].type = DescriptorType::StorageBuffer;
	treeComputeDescriptorConfigs[5].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	treeComputeDescriptorConfigs[6].type = DescriptorType::StorageBuffer;
	treeComputeDescriptorConfigs[6].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	treeComputeDescriptorConfigs[7].type = DescriptorType::StorageBuffer;
	treeComputeDescriptorConfigs[7].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	treeComputeDescriptor.Create(1, treeComputeDescriptorConfigs);

	//treeComputeDescriptor.GetNewSetDynamic();
	//treeComputeDescriptor.UpdateDynamic(0, 0, Utilities::Pointerize(treeDataBuffers));
	//treeComputeDescriptor.UpdateDynamic(0, 1, Utilities::Pointerize(treeDrawBuffers));

	treeComputeDescriptor.GetNewSet();
	treeComputeDescriptor.Update(0, 0, treeSetupDataBuffer);
	treeComputeDescriptor.Update(0, 1, treeRenderDataBuffer);
	treeComputeDescriptor.Update(0, 2, treeShadowRenderDataBuffer);
	treeComputeDescriptor.Update(0, 3, treeDrawBuffer);
	treeComputeDescriptor.Update(0, 4, treeShadowDrawBuffer);
	treeComputeDescriptor.Update(0, 5, leafDrawBuffer);
	treeComputeDescriptor.Update(0, 6, leafShadowDrawBuffer);
	treeComputeDescriptor.Update(0, 7, treeComputeConfigBuffer);

	std::vector<DescriptorConfig> treeDescriptorConfigs(4);
	treeDescriptorConfigs[0].type = DescriptorType::StorageBuffer;
	treeDescriptorConfigs[0].stages = VK_SHADER_STAGE_VERTEX_BIT;
	treeDescriptorConfigs[1].type = DescriptorType::StorageBuffer;
	treeDescriptorConfigs[1].stages = VK_SHADER_STAGE_VERTEX_BIT;
	treeDescriptorConfigs[2].type = DescriptorType::CombinedSampler;
	treeDescriptorConfigs[2].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	treeDescriptorConfigs[2].count = 3;
	treeDescriptorConfigs[3].type = DescriptorType::UniformBuffer;
	treeDescriptorConfigs[3].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	treeDescriptor.Create(1, treeDescriptorConfigs);

	treeDescriptor.GetNewSet();
	treeDescriptor.Update(0, 0, treeRenderDataBuffer);
	treeDescriptor.Update(0, 1, treeShadowRenderDataBuffer);
	treeDescriptor.Update(0, 2, {&bark_diff, &bark_norm, &bark_arm});
	treeDescriptor.Update(0, 3, treeShaderConfigBuffer);

	std::vector<DescriptorConfig> leafDescriptorConfigs(6);
	leafDescriptorConfigs[0].type = DescriptorType::StorageBuffer;
	leafDescriptorConfigs[0].stages = VK_SHADER_STAGE_VERTEX_BIT;
	leafDescriptorConfigs[1].type = DescriptorType::StorageBuffer;
	leafDescriptorConfigs[1].stages = VK_SHADER_STAGE_VERTEX_BIT;
	leafDescriptorConfigs[2].type = DescriptorType::StorageBuffer;
	leafDescriptorConfigs[2].stages = VK_SHADER_STAGE_VERTEX_BIT;
	leafDescriptorConfigs[3].type = DescriptorType::StorageBuffer;
	leafDescriptorConfigs[3].stages = VK_SHADER_STAGE_VERTEX_BIT;
	leafDescriptorConfigs[4].type = DescriptorType::StorageBuffer;
	leafDescriptorConfigs[4].stages = VK_SHADER_STAGE_VERTEX_BIT;
	leafDescriptorConfigs[5].type = DescriptorType::UniformBuffer;
	leafDescriptorConfigs[5].stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	leafDescriptor.Create(1, leafDescriptorConfigs);

	leafDescriptor.GetNewSet();
	leafDescriptor.Update(0, 0, treeRenderDataBuffer);
	leafDescriptor.Update(0, 1, treeShadowRenderDataBuffer);
	leafDescriptor.Update(0, 2, leafPositionsBuffer);
	leafDescriptor.Update(0, 3, leafLodPositionsBuffer);
	leafDescriptor.Update(0, 4, treeComputeConfigBuffer);
	leafDescriptor.Update(0, 5, leafShaderConfigBuffer);

	std::vector<DescriptorConfig> retrieveDescriptorConfigs(1);
	retrieveDescriptorConfigs[0].type = DescriptorType::StorageBuffer;
	retrieveDescriptorConfigs[0].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	retrieveDescriptor.Create(1, retrieveDescriptorConfigs);

	retrieveDescriptor.GetNewSet();
	retrieveDescriptor.Update(0, 0, retrieveBuffer);

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

	for (int i = 0; i < Renderer::GetFrameCount(); i++)
	{
		luminanceDescriptor.GetNewSet();
		luminanceDescriptor.Update(i, 0, luminanceImages[i]);
		luminanceDescriptor.Update(i, 1, luminanceBuffer);
		luminanceDescriptor.Update(i, 2, luminanceVariablesBuffer);
	}

	BufferConfig postBufferConfig{};
	postBufferConfig.mapped = true;
	postBufferConfig.size = sizeof(PostData);
	postBuffer.Create(postBufferConfig, &postData);

	int j = 0;
	std::vector<DescriptorConfig> postDescriptorConfigs(9);
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
	postDescriptorConfigs[j].type = DescriptorType::UniformBuffer;
	postDescriptorConfigs[j++].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	postDescriptorConfigs[j].type = DescriptorType::CombinedSampler;
	postDescriptorConfigs[j++].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	postDescriptor.Create(1, postDescriptorConfigs);

	for (int i = 0; i < Renderer::GetFrameCount(); i++)
	{
		int k = 0;

		postDescriptor.GetNewSet();
		//postDescriptor.Update(i, k++, *pass.GetImage(i));
		postDescriptor.Update(i, k++, *pass.GetAttachmentImage(0, i));
		postDescriptor.Update(i, k++, *pass.GetAttachmentImage(2, i));
		postDescriptor.Update(i, k++, transmittanceImage);
		postDescriptor.Update(i, k++, scatteringImage);
		postDescriptor.Update(i, k++, skyImage);
		postDescriptor.Update(i, k++, aerialImage);
		postDescriptor.Update(i, k++, luminanceBuffer);
		postDescriptor.Update(i, k++, postBuffer);
		postDescriptor.Update(i, k++, *shadowPass.GetAttachmentImage(0, i)); //remoooooveeeeee!!!!
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

	BufferConfig glillBufferConfig{};
	glillBufferConfig.mapped = true;
	glillBufferConfig.size = sizeof(GlillData);
	glillBuffers.resize(glillImages.size());
	glillDatas.resize(glillImages.size());
	for (int i = 0; i < glillImages.size(); i++)
	{
		glillDatas[i].offset = point4D(0.0, 0.0, 0.0, data.glillmapOffsets[i].w());
		//glillDatas[i].settings = point4D(10.0, 200.0 * (i == 0 ? 1.0 : 7.5), 8.0, 0.0);
		//glillDatas[i].settings = point4D(10.0, 200.0, 8.0, glillResolutions[i]);
		glillDatas[i].settings = point4D(10.0, 450.0, 16.0, glillResolutions[i]);
		if (i == 0) {glillDatas[i].interPower = 2.0;}
		glillBuffers[i].Create(glillBufferConfig, &glillDatas[i]);
	}

	std::vector<DescriptorConfig> glillDescriptorConfigs(4);
	glillDescriptorConfigs[0].type = DescriptorType::StorageImage;
	glillDescriptorConfigs[0].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	glillDescriptorConfigs[1].type = DescriptorType::UniformBuffer;
	glillDescriptorConfigs[1].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	glillDescriptorConfigs[2].type = DescriptorType::StorageBuffer;
	glillDescriptorConfigs[2].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	glillDescriptorConfigs[3].type = DescriptorType::StorageBuffer;
	glillDescriptorConfigs[3].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	glillDescriptor.Create(1, glillDescriptorConfigs);

	for (int i = 0; i < glillImages.size(); i++)
	{
		glillDescriptor.GetNewSet();
		glillDescriptor.Update(i, 0, glillImages[i]);
		glillDescriptor.Update(i, 1, glillBuffers[i]);
		glillDescriptor.Update(i, 2, treeSetupDataBuffer);
		glillDescriptor.Update(i, 3, treeComputeConfigBuffer);
	}

	BufferConfig aerialBufferConfig{};
	aerialBufferConfig.mapped = true;
	aerialBufferConfig.size = sizeof(AerialData);
	aerialBuffer.Create(aerialBufferConfig, &aerialData);

	std::vector<DescriptorConfig> aerialDescriptorConfigs(1);
	aerialDescriptorConfigs[0].type = DescriptorType::UniformBuffer;
	aerialDescriptorConfigs[0].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	aerialDescriptor.Create(2, aerialDescriptorConfigs);

	aerialDescriptor.GetNewSet();
	aerialDescriptor.Update(0, 0, aerialBuffer);

	BufferConfig scatteringBufferConfig{};
	scatteringBufferConfig.mapped = true;
	scatteringBufferConfig.size = sizeof(ScatteringData);
	scatteringBuffer.Create(scatteringBufferConfig, &scatteringData);

	std::vector<DescriptorConfig> scatteringDescriptorConfigs(1);
	scatteringDescriptorConfigs[0].type = DescriptorType::UniformBuffer;
	scatteringDescriptorConfigs[0].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	scatteringDescriptor.Create(2, scatteringDescriptorConfigs);

	scatteringDescriptor.GetNewSet();
	scatteringDescriptor.Update(0, 0, scatteringBuffer);

	BufferConfig skyBufferConfig{};
	skyBufferConfig.mapped = true;
	skyBufferConfig.size = sizeof(SkyData);
	skyBuffer.Create(skyBufferConfig, &skyData);

	std::vector<DescriptorConfig> skyDescriptorConfigs(1);
	skyDescriptorConfigs[0].type = DescriptorType::UniformBuffer;
	skyDescriptorConfigs[0].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	skyDescriptor.Create(2, skyDescriptorConfigs);

	skyDescriptor.GetNewSet();
	skyDescriptor.Update(0, 0, skyBuffer);

	BufferConfig terrainShadowBufferConfig{}; //remove computeCascade as size!!!!!!!!!!
	terrainShadowBufferConfig.mapped = true;
	terrainShadowBufferConfig.size = sizeof(point4D);
	terrainShadowBuffers.resize(terrainShadowImages.size());
	for (int i = 0; i < terrainShadowImages.size(); i++)
	{
		point4D temp = point4D(100000.0, 0.0, 100000.0, 0.0);
		data.shadowmapOffsets[i] = temp;
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
	pipelineConfig.subpass = 0;
	//pipelineConfig.rasterization.cullMode = VK_CULL_MODE_FRONT_BIT;
	//pipelineConfig.rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pipeline.Create(pipelineConfig);

	//PipelineConfig prePipelineConfig = pipelineConfig;
	////prePipelineConfig.prepass = true;
	//prePipelineConfig.subpass = 0;
	//prePipeline.Create(prePipelineConfig);

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
	scatteringPipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), atmosphereDescriptor.GetLayout(), scatteringDescriptor.GetLayout() };
	scatteringPipeline.Create(scatteringPipelineConfig);

	PipelineConfig skyPipelineConfig{};
	skyPipelineConfig.shader = "sky";
	skyPipelineConfig.type = PipelineType::Compute;
	skyPipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), atmosphereDescriptor.GetLayout(), skyDescriptor.GetLayout() };
	skyPipeline.Create(skyPipelineConfig);

	PipelineConfig aerialPipelineConfig{};
	aerialPipelineConfig.shader = "aerial";
	aerialPipelineConfig.type = PipelineType::Compute;
	aerialPipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), atmosphereDescriptor.GetLayout(), aerialDescriptor.GetLayout() };
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

	PipelineConfig retrievePipelineConfig{};
	retrievePipelineConfig.shader = "retrieve";
	retrievePipelineConfig.type = PipelineType::Compute;
	retrievePipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), retrieveDescriptor.GetLayout() };
	retrievePipeline.Create(retrievePipelineConfig);

	PipelineConfig treeSetupComputePipelineConfig{};
	treeSetupComputePipelineConfig.shader = "treeSetupCompute";
	treeSetupComputePipelineConfig.type = PipelineType::Compute;
	treeSetupComputePipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), treeComputeDescriptor.GetLayout() };
	treeSetupComputePipeline.Create(treeSetupComputePipelineConfig);

	PipelineConfig treeRenderComputePipelineConfig{};
	treeRenderComputePipelineConfig.shader = "treeRenderCompute";
	treeRenderComputePipelineConfig.type = PipelineType::Compute;
	treeRenderComputePipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), treeComputeDescriptor.GetLayout() };
	treeRenderComputePipeline.Create(treeRenderComputePipelineConfig);

	PipelineConfig treePipelineConfig = Pipeline::DefaultConfig();
	treePipelineConfig.shader = "tree";
	treePipelineConfig.vertexInfo = treeMesh.GetVertexInfo();
	treePipelineConfig.renderpass = pass.GetRenderpass();
	treePipelineConfig.type = PipelineType::Graphics;
	treePipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), treeDescriptor.GetLayout() };
	treePipelineConfig.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	treePipelineConfig.dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
	treePipelineConfig.subpass = 0;
	treePipeline.Create(treePipelineConfig);

	VkPushConstantRange pc{};
	pc.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pc.offset = 0;
	pc.size = sizeof(uint32_t);

	PipelineConfig treeShadowPipelineConfig = Pipeline::DefaultConfig();
	treeShadowPipelineConfig.shader = "treeShadow";
	treeShadowPipelineConfig.vertexInfo = treeMesh.GetVertexInfo();
	treeShadowPipelineConfig.renderpass = shadowPass.GetRenderpass();
	treeShadowPipelineConfig.type = PipelineType::Graphics;
	treeShadowPipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), treeDescriptor.GetLayout() };
	treeShadowPipelineConfig.pushConstants = { pc };
	treeShadowPipelineConfig.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	treeShadowPipelineConfig.dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
	treeShadowPipelineConfig.subpass = 0;
	treeShadowPipelineConfig.rasterization.cullMode = VK_CULL_MODE_FRONT_BIT;
	treeShadowPipelineConfig.rasterization.depthBiasEnable = VK_TRUE;
	//treeShadowPipelineConfig.rasterization.depthBiasConstantFactor = 4.0;
	//treeShadowPipelineConfig.rasterization.depthBiasSlopeFactor = 3.0;
	treeShadowPipelineConfig.rasterization.depthBiasConstantFactor = -2.0;
	treeShadowPipelineConfig.rasterization.depthBiasSlopeFactor = -2.0;
	treeShadowPipeline.Create(treeShadowPipelineConfig);

	PipelineConfig leafPipelineConfig = Pipeline::DefaultConfig();
	leafPipelineConfig.shader = "leaf";
	leafPipelineConfig.vertexInfo = leafMesh.GetVertexInfo();
	leafPipelineConfig.renderpass = pass.GetRenderpass();
	leafPipelineConfig.type = PipelineType::Graphics;
	leafPipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), leafDescriptor.GetLayout() };
	leafPipelineConfig.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	leafPipelineConfig.dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
	leafPipelineConfig.subpass = 0;
	leafPipelineConfig.rasterization.cullMode = VK_CULL_MODE_NONE;
	leafPipeline.Create(leafPipelineConfig);

	PipelineConfig leafShadowPipelineConfig = Pipeline::DefaultConfig();
	leafShadowPipelineConfig.shader = "leafShadow";
	leafShadowPipelineConfig.vertexInfo = leafMesh.GetVertexInfo();
	leafShadowPipelineConfig.renderpass = shadowPass.GetRenderpass();
	leafShadowPipelineConfig.type = PipelineType::Graphics;
	leafShadowPipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), leafDescriptor.GetLayout() };
	leafShadowPipelineConfig.pushConstants = { pc };
	leafShadowPipelineConfig.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	leafShadowPipelineConfig.dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
	leafShadowPipelineConfig.subpass = 0;
	leafShadowPipelineConfig.rasterization.depthBiasEnable = VK_TRUE;
	leafShadowPipelineConfig.rasterization.depthBiasConstantFactor = 2.0;
	leafShadowPipelineConfig.rasterization.depthBiasSlopeFactor = 2.0;
	leafShadowPipelineConfig.rasterization.cullMode = VK_CULL_MODE_NONE;
	leafShadowPipeline.Create(leafShadowPipelineConfig);

	//Manager::GetCamera().Move(point3D(-5000, 2500, 5000));
	Input::TriggerMouse();
	//Manager::GetCamera().Move(point3D(-1486.45, -1815.79, -3094.54));
	//Manager::GetCamera().Move(point3D(-931.948, -2051.53, -2499.46));
	//Manager::GetCamera().Move(point3D(-24.9147, -904.676, 5320.23));
	//Manager::GetCamera().Move(point3D(3884.26, -1783.41, 11323.2));
	//Manager::GetCamera().Move(point3D(0, 0, 0));
	//Manager::GetCamera().Move(point3D(1242.74, -1730.73, 5410.96));
	//Manager::GetCamera().Move(point3D(4641.78, -2376.92, 8547.5));
	//Manager::GetCamera().Move(point3D(4603.41, -2411.25, 8862.13));
	//Manager::GetCamera().Move(point3D(4638.89, -2356.18, 8880.5));
	Manager::GetCamera().Move(point3D(3378.99, -2204.17, -3144.48));
	//Manager::GetCamera().Move(point3D(3310.14, -2324.92, -3054.38));
	//Manager::GetCamera().Move(point3D(428.953, -2005.17, 2031.84));
	//Manager::GetCamera().Move(point3D(1249.58, -1968.5, 6361.08));
	//Manager::GetCamera().Move(point3D(4692.71, -2418.12, 9021.92));
	//Manager::GetCamera().Move(point3D(10379.6, 362.507, 824.909));
	//Manager::GetCamera().Rotate(point3D(12.8998, -149.9, 0.0));
	//Manager::GetCamera().Rotate(point3D(26.0998, -151.9, 0.0));
	//Manager::GetCamera().Rotate(point3D(4.89981, 306.799, 0.0));
	//Manager::GetCamera().Rotate(point3D(-3.40032, 292.801, 0.0));
	//Manager::GetCamera().Rotate(point3D(6.39966, 343.702, 0.0));
	//Manager::GetCamera().Rotate(point3D(24.7997, 338.802, 0.0));
	//Manager::GetCamera().Rotate(point3D(12.1997, 669.309, 0.0));
	//Manager::GetCamera().Rotate(point3D(-5.8003, 1071.71, 0.0));
	Manager::GetCamera().Rotate(point3D(0.399712, 1401.68, 0.0));
	//Manager::GetCamera().Rotate(point3D(28.2997, 19.78, 0.0));
	//Manager::GetCamera().Rotate(point3D(-3.80028, 1728.39, 0.0));
	//Manager::GetCamera().Rotate(point3D(-46.101, 1074.21, 0.0));
	//Manager::GetCamera().Rotate(point3D(3.49968, 781.528, 0.0));
	//Manager::GetCamera().Rotate(point3D(31.1995, 454.71, 0.0));
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

	Menu& menu = UI::NewMenu("Atmosphere");
	menu.TriggerNode("variables", UpdateAtmosphereData);
	menu.AddSlider("mie phase function", atmosphereData.miePhaseFunction, 0.0, 1.0);
	menu.AddSlider("offset radius", atmosphereData.offsetRadius, 0.0, 1.0);
	menu.AddSlider("rayleigh scattering strength", atmosphereData.rayleighScatteringStrength, 0.0, 3.0);
	menu.AddSlider("mie scattering strength", atmosphereData.mieScatteringStrength, 0.0, 3.0);
	menu.AddSlider("mie Extinction strength", atmosphereData.mieExtinctionStrength, 0.0, 3.0);
	menu.AddSlider("absorption Extinction strength", atmosphereData.absorptionExtinctionStrength, 0.0, 3.0);
	menu.AddSlider("mist strength", atmosphereData.mistStrength, 0.0, 100.0);
	menu.AddSlider("sky strength", atmosphereData.skyStrength, 0.0, 30.0);
	menu.AddSlider("rayleigh scale height", atmosphereData.rayleighScaleHeight, 0.0, 24.0);
	menu.AddSlider("mie scale height", atmosphereData.mieScaleHeight, 0.0, 12.0);
	menu.AddSlider("camera scale", atmosphereData.cameraScale, 0.0, 0.1);
	menu.AddSlider("absorption density height", atmosphereData.absorptionDensityHeight, 0.0, 100.0);
	menu.AddSlider("absorption1", atmosphereData.absorption1, 0.0, 1.0);
	menu.AddSlider("absorption2", atmosphereData.absorption2, -3.0, 0.0);
	menu.AddSlider("absorption3", atmosphereData.absorption3, -1.0, 0.0);
	menu.AddSlider("absorption4", atmosphereData.absorption4, 0.0, 5.0);
	menu.AddSlider("calculate in shadow", atmosphereData.calculateInShadow, 0.0, 1.0);
	menu.AddSlider("sky power", atmosphereData.skyPower, 0.0, 8.0);
	menu.AddSlider("default sky power", atmosphereData.defaultSkyPower, 0.0, 1000.0);
	menu.AddSlider("sky dilute", atmosphereData.skyDilute, 0.0, 512.0);
	menu.TriggerNode("variables");

	menu.TriggerNode("aerial settings", UpdateAerialData);
	menu.AddCheckbox("shadows enabled", aerialData.shadowsEnabled);
	menu.AddCheckbox("mist enabled", aerialData.mistEnabled);
	menu.AddSlider("mist strength", aerialData.mistStrength, 0.0, 32.0);
	menu.AddSlider("mist height", aerialData.mistHeight, 0.0, 0.125);
	menu.AddSlider("mist height power", aerialData.mistHeightPower, 0.0, 2.0);
	menu.AddSlider("mist buildup power", aerialData.mistBuildupPower, 0.0, 8.0);
	menu.AddSlider("slice offset", aerialData.sliceOffset, 0.0, 1.0);
	menu.AddSlider("maximum height", aerialData.maximumHeight, -500.0, 1000.0);
	menu.AddSlider("decrease height", aerialData.decreaseHeight, 0.0, 500.0);
	menu.AddSlider("decrease power", aerialData.decreasePower, 0.0, 4.0);
	menu.AddSlider("blend distance", aerialData.blendDistance, 0.0, 1000.0);
	menu.AddSlider("default occlusion", aerialData.defaultOcclusion, 0.0, 1.0);
	menu.AddSlider("phase strength", aerialData.phaseStrength, 0.0, 2.0);
	menu.AddSlider("sun strength", aerialData.sunStrength, 0.0, 2.0);
	menu.AddSlider("sample count mult", aerialData.sampleCountMult, 0.0, 4.0);
	menu.AddCheckbox("use occlusion", aerialData.useOcclusion);
	menu.AddCheckbox("lod occlusion", aerialData.lodOcclusion);
	menu.AddCheckbox("blend occlusion", aerialData.blendOcclusion);
	menu.AddCheckbox("sun mist", aerialData.sunMist);
	menu.TriggerNode("aerial settings");

	menu.TriggerNode("scattering settings", UpdateScatteringData);
	menu.AddSlider("scattering strength", scatteringData.scatteringStrength, 0.0, 3.0);
	menu.AddSlider("light strength", scatteringData.lightStrength, 0.0, 2.0);
	menu.AddSlider("transmittance strength", scatteringData.transmittanceStrength, 0.0, 0.2);
	menu.TriggerNode("scattering settings");

	menu.TriggerNode("sky settings", UpdateSkyData);
	menu.AddSlider("rayleigh strength", skyData.rayleighStrength, 0.0, 2.0);
	menu.TriggerNode("sky settings");
	
	Menu& glillMenu = UI::NewMenu("Global illumination");
	glillMenu.TriggerNode("global variables", UpdateGlillData);
	glillMenu.AddSlider("global sample power", globalGlillSamplePower, 0.0, 2.0);
	glillMenu.AddSlider("tree density strength", globalTreeDensityStrength, 0.0, 2.0);
	glillMenu.AddSlider("tree density power", globalTreeDensityPower, 0.0, 4.0);
	glillMenu.TriggerNode("global variables");
	for (int i = 0; i < glillDatas.size(); i++)
	{
		glillMenu.TriggerNode(std::string("glill settings ").append(std::to_string(i)), UpdateGlillData);

		glillMenu.AddSlider("sample count", glillDatas[i].settings.x(), 1.0, 50.0);
		glillMenu.AddSlider("sample distance", glillDatas[i].settings.y(), 1.0, 1000.0);
		glillMenu.AddSlider("sample power", glillDatas[i].settings.z(), 1.0, 32.0);
		glillMenu.AddSlider("inter power", glillDatas[i].interPower, 1.0, 4.0);

		glillMenu.TriggerNode(std::string("glill settings ").append(std::to_string(i)));
	}

	Menu& terrainMenu = UI::NewMenu("Terrain");
	terrainMenu.AddCheckbox("terrain enabled", terrainEnabled);
	terrainMenu.TriggerNode("Shader", UpdateTerrainShaderData);
	terrainMenu.AddDropdown("debug mode", terrainShaderData.debugMode, {"none", "base normal", "texture normal", "heightmap lod edge", "heightmap lod full", "chunk lod", "chunk ID", "steepness range"});
	terrainMenu.AddSlider("rock steepness", terrainShaderData.rockSteepness, 0.0, 0.25);
	terrainMenu.AddSlider("rock transition", terrainShaderData.rockTransition, 0.0, 0.1);
	terrainMenu.AddSlider("snow height", terrainShaderData.snowHeight, 500.0, 2500.0);
	terrainMenu.AddSlider("snow blend", terrainShaderData.snowBlend, 500.0, 2500.0);
	terrainMenu.AddSlider("snow steepness", terrainShaderData.snowSteepness, 0.0, 0.5);
	terrainMenu.AddCheckbox("snow enabled", terrainShaderData.snowEnabled);
	terrainMenu.TriggerNode("Shader");
	terrainMenu.TriggerNode("Generation");
	terrainMenu.AddSlider("seed", terrainComputeData.seed, 0.0001, 1.0);
	terrainMenu.AddSlider("erode factor", terrainComputeData.erodeFactor, 0.0, 4.0);
	terrainMenu.AddSlider("steepness", terrainComputeData.steepness, 0.0, 4.0);
	terrainMenu.AddButton("Regenerate", UpdateTerrainComputeData);
	terrainMenu.TriggerNode("Generation");

	Menu& cameraMenu = UI::NewMenu("Camera");
	cameraMenu.TriggerNode("Settings", UpdateCameraSensitivity);
	cameraMenu.AddSlider("sensitivity", cameraSensitivity, 0.0, 0.2);
	cameraMenu.TriggerNode("Settings");
	//cameraMenu.TriggerNode("Info");
	//cameraMenu.AddText()
	//cameraMenu.TriggerNode("Info");

	Menu& postMenu = UI::NewMenu("Post");
	postMenu.TriggerNode("Settings", UpdatePostData);
	postMenu.AddCheckbox("use linear depth", postData.useLinearDepth);
	postMenu.AddDropdown("aerial blend mode", postData.aerialBlendMode, {"none", "texel corners", "weighted"});
	postMenu.AddCheckbox("tonemapping", postData.toneMapping);
	postMenu.AddSlider("exposure", postData.exposure, 0.0, 2.0);
	postMenu.TriggerNode("Settings");

	Menu& treeMenu = UI::NewMenu("Trees");
	treeMenu.AddCheckbox("trees enabled", treesEnabled);
	treeMenu.TriggerNode("Compute", UpdateTreeComputeData);
	treeMenu.AddSlider("spacing", treeComputeConfig.treeSpacing, 0.1, 100.0);
	treeMenu.AddSlider("random offset", treeComputeConfig.treeOffset, 0.0, 50.0);
	treeMenu.AddSlider("max steepness", treeComputeConfig.maxSteepness, 0.0, 0.2);
	treeMenu.AddSlider("noise scale", treeComputeConfig.noiseScale, 0.0, 0.004);
	treeMenu.AddSlider("noise octaves", treeComputeConfig.noiseOctaves, 0, 4);
	treeMenu.AddSlider("noise cutoff", treeComputeConfig.noiseCutoff, 0.0, 1.0);
	treeMenu.AddSlider("cutoff randomness", treeComputeConfig.noiseCutoffRandomness, 0.0, 0.5);
	treeMenu.AddSlider("lod 0 radius", treeComputeConfig.radiuses[0], 0, 10);
	treeMenu.AddSlider("lod 1 radius", treeComputeConfig.radiuses[1], 3, 25);
	treeMenu.AddSlider("lod 2 radius", treeComputeConfig.radiuses[2], 15, 50);
	treeMenu.AddSlider("lod 3 radius", treeComputeConfig.radiuses[3], 40, 100);
	treeMenu.AddSlider("render radius", treeComputeConfig.radiuses[4], 0, treeComputeBase);
	treeMenu.AddSlider("cascade 0 tolerance", treeComputeConfig.cascadeTolerances[0], 0.0, 0.1);
	treeMenu.AddSlider("cascade 1 tolerance", treeComputeConfig.cascadeTolerances[1], 0.0, 0.1);
	treeMenu.AddSlider("cascade 2 tolerance", treeComputeConfig.cascadeTolerances[2], 0.0, 0.1);
	treeMenu.AddSlider("cascade 3 tolerance", treeComputeConfig.cascadeTolerances[3], 0.0, 0.1);
	treeMenu.AddCheckbox("occlusion culling", treeComputeConfig.occlusionCulling);
	treeMenu.AddSlider("cull iterations", treeComputeConfig.cullIterations, 1, 25);
	treeMenu.AddDropdown("cull exponent", treeComputeConfig.cullExponent, {"none", "in out quad", "in out cubic", "quad"});
	treeMenu.AddSlider("cull start distance", treeComputeConfig.cullStartDistance, 0.0, 250.0);
	treeMenu.AddSlider("cull height", treeComputeConfig.cullHeight, 0.0, 50.0);
	treeMenu.AddCheckbox("overdraw culling", treeComputeConfig.overdrawCulling);
	treeMenu.AddSlider("overdraw culling minimum", treeComputeConfig.overdrawCullMinimum, 0, 50);
	treeMenu.AddSlider("overdraw culling maximum", treeComputeConfig.overdrawCullMaximum, 0, 100);
	treeMenu.AddSlider("overdraw culling height base", treeComputeConfig.overdrawCullHeightBase, 0.0, 50.0);
	treeMenu.AddSlider("overdraw culling height offset", treeComputeConfig.overdrawCullHeightOffset, 0.0, 50.0);
	treeMenu.AddCheckbox("overdraw culling height only", treeComputeConfig.overdrawCullHeightOnly);
	treeMenu.AddCheckbox("overdraw lod cull", treeComputeConfig.overdrawLodCull);
	treeMenu.AddCheckbox("overdraw misses", treeComputeConfig.overdrawMisses);
	treeMenu.AddCheckbox("overdraw heavy lod cull", treeComputeConfig.overdrawLodCullHeavy);
	treeMenu.AddSlider("overdraw lod cull minimum", treeComputeConfig.overdrawLodCullMinimum, 0, 25);
	treeMenu.AddSlider("overdraw lod cull maximum", treeComputeConfig.overdrawLodCullMaximum, 0, 100);
	treeMenu.AddSlider("tree scale minimum", treeComputeConfig.minTreeScale, 0.0, 1.0);
	treeMenu.AddSlider("tree scale maximum", treeComputeConfig.maxTreeScale, 1.0, 2.0);
	treeMenu.AddSlider("tree scale power", treeComputeConfig.treeScalePower, 0.0, 4.0);
	treeMenu.AddCheckbox("high quality lod leaves", treeComputeConfig.highQualityLodLeaves);
	treeMenu.AddCheckbox("overdraw force ground", treeComputeConfig.overdrawTreeForceGround);
	treeMenu.AddSlider("cascade cull bias", treeComputeConfig.cascadeCullBias, 0.5, 1.5);
	treeMenu.TriggerNode("Compute");
	treeMenu.TriggerNode("Shader", UpdateTreeShaderData);
	treeMenu.AddSlider("weight power", treeShaderConfig.weightPower, 0, 72);
	treeMenu.AddSlider("uv scale", treeShaderConfig.uvScale, 0.0, 1.0);
	treeMenu.AddSlider("glill normal mix", treeShaderConfig.glillNormalMix, 0.0, 1.0);
	treeMenu.AddSlider("normal strength", treeShaderConfig.normalStrength, 0.0, 4.0);
	treeMenu.AddSlider("texture lod", treeShaderConfig.textureLod, 0, 4);
	treeMenu.AddSlider("ambient strength", treeShaderConfig.ambientStrength, 0.0, 1.0);
	treeMenu.AddCheckbox("sample tree occlusion", treeShaderConfig.sampleOcclusion);
	treeMenu.AddSlider("default tree occlusion", treeShaderConfig.defaultOcclusion, 0.0, 1.0);
	treeMenu.AddDropdown("debug mode", treeShaderConfig.debugMode, {"none", "base normal", "world normal", "lod"});
	treeMenu.TriggerNode("Shader");

	Menu& leafMenu = UI::NewMenu("Leaves");
	leafMenu.AddCheckbox("leaves enabled", leavesEnabled);
	leafMenu.TriggerNode("shader", UpdateLeafShaderData);
	leafMenu.AddSlider("local normal blend", leafShaderConfig.localNormalBlend, 0.0, 1.0);
	leafMenu.AddSlider("world normal height", leafShaderConfig.worldNormalHeight, 0.0, 25.0);
	leafMenu.AddSlider("translucency blend", leafShaderConfig.translucencyBlend, 0.0, 1.0);
	leafMenu.AddSlider("shadow translucency dim", leafShaderConfig.shadowTranslucencyDim, 0.95, 1.0);
	leafMenu.AddSlider("translucency bias", leafShaderConfig.translucencyBias, 0.0, 1.0);
	leafMenu.AddSlider("translucency range", leafShaderConfig.translucencyRange, 0.0, 1.0);
	leafMenu.AddSlider("lod 0 size", leafShaderConfig.lod0Size, 0.0, 2.0);
	leafMenu.AddSlider("lod 1 size", leafShaderConfig.lod1Size, 0.0, 4.0);
	leafMenu.AddSlider("lod 2 size", leafShaderConfig.lod2Size, 0.0, 7.0);
	leafMenu.AddSlider("lod 3 size", leafShaderConfig.lod3Size, 0.0, 16.0);
	leafMenu.AddSlider("lod 4 size", leafShaderConfig.lod4Size, 0.0, 32.0);
	leafMenu.AddCheckbox("lod inter modification", leafShaderConfig.lodInterMod);
	leafMenu.AddSlider("lod inter power", leafShaderConfig.lodInterPow, 0.0, 2.0);
	leafMenu.AddCheckbox("scale with tree", leafShaderConfig.scaleWithTree);
	leafMenu.AddCheckbox("sample leaves occlusion", leafShaderConfig.sampleOcclusion);
	leafMenu.AddCheckbox("flip local normal", leafShaderConfig.flipLocalNormal);
	leafMenu.AddSlider("default leaf occlusion", leafShaderConfig.defaultOcclusion, 0.0, 1.0);
	leafMenu.AddDropdown("debug mode", leafShaderConfig.debugMode, {"none", "lod", "base normal", "world normal"});
	leafMenu.AddSlider("flat local normal blend", leafShaderConfig.flatLocalNormalBlend, 0.0, 1.0);
	leafMenu.AddSlider("quality normal blend lod start", leafShaderConfig.qualityNormalBlendLodStart, 0.0, 3.0);
	leafMenu.AddSlider("quality normal blend lod power", leafShaderConfig.qualityNormalBlendLodPower, 0.0, 3.0);
	leafMenu.AddSlider("quality smoothness", leafShaderConfig.qualitySmoothness, 0.0, 1.0);
	leafMenu.AddSlider("color mult", leafShaderConfig.colorMult, 0.0, 2.0);
	leafMenu.TriggerNode("shader");

	Menu& shadowMenu = UI::NewMenu("Shadows");
	shadowMenu.TriggerNode("shadow settings", UpdateShadowData);
	shadowMenu.AddCheckbox("shadows enabled", shadowData.enabled);
	shadowMenu.AddSlider("blend distance 0", shadowData.blend0Dis, 0.0, 0.001);
	shadowMenu.AddSlider("blend distance 1", shadowData.blend1Dis, 0.0, 0.01);
	shadowMenu.AddCheckbox("test", shadowData.test);
	shadowMenu.AddSlider("distance", shadowDistance, 0.0, 250.0);
	shadowMenu.AddSlider("depth multiplier", shadowDepthMultiplier, 0.0, 32.0);
	shadowMenu.TriggerNode("shadow settings");

	UI::CreateContext(pass.GetRenderpass(), 1);

	Manager::RegisterResizeCall(Resize);

	//Improve renderer to allow call registering for specific subpasses!

	//Renderer::RegisterCall(0, RenderPre);
	Renderer::RegisterCall(shadowCascades, Render);
	Renderer::RegisterCall(shadowCascades, UI::Render);
	//Renderer::RegisterCall(1, RenderPost);
	//Renderer::RegisterCall(0, BlitFrameBuffer, true);
	Renderer::RegisterCall(0, ComputeLuminance, true);
	Renderer::RegisterCall(0, ComputeTerrainShadow, true);
	Renderer::RegisterCall(0, ComputeTerrainGlill, true);
	Renderer::RegisterCall(0, ComputeTrees, true);

	Renderer::RegisterCall(0, RenderShadows);
	Renderer::RegisterCall(1, RenderShadows2);
	Renderer::RegisterCall(2, RenderShadows3);
	if (shadowCascades > 3) {Renderer::RegisterCall(3, RenderShadows4);}

	std::cout << "rfc: " << Manager::GetSwapchain().GetFrameCount() << std::endl;

	//mat4 test = mat4::Translation(point3D(10, 5, 20));
	//std:: cout << "test: " << test << std::endl << std::endl;
	//mat4 testi = test.Inversed();
	//std:: cout << "testi: " << testi << std::endl;

	std::cout << "leaf pos count: " << leafPositions.size() << std::endl;

	std::cout << "frame buffer count: " << frameBuffers.size() << std::endl;
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
			if (!allMapsComputed) {allMapsComputed = true;}
		}
		else
		{
			temporaryComputeImages[1].CopyTo(computeImages[lod], computeCommands[Renderer::GetCurrentFrame()], false);
		}
	}

	computeCommands[Renderer::GetCurrentFrame()].End();
	computeCommands[Renderer::GetCurrentFrame()].Submit();
}

float minF(float a, float b)
{
	if (a < b) {return (a);}
	return (b);
}

float maxF(float a, float b)
{
	if (a > b) {return (a);}
	return (b);
}

mat4 ComputeShadowMatrix(float near, float far)
{
	//float distance = 75.0;

	CameraConfig cc = Manager::GetCamera().GetConfig();
	point3D pos = Manager::GetCamera().GetPosition();
	mat4 proj = mat4::Projection(cc.fov, Manager::GetCamera().GetAspect(), near, far);
	//mat4 view = mat4::Look(point3D(0.0), Manager::GetCamera().GetDirection(), point3D(0, 1, 0));
	mat4 invPV = (proj * Manager::GetCamera().GetView()).Inversed();

	std::vector<point4D> corners;
	corners.resize(8);
	corners[0] = invPV * point4D(-1.0, -1.0, 0.0, 1.0);
	corners[1] = invPV * point4D(1.0, -1.0, 0.0, 1.0);
	corners[2] = invPV * point4D(1.0, 1.0, 0.0, 1.0);
	corners[3] = invPV * point4D(-1.0, 1.0, 0.0, 1.0);
	corners[4] = invPV * point4D(-1.0, -1.0, 1.0, 1.0);
	corners[5] = invPV * point4D(1.0, -1.0, 1.0, 1.0);
	corners[6] = invPV * point4D(1.0, 1.0, 1.0, 1.0);
	corners[7] = invPV * point4D(-1.0, 1.0, 1.0, 1.0);

	point3D center = point3D(0.0);
	for (int i = 0; i < 8; i++)
	{
		corners[i] /= corners[i].w();
		//corners[i].x() = floor(corners[i].x() * 0.1) * 10.0;
		//corners[i].y() = floor(corners[i].y() * 0.1) * 10.0;
		//corners[i].z() = floor(corners[i].z() * 0.1) * 10.0;
		center += corners[i];
	}
	center *= 0.125;

	float radius = 0.0;
	for (int i = 0; i < 8; i++) {radius = maxF(radius, (corners[i] - center).Length());}

	radius = std::ceil(radius * 16.0) / 16.0;

	float extent = radius * 2.0;
	float texelSize = extent / 4096.0;

	//float step = (distance / 4096.0);
	//center.x() = floor(center.x() * 0.1) * 10.0;
	//center.y() = floor(center.y() * 0.1) * 10.0;
	//center.z() = floor(center.z() * 0.1) * 10.0;

	//center += pos;

	//center += pos;

	//point3D front = point3D(data.lightDirection.Unitized() * -1).Unitized();
	//point3D side = point3D::Cross(front, point3D(0.0, 1.0, 0.0)).Unitized();
	//point3D up = point3D::Cross(side, front).Unitized();

	//point3D eye = center + (point3D(data.lightDirection).Unitized() * far * (0.5 * shadowDepthMultiplier));
	point3D eye = center + (point3D(data.lightDirection).Unitized() * far * (0.5 * shadowDepthMultiplier));

	mat4 shadowView = mat4::Look(eye, center, point3D(0.0, 1.0, 0.0));
	//mat4 shadowView = mat4::Look(eye, eye + front, up);

	point3D centerSV = shadowView * point4D(center, 1.0);

	centerSV.x() = std::floor(centerSV.x() / texelSize) * texelSize;
	centerSV.y() = std::floor(centerSV.y() / texelSize) * texelSize;

	float left = centerSV.x() - radius;
	float right = centerSV.x() + radius;
	float bottom = centerSV.y() - radius;
	float top = centerSV.y() + radius;

	for (int i = 0; i < 8; i++) {corners[i] = shadowView * corners[i];}

	float minX = corners[0].x();
	float maxX = corners[0].x();
	float minY = corners[0].y();
	float maxY = corners[0].y();
	float minZ = corners[0].z();
	float maxZ = corners[0].z();

	for (int i = 1; i < 8; i++)
	{
		minX = minF(minX, corners[i].x());
		maxX = maxF(maxX, corners[i].x());
		minY = minF(minY, corners[i].y());
		maxY = maxF(maxY, corners[i].y());
		minZ = minF(minZ, corners[i].z());
		maxZ = maxF(maxZ, corners[i].z());
	}

	/*if (Time::newSecond)
	{
		//std::cout << "mid: " << (invPV * point4D(0.0, 0.0, 0.0, 1.0)) << std::endl;
		std::cout << "cen: " << center << std::endl;
		std::cout << "diff: " << center - pos << std::endl;
		std::cout << "minX: " << minX << std::endl;
		std::cout << "maxX: " << maxX << std::endl;
		std::cout << "minY: " << minY << std::endl;
		std::cout << "maxY: " << maxY << std::endl;
		std::cout << "minZ: " << minZ << std::endl;
		std::cout << "maxZ: " << maxZ << std::endl << std::endl;
	}*/

	float xb = maxX - minX;
	float yb = maxY - minY;
	float zb = maxZ - minZ;

	//mat4 shadowOrtho = mat4::Orthographic(minX, maxX, minY, maxY, minZ, maxZ);
	mat4 shadowOrtho = mat4::Orthographic(-xb * 0.5, xb * 0.5, -yb * 0.5, yb * 0.5, 0.0, zb * shadowDepthMultiplier);
	//mat4 shadowOrtho = mat4::Orthographic(centerSV.x() + minX, centerSV.x() + maxX, centerSV.y() + minY, centerSV.y() + maxY, centerSV.z() + minZ, centerSV.z() + maxZ);
	//mat4 shadowOrtho = mat4::Orthographic(-radius, radius, -radius, radius, 0.0, zb * 2.0);
	//mat4 shadowOrtho = mat4::Orthographic(left, right, bottom, top, 0.0, zb * 2.0);
	//mat4 shadowOrtho = mat4::Orthographic(minX, maxX, minY, maxY, 0.0, maxZ);

	mat4 result = (shadowOrtho * shadowView);

	return (result);
}

static bool flying = true;

bool testMove = false;

void Frame()
{
	//static double frameTime = 0;
	//static double frameTimeCount = 0;

	if (Input::GetKey(GLFW_KEY_M).pressed)
	{
		Input::TriggerMouse();
	}

	if (Input::GetKey(GLFW_KEY_F).pressed) {flying = !flying;}

	if (!flying)
	{
		float cameraHeight = (Manager::GetCamera().GetPosition() + data.terrainOffset * 10000.0).y();
		float cameraTerrainHeight = (*reinterpret_cast<RetrieveData*>(retrieveBuffer.GetAddress())).viewTerrainHeight;

		Manager::GetCamera().Move(point3D(0.0, cameraTerrainHeight - cameraHeight + 1.75, 0.0));
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
		//std::cout << "Camera position: " << Manager::GetCamera().GetPosition() << std::endl;
		std::cout << "Camera Rotation: " << Manager::GetCamera().GetAngles() << std::endl;
		std::cout << "Camera Direction: " << Manager::GetCamera().GetDirection() << std::endl;
		std::cout << "Light direction: " << data.lightDirection << std::endl;
	}

	static float sunSpeed = 1.0f;

	if (Input::GetKey(GLFW_KEY_L).pressed) {sunSpeed = sunSpeed == 1.0f ? 0.2f : 1.0f;}

	point2D angles = point3D(data.lightDirection).Angles() * -57.2957795;
	if (Input::GetKey(GLFW_KEY_RIGHT).down) {angles.y() += Time::deltaTime * -45.0f * sunSpeed;}
	else if (Input::GetKey(GLFW_KEY_LEFT).down) {angles.y() += Time::deltaTime * 45.0f * sunSpeed;}
	else if (Input::GetKey(GLFW_KEY_UP).down) {angles.x() += Time::deltaTime * -45.0f * sunSpeed;}
	else if (Input::GetKey(GLFW_KEY_DOWN).down) {angles.x() += Time::deltaTime * 45.0f * sunSpeed;}
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

		shouldComputeTrees = true;
	}

	if (fabs(Manager::GetCamera().GetPosition().y()) > terrainResetDis)
	{
		float camOffset = ((int(Manager::GetCamera().GetPosition().y()) / int(terrainResetDis)) * terrainResetDis);
		data.terrainOffset.y() += camOffset / 10000.0f;
		Manager::GetCamera().Move(point3D(0, -camOffset, 0));

		shouldComputeTrees = true;
	}

	if (fabs(Manager::GetCamera().GetPosition().z()) > terrainResetDis)
	{
		float camOffset = ((int(Manager::GetCamera().GetPosition().z()) / int(terrainResetDis)) * terrainResetDis);
		data.terrainOffset.z() += camOffset / 10000.0f;
		Manager::GetCamera().Move(point3D(0, 0, -camOffset));

		shouldComputeTrees = true;
	}

	//if (Time::newTick)
	//{
	//	double fps = frameTime / frameTimeCount;
	//	glfwSetWindowTitle(Manager::GetWindow().GetData(), std::to_string(int(1.0 / fps)).c_str());
	//	frameTime = 0;
	//	frameTimeCount = 0;
	//}
	//frameTime += Time::deltaTime;
	//frameTimeCount += 1;

	/*point3D pos = Manager::GetCamera().GetPosition() + data.lightDirection.Unitized() * 250.0;
	//float step = (300.0 / 2048.0);
	//pos.x() = floor(pos.x() / step) * step;
	//pos.y() = floor(pos.y() / step) * step;
	//pos.z() = floor(pos.z() / step) * step;
	pos.x() = floor(pos.x() * 0.1) * 10.0;
	pos.y() = floor(pos.y() * 0.1) * 10.0;
	pos.z() = floor(pos.z() * 0.1) * 10.0;
	//pos.x() = floor(pos.x());
	//pos.y() = floor(pos.y());
	//pos.z() = floor(pos.z());

	point3D front = point3D(data.lightDirection.Unitized() * -1).Unitized();
	point3D side = point3D::Cross(front, point3D(0.0, 1.0, 0.0)).Unitized();
	point3D up = point3D::Cross(side, front).Unitized();
	
	mat4 shadowView = mat4::Look(pos, pos + front, up);

	mat4 shadowOrtho = mat4::Orthographic(-250.0, 250.0, -250.0, 250.0, 0.0, 500.0);*/

	//if (Input::GetKey(GLFW_KEY_X).pressed) {testMove = !testMove;}
	//if (testMove)
	//{
	//	Manager::GetCamera().Move(Manager::GetCamera().GetDirection() * Time::deltaTime * Manager::GetCamera().GetConfig().speed);
	//	Manager::GetCamera().Move(Manager::GetCamera().GetRight() * Time::deltaTime * Manager::GetCamera().GetConfig().speed);
	//}

	//Manager::GetCamera().Rotate(point3D(0.0, 45.0, 0.0) * Time::deltaTime);
	//Manager::GetCamera().Rotate(point3D(0.0, 45.0, 0.0) * Time::deltaTime * (sin(Time::GetCurrentTime() * 4.0) > 0.0 ? 1.0 : -1.0));

	//Manager::GetCamera().Frame();
	Manager::GetCamera().UpdateView();
	data.view = Manager::GetCamera().GetView();
	data.projection = Manager::GetCamera().GetProjection();
	//data.shadowMatrix = shadowOrtho * shadowView;
	data.viewPosition = Manager::GetCamera().GetPosition();
	data.viewDirection = Manager::GetCamera().GetDirection();

	data.shadowMatrices[0] = ComputeShadowMatrix(0.1, shadowDistance);
	data.shadowMatrices[1] = ComputeShadowMatrix(shadowDistance, shadowDistance * 5.0);
	data.shadowMatrices[2] = ComputeShadowMatrix(shadowDistance * 5.0, shadowDistance * 25.0);
	if (shadowCascades > 3) {data.shadowMatrices[3] = ComputeShadowMatrix(shadowDistance * 25.0, shadowDistance * 125.0);}

	data.resolution.x() = Manager::GetCamera().GetConfig().width;
	data.resolution.y() = Manager::GetCamera().GetConfig().height;

	//if (Input::GetKey(GLFW_KEY_E).pressed)
	//{
	//	data.resolution.z() = 1.0 - data.resolution.z();
	//}

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
		if (allMapsComputed && (fabs(data.terrainOffset.x() + (Manager::GetCamera().GetPosition().x() / 10000.0f) - data.shadowmapOffsets[i].x()) > data.shadowmapOffsets[i].w() * 0.0001 * 0.125 || 
			fabs(data.terrainOffset.z() + (Manager::GetCamera().GetPosition().z() / 10000.0f) - data.shadowmapOffsets[i].z()) > data.shadowmapOffsets[i].w() * 0.0001 * 0.125))
		{
			SetTerrainShadowValues(i);
		}
	}

	/*if (Input::GetKey(GLFW_KEY_APOSTROPHE).pressed)
	{
		float val = 500.0; 

		for (int i = 0; i < glillDatas.size(); i++)
		{
			glillDatas[i].settings.z() = 8.0;
			glillBuffers[i].Update(&glillDatas[i], sizeof(GlillData));
			glillQueue[i] = true;
		}
	}

	if (Input::GetKey(GLFW_KEY_SEMICOLON).pressed)
	{
		float val = 200.0; 

		for (int i = 0; i < glillDatas.size(); i++)
		{
			glillDatas[i].settings.z() = 4.0;
			glillBuffers[i].Update(&glillDatas[i], sizeof(GlillData));
			glillQueue[i] = true;
		}
	}*/

	for (int i = glillDatas.size() - 1; i >= 0; i--)
	{
		if (allMapsComputed && (glillQueue[i] || fabs(data.terrainOffset.x() + (Manager::GetCamera().GetPosition().x() / 10000.0f) - data.glillmapOffsets[i].x()) > data.glillmapOffsets[i].w() * 0.0001 * 0.125 || 
			fabs(data.terrainOffset.z() + (Manager::GetCamera().GetPosition().z() / 10000.0f) - data.glillmapOffsets[i].z()) > data.glillmapOffsets[i].w() * 0.0001 * 0.125))
		{
			data.glillmapOffsets[i].x() = data.terrainOffset.x() + (Manager::GetCamera().GetPosition().x() / 10000.0f);
			data.glillmapOffsets[i].z() = data.terrainOffset.z() + (Manager::GetCamera().GetPosition().z() / 10000.0f);

			glillDatas[i].offset = data.glillmapOffsets[i];
			glillDatas[i].globalSamplePower = globalGlillSamplePower;
			glillDatas[i].treeDensityStrength = globalTreeDensityStrength;
			glillDatas[i].treeDensityPower = globalTreeDensityPower;
			if (i != 0)
			{
				glillDatas[i].treeDensityStrength = -1.0;
				glillDatas[i].treeDensityPower = -1.0;
			}
			glillBuffers[i].Update(&glillDatas[i], sizeof(GlillData));

			glillQueue[i] = true;
		}
	}

	if (Input::GetKey(GLFW_KEY_I).pressed)
	{
		for (int i = 0; i < glillDatas.size(); i++)
		{
			data.glillmapOffsets[i].y() = (1.0 - data.glillmapOffsets[i].y());
		}
	}

	if (Input::GetKey(GLFW_KEY_V).pressed)
	{
		CameraConfig cameraConfig = Manager::GetCamera().GetConfig();
		cameraConfig.fov = cameraConfig.fov == 60.0 ? 75.0 : 60.0;
		Manager::GetCamera().SetConfig(cameraConfig);
		Manager::GetCamera().UpdateProjection();
		data.projection = Manager::GetCamera().GetProjection();
		//Manager::GetCamera().UpdateProjection();
	}

	if (Input::GetKey(GLFW_KEY_U).pressed)
	{
		//leafShaderConfig.qualityNormalBlendLodPower = 3.0;
		//leafShaderConfig.lod3Size = 12.5;
		//leafShaderConfig.lod4Size = 20.0;
		//UpdateLeafShaderData();

		//terrainComputeData.erodeFactor = 3.0;
		//terrainComputeData.steepness = 1.25;
		//terrainShaderData.rockSteepness = 0.1;
		//terrainShaderData.rockTransition = 0.025;
		//terrainShaderData.snowHeight = 1250.0;
		//UpdateTerrainShaderData();

		//treeComputeConfig.overdrawLodCullHeavy = 1;
		//treeComputeConfig.overdrawMisses = 1;
		//UpdateTreeComputeData();

		//atmosphereData.mistStrength = 32.0;
		//UpdateAtmosphereData();

		//atmosphereData.skyPower = 1.0;
		//skyData.rayleighStrength = 1.0;
		//UpdateAtmosphereData();
		//UpdateSkyData();

		//terrainComputeData.erodeFactor = 2.5;
		//UpdateTerrainComputeData();

		//postData.toneMapping = 0;
		//UpdatePostData();
		//leafShaderConfig.colorMult = 0.75;
		//UpdateLeafShaderData();

		atmosphereData.mieScatteringStrength = 2.0;
		UpdateAtmosphereData();
	}
	if (Input::GetKey(GLFW_KEY_Y).pressed)
	{
		//leafShaderConfig.qualityNormalBlendLodPower = 1.0;
		//leafShaderConfig.lod3Size = 14.0;
		//leafShaderConfig.lod4Size = 24.0;
		//UpdateLeafShaderData();

		//terrainComputeData.erodeFactor = 2.0;
		//terrainComputeData.steepness = 1.5;
		//terrainShaderData.rockSteepness = 0.1;
		//terrainShaderData.rockTransition = 0.075;
		//terrainShaderData.snowHeight = 1500.0;
		//UpdateTerrainShaderData();

		//atmosphereData.skyPower = 2.0;
		//skyData.rayleighStrength = 0.25;
		//UpdateAtmosphereData();
		//UpdateSkyData();

		//terrainComputeData.erodeFactor = 3.0;
		//UpdateTerrainComputeData();

		//treeComputeConfig.overdrawLodCullHeavy = 0;
		//treeComputeConfig.overdrawMisses = 0;
		//UpdateTreeComputeData();

		//atmosphereData.mistStrength = 24.0;
		//UpdateAtmosphereData();

		//postData.toneMapping = 1;
		//UpdatePostData();
		//leafShaderConfig.colorMult = 1.0;
		//UpdateLeafShaderData();

		atmosphereData.mieScatteringStrength = 1.0;
		UpdateAtmosphereData();
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
	treeMesh.Destroy();
	leafMesh.Destroy();

	for (Buffer& buffer : frameBuffers) { buffer.Destroy(); }
	frameBuffers.clear();

	for (Buffer& buffer : objectBuffers) { buffer.Destroy(); }
	objectBuffers.clear();

	for (Buffer& buffer : computeBuffers) { buffer.Destroy(); }
	computeBuffers.clear();

	for (Buffer& buffer : terrainShadowBuffers) { buffer.Destroy(); }
	terrainShadowBuffers.clear();

	for (Buffer& buffer : glillBuffers) { buffer.Destroy(); }
	glillBuffers.clear();

	//for (Buffer& buffer : treeDataBuffers) { buffer.Destroy(); }
	//treeDataBuffers.clear();

	//for (Buffer& buffer : treeDrawBuffers) { buffer.Destroy(); }
	//treeDrawBuffers.clear();

	atmosphereBuffer.Destroy();
	aerialBuffer.Destroy();
	scatteringBuffer.Destroy();
	skyBuffer.Destroy();
	terrainComputeBuffer.Destroy();
	terrainShaderBuffer.Destroy();
	postBuffer.Destroy();
	retrieveBuffer.Destroy();
	treeSetupDataBuffer.Destroy();
	treeRenderDataBuffer.Destroy();
	treeShadowRenderDataBuffer.Destroy();
	treeDrawBuffer.Destroy();
	treeShadowDrawBuffer.Destroy();
	leafDrawBuffer.Destroy();
	leafShadowDrawBuffer.Destroy();
	treeComputeConfigBuffer.Destroy();
	treeShaderConfigBuffer.Destroy();
	leafShaderConfigBuffer.Destroy();
	shadowDataBuffer.Destroy();
	leafPositionsBuffer.Destroy();
	leafLodPositionsBuffer.Destroy();

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

	for (Image& image : glillImages) { image.Destroy(); }
	glillImages.clear();

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
	bark_diff.Destroy();
	bark_norm.Destroy();
	bark_arm.Destroy();

	pass.Destroy();
	postPass.Destroy();
	shadowPass.Destroy();

	frameDescriptor.Destroy();
	materialDescriptor.Destroy();
	objectDescriptor.Destroy();
	postDescriptor.Destroy();
	luminanceDescriptor.Destroy();
	atmosphereDescriptor.Destroy();
	aerialDescriptor.Destroy();
	scatteringDescriptor.Destroy();
	skyDescriptor.Destroy();
	terrainShadowDescriptor.Destroy();
	glillDescriptor.Destroy();
	computeDescriptor.Destroy();
	retrieveDescriptor.Destroy();
	treeComputeDescriptor.Destroy();
	treeDescriptor.Destroy();
	leafDescriptor.Destroy();
	pipeline.Destroy();
	prePipeline.Destroy();
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
	retrievePipeline.Destroy();
	treeSetupComputePipeline.Destroy();
	treeRenderComputePipeline.Destroy();
	treePipeline.Destroy();
	treeShadowPipeline.Destroy();
	leafPipeline.Destroy();
	leafShadowPipeline.Destroy();

	//UI::DestroyContext();
}

int main(int argc, char** argv)
{
	Manager::ParseArguments(argv, argc);
	Manager::GetConfig().deviceConfig.anisotropic = true;
	Manager::GetConfig().deviceConfig.tesselation = true;
	Manager::GetConfig().deviceConfig.multiDrawIndirect = true;
	Manager::GetConfig().deviceConfig.synchronization2 = true;
	Manager::GetConfig().deviceConfig.nonUniformIndexingShaderSampledImageArray = true;
	Manager::GetConfig().framesInFlight = 1;
	//Manager::GetConfig().uncapped = true;
	//Manager::GetConfig().wireframe = true;

	Manager::Create();

	CameraConfig cameraConfig = Manager::GetCamera().GetConfig();
	cameraConfig.near = 0.1;
	cameraConfig.far = 50000.0;
	//cameraConfig.fov = 60;
	cameraConfig.fov = 75;
	Manager::GetCamera().SetConfig(cameraConfig);

	Manager::RegisterStartCall(Start);
	Manager::RegisterFrameCall(Frame);
	//Manager::RegisterFrameCall(UI::Frame);
	Manager::RegisterEndCall(End);

	Manager::Run();

	Manager::Destroy();

	exit(EXIT_SUCCESS);
}