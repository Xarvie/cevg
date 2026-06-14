#ifdef _MSC_VER
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- PPM output (no dependencies) ---- */
static inline void test_write_ppm(const char* path, const unsigned char* rgba, int w, int h) {
    FILE* f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "Cannot write %s\n", path); return; }
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int i = 0; i < w * h; i++) {
        unsigned char r = rgba[4*i+0], g = rgba[4*i+1], b = rgba[4*i+2], a = rgba[4*i+3];
        if (a > 0 && a < 255) { r = (unsigned char)(r*255/a); g = (unsigned char)(g*255/a); b = (unsigned char)(b*255/a); }
        fputc(r, f); fputc(g, f); fputc(b, f);
    }
    fclose(f);
    printf("  Wrote %s (%dx%d)\n", path, w, h);
}

/* ---- PNG output (via stb_image_write) ---- */
#ifdef TEST_UTIL_STB_IMAGE_WRITE_IMPL
  #define STB_IMAGE_WRITE_IMPLEMENTATION
  #include "stb_image_write.h"
#endif

int stbi_write_png(const char* filename, int w, int h, int comp,
                   const void* data, int stride_in_bytes);

static inline void test_write_png(const char* path, const unsigned char* rgba, int w, int h) {
    /* Un-premultiply alpha for PNG (RGBA straight) */
    unsigned char* buf = (unsigned char*)malloc(w * h * 4);
    if (!buf) return;
    for (int i = 0; i < w * h; i++) {
        unsigned char r = rgba[4*i+0], g = rgba[4*i+1], b = rgba[4*i+2], a = rgba[4*i+3];
        if (a > 0 && a < 255) {
            buf[4*i+0] = (unsigned char)(r*255/a);
            buf[4*i+1] = (unsigned char)(g*255/a);
            buf[4*i+2] = (unsigned char)(b*255/a);
        } else {
            buf[4*i+0] = r; buf[4*i+1] = g; buf[4*i+2] = b;
        }
        buf[4*i+3] = a;
    }
    int ok = stbi_write_png(path, w, h, 4, buf, w * 4);
    free(buf);
    if (ok) printf("  Wrote %s (%dx%d)\n", path, w, h);
    else    fprintf(stderr, "  Failed to write %s\n", path);
}
