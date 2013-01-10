/* dnw2 linux main file. This depends on libusb.
 *
 * Author:         Fox <hulifox008@163.com>
 * License:        GPL
 *
 */

#include <stdio.h>
#include <usb.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define QQ2440_SECBULK_IDVENDOR		0x5345
#define QQ2440_SECBULK_IDPRODUCT	0x1234

struct usb_dev_handle * open_port() {
	struct usb_bus *busses, *bus;

	usb_init();
	usb_find_busses();
	usb_find_devices();

	busses = usb_get_busses();
	for (bus = busses; bus; bus = bus->next) {
		struct usb_device *dev;
		for (dev = bus->devices; dev; dev = dev->next) {
			printf("idVendor:0x%x\t,ipProduct:0x%x\n",
					dev->descriptor.idVendor, dev->descriptor.idProduct);
			if (QQ2440_SECBULK_IDVENDOR == dev->descriptor.idVendor
					&& QQ2440_SECBULK_IDPRODUCT == dev->descriptor.idProduct) {
				printf("Target usb device found!\n");
				struct usb_dev_handle *hdev = usb_open(dev);
				if (!hdev) {
					perror("Cannot open device");
				} else {
					if (0 != usb_claim_interface(hdev, 0)) {
						perror("Cannot claim interface");
						usb_close(hdev);
						hdev = NULL;
					}
				}
				return hdev;
			}
		}
	}

	printf("Target usb device not found!\n");

	return NULL;
}

void usage() {
	printf("Usage: dnw2 <file>\n\n");
}

unsigned char* prepare_write_buf(char *filename, unsigned int *len) {
	unsigned char *write_buf = NULL;
	struct stat fs;

	int fd = open(filename, O_RDONLY);
	if (-1 == fd) {
		perror("Cannot open file");
		return NULL;
	}
	if (-1 == fstat(fd, &fs)) {
		perror("Cannot get file size");
		goto error;
	}
	write_buf = (unsigned char*) malloc(fs.st_size + 10);
	if (NULL == write_buf) {
		perror("malloc failed");
		goto error;
	}

	if (fs.st_size != read(fd, write_buf + 8, fs.st_size)) {
		perror("Reading file failed");
		goto error;
	}

	printf("Filename : %s\n", filename);
	printf("Filesize : %d bytes\n", fs.st_size);

	*((u_int32_t*) write_buf) = 0x30000000; //download address
	*((u_int32_t*) write_buf + 1) = fs.st_size + 10; //download size;

	*len = fs.st_size + 10;
	return write_buf;

	error: if (fd != -1)
		close(fd);
	if (NULL != write_buf)
		free(write_buf);
	fs.st_size = 0;
	return NULL;

}

int main(int argc, char *argv[]) {
	if (2 != argc) {
		usage();
		return 1;
	}

	struct usb_dev_handle *hdev = open_port();
	if (!hdev) {
		return 1;
	}

	unsigned int len = 0;
	unsigned char* write_buf = prepare_write_buf(argv[1], &len);
	if (NULL == write_buf)
		return 1;

	unsigned int remain = len;
	unsigned int towrite;
	printf("Writing data ...\n");
	while (remain) {
		towrite = remain > 512 ? 512 : remain;
		if (towrite != usb_bulk_write(hdev, 0x03, write_buf + (len - remain),
				towrite, 3000)) {
			perror("usb_bulk_write failed");
			break;
		}
		remain -= towrite;
		printf("\r%d%\t %d bytes     ", (len - remain) * 100 / len, len
				- remain);
		fflush(stdout);
	}
	if (0 == remain)
		printf("Done!\n");
	return 0;
}
