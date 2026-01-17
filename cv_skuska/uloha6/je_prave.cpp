#include <iostream>
#include <unistd.h>

int main()
{
% TOTO JE ZLY KOMENTAR - server to NESMIE zapisat do clock.cpp
% dalsi zly riadok, stale musi byt ignorovany

    std::cout << "clock: start" << std::endl;

% aj toto ignoruj, nech vidis ze filtrovanie funguje aj uprostred

    for (int i = 1; i <= 5; i++)
    {
        std::cout << "tick " << i << std::endl;
        usleep(200 * 1000); // 200 ms
    }

    std::cout << "clock: end" << std::endl;
    return 0;
}



