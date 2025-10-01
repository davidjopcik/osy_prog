#include <cstdio>
#include <cstring>
#include <ctime>
#include <cstdlib>

int main(int argc, char **argv)
{

    if (argc < 4 || argc > 5)
    {
        fprintf(stderr, "Nespravny pocet argumentov");
    }
    

    int min = std::atoi(argv[1]);
    int max = std::atoi(argv[2]);
    int count = std::atoi(argv[3]);

    std::srand((unsigned)std::time(nullptr));

    FILE *out = stdout;
    if (argc == 5)
    {
        out = fopen(argv[4], "w");
        if (!out)
        {
            std::fprintf(stderr, "Subor sa neda otvorit");
        }
        
    }
    else {
        out = stdout;
    }
    

    for (int i = 0; i < count; i++)
        {
            int value = min + std::rand() % (max - min);
            fprintf(out, "%d\n", value);
        };

    return 0;
}