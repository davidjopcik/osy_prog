//***************************************************************************
//
// Program example for subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2021
//
// Example of socket server/client.
//
// This program is example of socket client.
// The mandatory arguments of program is IP adress or name of server and
// a port number.
//
//***************************************************************************

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>  
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>

#define STR_CLOSE "close"

//***************************************************************************
// log messages

#define LOG_ERROR 0 // errors
#define LOG_INFO 1  // information and notifications
#define LOG_DEBUG 2 // debug messages

// debug flag
int g_debug = LOG_INFO;

void log_msg(int t_log_level, const char *t_form, ...)
{
    const char *out_fmt[] = {
        "ERR: (%d-%s) %s\n",
        "INF: %s\n",
        "DEB: %s\n"};

    if (t_log_level && t_log_level > g_debug)
        return;

    char l_buf[1024];
    va_list l_arg;
    va_start(l_arg, t_form);
    vsprintf(l_buf, t_form, l_arg);
    va_end(l_arg);

    switch (t_log_level)
    {
    case LOG_INFO:
    case LOG_DEBUG:
        fprintf(stdout, out_fmt[t_log_level], l_buf);
        break;

    case LOG_ERROR:
        fprintf(stderr, out_fmt[t_log_level], errno, strerror(errno), l_buf);
        break;
    }
}

//***************************************************************************
// help

void help(int t_narg, char **t_args)
{
    if (t_narg <= 1 || !strcmp(t_args[1], "-h"))
    {
        printf(
            "\n"
            "  Socket client example.\n"
            "\n"
            "  Use: %s [-h -d] ip_or_name port_number\n"
            "\n"
            "    -d  debug mode \n"
            "    -h  this help\n"
            "\n",
            t_args[0]);

        exit(0);
    }

    if (!strcmp(t_args[1], "-d"))
        g_debug = LOG_DEBUG;
}

//***************************************************************************

int main(int t_narg, char **t_args)
{

    if (t_narg <= 2)
        help(t_narg, t_args);

    int l_port = 0;
    char *l_host = nullptr;
    char *l_resolution = nullptr;
    char *l_name = nullptr;

    // parsing arguments
    for (int i = 1; i < t_narg; i++)
    {
        if (!strcmp(t_args[i], "-d"))
            g_debug = LOG_DEBUG;

        if (!strcmp(t_args[i], "-h"))
            help(t_narg, t_args);

        if (*t_args[i] != '-')
        {
            if (!l_host)
                l_host = t_args[i];
            else if (!l_port)
                l_port = atoi(t_args[i]);
            else if (!l_resolution)
                l_resolution = t_args[i];
            else if (!l_name)
                l_name = t_args[i];
        }
    }

    if (!l_host || !l_port)
    {
        log_msg(LOG_INFO, "Host or port is missing!");
        help(t_narg, t_args);
        exit(1);
    }

    log_msg(LOG_INFO, "Connection to '%s':%d. Resolution is: %s\n", l_host, l_port, l_resolution);

    addrinfo l_ai_req, *l_ai_ans;
    bzero(&l_ai_req, sizeof(l_ai_req));
    l_ai_req.ai_family = AF_INET;
    l_ai_req.ai_socktype = SOCK_STREAM;

    int l_get_ai = getaddrinfo(l_host, nullptr, &l_ai_req, &l_ai_ans);
    if (l_get_ai)
    {
        log_msg(LOG_ERROR, "Unknown host name!");
        exit(1);
    }

    sockaddr_in l_cl_addr = *(sockaddr_in *)l_ai_ans->ai_addr;
    l_cl_addr.sin_port = htons(l_port);
    freeaddrinfo(l_ai_ans);

    // socket creation
    int l_sock_server = socket(AF_INET, SOCK_STREAM, 0);
    if (l_sock_server == -1)
    {
        log_msg(LOG_ERROR, "Unable to create socket.");
        exit(1);
    }

    // connect to server
    if (connect(l_sock_server, (sockaddr *)&l_cl_addr, sizeof(l_cl_addr)) < 0)
    {
        log_msg(LOG_ERROR, "Unable to connect server.");
        exit(1);
    }

    uint l_lsa = sizeof(l_cl_addr);
    // my IP
    getsockname(l_sock_server, (sockaddr *)&l_cl_addr, &l_lsa);
    log_msg(LOG_INFO, "My IP: '%s'  port: %d",
            inet_ntoa(l_cl_addr.sin_addr), ntohs(l_cl_addr.sin_port));
    // server IP
    getpeername(l_sock_server, (sockaddr *)&l_cl_addr, &l_lsa);
    log_msg(LOG_INFO, "Server IP: '%s'  port: %d",
            inet_ntoa(l_cl_addr.sin_addr), ntohs(l_cl_addr.sin_port));

    log_msg(LOG_INFO, "Enter 'close' to close application.");

    char line[128];
    int n = snprintf(line, sizeof(line), "%s\n", l_name);


    //write data
    for (int off = 0; off < n;)
    {
        ssize_t w = write(l_sock_server, line + off, n - off);
        off += (int)w;
    } 


    //read data
    int out_fd = open("runme.xz", O_CREAT | O_TRUNC | O_WRONLY, 0644);

    char buf[4096];
    while (1)
    {
        ssize_t r = read(l_sock_server, buf, sizeof(buf));
        if (r == 0)
            break;

        ssize_t off = 0;
        while (off < r)
        {
            ssize_t w = write(out_fd, buf + off, r - off);
            off += w;
        }
    }
    close(out_fd);

    //struct stat st{};
    //stat("runme.xz", &st);
    //if(S_ISREG(st.st_mode)) {
     
        pid_t p1 = fork();

        if (p1 == 0)
        {
            execlp("xz", "xz", "-d", "runme.xz", NULL);
            _exit(1);
        }
        wait(NULL);

        pid_t p2 = fork();
        if (p2 == 0)
        {
            chmod("runme", 0755);
            close(l_sock_server);


            _exit(1);
        }
        wait(NULL);
        //waitpid(p2, NULL, 0);

        execlp("./runme", "runme", NULL);



    // close socket


    return 0;
}
