#include "../header/aes.hpp"
#include <algorithm>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <iterator>
#include <unordered_map>

AES::AES() {
    nb = 4;
    nk = 4;
    nr = 10;
    //create keySchedule to save keys
    keySchedule = new uint8_t *[nb];
    for (int i = 0; i < 4; ++i)
        keySchedule[i] = new uint8_t[nb*(nr + 1)];
}

AES::~AES() {
    for (int i = 0; i < nb; ++i)
        delete [] keySchedule[i];
    delete [] keySchedule;
}

uint8_t* AES::encrypt(uint8_t *s) {
    uint8_t **tab;
    std::unordered_map<uint8_t*, uint8_t*> map;

    if (map.empty() || map.find(s) == map.end())
    tab = createState(s);

    addRoundKey(tab, 0);
    for (int i = 1; i < nr; ++i) {
        subBytes(tab, ENC);
        shiftRows(tab, ENC);
        mixColumns(tab, ENC);
        addRoundKey(tab, i);
    }
    subBytes(tab, ENC);
    shiftRows(tab, ENC);
    addRoundKey(tab, nr);

    createLine(tab, s);
    for (int i = 0; i < 4; ++i)
        delete [] tab[i];
    delete [] tab;
    return s;
}

uint8_t* AES::decrypt(uint8_t *s) {
    uint8_t **tab;

    tab = createState(s);
    addRoundKey(tab, nr);
    for (int i = nr - 1; i > 0; --i) {
        shiftRows(tab, DEC);
        subBytes(tab, DEC);
        addRoundKey(tab, i);
        mixColumns(tab, DEC);
    }
    shiftRows(tab, DEC);
    subBytes(tab, DEC);
    addRoundKey(tab, 0);

    createLine(tab, s);
    for (int i = 0; i < 4; ++i)
        delete [] tab[i];
    delete [] tab;
    return s;
}

void AES::readFile(int flag, const char *fFile, const char *sFile, char *key) {
    int readSize;
    int filesize;
    uint8_t *buf;

    readKey(flag, key); 
    
    FILE *in = fopen(fFile, "rb");
    FILE *out = fopen(sFile, "w");
    if (in == NULL || out == NULL) 
        std::cout << "Error: can't open\n";

    if (flag & PIC) {
        fseek(in, 0, SEEK_END);

        filesize = ftell(in);
        //return pointer to position after bmp header

        fseek(in, 0, SEEK_SET);
        buf = new uint8_t[BMP_HEADER_SIZE];
        fread(buf, 1, BMP_HEADER_SIZE, in);
        fwrite(buf, 1, BMP_HEADER_SIZE, out);
        delete [] buf;
    }
    buf = new uint8_t[16];
    while ((readSize = std::fread(buf, 1, 16, in)) > 0) {
        if (readSize != 16) {
            memset(buf + readSize, 0, 16 - readSize);
            buf[15] = (uint8_t)16 - readSize; 
        }
        buf = (flag & ENC) ? encrypt(buf) : decrypt(buf);
        //check for block additing
        if ((flag & DEC) == 1 && buf[14] == 0 && buf[15] < 0x10)
            fwrite(buf, 1, (int)(16 - buf[15]), out);
        else
            fwrite(buf, 1, 16, out);
    }
    if ((flag & PIC) ) {
        if ((flag & ENC) && (filesize - BMP_HEADER_SIZE % 16) != 0) {
            filesize += (filesize - BMP_HEADER_SIZE) % 16;
            fseek(out, 2, SEEK_SET);
            fwrite(&filesize, 1, sizeof(filesize), out);
        } else if ((flag & DEC) == 1 && buf[14] == 0 && buf[15] < 0x10) {
            filesize -= (int)(16 - buf[15]);
            fseek(out, 2, SEEK_SET);
            fwrite(&filesize, 1, sizeof(filesize), out);
        }
    }
    delete [] buf;
    fclose(in);
    fclose(out);
}

void AES::readKey(int flag, char *key) {
    char c;

    if (flag & ENC && key == NULL) {
        key = new char[16];
        srand(time(0));
        for (int i = 0; i < 16; ++i)
            key[i] = rand() % 96 + 33; 
    }

    if (flag & ENC)
        std::cout << key << std::endl;
    keyExpansion(key);
}

void AES::keyExpansion(std::string key) {
    //save key to first block of keySchedule
    int p = 0;
    for (int i = 0; i < 4; ++i) 
        for (int j = 0; j < nb; ++j)
            keySchedule[j][i] = static_cast<uint8_t>(key[p++]);

    //fill in other blocks of keySchedule
    for (int i = 4; i < nb*(nr + 1); ++i) {
       if (i % nk == 0) {
            int x, y;

            //fill our column with left shifting
            for (int j = 0; j < 3; ++j)
                keySchedule[j][i] = keySchedule[j + 1][i - 1];
            keySchedule[3][i] = keySchedule[0][i - 1];
            //find values from sBox using column's value and XOR it with rCol
            //and i - nk column
            for (int j = 0; j < 4; ++j) {
                //get x and y for sBox
                x = (0xf0 & keySchedule[j][i]) >> 4;
                y = 0x0f & keySchedule[j][i];
                keySchedule[j][i] = keySchedule[j][i - 4] ^ sBox[x][y]
                    ^ rCon[j][i / (3)];
            }
       } else {
           for (int j = 0; j < 4; ++j)
               keySchedule[j][i] = keySchedule[j][i - 4] ^ keySchedule[j][i - 1];
       }
    }
}

void AES::subBytes(uint8_t **s, Mode m) {
    int x, y;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < nb; ++j) {
            x = (0xf0 & s[j][i]) >> 4;
            y = 0x0f & s[j][i];
            s[j][i] = (m == ENC) ? sBox[x][y] : invSBox[x][y];
        }
    }
}

//cyclic shif left of state
void AES::shiftRows(uint8_t **s, Mode m) {
    if (m == ENC) {
        for (int i = 1; i < 4; ++i)
            std::rotate(&(s[i][0]), &(s[i][i]), &(s[i][nb]));
    } else {
        for (int i = 1; i < 4; ++i)
            std::rotate(&(s[i][0]), &(s[i][nb - i]), &(s[i][nb]));
    }
}

void AES::mixColumns(uint8_t **s, Mode m) {
    int s0, s1, s2, s3;

    for (int i = 0; i < nb; ++i) {
        if (m == ENC) {
            s0 = mult_by_02(s[0][i]) ^ mult_by_03(s[1][i]) 
                ^ s[2][i] ^ s[3][i];
            s1 = s[0][i] ^ mult_by_02(s[1][i])
                ^ mult_by_03(s[2][i]) ^ s[3][i];
            s2 = s[0][i] ^ s[1][i] 
                ^ mult_by_02(s[2][i]) ^ mult_by_03(s[3][i]);
            s3 = mult_by_03(s[0][i]) ^ s[1][i] 
                ^ s[2][i] ^ mult_by_02(s[3][i]);
        } else {
            s0 = mult_by_0e(s[0][i]) ^ mult_by_0b(s[1][i])
                ^ mult_by_0d(s[2][i]) ^ mult_by_09(s[3][i]);
            s1 = mult_by_09(s[0][i]) ^ mult_by_0e(s[1][i])
                ^ mult_by_0b(s[2][i]) ^ mult_by_0d(s[3][i]);
            s2 = mult_by_0d(s[0][i]) ^ mult_by_09(s[1][i])
                ^ mult_by_0e(s[2][i]) ^ mult_by_0b(s[3][i]);
            s3 = mult_by_0b(s[0][i]) ^ mult_by_0d(s[1][i])
                ^ mult_by_09(s[2][i]) ^ mult_by_0e(s[3][i]);
        }
        s[0][i] = s0;
        s[1][i] = s1;
        s[2][i] = s2;
        s[3][i] = s3;
    }
}

void AES::addRoundKey(uint8_t **s, int n) {
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < nb; j++) {
            s[i][j] ^= keySchedule[i][j + nb * n]; 
        }
}

uint8_t** AES::createState(uint8_t *s) {
    uint8_t **tab;
    int p;

    p = 0;
    tab = new uint8_t *[4];
    for (int i = 0; i < 4; ++i)
        tab[i] = new uint8_t[4];

    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            tab[j][i] = s[p++];
    return tab;
}

void AES::createLine(uint8_t **tab, uint8_t *s) {
    int p;

    p = 0;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            s[p++] = tab[j][i];
}

int mult_by_02(uint8_t n) {
    int res;

    if (n < 0x80)
        res = (n << 1);
    else
        res = (n << 1) ^ 0x1b;
    return res % 0x100;
}

int mult_by_03(uint8_t n) {
    return mult_by_02(n) ^ n;
}

int mult_by_09(uint8_t n) {
    return mult_by_02(mult_by_02(mult_by_02(n))) ^ n;
}

int mult_by_0b(uint8_t n) {
     return mult_by_02(mult_by_02(mult_by_02(n))) ^ mult_by_02(n) ^ n;
}

int mult_by_0d(uint8_t n) {
     return mult_by_02(mult_by_02(mult_by_02(n))) ^ mult_by_02(mult_by_02(n)) ^ n;
}

int mult_by_0e(uint8_t n) {
     return mult_by_02(mult_by_02(mult_by_02(n)))
        ^ mult_by_02(mult_by_02(n)) ^ mult_by_02(n);
}

uint8_t* invert_half(uint8_t* s) {
    uint8_t *res;

    res = new uint8_t[16];
    for (int i = 0; i < 16; ++i)
        res[i] = (i < 8) ? ~s[i] : s[i];
    return res;
}

uint8_t* invert_even(uint8_t* s) {
    uint8_t *res;
    uint8_t v = 0x55;

    res = new uint8_t[16];
    for (int i = 0; i < 16; ++i)
        res[i] = s[i] ^ 0x55;
    return res;
}
