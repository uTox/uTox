#ifndef STB_H
#define STB_H

#include "stb_image.h"
#include "stb_image_write.h"

// uTox can't find this function unless declared here.
extern unsigned char *stbi_write_png_to_mem(unsigned char *pixels, int stride_bytes,
                                            int x, int y, int n, int *out_len);

#endif
