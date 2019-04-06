#include <stdint.h>

typedef struct native_image NATIVE_IMAGE;

void qr_setup(const char *id_str, uint8_t **qr_data, int *qr_data_size, NATIVE_IMAGE **qr_image, int *qr_image_size);
