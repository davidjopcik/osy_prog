// Jednoduchá verzia pre začiatočníkov: použije rand()/srand()
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cstdlib>

int main(int argc, char **argv) {
    // Očakávané použitie: ./numgen MIN MAX COUNT [vystup.txt]
    if (argc < 4 || argc > 5) {
        std::fprintf(stderr, "Usage: %s MIN MAX COUNT [vystup.txt]\n", argv[0]);
        return 1;
    }

    int min = std::atoi(argv[1]);
    int max = std::atoi(argv[2]);
    int count = std::atoi(argv[3]);

    if (min > max) {
        std::fprintf(stderr, "MIN must be <= MAX\n");
        return 1;
    }
    if (count < 0) {
        std::fprintf(stderr, "COUNT must be >= 0\n");
        return 1;
    }

    FILE *out = stdout;
    if (argc == 5) {
        out = std::fopen(argv[4], "w");
        if (!out) {
            std::fprintf(stderr, "Cannot open file: %s\n", argv[4]);
            return 2;
        }
    }

    // Seed náhodného generátora podľa aktuálneho času
    std::srand((unsigned)std::time(nullptr));

    // Vypíš COUNT náhodných čísel v rozsahu <min, max>, každé na nový riadok
    for (int i = 0; i < count; ++i) {
        int value;
        if (max == min) {
            value = min;
        } else {
            int span = (max - min + 1);
            value = min + std::rand() % span; // jednoduché riešenie pre začiatočníkov
        }
        std::fprintf(out, "%d\n", value);
    }

    if (out != stdout) {
        std::fclose(out);
    }

    return 0;
}
