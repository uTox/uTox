#ifndef UTOX_STB_H
#define UTOX_STB_H

// uTox needs this internal stb function.
unsigned char *stbi_write_png_to_mem(unsigned char *pixels, int stride_bytes,
                                     int x, int y, int n, int *out_len);

#endif
