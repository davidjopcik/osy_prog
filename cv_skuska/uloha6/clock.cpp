#include <iostream>
#include <unistd.h>

int main()
{

    std::cout << "clock: start" << std::endl;


    for (int i = 1; i <= 5; i++)
    {
        std::cout << "tick " << i << std::endl;
        usleep(200 * 1000); // 200 ms
    }

    std::cout << "clock: end" << std::endl;
    return 0;
}

