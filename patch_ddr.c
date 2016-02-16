#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>

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

// Stolen from the dd source code http://code.metager.de/source/xref/linux/klibc/usr/utils/dd.c
int safe_read(int fd, void *buf, size_t size)
{
	int ret, count = 0;
	char *p = buf;

	while (size) {
		ret = read(fd, p, size);

		/*
		 * If we got EINTR, go again.
		 */
		if (ret == -1 && errno == EINTR)
			continue;

		/*
		 * If we encountered an error condition
		 * or read 0 bytes (EOF) return what we
		 * have.
		 */
		if (ret == -1 || ret == 0)
			return count ? count : ret;

		/*
		 * We read some bytes.
		 */
		count += ret;
		size -= ret;
		p += ret;
	}

	return count;
}

// Also from dd source
int skip_blocks(int fd, void *buf, unsigned int blks, size_t size)
{
	unsigned int blk;
	int ret = 0;

	/*
	 * Try to seek.
	 */
	for (blk = 0; blk < blks; blk++) {
		ret = lseek(fd, size, SEEK_CUR);
		if (ret == -1)
			break;
	}

	/*
	 * If we failed to seek, read instead.
	 * FIXME: we don't handle short reads here, or
	 * EINTR correctly.
	 */
	if (blk == 0 && ret == -1 && errno == ESPIPE) {
		for (blk = 0; blk < blks; blk++) {
			ret = safe_read(fd, buf, size);
			if (ret != (int)size)
				break;
		}
	}

	if (ret == -1) {
		return -1;
	}
	return 0;
}
// read the IVT from eMMC
int read_block(char *ivt){
	int ret = 0;
	int d = open(MMC_DEVICE, O_RDONLY);
	if(d == -1) {
		return -1;
	}
	if((skip_blocks(d, ivt, 1, BLOCK_SIZE) != 0) || (safe_read(d, ivt, BLOCK_SIZE) != (int)BLOCK_SIZE)){
		ret = -1;
	}

	close(d);
	return ret;
}

// write the IVT to eMMC
int write_block(char *ivt){
	int ret = 0;
	int d = open(MMC_DEVICE, O_WRONLY|O_DIRECT|O_SYNC);
	if(d == -1) {
		return -1;
	}
	if((skip_blocks(d, ivt, 1, BLOCK_SIZE) != 0) || (write(d, ivt, BLOCK_SIZE) != (int)BLOCK_SIZE)){
		ret = -1;
	}

	close(d);
	return ret;
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