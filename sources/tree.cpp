#include "tree.hpp"

#include "ui.hpp"

#include <random>
#include <string>

shapePN32 Tree::GenerateBranch(TreeConfig config)
{
	if (config.start)
	{
		config.start = false;
		config.currentSegments = config.segments;
	}

	std::mt19937 generator(config.seed);
	std::uniform_real_distribution<> floatDistribution(0.0, 1.0);
	std::uniform_int_distribution<> intDistribution(1, 10000);
	std::uniform_int_distribution<> signDistribution(0, 1);

	config.horizontalAngle += std::lerp(config.horizontalAngleChange.x(), config.horizontalAngleChange.y(), floatDistribution(generator)) * (signDistribution(generator) * 2 - 1);
	config.verticalAngle += std::lerp(config.verticalAngleChange.x(), config.verticalAngleChange.y(), floatDistribution(generator)) * (signDistribution(generator) * 2 - 1);

	float splitInter = pow(float(config.currentSplits) / float(config.maxSplits), 4.0);

	float horizontalResInter = std::clamp((std::abs(config.previousHorizontalAngle - config.horizontalAngle) / config.dynamicResFactors.x()) + (config.branchEndQuality.x() * splitInter), 0.0f, 1.0f);
	float verticalResInter = std::clamp((std::abs(config.previousVerticalAngle - config.verticalAngle) / config.dynamicResFactors.y()) + (config.branchEndQuality.y() * splitInter), 0.0f, 1.0f);

	Point<int, 2> resolution = {config.horizontalResolution.y(), config.verticalResolution.y()};
	
	if (config.dynamicResolution == 1)
	{
		resolution.y() = std::round(std::lerp(float(config.verticalResolution.x()), float(config.verticalResolution.y()), (verticalResInter + horizontalResInter) * 0.5));
	}

	ShapeSettings shapeSettings{};
	shapeSettings.resolution = resolution;
	shapePN32 branch(ShapeType::Cylinder, shapeSettings);

	branch.SetColor(point3D(0.0));

	float targetScale = (config.thickness - config.segmentThicknessSubtraction) * config.segmentThicknessDecrease;
	if (targetScale < config.minimumThickness * 0.9) {targetScale = config.minimumThickness * 0.9;}

	branch.Move(point3D(0.0, 0.25, 0.0));
	
	for (int y = 0; y <= resolution.y(); y++)
	{
		for (int x = 0; x <= resolution.x(); x++)
		{
			float inter = float(y) / float(resolution.y());
			float interScale = std::lerp(targetScale, config.thickness, inter);
			branch.Scale(point3D(interScale, config.length, interScale), x * (resolution.y() + 1) + y);
		}
	}

	for (int y = 0; y <= resolution.y(); y++)
	{
		for (int x = 0; x <= resolution.x(); x++)
		{
			float inter = float(y) / float(resolution.y());
			float interHorizonAngle = std::lerp(config.horizontalAngle, config.previousHorizontalAngle, inter);
			float interVerticalAngle = std::lerp(config.verticalAngle, config.previousVerticalAngle, inter);
			branch.Rotate(interVerticalAngle , Axis::x, x * (resolution.y() + 1) + y);
			branch.Rotate(interHorizonAngle, Axis::y, x * (resolution.y() + 1) + y);
		}
	}

	if (config.currentSegments > 0 && targetScale > config.minimumThickness)
	{
		point3D offset = point3D(0.0, config.length * 0.5, 0.0);
		offset.Rotate(config.verticalAngle, Axis::x);
		offset.Rotate(config.horizontalAngle, Axis::y);

		TreeConfig branchConfig = config;
		branchConfig.currentSegments -= 1;
		branchConfig.length *= config.segmentLengthDecrease;
		branchConfig.thickness = targetScale;
		branchConfig.previousHorizontalAngle = branchConfig.horizontalAngle;
		branchConfig.previousVerticalAngle = branchConfig.verticalAngle;
		branchConfig.seed = intDistribution(generator);
		branchConfig.totalOffset += offset;
		shapePN32 segmentBranch = GenerateBranch(branchConfig);
		
		segmentBranch.Move(offset);
		branch.Join(segmentBranch);
	}
	else if (targetScale > config.minimumThickness && config.currentSplits < config.maxSplits)
	{
		int splitTimes = config.splitCount.y();
		bool centerBranch = false;
		//std::vector<float> branchAngles;
		for (int i = 0; i < splitTimes; i++)
		{
			TreeConfig branchConfig = config;
			branchConfig.main = false;
			branchConfig.segments -= branchConfig.splitSegmentDecrease;
			branchConfig.currentSegments = branchConfig.segments;
			branchConfig.length *= config.segmentLengthDecrease;
			branchConfig.thickness = targetScale;
			
			branchConfig.previousHorizontalAngle = branchConfig.horizontalAngle;
			branchConfig.previousVerticalAngle = branchConfig.verticalAngle;
			branchConfig.currentSplits++;

			float maxHorizontalAngle = (std::lerp(360.0, 45.0, std::clamp(std::abs(branchConfig.verticalAngle) / 90.0, 0.0, 1.0)) / float(splitTimes));
			float randomOffset = std::lerp(config.spacingRandomness.x(), config.spacingRandomness.y(), floatDistribution(generator)) * maxHorizontalAngle * (signDistribution(generator) * 2 - 1);
			branchConfig.horizontalAngle += (i - (splitTimes / 2)) * maxHorizontalAngle + randomOffset;
			//branchConfig.verticalAngle += std::lerp(config.splitAngle.x(), config.splitAngle.y(), floatDistribution(generator)) * (signDistribution(generator) * 2 - 1);
			branchConfig.verticalAngle = std::lerp(branchConfig.verticalAngle, 0.0, (float(branchConfig.currentSplits) / float(config.maxSplits)) * config.splitGravityModifier.y());
			branchConfig.verticalAngle += std::lerp(config.splitAngle.x(), config.splitAngle.y(), floatDistribution(generator));
			//int vertSign = branchConfig.verticalAngle > 0.0 ? 1 : -1;
			if (branchConfig.verticalAngle > -25.0)
			{
				if (!centerBranch)
				{
					centerBranch = true;
					if (branchConfig.verticalAngle > -5.0) {branchConfig.verticalAngle = -5.0;}
				}
				else
				{
					branchConfig.verticalAngle -= branchConfig.verticalAngle > 0.0 ? 45.0 : 25.0;
				}
			}

			float horAngleInter = std::clamp(std::abs(branchConfig.previousHorizontalAngle - branchConfig.horizontalAngle) / 180.0, 0.0, 1.0);

			branchConfig.previousHorizontalAngle = std::lerp(branchConfig.previousHorizontalAngle, branchConfig.horizontalAngle, horAngleInter * config.horizontalSplitStartAngle);
			branchConfig.previousVerticalAngle = std::lerp(branchConfig.previousVerticalAngle, branchConfig.verticalAngle, config.verticalSplitStartAngle);
		
			branchConfig.seed = intDistribution(generator);

			point3D offset = point3D(0.0, config.length * 0.5, 0.0);
			offset.Rotate(config.verticalAngle, Axis::x);
			offset.Rotate(config.horizontalAngle, Axis::y);
			branchConfig.totalOffset += offset;

			shapePN32 segmentBranch = GenerateBranch(branchConfig);
			
			segmentBranch.Move(offset);
			branch.Join(segmentBranch);
		}
	}
	else
	{
		for (int i = 0; i < 18; i++)
		{
			point3D offset = point3D(0.0, config.length * 0.5, 0.0);
			offset.Rotate(config.verticalAngle, Axis::x);
			offset.Rotate(config.horizontalAngle, Axis::y);
			offset += point3D(std::lerp(-1.0, 1.0, floatDistribution(generator)), std::lerp(-1.0, 1.0, floatDistribution(generator)), std::lerp(-1.0, 1.0, floatDistribution(generator))) * 0.375;
	
			LeafData leafData{};
			leafData.leafPosition = config.totalOffset + offset;
			float yRot = std::lerp(0.0, 1.0, floatDistribution(generator));
			float xRot = std::lerp(0.0, 1.0, floatDistribution(generator));
			float zRot = std::lerp(0.0, 1.0, floatDistribution(generator));
			float xr = (xRot * 360.0) * 0.0174532925;
			float xc = cos(xr);
			float xs = sin(xr);
			float yr = (yRot * 360.0) * 0.0174532925;
			float yc = cos(yr);
			float ys = sin(yr);
			float zr = (zRot * 360.0) * 0.0174532925;
			float zc = cos(zr);
			float zs = sin(zr);
			leafData.leafRotationXY = point4D(xc, xs, yc, ys);
			leafData.leafRotationZ = point4D(zc, zs, 0.0, 0.0);
	
			leafPositions.push_back(leafData);
		}


		//leafIDs.push_back(branch.GetVertices().size());
	}

	return (branch);
}

shapePN32 Tree::GenerateTree(TreeConfig config)
{
	leafPositions.clear();
	leafIDs.clear();

	shapePN32 result = GenerateBranch(config);

	return (result);
}

std::vector<LeafData> Tree::GetLeafPositions()
{
	return (leafPositions);
}

static meshPN32 *mesh = nullptr;
static TreeConfig *treeConfig = nullptr;
static std::string verticesCount = "0";
static std::string indicesCount = "0";

void RegenerateMesh()
{
	if (!treeConfig || !mesh) {return;}

	if (Tree::randomOnRegenerate) {treeConfig->seed = std::random_device{}();}

	mesh->Destroy();
	shapePN32 treeShape = Tree::GenerateTree(*treeConfig);
	mesh->Create(treeShape);

	verticesCount = std::to_string(mesh->GetVertices().size());
	indicesCount = std::to_string(mesh->GetIndices().size());
}

void OnValueChanged()
{
	if (Tree::regenerateOnChange == 1) {RegenerateMesh();}
}

void Tree::CreateTreeMenu(TreeConfig &config, meshPN32 &treeMesh)
{
	treeConfig = &config;
	mesh = &treeMesh;

	Menu &menu = UI::NewMenu("Tree generation");

	menu.TriggerNode("Base", OnValueChanged);
	menu.AddSlider("Seed", config.seed, 1, 10000);
	menu.AddSlider("Max splits", config.maxSplits, 0, 10);
	menu.AddSlider("Segments", config.segments, 0, 24);
	menu.AddSlider("Length", config.length, 0.0, 4.0);
	menu.AddSlider("Length decrease", config.segmentLengthDecrease, 0.0, 2.0);
	menu.AddSlider("Thickness", config.thickness, 0.0, 2.0);
	menu.AddSlider("Thickness decrease", config.segmentThicknessDecrease, 0.0, 1.0);
	menu.AddSlider("Thickness subtraction", config.segmentThicknessSubtraction, 0.0, 0.1);
	//menu.AddDropdown("Decrease type", config.thicknessDecreaseType, {"Multiplication", "Subtraction"});
	menu.AddRange("Horizontal angle", config.horizontalAngleChange, 0.0, 90.0);
	menu.AddRange("Vertical angle", config.verticalAngleChange, 0.0, 45.0);
	menu.AddRange("Horizontal resolution", config.horizontalResolution, 3, 16);
	menu.AddRange("Vertical resolution", config.verticalResolution, 1, 16);
	menu.AddCheckbox("Dynamic resolution", config.dynamicResolution);
	menu.AddSlider("H tip quality", config.branchEndQuality.x(), 0.0, 1.0);
	menu.AddSlider("V tip quality", config.branchEndQuality.y(), 0.0, 1.0);
	menu.AddSlider("H factor", config.dynamicResFactors.x(), 0.0, 180.0);
	menu.AddSlider("V factor", config.dynamicResFactors.y(), 0.0, 90.0);
	menu.TriggerNode("Base");

	menu.TriggerNode("Split", OnValueChanged);
	menu.AddRange("Count", config.splitCount, 0, 4);
	menu.AddSlider("Segment decrease", config.splitSegmentDecrease, 0, config.segments);
	menu.AddRange("Angle", config.splitAngle, -90.0, 90.0);
	menu.AddRange("Spacing randomness", config.spacingRandomness, 0.0, 0.75);
	menu.AddSlider("Horizontal angle start", config.horizontalSplitStartAngle, 0.0, 1.0);
	menu.AddSlider("Vertical angle start", config.verticalSplitStartAngle, 0.0, 1.0);
	menu.AddRange("Gravity", config.splitGravityModifier, 0.0, 1.0);
	menu.AddCheckbox("Generate leaves", config.generateLeaves);
	menu.TriggerNode("Split");

	menu.TriggerNode("Side", OnValueChanged);
	menu.AddSlider("Chance", config.sideSplitChance, 0.0, 1.0);
	menu.AddSlider("Max splits", config.maxSideSplits, 0, 4);
	menu.TriggerNode("Side");

	menu.TriggerNode("Info");
	menu.AddText("Vertices", verticesCount);
	menu.AddText("Indices", indicesCount);
	menu.TriggerNode("Info");

	menu.TriggerNode("Limits", OnValueChanged);
	menu.AddSlider("Minimum thickness", config.minimumThickness, 0.0, config.thickness);
	menu.TriggerNode("Limits");

	menu.AddCheckbox("Regenerate on change", Tree::regenerateOnChange);
	menu.AddCheckbox("Random on regenerate", Tree::randomOnRegenerate);
	menu.AddButton("Regenerate", RegenerateMesh);
}

std::vector<int> Tree::leafIDs;
std::vector<LeafData> Tree::leafPositions;
uint32_t Tree::regenerateOnChange = 1;
uint32_t Tree::randomOnRegenerate = 0;