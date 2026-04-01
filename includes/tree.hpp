#pragma once

#include "point.hpp"
#include "shape.hpp"
#include "mesh.hpp"

struct TreeConfig
{
	int seed = 1;

	Point<int, 2> horizontalResolution = {3, 9};
	Point<int, 2> verticalResolution = {1, 5};
	//Point<int, 2> horizontalResolution = {3, 4};
	//Point<int, 2> verticalResolution = {1, 4};
	point2D dynamicResFactors = {75.0, 38.0};
	point2D branchEndQuality = {0.0, 0.5};

	bool start = true;
	bool main = true;
	uint32_t dynamicResolution = 1;
	uint32_t generateLeaves = 0;
	int currentSplits = 0;
	int maxSplits = 5;
	int currentSideSplits = 0;
	int maxSideSplits = 1;

	float length = 1.05;
	//float thickness = 0.525;
	//float thickness = 0.619;
	float thickness = 0.568;
	float horizontalAngle = 0.0;
	float verticalAngle = 0.0;
	float previousHorizontalAngle = 0.0;
	float previousVerticalAngle = 0.0;

	int segments = 4;
	int currentSegments = segments;
	float segmentLengthDecrease = 1.06;
	float segmentThicknessDecrease = 1.0;
	float segmentThicknessSubtraction = 0.053;
	point2D horizontalAngleChange = {0.0, 15.0};
	point2D verticalAngleChange = {0.0, 10.0};

	Point<int, 2> splitCount = {2, 3};
	int splitSegmentDecrease = 3;
	point2D splitAngle = {-40.0, 10.0};
	point2D spacingRandomness = {0.0, 0.0};
	float horizontalSplitStartAngle = 1.0;
	float verticalSplitStartAngle = 0.0;
	point2D splitGravityModifier = {0.0, 0.2};

	float sideSplitChance = 0.0;

	//float minimumThickness = 0.03;
	//float minimumThickness = 0.014;
	float minimumThickness = 0.003;
};

class Tree
{
	private:
		static shape32 GenerateBranch(TreeConfig config);

	public:
		static uint32_t regenerateOnChange;
		static uint32_t randomOnRegenerate;

		static shape32 GenerateTree(TreeConfig config);

		static void CreateTreeMenu(TreeConfig &config, mesh32 &treeMesh);
		
};