#include <stdio.h>
#include <stdlib.h>

#include <time.h> // for time spent calculate

#include "zbar.h"
#include "image.h"

#define BITMAP
#ifdef BITMAP
// 檔案結構
#pragma pack(2)
struct BitmapFileHeader
{
    unsigned short bfTybe;
    unsigned int bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffBits;
};
struct BitmapInfoHeader
{
    unsigned int biSize;
    unsigned int biWidth;
    unsigned int biHeight;
    unsigned short biPlanes; // 1=defeaul, 0=custom
    unsigned short biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    unsigned int biXPelsPerMeter; // 72dpi=2835, 96dpi=3780
    unsigned int biYPelsPerMeter; // 120dpi=4724, 300dpi=11811
    unsigned int biClrUsed;
    unsigned int biClrImportant;
};
#pragma pack()

void Write2Bitmap(const char *name, const unsigned char *raw_img,
                  unsigned int width, unsigned int height, unsigned short bits)
{
    if (!(name && raw_img))
    {
        perror("Error Write2Bitmap.");
        return;
    }
    // 檔案資訊
    struct BitmapFileHeader file_h = {
        .bfTybe = 0x4d42,
        .bfReserved1 = 0,
        .bfReserved2 = 0,
        .bfOffBits = 54,
    };
    file_h.bfSize = file_h.bfOffBits + width * height * bits / 8;
    if (bits == 8)
    {
        file_h.bfSize += 1024, file_h.bfOffBits += 1024;
    }
    // 圖片資訊
    struct BitmapInfoHeader info_h = {
        .biSize = 40,
        .biPlanes = 1,
        .biCompression = 0,
        .biXPelsPerMeter = 0,
        .biYPelsPerMeter = 0,
        .biClrUsed = 0,
        .biClrImportant = 0,
    };
    info_h.biWidth = width;
    info_h.biHeight = height;
    info_h.biBitCount = bits;
    info_h.biSizeImage = width * height * bits / 8;
    if (bits == 8)
    {
        info_h.biClrUsed = 256;
    }
    // 寫入檔頭
    FILE *pFile = NULL;
    // pFile = fopen(name,"wb+");
    fopen_s(&pFile, name, "wb+");
    if (!pFile)
    {
        perror("Error opening file.");
        return;
    }
    fwrite((char *)&file_h, sizeof(char), sizeof(file_h), pFile);
    fwrite((char *)&info_h, sizeof(char), sizeof(info_h), pFile);
    // 寫調色盤
    if (bits == 8)
    {
        for (unsigned i = 0; i < 256; ++i)
        {
            unsigned char c = i;
            fwrite((char *)&c, sizeof(char), sizeof(unsigned char), pFile);
            fwrite((char *)&c, sizeof(char), sizeof(unsigned char), pFile);
            fwrite((char *)&c, sizeof(char), sizeof(unsigned char), pFile);
            fwrite("", sizeof(char), sizeof(unsigned char), pFile);
        }
    }
    // 寫入圖片資訊
    size_t alig = ((width * bits / 8) * 3) % 4;
    for (int j = height - 1; j >= 0; --j)
    {
        for (unsigned i = 0; i < width; ++i)
        {
            unsigned int idx = j * width + i;
            if (bits == 24)
            { // RGB圖片
                fwrite((char *)&raw_img[idx * 3 + 2],
                       sizeof(char), sizeof(unsigned char), pFile);
                fwrite((char *)&raw_img[idx * 3 + 1],
                       sizeof(char), sizeof(unsigned char), pFile);
                fwrite((char *)&raw_img[idx * 3 + 0],
                       sizeof(char), sizeof(unsigned char), pFile);
            }
            else if (bits == 8)
            { // 灰階圖
                fwrite((char *)&raw_img[idx],
                       sizeof(char), sizeof(unsigned char), pFile);
            }
        }
        // 對齊4byte
        for (size_t i = 0; i < alig; ++i)
        {
            fwrite("", sizeof(char), sizeof(unsigned char), pFile);
        }
    }
    fclose(pFile);
}
void ReadFromBitmap(const char *name, unsigned char **raw_img,
                    unsigned int *width, unsigned int *height, unsigned short *bits)
{
    if (!(name && raw_img && width && height && bits))
    {
        perror("Error ReadFromBitmap.");
        return;
    }
    // 檔案資訊
    struct BitmapFileHeader file_h;
    // 圖片資訊
    struct BitmapInfoHeader info_h;
    // 讀取檔頭
    FILE *pFile = NULL;
    // pFile = fopen(name, "rb+");
    fopen_s(&pFile, name, "rb+");
    if (!pFile)
    {
        perror("Error opening file.");
        return;
    }
    fread((char *)&file_h, sizeof(char), sizeof(file_h), pFile);
    fread((char *)&info_h, sizeof(char), sizeof(info_h), pFile);
    // 讀取長寬
    *width = info_h.biWidth;
    *height = info_h.biHeight;
    *bits = info_h.biBitCount;
    size_t ImgSize = ((size_t)*width) * ((size_t)*height) * 3;
    *raw_img = (unsigned char *)calloc(ImgSize, sizeof(unsigned char));
    // 讀取讀片資訊轉RAW檔資訊
    fseek(pFile, file_h.bfOffBits, SEEK_SET);
    size_t alig = ((info_h.biWidth * info_h.biBitCount / 8) * 3) % 4;
    for (int j = *height - 1; j >= 0; --j)
    {
        for (unsigned i = 0; i < *width; ++i)
        {
            unsigned int idx = j * (*width) + i;
            if (*bits == 24)
            { // RGB圖片
                fread((char *)&(*raw_img)[idx * 3 + 2],
                      sizeof(char), sizeof(unsigned char), pFile);
                fread((char *)&(*raw_img)[idx * 3 + 1],
                      sizeof(char), sizeof(unsigned char), pFile);
                fread((char *)&(*raw_img)[idx * 3 + 0],
                      sizeof(char), sizeof(unsigned char), pFile);
            }
            else if (*bits == 8)
            { // 灰階圖
                fread((char *)&(*raw_img)[idx],
                      sizeof(char), sizeof(unsigned char), pFile);
            }
        }
        fseek(pFile, (long)alig, SEEK_CUR);
    }
    fclose(pFile);
}

// 圖像結構
typedef struct bitmap_t
{
    unsigned int width, height;
    unsigned short bits;
    unsigned char *data;
} bitmap_t;

void BitmapRead(const bitmap_t *_this, const char *name)
{
    const bitmap_t *p = _this;
    ReadFromBitmap(name, (unsigned char **)&p->data, (unsigned int *)&p->width, (unsigned int *)&p->height, (unsigned short *)&p->bits);
}

void BitmapWrite(const bitmap_t *_this, const char *name)
{
    const bitmap_t *p = _this;
    Write2Bitmap(name, p->data, p->width, p->height, p->bits);
}
#endif

void DeleteChar(char *str, char ch)
{
    char *p = str;
    char *q = str;
    while (*q)
    {
        if (*q != ch)
        {
            *p++ = *q;
        }
        q++;
    }
    *p = '\0';
}

int main()
{
#ifdef BITMAP
    char *fileName = malloc(128 * sizeof(char));
    printf("Please Input QR Code Bmp File Path: ");
    scanf("%s", fileName);
    DeleteChar(fileName, '\"');
    bitmap_t bitmap = {0, 0, 0, NULL};
    BitmapRead(&bitmap, fileName);
    printf("BmpSize: %dx%d\r\n", bitmap.width, bitmap.height);
#endif
    clock_t t1, t2;

    t1 = clock();

    int ret;
    zbar_image_scanner_t *scanner = zbar_image_scanner_create();
    ret = zbar_image_scanner_set_config(scanner, ZBAR_NONE, ZBAR_CFG_ENABLE, 0);
    ret = zbar_image_scanner_set_config(scanner, ZBAR_QRCODE, ZBAR_CFG_ENABLE, 1);

#ifdef BITMAP
    int width = bitmap.width;
    int height = bitmap.height;
    const unsigned char *raw = bitmap.data;
#else
    int width = 216;
    int height = 216;
    const void *raw = malloc(width * height);
#endif
    zbar_image_t *image = zbar_image_create();
    zbar_image_set_size(image, width, height);
    unsigned long format = zbar_fourcc_parse("Y800");
    zbar_image_set_format(image, format);
    zbar_image_set_data(image, (unsigned char *)raw, width * height, NULL);

    ret = zbar_scan_image(scanner, image);

    t2 = clock();

    if (ret > 0)
        printf("symbols were successfully decoded\r\n");
    else if (ret == 0)
        printf("no symbols were found\r\n");
    else
        printf("an error occurs\r\n");

    if (ret > 0)
    {
        const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
        for (; symbol; symbol = zbar_symbol_next(symbol))
        {
            zbar_symbol_type_t type = zbar_symbol_get_type(symbol);
            const char *data = zbar_symbol_get_data(symbol);
            printf("decoded: %s\r\n", data);
        }
    }

    zbar_image_destroy(image);
    zbar_image_scanner_destroy(scanner);

    printf("time spent: %d ms\r\n", t2 - t1);
    printf("press ENTER to EXIT\r\n");

    fflush(stdin);   // 清除輸入緩衝區
    (void)getchar(); // wait for exit
    return 0;
}