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
};

UniformData data{};
std::vector<mat4> models(1);
std::vector<Buffer> frameBuffers;
std::vector<Buffer> objectBuffers;

Pass pass;

Pipeline pipeline;
meshP16 planeMesh;
meshP16 planeLodMesh;
meshP16 planeLod2Mesh;

//Pipeline screenPipeline;
//meshP16 quadMesh;

Pipeline computePipeline;
Descriptor computeDescriptor;

std::vector<Image> computeImages(computeCascade);
std::vector<Image> temporaryComputeImages(2);

//Image computeImage;
//Image computeImageLod;
//Image computeImageLod2;
//Image computeImageLod3;
//Image computeImageLod4;
//Image computeImageLod5;
//Image computeImageLod6;

std::vector<Buffer> computeBuffers;
std::vector<point4D> computeDatas;
std::vector<VkSemaphore> computeSemaphores;
std::vector<Command> computeCommands;
std::vector<Command> computeCopyCommands;

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

//VkQueryPool timestampPool;
//int queryCount = 2;
//double nsPerTick;
//bool timeStamped = false;

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

void Render(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
	frameDescriptor.BindDynamic(0, commandBuffer, pipeline);
	materialDescriptor.Bind(0, commandBuffer, pipeline);

	pipeline.Bind(commandBuffer);

	//int centerIndex = 3 * 7 + 3;
	planeMesh.Bind(commandBuffer);
	objectDescriptor.BindDynamic(0, commandBuffer, pipeline, 0 * sizeof(mat4));
	vkCmdDrawIndexed(commandBuffer, planeMesh.GetIndices().size(), 1, 0, 0, 0);


	//objectDescriptor.BindDynamic(0, commandBuffer, pipeline, 0);
	//vkCmdDrawIndexed(commandBuffer, planeMesh.GetIndices().size(), 1, 0, 0, 0);

	planeLodMesh.Bind(commandBuffer);
	vkCmdDrawIndexed(commandBuffer, planeLodMesh.GetIndices().size(), terrainCount - 1, 0, 0, 1);

	planeLod2Mesh.Bind(commandBuffer);
	vkCmdDrawIndexed(commandBuffer, planeLod2Mesh.GetIndices().size(), terrainLodCount - terrainCount, 0, 0, terrainCount);
	//vkCmdDrawIndexed(commandBuffer, planeLodMesh.GetIndices().size(), terrainCount - 1, 0, 0, 1);
	//for (size_t i = 0; i < models.size(); i++)
	//{
	//	if (i == centerIndex) continue;
	//	objectDescriptor.BindDynamic(0, commandBuffer, pipeline, i * sizeof(mat4));
	//	vkCmdDrawIndexed(commandBuffer, planeLodMesh.GetIndices().size(), 1, 0, 0, 0);
	//}

	//screenPipeline.Bind(commandBuffer);
	//objectDescriptor.BindDynamic(0, commandBuffer, screenPipeline, 1 * sizeof(mat4));
	//quadMesh.Bind(commandBuffer);
	//vkCmdDrawIndexed(commandBuffer, quadMesh.GetIndices().size(), 1, 0, 0, 0);
}

void Start()
{
	PassConfig passConfig = Pass::DefaultConfig(true);
	pass.Create(passConfig);

	PassInfo passInfo{};
	passInfo.pass = &pass;
	passInfo.useWindowExtent = true;

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

	//quadMesh.Create(ShapeType::Quad);

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
	data.lightDirection = point4D(point3D(0.2, 0.25, -0.4).Unitized());
	data.resolution = point4D(Manager::GetCamera().GetConfig().width, Manager::GetCamera().GetConfig().height, 
		Manager::GetCamera().GetConfig().near, Manager::GetCamera().GetConfig().far);
	data.terrainOffset = point4D(0.0);
	//data.terrainOffset.w() = 15;
	data.terrainOffset.w() = 0;

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

	std::vector<DescriptorConfig> frameDescriptorConfigs(2);
	frameDescriptorConfigs[0].type = DescriptorType::UniformBuffer;
	frameDescriptorConfigs[0].stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | 
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
	frameDescriptorConfigs[1].type = DescriptorType::CombinedSampler;
	frameDescriptorConfigs[1].stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | 
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
	frameDescriptorConfigs[1].count = computeCascade;
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
	for (int i = 0; i < Renderer::GetFrameCount(); i++) frameDescriptor.Update(i, 1, Utilities::Pointerize(computeImages));

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

	//PipelineConfig screenPipelineConfig = Pipeline::DefaultConfig();
	//screenPipelineConfig.shader = "screen";
	//screenPipelineConfig.vertexInfo = quadMesh.GetVertexInfo();
	//screenPipelineConfig.renderpass = pass.GetRenderpass();
	//screenPipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), materialDescriptor.GetLayout(), objectDescriptor.GetLayout() };
	//screenPipelineConfig.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	//screenPipelineConfig.dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
	//screenPipelineConfig.rasterization.cullMode = VK_CULL_MODE_NONE;
	//screenPipeline.Create(screenPipelineConfig);

	PipelineConfig computePipelineConfig{};
	computePipelineConfig.shader = "heightmap";
	computePipelineConfig.type = PipelineType::Compute;
	computePipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), computeDescriptor.GetLayout() };
	computePipeline.Create(computePipelineConfig);

	//Manager::GetCamera().Move(point3D(-5000, 2500, 5000));
	Manager::GetCamera().Move(point3D(0, 0, 0));
	//Manager::GetCamera().Move(point3D(7523.26, 643.268, 518.602));
	//Manager::GetCamera().Move(point3D(0, 10, 0));

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

	computeCopyCommands.resize(Renderer::GetFrameCount());
	for (int i = 0; i < Renderer::GetFrameCount(); i++)
	{
		CommandConfig commandConfig{};
		commandConfig.queueIndex = Manager::GetDevice().GetQueueIndex(QueueType::Graphics);
		commandConfig.wait = false;
		commandConfig.waitSemaphores = {computeSemaphores[i]};
		commandConfig.waitDestinations = {VK_PIPELINE_STAGE_TRANSFER_BIT};
		commandConfig.signalSemaphores = {computeSemaphores[i]};
		computeCopyCommands[i].Create(commandConfig, i, &Manager::GetDevice());
	}

	/*VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(Manager::GetDevice().GetPhysicalDevice(), &props);
	nsPerTick = props.limits.timestampPeriod;

	VkQueryPoolCreateInfo timestampPoolInfo{};
	timestampPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	timestampPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
	timestampPoolInfo.queryCount = queryCount;

	vkCreateQueryPool(Manager::GetDevice().GetLogicalDevice(), &timestampPoolInfo, nullptr, &timestampPool);

	//vkResetQueryPool(Manager::GetDevice().GetLogicalDevice(), timestampPool, 0, queryCount);*/

	Renderer::RegisterCall(0, Render);
}

void Compute(int lod)
{
	Renderer::AddFrameSemaphore(computeSemaphores[Renderer::GetCurrentFrame()], VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);

	computeBuffers[lod].Update(&computeDatas[lod], sizeof(point4D));

	//CommandConfig commandConfig{};
	////commandConfig.wait = false;
	//Command computeCommand(commandConfig);
	computeCommands[Renderer::GetCurrentFrame()].Begin();

	//vkCmdResetQueryPool(computeCommands[Renderer::GetCurrentFrame()].GetBuffer(), timestampPool, 0, queryCount);
	//vkCmdWriteTimestamp(computeCommands[Renderer::GetCurrentFrame()].GetBuffer(), 
	//	VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, timestampPool, 0);

	frameDescriptor.BindDynamic(0, computeCommands[Renderer::GetCurrentFrame()].GetBuffer(), computePipeline);

	computeDescriptor.Bind(lod, computeCommands[Renderer::GetCurrentFrame()].GetBuffer(), computePipeline);
	computePipeline.Bind(computeCommands[Renderer::GetCurrentFrame()].GetBuffer());
	//vkCmdDispatch(computeCommands[Renderer::GetCurrentFrame()].GetBuffer(), (heightmapResolution) / 8, (heightmapResolution) / 8, 1);
	vkCmdDispatch(computeCommands[Renderer::GetCurrentFrame()].GetBuffer(), (heightmapResolution / int(pow(2, computeIterations))) / 8, 
		(heightmapResolution / int(pow(2, computeIterations))) / 8, 1);

	//vkCmdWriteTimestamp(computeCommands[Renderer::GetCurrentFrame()].GetBuffer(), 
	//	VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, timestampPool, 1);

	computeCommands[Renderer::GetCurrentFrame()].End();
	computeCommands[Renderer::GetCurrentFrame()].Submit();

	if (computeIterations > 0 && computeDatas[lod].w() == (totalComputeIterations - 1)) 
	{
		if (lod == 0)
		{
			temporaryComputeImages[0].CopyTo(computeImages[lod], computeCopyCommands[Renderer::GetCurrentFrame()]);
		}
		else
		{
			temporaryComputeImages[1].CopyTo(computeImages[lod], computeCopyCommands[Renderer::GetCurrentFrame()]);
		}
	}

	//timeStamped = true;

	//std::cout << "Compute shader executed at: " << Time::GetCurrentTime() << std::endl;
}

void Frame()
{
	//static int lodMoved = 0;
	//static bool created = false;
	static double frameTime = 0;
	static double frameTimeCount = 0;

	if (Input::GetKey(GLFW_KEY_M).pressed)
	{
		Input::TriggerMouse();
	}

	/*if (timeStamped)
	{
		std::vector<uint64_t> timestamps(queryCount);
		vkGetQueryPoolResults(Manager::GetDevice().GetLogicalDevice(), timestampPool, 0, queryCount, 
			sizeof(uint64_t) * queryCount, timestamps.data(), sizeof(uint64_t), 
			VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

		double timestampDuration  = (timestamps[1] - timestamps[0]) * nsPerTick / 1e6;
		std::cout << "Compute duration: " << timestampDuration << std::endl;

		//vkResetQueryPool(Manager::GetDevice().GetLogicalDevice(), timestampPool, 0, queryCount);
	}

	timeStamped = false;*/

	if (Input::GetKey(GLFW_KEY_C).pressed)
	{
		//std::cout << Manager::GetCamera().GetPosition() + data.terrainOffset * 10000.0 << std::endl;
		std::cout << Manager::GetCamera().GetPosition().y() << std::endl;
	}

	if (Input::GetKey(GLFW_KEY_UP).pressed) data.terrainOffset.w() += 1;
	if (Input::GetKey(GLFW_KEY_DOWN).pressed) data.terrainOffset.w() -= 1;
	data.terrainOffset.w() = std::clamp(data.terrainOffset.w(), 0.0f, 2.0f);

	//if (std::round(fabs(Manager::GetCamera().GetPosition().x())) > (heightmapBaseSize * 10000.0f * 0.125f))
	if (fabs(Manager::GetCamera().GetPosition().x()) > terrainResetDis)
	{
		//float camOffset = std::round(Manager::GetCamera().GetPosition().x()) / 10000.0f;
		float camOffset = ((int(Manager::GetCamera().GetPosition().x()) / int(terrainResetDis)) * terrainResetDis);
		data.terrainOffset.x() += camOffset / 10000.0f;
		Manager::GetCamera().Move(point3D(-camOffset, 0, 0));
		//lodMoved = 0;

		//std::cout << "camOffset: " << data.terrainOffset << std::endl;
	}
	//if (std::round(fabs(Manager::GetCamera().GetPosition().z())) > (heightmapBaseSize * 10000.0f * 0.125f))
	if (fabs(Manager::GetCamera().GetPosition().z()) > terrainResetDis)
	{
		//float camOffset = std::round(Manager::GetCamera().GetPosition().z()) / 10000.0f;
		float camOffset = ((int(Manager::GetCamera().GetPosition().z()) / int(terrainResetDis)) * terrainResetDis);
		data.terrainOffset.z() += camOffset / 10000.0f;
		Manager::GetCamera().Move(point3D(0, 0, -camOffset));
		//lodMoved = 0;
	}

	//if (abs((data.heightmapOffsets[0].x() * 10000.0f) - Manager::GetCamera().GetPosition().x()) > 25.0)
	//{
	//	float camOffset = std::round(Manager::GetCamera().GetPosition().x()) / 10000.0f;
	//	data.heightmapOffsets[0].x() = camOffset;
	//	lodMoved = 0;
	//}
	//if (abs((data.heightmapOffsets[0].y() * 10000.0f) - Manager::GetCamera().GetPosition().z()) > 25.0)
	//{
	//	float camOffset = std::round(Manager::GetCamera().GetPosition().z()) / 10000.0f;
	//	data.heightmapOffsets[0].y() = camOffset;
	//	lodMoved = 0;
	//}

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
			//if (fabs(data.terrainOffset.x() - data.heightmapOffsets[i].x()) > (heightmapBaseSize * pow(2.0, i)) * 0.125 || 
			//	fabs(data.terrainOffset.z() - data.heightmapOffsets[i].y()) > (heightmapBaseSize * pow(2.0, i)) * 0.125)
			if (fabs(data.terrainOffset.x() + (Manager::GetCamera().GetPosition().x() / 10000.0f) - data.heightmapOffsets[i].x()) > (heightmapBaseSize * pow(2.0, i)) * 0.125 || 
				fabs(data.terrainOffset.z() + (Manager::GetCamera().GetPosition().z() / 10000.0f) - data.heightmapOffsets[i].y()) > (heightmapBaseSize * pow(2.0, i)) * 0.125)
			{
				currentLod = i;
				//computeDatas[currentLod].y() = data.terrainOffset.x();
				//computeDatas[currentLod].z() = data.terrainOffset.z();
				computeDatas[currentLod].y() = data.terrainOffset.x() + (Manager::GetCamera().GetPosition().x() / 10000.0f);
				computeDatas[currentLod].z() = data.terrainOffset.z() + (Manager::GetCamera().GetPosition().z() / 10000.0f);
				break;
			}
		}
	}

	//if (lodMoved <= 5)
	if (currentLod >= 0 && (computeIterations == 0 || computeDatas[currentLod].w() == (totalComputeIterations - 1)))
	{
		//data.heightmapOffsets[currentLod].x() = data.terrainOffset.x();
		//data.heightmapOffsets[currentLod].y() = data.terrainOffset.z();
		data.heightmapOffsets[currentLod].x() = computeDatas[currentLod].y();
		data.heightmapOffsets[currentLod].y() = computeDatas[currentLod].z();
	}

	frameBuffers[Renderer::GetCurrentFrame()].Update(&data, sizeof(data));

	objectBuffers[Renderer::GetCurrentFrame()].Update(models.data(), sizeof(mat4) * models.size());

	//if (lodMoved <= 5)
	if (currentLod >= 0)
	{
		//data.heightmapOffsets[lodMoved].x() = data.terrainOffset.x();
		//data.heightmapOffsets[lodMoved].y() = data.terrainOffset.z();
		Compute(currentLod);
		//lodMoved = lodMoved + 1;
		//Compute(0);
		//Compute(1);
		//Compute(2);
		//Compute(3);
		//Compute(4);

		//if (!created && lodMoved >= 6) created = true;
	}
}

void End()
{
	//vkDestroyQueryPool(Manager::GetDevice().GetLogicalDevice(), timestampPool, nullptr);

	planeMesh.Destroy();
	planeLodMesh.Destroy();
	planeLod2Mesh.Destroy();
	//quadMesh.Destroy();

	for (Buffer& buffer : frameBuffers) { buffer.Destroy(); }
	frameBuffers.clear();

	for (Buffer& buffer : objectBuffers) { buffer.Destroy(); }
	objectBuffers.clear();

	for (Buffer& buffer : computeBuffers) { buffer.Destroy(); }
	computeBuffers.clear();

	for (Image& image : computeImages) { image.Destroy(); }
	computeImages.clear();

	for (Image& image : temporaryComputeImages) { image.Destroy(); }
	temporaryComputeImages.clear();

	//temporaryComputeImage.Destroy();

	for (VkSemaphore& semaphore : computeSemaphores) {vkDestroySemaphore(Manager::GetDevice().GetLogicalDevice(), semaphore, nullptr);}
	computeSemaphores.clear();

	for (Command& command : computeCommands) {command.Destroy();}
	computeCommands.clear();

	for (Command& command : computeCopyCommands) {command.Destroy();}
	computeCopyCommands.clear();

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
	frameDescriptor.Destroy();
	materialDescriptor.Destroy();
	objectDescriptor.Destroy();
	computeDescriptor.Destroy();
	pipeline.Destroy();
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