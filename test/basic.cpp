#include <config.h>

bool t01()
{
    return true;
}

int main()
{
    if (
        t01() &&
        true)
    {
        return 0;
    }
    return -1;
}
