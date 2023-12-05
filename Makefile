all: tgdsBuild

tgdsBuild: 
	$(MAKE)	-R	-C	targetClientRenderers/NintendoDS/Env_Mapping/
	$(MAKE)	-R	-C	targetClientRenderers/NintendoDS/TexturedBlenderModelNDSRender/
	
clean:
	$(MAKE) -C targetClientRenderers/NintendoDS/Env_Mapping/ clean
	$(MAKE) -C targetClientRenderers/NintendoDS/TexturedBlenderModelNDSRender/ clean
	
#---------------------------------------------------------------------------------

commitChanges:
	-@git commit -a	-m '$(COMMITMSG)'
	-@git push origin HEAD
	
#---------------------------------------------------------------------------------

switchStable:
	-@git checkout -f	'TGDS1.65'
	
#---------------------------------------------------------------------------------

switchMaster:
	-@git checkout -f	'master'
