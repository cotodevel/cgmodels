Textured Cube made in Blender 2.49b then exported as .H NDS CallList + .PCX Paletted Texture 
![ToolchainGenericDS](img/blender-nds-example.png)
--
Textured OBJ imported on Blender 2.49b then exported as .H NDS CallList + .PCX Paletted Texture 
![ToolchainGenericDS](img/NDSmodel2.png)

master: Development branch. Use TGDS1.65: branch for stable features.

3D Models exported from Blender and/or other 3D modelling tools into different target platforms such as NintendoDS.

Folder description:
/img: Shows the final result
/misc: Tools used to create these results
/models: Models themselves, the native format can be (.obj, .fbx, .dae, etc). 
/targetClientRenderers: Are the engine software performing the model rendering

-

Note:
targetClientRenderers/NintendoDS/Env_Mapping has a TGDSProject3D environment (https://github.com/cotodevel/toolchaingenericds) if you want to use Visual Studio 2012 to develop.
targetClientRenderers/NintendoDS/TexturedBlenderModelNDSRender does not have a TGDSProject3D environment, because it's drawing GX Display List code directly emitted from blender (non compatible with VS2012)

Latest stable release:
http://github.com/cotodevel/cgmodels/archive/TGDS1.65.zip
