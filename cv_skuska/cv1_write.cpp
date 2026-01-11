#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define LINE 1024
    
int main() {
    char buf[8];
    char line[LINE];
    int line_len = 0;

    int n = 0;
    int line_no = 1;

    while ((n = read(STDIN_FILENO, buf, sizeof(buf))) > 0)
    {

     for (int i = 0; i < n; i++)
     {

        if (buf[i] == '\n')
        {   
            char num[20];
            int len = snprintf(num, sizeof(num), "%d", line_no);
            num[len] = '\0';
            write(STDOUT_FILENO, num, len);
            write(STDOUT_FILENO, ":", 1);
            write(STDOUT_FILENO, line, line_len);
            write(STDOUT_FILENO, "\n", 1);
            line_len = 0;
            line_no++;
        }
        else {
            if (line_len < LINE - 1)
            {
                line[line_len] = buf[i];
            }
            line_len++;
        }
     }
    }
    return 0;
}