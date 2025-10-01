#include <cstdlib>
#include <cstdio>
#include <cstring>

int main() {

    FILE* in = fopen("vstup.txt", "r");
    char line[256];
    int sum = 0;
    while (fgets(line, sizeof line, in))
    {
        printf("%s", line);
        sum = sum + atoi(line);
    }

    printf("\n%d", sum);


    return 0;
}