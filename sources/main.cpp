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

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

struct UniformData
{
	mat4 view;
	mat4 projection;
	point4D viewPosition;
	point4D lightDirection;
	point4D resolution;
};

UniformData data{};
std::vector<mat4> models(1);
std::vector<Buffer> frameBuffers;
std::vector<Buffer> objectBuffers;

Pass pass;

Pipeline pipeline;
meshP32 planeMesh;

//Pipeline screenPipeline;
//meshP16 quadMesh;

//Pipeline computePipeline;
//Descriptor computeDescriptor;
//Image computeImage;

Descriptor frameDescriptor;
Descriptor materialDescriptor;
Descriptor objectDescriptor;

Image mud_diff;
Image mud_norm;
Image mud_arm;
Image grass_diff;
Image grass_norm;
Image grass_arm;

void Render(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
	frameDescriptor.BindDynamic(0, commandBuffer, pipeline);
	materialDescriptor.Bind(0, commandBuffer, pipeline);

	pipeline.Bind(commandBuffer);
	objectDescriptor.BindDynamic(0, commandBuffer, pipeline, 0);
	planeMesh.Bind(commandBuffer);
	vkCmdDrawIndexed(commandBuffer, planeMesh.GetIndices().size(), 1, 0, 0, 0);

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
	shapeSettings.resolution = 128;
	shapeP32 planeShape(ShapeType::Plane, shapeSettings);
	planeMesh.Create(planeShape);

	//quadMesh.Create(ShapeType::Quad);

	//ImageConfig imageStorageConfig = Image::DefaultStorageConfig();
	//imageStorageConfig.width = 1024;
	//imageStorageConfig.height = 1024;
	//imageStorageConfig.samplerConfig.repeatMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	//computeImage.Create(imageStorageConfig);

	ImageConfig imageConfig = Image::DefaultConfig();
	imageConfig.createMipmaps = true;
	imageConfig.samplerConfig.anisotropyEnabled = VK_TRUE;
	imageConfig.samplerConfig.maxAnisotropy = 8;
	ImageConfig imageNormalConfig = Image::DefaultNormalConfig();
	imageNormalConfig.createMipmaps = true;
	imageNormalConfig.samplerConfig.anisotropyEnabled = VK_TRUE;
	imageNormalConfig.samplerConfig.maxAnisotropy = 8;

	std::vector<ImageLoader*> loaders = ImageLoader::LoadImages({
		{"rock_diff", ImageType::Jpg},
		{"rock_norm", ImageType::Jpg},
		{"rock_arm", ImageType::Jpg},
		{"rocky_terrain_diff", ImageType::Jpg},
		{"rocky_terrain_norm", ImageType::Jpg},
		{"rocky_terrain_arm", ImageType::Jpg},
	});

	mud_diff.Create(*loaders[0], imageConfig);
	mud_norm.Create(*loaders[1], imageNormalConfig);
	mud_arm.Create(*loaders[2], imageNormalConfig);
	grass_diff.Create(*loaders[3], imageConfig);
	grass_norm.Create(*loaders[4], imageNormalConfig);
	grass_arm.Create(*loaders[5], imageNormalConfig);

	for (size_t i = 0; i < loaders.size(); i++) {delete (loaders[i]);}
	loaders.clear();

	data.projection = Manager::GetCamera().GetProjection();
	data.lightDirection = point4D(point3D(0.2, 0.5, -0.4).Unitized());
	data.resolution = point4D(Manager::GetCamera().GetConfig().width, Manager::GetCamera().GetConfig().height, 0, 0);

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

	std::vector<DescriptorConfig> frameDescriptorConfigs(1);
	frameDescriptorConfigs[0].type = DescriptorType::UniformBuffer;
	frameDescriptorConfigs[0].stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | 
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
	frameDescriptor.Create(0, frameDescriptorConfigs);

	std::vector<DescriptorConfig> materialDescriptorConfigs(2);
	materialDescriptorConfigs[0].type = DescriptorType::CombinedSampler;
	materialDescriptorConfigs[0].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	materialDescriptorConfigs[0].count = 3;
	materialDescriptorConfigs[1].type = DescriptorType::CombinedSampler;
	materialDescriptorConfigs[1].stages = VK_SHADER_STAGE_FRAGMENT_BIT;
	materialDescriptorConfigs[1].count = 3;
	materialDescriptor.Create(1, materialDescriptorConfigs);

	std::vector<DescriptorConfig> objectDescriptorConfigs(1);
	objectDescriptorConfigs[0].type = DescriptorType::DynamicUniformBuffer;
	objectDescriptorConfigs[0].stages = VK_SHADER_STAGE_VERTEX_BIT;
	objectDescriptor.Create(2, objectDescriptorConfigs);

	frameDescriptor.GetNewSetDynamic();
	frameDescriptor.UpdateDynamic(0, 0, Utilities::Pointerize(frameBuffers));

	materialDescriptor.GetNewSet();
	materialDescriptor.Update(0, 0, {&mud_diff, &mud_norm, &mud_arm});
	materialDescriptor.Update(0, 1, {&grass_diff, &grass_norm, &grass_arm});

	objectDescriptor.GetNewSetDynamic();
	objectDescriptor.UpdateDynamic(0, 0, Utilities::Pointerize(objectBuffers), sizeof(mat4));

	//std::vector<DescriptorConfig> computeDescriptorConfigs(1);
	//computeDescriptorConfigs[0].type = DescriptorType::StorageImage;
	//computeDescriptorConfigs[0].stages = VK_SHADER_STAGE_COMPUTE_BIT;
	//computeDescriptor.Create(1, computeDescriptorConfigs);
	//computeDescriptor.GetNewSet();
	//computeDescriptor.Update(0, 0, computeImage);

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

	//PipelineConfig computePipelineConfig{};
	//computePipelineConfig.shader = "heightmap";
	//computePipelineConfig.type = PipelineType::Compute;
	//computePipelineConfig.descriptorLayouts = { frameDescriptor.GetLayout(), computeDescriptor.GetLayout() };
	//computePipeline.Create(computePipelineConfig);

	Manager::GetCamera().Move(point3D(-5000, 2500, 5000));
	//Manager::GetCamera().Move(point3D(0, 10, 0));

	Renderer::RegisterCall(0, Render);
}

void Frame()
{
	if (Input::GetKey(GLFW_KEY_M).pressed)
	{
		Input::TriggerMouse();
	}

	Manager::GetCamera().UpdateView();

	data.view = Manager::GetCamera().GetView();
	data.viewPosition = Manager::GetCamera().GetPosition();

	data.resolution = point4D(Manager::GetCamera().GetConfig().width, Manager::GetCamera().GetConfig().height, 0, 0);

	frameBuffers[Renderer::GetCurrentFrame()].Update(&data, sizeof(data));

	models[0] = mat4::Identity();
	models[0].Scale(point3D(10000, 10000, 10000));

	objectBuffers[Renderer::GetCurrentFrame()].Update(models.data(), sizeof(mat4) * models.size());

	/*if (Input::GetKey(GLFW_KEY_P).pressed)
	{
		CommandConfig commandConfig{};
		commandConfig.wait = false;
		Command computeCommand(commandConfig);
		computeCommand.Begin();

		frameDescriptor.BindDynamic(0, computeCommand.GetBuffer(), computePipeline);
		computeDescriptor.Bind(0, computeCommand.GetBuffer(), computePipeline);
		computePipeline.Bind(computeCommand.GetBuffer());
		vkCmdDispatch(computeCommand.GetBuffer(), 1024 / 8, 1024 / 8, 1);

		computeCommand.End();
		computeCommand.Submit();

		//std::cout << "Compute shader executed." << std::endl;
	}*/
}

void End()
{
	planeMesh.Destroy();
	//quadMesh.Destroy();

	for (Buffer& buffer : frameBuffers) { buffer.Destroy(); }
	frameBuffers.clear();

	for (Buffer& buffer : objectBuffers) { buffer.Destroy(); }
	objectBuffers.clear();

	//computeImage.Destroy();
	mud_diff.Destroy();
	mud_norm.Destroy();
	mud_arm.Destroy();
	grass_diff.Destroy();
	grass_norm.Destroy();
	grass_arm.Destroy();

	pass.Destroy();
	frameDescriptor.Destroy();
	materialDescriptor.Destroy();
	objectDescriptor.Destroy();
	//computeDescriptor.Destroy();
	pipeline.Destroy();
	//screenPipeline.Destroy();
	//computePipeline.Destroy();
}

int main(int argc, char** argv)
{
	Manager::ParseArguments(argv, argc);
	Manager::GetConfig().deviceConfig.anisotropic = true;
	Manager::GetConfig().deviceConfig.tesselation = true;
	//Manager::GetConfig().wireframe = true;
	Manager::Create();

	Manager::RegisterStartCall(Start);
	Manager::RegisterFrameCall(Frame);
	Manager::RegisterEndCall(End);

	Manager::Run();

	Manager::Destroy();

	exit(EXIT_SUCCESS);
}