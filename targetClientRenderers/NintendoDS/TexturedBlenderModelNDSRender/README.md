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


____Remoteboot____
Also, it's recommended to use the remoteboot feature. It allows to send the current TGDS Project over wifi removing the necessity
to take out the SD card repeteadly and thus, causing it to wear out and to break the SD slot of your unit.

Usage:
- Make sure the wifi settings in the NintendoDS are properly set up, so you're already able to connect to internet from it.

- Get a copy of ToolchainGenericDS-multiboot: https://bitbucket.org/Coto88/ToolchainGenericDS-multiboot/get/TGDS1.65.zip
Follow the instructions there and get either the TWL or NTR version. Make sure you update the computer IP address used to build TGDS Projects, 
in the file: toolchaingenericds-multiboot-config.txt of said repository before moving it into SD card.

For example if you're running NTR mode (say, a DS Lite), you'll need ToolchainGenericDS-multiboot.nds, tgds_multiboot_payload_ntr.bin
and toolchaingenericds-multiboot-config.txt (update here, the computer's IP you use to build TGDS Projects) then move all of them to root SD card directory.

- Build the TGDS Project as you'd normally would, and run these commands from the shell.
<make clean>
<make>

- Then if you're on NTR mode:
<remoteboot ntr_mode computer_ip_address>

- Or if you're on TWL mode:
<remoteboot twl_mode computer_ip_address>

- And finally boot ToolchainGenericDS-multiboot, and press (X), wait a few seconds and TGDS Project should boot remotely.
  After that, everytime you want to remoteboot a TGDS Project, repeat the last 2 steps. ;-)




/release folder has the latest binary precompiled for your convenience.

Coto
