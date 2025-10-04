#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include "verify.h"

static void print_padded_u64(uint64_t x){ printf("%010llu",(unsigned long long)x); }

int main(int argc, char **argv){
    int only_valid=0, binary_mode=0;
    for (int i=1;i<argc;i++){
        if (argv[i][0]=='-' && argv[i][1]=='v') only_valid=1;
        else if (argv[i][0]=='-' && argv[i][1]=='b') binary_mode=1;
    }
    if (binary_mode){
        uint64_t v; ssize_t n;
        while((n=read(STDIN_FILENO,&v,sizeof(v)))==sizeof(v)){
            int ok=verify(v);
            if(!only_valid||ok){ print_padded_u64(v); printf("  %s\n", ok?"platny":"neplatny"); }
        }
    } else {
        uint64_t v;
        while (scanf("%llu",(unsigned long long*)&v)==1){
            int ok=verify(v);
            if(!only_valid||ok){ print_padded_u64(v); printf("  %s\n", ok?"platny":"neplatny"); }
        }
    }
    return 0;
}
