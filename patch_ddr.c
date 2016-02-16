#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ARCH_DMA_MINALIGN 64
#define BLOCK_SIZE 1024
#define CPU_INFO_FILE "/proc/cpuinfo"
#define MMC_DEVICE "/dev/mmcblk1"
#define IVT_FLAG_ADDRESS 0x215
#define WRONG_VALUE 0x19
#define CORRECT_VALUE 0x1a

int is_mx6dl() {
	// Read /proc/cpuinfo
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	int ret = 0;

	char * cpu_str = "Freescale i.MX6 Quad/DualLite";

	fp = fopen(CPU_INFO_FILE, "r");
	if (fp == NULL)
		return -1;

	while ((read = getline(&line, &len, fp)) != -1) {
		if (strstr(line, cpu_str) != NULL) {
			ret = 1;
		}
	}

	fclose(fp);
	if (line)
		free(line);
	return ret;

}

// To-Do: read the IVT from eMMC
int read_block(){
	return 0;
}

// To-Do: write the IVT to eMMC
int write_block(){
	return 0;
}

int apply_patch(){
	char *ivt = memalign(ARCH_DMA_MINALIGN, BLOCK_SIZE);
	int ret = 0;
	if (ivt == NULL) {
		printf("Failed to allocate memory");
		return -1;
	} else if(read_block(ivt) != 0) {
		printf("Failed reading block");
		ret = -1;
	} else if(ivt[IVT_FLAG_ADDRESS] == WRONG_VALUE) {
		ivt[IVT_FLAG_ADDRESS] = CORRECT_VALUE;
		if(write_block(ivt) != 0) {
			printf("Failed writing block");
			ret = -1;
		}
	} else {
		printf("Patch already applied");
	}
	free(ivt);
	return ret;


}

int main() {
	int ret;
	if ((ret = is_mx6dl()) == -1) {
		exit(1);
	} else if (ret == 0) {
		printf("Not a DualLite, nothing to do here.");
		exit(0);
	} else if(apply_patch() != 0){
		exit(1);
	}
	exit(0);
}