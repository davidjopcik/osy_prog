#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define BUF 8
#define LINE 1024

static int int_to_str(int x, char *out, int max)
{
    // prevod cisla na text, vrati dlzku
    // max musi byt aspon 2
    int len = 0;

    if (max <= 0) return 0;

    if (x == 0) {
        if (max < 2) return 0;
        out[0] = '0';
        out[1] = '\0';
        return 1;
    }

    char tmp[32];
    int t = 0;

    while (x > 0 && t < (int)sizeof(tmp)) {
        tmp[t++] = (char)('0' + (x % 10));
        x /= 10;
    }

    // otocime
    while (t > 0 && len + 1 < max) {
        out[len++] = tmp[--t];
    }
    out[len] = '\0';
    return len;
}

int main()
{
    char buf[BUF];
    char line[LINE];
    int line_len = 0;
    int line_no = 1;

    int n;

    while ((n = read(STDIN_FILENO, buf, sizeof(buf))) > 0)
    {
        for (int i = 0; i < n; i++)
        {
            char c = buf[i];

            if (c == '\n')
            {
                // ukoncime riadok ako string
                line[line_len] = '\0';

                // pripravime hlavicku "N: "
                char num[32];
                int num_len = int_to_str(line_no, num, (int)sizeof(num));

                // vypis: cislo
                if (num_len > 0) write(STDOUT_FILENO, num, num_len);
                write(STDOUT_FILENO, ": ", 2);

                // vypis: text riadku
                if (line_len > 0) write(STDOUT_FILENO, line, line_len);

                // newline
                write(STDOUT_FILENO, "\n", 1);

                // reset pre dalsi riadok
                line_len = 0;
                line_no++;
            }
            else
            {
                // pridame znak do skladaneho riadku (ak sa zmesti)
                if (line_len < LINE - 1) {
                    line[line_len++] = c;
                } else {
                    // riadok je prilis dlhy, ignorujeme dalsie znaky az do '\n'
                    // (na skuske je lepsie nez spadnut)
                }
            }
        }
    }

    // ak EOF prisiel bez \n a mame rozpracovany riadok, vypiseme ho
    if (line_len > 0) {
        line[line_len] = '\0';

        char num[32];
        int num_len = int_to_str(line_no, num, (int)sizeof(num));

        if (num_len > 0) write(STDOUT_FILENO, num, num_len);
        write(STDOUT_FILENO, ": ", 2);
        write(STDOUT_FILENO, line, line_len);
        write(STDOUT_FILENO, "\n", 1);
    }

    // n == 0 -> EOF, n == -1 -> chyba (teraz len koncime)
    return 0;
}