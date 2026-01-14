//***************************************************************************
//
// animal.cpp
//
// Testovaci program pre OSY zadanie.
// Kompiluje sa pomocou:
//   g++ -D CAT animal.cpp -o animal
//   g++ -D DOG animal.cpp -o animal
//   g++ -D LION animal.cpp -o animal
//   g++ -D SNAKE animal.cpp -o animal
//
//***************************************************************************

#include <stdio.h>
#include <unistd.h>

int main()
{
#if defined(CAT)

    printf("🐱 CAT\n");
    printf("Meow! Meow!\n");

#elif defined(DOG)

    printf("🐶 DOG\n");
    printf("Woof! Woof!\n");

#elif defined(LION)

    printf("🦁 LION\n");
    printf("Roooar!!!\n");

#elif defined(SNAKE)

    printf("🐍 SNAKE\n");
    printf("Sssssss...\n");

#else

    printf("Unknown animal!\n");

#endif

    printf("PID: %d\n", getpid());
    printf("Program finished.\n");

    return 0;
}