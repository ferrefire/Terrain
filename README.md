# Terrain

### A real-time procedural landscape renderer built from scratch with C++, Vulkan and GLSL.

Important note: I originally intended for this project to be a small test for the [Limcore](https://github.com/ferrefire/Limcore) library (A Vulkan Graphics Framework I wrote myself). However it has far outgrown its original scope and because of that the code is structured rather poorly and located in a single main file. For a correct evaluation of my coding and project management skills, please use the [Limcore](https://github.com/ferrefire/Limcore) project.

This project generates and renders large-scale procedural landscapes consisting of detailed terrain, hundreds of thousands of trees, dynamic atmospheric scattering, volumetric lighting, and long-distance shadows.

The project focuses on GPU-driven rendering, runtime procedural generation, visual scalability, high performance, configurability, and stable frame times. Its core terrain, vegetation, lighting, atmosphere, shadow, and rendering systems were designed and implemented from scratch.

## Demo

[Watch a demonstration video.](https://www.youtube.com/watch?v=gwGd-PgFRMI)

Some screenshots:

<img width="2560" height="1440" alt="Schermafbeelding 2026-06-18 151447" src="https://github.com/user-attachments/assets/4dfdb88b-e5ce-4aee-8085-ae50e3e99edb" />

<img width="2560" height="1440" alt="Schermafbeelding 2026-06-18 152005" src="https://github.com/user-attachments/assets/4a834e20-c432-4a1b-8fcf-0fd32696edeb" />

<img width="2560" height="1440" alt="Schermafbeelding 2026-06-18 153209" src="https://github.com/user-attachments/assets/775d51d7-339d-449d-b22f-bbcca478f6aa" />

<img width="2560" height="1440" alt="Schermafbeelding 2026-06-18 152907" src="https://github.com/user-attachments/assets/69f19212-2afa-4030-a2e9-d2b39434bc25" />

<img width="2560" height="1440" alt="Schermafbeelding 2026-06-18 153401" src="https://github.com/user-attachments/assets/ab758a4c-7f06-4481-b3c3-bf78c5bc3126" />

## Table of Contents
- [Overview](#overview)
- [Focus](#focus)
- [Features](#features)
- [Building](#building)
- [License](#license)

## Overview

The project combines several tightly integrated systems:
- A GPU-generated, heightmap-based terrain renderer
- Procedurally generated tree meshes with multiple levels of detail
- GPU-driven tree placement, visibility testing and LOD selection
- Cascaded terrain and vegetation shadows
- Dynamic atmospheric rendering
- Terrain-wide volumetric lighting, light shafts, and god rays

Except for PBR textures, the landscape and all its components are completely generated at runtime. The generation code is highly optimized and expensive workloads are distributed across multiple frames. This allows high quality procedural content to be created without introducing noticeable frame-time spikes, including on lower-end integrated GPUs. Almost all aspects of the landscape are configurable and can be updated at runtime.

## Focus

The main focus for this project was:
- Testing my Vulkan Graphics library [Limcore](https://github.com/ferrefire/Limcore)
- Full procedural generation at run-time
- Large-scale real-time environment rendering
- GPU-driven systems
- High performance on both high-end and integraded machines
- Run-time configurability
- Extreme scale
- Vulkan renderer architecture

It was originaly made as a test for the [Limcore](https://github.com/ferrefire/Limcore) library. However it soon evolved into much more as I continued to delve deeper into GPU computing and the urge to optimize it to its full extent. It serves as a technical demonstration of my capabilities as a graphics programmer. It also displays the workings and usage of the Limcore library as an easy to use framework while still providing full control.

## Features

### Terrain Generation

The terrain system is designed to render and generate extremely large topography while preserving detail near the camera, having low memory usage, and being very performant.

Terrain geometry is produced by sampling multiple heightmap cascades. Each cascade covers a different world-space scale, allowing the renderer to provide both close-range detail and large-scale landscape features.

The heightmaps are generated entirely on the GPU using compute shaders and analytical derivatives of value noise. A heightmap contains the height and normal for a given position packed into either rgba8 or rgba16 format.

When a heightmap cascade needs to be generated or updated, its workload is divided across several frames. This avoids large compute spikes and keeps interaction smooth while terrain data is being produced at runtime.

### Terrain Rendering

The terrain is rendered using tessellation shaders for adaptive geometric detail and GPU culling.

Triplanar sampling is used for correct and realistic texturing and normal mapping. This way the terrain is rendered in a dynamic yet consistant manner.

Several different textures and scales are blended based on terrain data such as steepness or height. This provides depth and variety for both close and far terrain.

Terrain-wide shadows are caluculated by efficiently ray marching across the generated heightmaps. The results are then stored in cascaded shadow maps, providing shadow coverage across the full visible landscape without requiring conventional geometry-based shadow mapping.

### Trees

The tree system can generate and render hundreds of thousands of trees across the entire landscape. It directly uses the terrain data for dynamic placement and culling based on configurable rules.

Tree geometry is generated at run-time from a user defined configuration rather than loaded from pre-made models.

The mesh generator produces:
- Trunk and branch geometry
- Leaf positions and rotations
- Surface normals and UV coordinates
- Several levels of detail packed into a single mesh

Tree and leaf positions are processed into draw command buffers on the GPU using compute shaders, after which they are rendered using instanced indexed indirect draw commands. This results in minimal CPU-GPU travel.

The trees and leaves consist of several levels of detail between which they are seamlessly transitioned and morphed. The LOD level is calculated on several factors such as distance, forest density, and screen coverage. All LODs are 3D and interact with the environment, avoiding obvious billboarding.

Traditional cascade shadow mapping combined with custom optimizations allows for visible trees to continue casting shadows at extreme distances, including vegetation located more than 100 kilometers from the camera.

### Atmosphere and Volumetric Lighting

The sky is rendered using a configurable atmospheric rendering system. Its parameters can be changed at runtime, allowing the environment and lighting conditions to respond in real-time. The atmosphere is simulated using physically correct models based on the Earth's biosphere.

Alongside the sky, a 3D array of aerial perspective slices are computed each frame. This provides full terrain-wide volumetric lighting based on the atmospheric parameters. The system integrates data from both the terrain and vegetation. For example shadow data is used to create light shafts and occlusion is used for mist.

## Building

### Requirements

For Linux:
- [CMake](https://cmake.org/) version 3.22 or newer

For Windows:
- [Visual Studio](https://visualstudio.microsoft.com/) 2022 or newer
- [CMake](https://cmake.org/) version 3.22 or newer

### Dependencies

This project uses the [Limcore](https://github.com/ferrefire/Limcore) library which handles and fetches all dependencies for you. So no need to have them pre-installed.

Still, here is a list of external libraries used:
- [Limcore](https://github.com/ferrefire/Limcore)
- [GLFW](https://github.com/glfw/glfw) (Used by Limcore)
- [ImGui](https://github.com/ocornut/imgui) (Used by Limcore)

### Pre-compiled

Provided are pre-compiled binaries for Windows and Linux. Simply download and extract the file. Then run the Terrain executable.

For Windows: [TerrainBinWin.zip](https://github.com/user-attachments/files/29554524/TerrainBinWin.zip)
For Linux: Will do later.

Controls:
- Press the `M` key to enable and disable the mouse
- Press the `ESC` key to quit the application
- Use `WASD` for relative horizontal movement
- Use `CTRL` and `SPACEBAR` for relative vertical movement
- Use the scrollwheel to increase or decrease movement speed
- Press the `F` key to lock onto the terrain height
- Use the arrow keys to rotate the sun direction 

### Build

For Linux:
1. Download or clone the repository
2. Use a terminal to navigate into the project directory
3. Make sure the `setup.sh` file has the correct permissions: `chmod 777 setup.sh`
4. Build the project: `./setup.sh build`
5. Compile the executeable: `./setup.sh compile`
6. Run the project: `./setup.sh r`

For Windows:
1. Download the repository and extract the file
2. Start [Visual Studio](https://visualstudio.microsoft.com/) and click `Open a folder`
3. Select the extracted folder, the project should now automatically start building
5. Make sure to use the x64-Release configuration for the best results
6. Select `Terrain.exe` as the executeable and click the green triangle to run it

## License

This project is licensed under the MIT License, see LICENSE.txt for more information.
