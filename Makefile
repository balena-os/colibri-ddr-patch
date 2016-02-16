
all: patch_ddr

patch_ddr: patch_ddr.c
	gcc patch_ddr.c -o patch_ddr
