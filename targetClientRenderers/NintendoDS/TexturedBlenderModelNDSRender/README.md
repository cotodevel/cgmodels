![ToolchainGenericDS](img/TGDS-Logo.png)

This is the Toolchain Generic TexturedBlenderModelNDSRender project:

Compile Toolchain: To compile this project you will need to follow the steps at https://bitbucket.org/Coto88/toolchaingenericds : Then simply extract the project somewhere.

Compile this project: Open msys, through msys commands head to the directory your extracted this project. Then write: make clean make

After compiling, run the project in NDS.

Project Specific description: 
3D model renderer of textured models read from native NDS CallLists (Format: model.H + model.PCX Texture)

Blender 2.49b workflow to export a textured 3D object:
Source model format (like .blender) -> export to OBJ -> load textures -> NDS CallList export (.H + .PCX Texture)

Setup:
1)
https://bitbucket.org/Coto88/cgmodels/src/master/misc/blender2_49b_win8x64/
2)
https://bitbucket.org/Coto88/blender-nds-exporter 
3)
Build TGDS 
https://bitbucket.org/Coto88/newlib-nds
4)
Build this TGDS Project
make clean -> make

/release folder has the latest binary precompiled for your convenience.

Coto
