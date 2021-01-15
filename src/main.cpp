#include "../header/aes.hpp"
#include "../header/bmp.hpp"
#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
    AES tmp;
    int flag;

    flag = 0;
    if (argc < 2) {
        std::cout << argv[0] << " <flag(-h for help)>" 
            << " <first file name> <second file name> <128 bits key>\n";
    }
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            char *c;

            c = argv[i];
            c++;
            while (*c != '\0') {
                switch (*c) {
                    case 'h':
                        std::cout << " Flag:\n" << "-h for help\n"
                            << "-t for text\n" << "-p for picture\n"
                            << "-d to decrypt\n" << "-e to encrypt\n"
                            << "default -te\n";
                            return 1;
                    case 't':
                        flag |= TEXT;
                        std::cout << "set flag t\n";
                        break;
                    case 'p':
                        flag |= PIC;
                        std::cout << "set flag p\n";
                        break;
                    case 'd':
                        flag |= DEC;
                        std::cout << "set flag d\n";
                        break;
                    case 'e':
                        flag |= ENC; 
                        std::cout << "set flag e\n";
                        break;
                    default:
                        std::cout << "Unknown flag " << *c << std::endl;
                        break;
                }
                c++;
            }
        } else {
            if (flag < 5) {
                std::cout << "No flag specified\n";
                return 1;
            }
            if (argc - i == 3) {
                tmp.readFile(flag, argv[i], argv[i + 1], argv[i + 2]);
                break;
            } else if (argc - i == 2) {
                tmp.readFile(flag, argv[i], argv[i + 1], NULL);
                break;
            } else {
                std::cout << "Wrong arguments\n";
                return 1;
            }
        }
    }
    return 0;
}
