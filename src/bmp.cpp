#include "../header/bmp.hpp"
#include <iostream>
#include <string>
#include <cstdlib>

int *loadBMP(const char *fname) {
    FILE *f = fopen(fname, "rb" );
    if(!f )
        return NULL;
    BMPheader bh;    // File header sizeof(BMPheader) = 56
    size_t res;

    // читаем заголовок
    res = fread( &bh, 1, sizeof(BMPheader), f );
    if( res != sizeof(BMPheader)) {
        fclose(f);
        return NULL;
    }

    // проверка размера файла
    fseek( f, 0, SEEK_END);
    int filesize = ftell(f);
    // восстановим указатель в файле:
    fseek( f, sizeof(BMPheader), SEEK_SET);

    // проверяем сигнатуру
    if( bh.bfType!=0x4d42 && bh.bfType!=0x4349
            && bh.bfType!=0x5450) {
        fclose(f);
        return NULL;
    }
    if ((filesize - sizeof(BMPheader)) % 16 != 0)
        std::cout << "Need to change file size\n";
    std::cout << "File size " << filesize << std::endl;
    return NULL;
}
