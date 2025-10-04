#include <stdio.h>
#include <stdbool.h>

static int days_in_month(int y, int m)
{
    if (m == 4 || m == 6 || m == 9 || m == 11)
        return 30;
    if (m == 2)
    {
        int leap = ((y % 4 == 0) && ((y % 100 != 0) || (y % 400 == 0)));
        return leap ? 29 : 28;
    }
    return 31;
}

void verification(long S, long *weight, bool onlyValid)
{
    int d[10] = {0};
    long tmp = S;
    for (int i = 9; i >= 0; --i)
    {
        d[i] = (int)(tmp % 10);
        tmp /= 10;
    }
    if (tmp > 0)
    {
        if (!onlyValid)
            printf("RC: %010ld: Neplatny\n", S);
        return;
    }

    int yearNum = d[0] * 10 + d[1];
    int monthRaw = d[2] * 10 + d[3];
    int dayNum = d[4] * 10 + d[5];

    int monthNum = -1;
    if (monthRaw >= 1 && monthRaw <= 12)
        monthNum = monthRaw;
    else if (monthRaw >= 51 && monthRaw <= 62)
        monthNum = monthRaw - 50;

    int year = (yearNum >= 54) ? (1900 + yearNum) : (2000 + yearNum);
    int ok_date = (monthNum >= 1 && monthNum <= 12 && dayNum >= 1 && dayNum <= days_in_month(year, monthNum));

    int sum = 0;
    for (int i = 0; i < 10; i++)
        sum += d[i];
    int ok_mod11 = (sum % 11 == 0);

    int ok = ok_date && ok_mod11;

    if (onlyValid)
    {
        if (ok)
            printf("RC: %010ld: Platny\n", S);
    }
    else
    {
        printf("RC: %010ld: %s\n", S, ok ? "Platny" : "Neplatny");
    }
}