#include <iostream>
#include "yac.hpp"

void foo()
{
    for (int i = 0; i < 10; ++i)
    {
        std::cout << i << std::endl;
        yac::yield();
    }
}

int main()
{
    int id1 = yac::create(foo);
    int id2 = yac::create(foo);

    for (int i = 0; i < 10; ++i)
    {
        yac::resume(id1);
        yac::resume(id2);
    }

    yac::destory(id1);
    yac::destory(id2);
    return 0;
}
