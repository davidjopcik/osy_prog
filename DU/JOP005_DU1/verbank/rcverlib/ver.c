#include <stdio.h>
#include <stdbool.h>
#include "ver.h"

static bool is_bank_ok(long S, const long w[10]) {
    long x = S;
    long sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += (x % 10) * w[i];
        x /= 10;
    }
    return (sum % 11) == 0;
}

static int days_in_month(int y, int m) {
    if (m==4||m==6||m==9||m==11) return 30;
    if (m==2) {
        int leap = ((y%4==0) && ((y%100!=0) || (y%400==0)));
        return leap ? 29 : 28;
    }
    return 31;
}

static bool is_rc_ok(long S) {
    int d[10]={0}; long t=S;
    for (int i=9;i>=0;--i){ d[i]=(int)(t%10); t/=10; }
    if (t>0) return false; 

    int YY = d[0]*10 + d[1];
    int MMraw = d[2]*10 + d[3];
    int DD = d[4]*10 + d[5];

    int MM = -1;
    if      (MMraw>= 1 && MMraw<=12) MM = MMraw;       
    else if (MMraw>=51 && MMraw<=62) MM = MMraw - 50;  
    else                             return false;

    int year = (YY >= 54) ? (1900+YY) : (2000+YY);
    if (!(MM>=1 && MM<=12 && DD>=1 && DD<=days_in_month(year,MM))) return false;

    int sum=0; for (int i=0;i<10;i++) sum += d[i];
    return (sum % 11) == 0;
}

void verification(long S, long *weight, bool valid)
{
    const long default_w[10] = {1,2,4,8,5,10,9,7,3,6};
    const long *w = weight ? weight : default_w;

    bool ok = is_rc_ok(S) && is_bank_ok(S, w);

    if (valid) {
        if (ok) printf("Platne: %010ld\n", S);
    } else {
        printf("RC aj BANK: %010ld: %s\n", S, ok ? "Platny" : "Neplatny");
    }
}
