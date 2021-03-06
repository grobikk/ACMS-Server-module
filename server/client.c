#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include "socket.h"
#include "msg.h"
#include "console.h"
#include "../protocol/proto-server.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
socket_peer server;
char client_name[256];

void shutdown_properly(int code);

int get_client_name(int argc, char **argv)
{
    if (argc > 1) strcpy(client_name, argv[1]);
    else strcpy(client_name, "dispatcher-x");
    return 0;
}

int connect_server(socket_peer *server)
{
    server->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->socket < 0) {
    perror("socket()");
    return -1;
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IPV4_ADDR);
    server_addr.sin_port = htons(SERVER_LISTEN_PORT);
    server->address = server_addr;
    if (connect(server->socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) != 0) {
    perror("connect()");
    return -1;
    }
    printf("[ACMS Client ] Connected! %s:%d.\n", SERVER_IPV4_ADDR, SERVER_LISTEN_PORT);
    return 0;
}

int build_fd_sets(socket_peer *server, fd_set *read_fds, fd_set *write_fds, fd_set *except_fds)
{
    FD_ZERO(read_fds);
    FD_SET(STDIN_FILENO, read_fds);
    FD_SET(server->socket, read_fds);
    FD_ZERO(write_fds);
    if (server->send_buffer.current > 0)
    FD_SET(server->socket, write_fds);
    FD_ZERO(except_fds);
    FD_SET(STDIN_FILENO, except_fds);
    FD_SET(server->socket, except_fds);
    return 0;
}

int handle_read_from_stdin(socket_peer *server, char *client_name)
{
    printf("Select command (AUTH - 0):");
    char read_buffer[DATA_MAXSIZE]; // buffer for stdin
    if (read_console(read_buffer, DATA_MAXSIZE) != 0) return -1;
//    g_msg new_message;
//    init_handlers();
//    p_auth p = {"Aziz", "ajganiev", "123"};
//    prepare_packet(0, client_name, &new_message, &p);
//    log_msg(&new_message);
//    mq_send(server, &new_message);
    return 0;
}

void shutdown_properly(int code)
{
    sp_delete(&server);
    printf("[ACMS Client] Going down.\n");
    exit(code);
}

void send_auth() {
    g_msg msg;
    p_auth p = {"Aziz", "ajganiev", "123"};
    prepare_packet_id(P_AUTH, client_name, &msg, &p, sizeof(p_auth));
    server_mq_send(&server, &msg);
}

int main(int argc, char **argv)
{
    get_client_name(argc, argv);
    printf("[ACMS Client] User '%s'\n", client_name);
    sp_create(&server);
    if (connect_server(&server) != 0) shutdown_properly(EXIT_FAILURE);
    int flag = fcntl(STDIN_FILENO, F_GETFL, 0);
    flag |= O_NONBLOCK;
    fcntl(STDIN_FILENO, F_SETFL, flag);
    fd_set read_fds;
    fd_set write_fds;
    fd_set except_fds;
    int maxfd = server.socket;
    send_auth();
    while (1) {
        build_fd_sets(&server, &read_fds, &write_fds, &except_fds);
        int activity = select(maxfd + 1, &read_fds, &write_fds, &except_fds, NULL);
        switch (activity) {
          case -1:
            perror("select()");
            shutdown_properly(EXIT_FAILURE);
          case 0:
            printf("select() returns 0.\n");
            shutdown_properly(EXIT_FAILURE);
          default:
            if (FD_ISSET(STDIN_FILENO, &read_fds)) {
              if (handle_read_from_stdin(&server, client_name) != 0)
                shutdown_properly(EXIT_FAILURE);
            }
            if (FD_ISSET(STDIN_FILENO, &except_fds)) {
              printf("except_fds for stdin.\n");
              shutdown_properly(EXIT_FAILURE);
            }
            if (FD_ISSET(server.socket, &read_fds)) {
              if (sp_recv(&server, &client_message_handler) != 0)
                shutdown_properly(EXIT_FAILURE);
            }
            if (FD_ISSET(server.socket, &write_fds)) {
              if (sp_send(&server) != 0)
                shutdown_properly(EXIT_FAILURE);
            }
            if (FD_ISSET(server.socket, &except_fds)) {
              printf("except_fds for server.\n");
              shutdown_properly(EXIT_FAILURE);
            }
        }
  }
}

#pragma clang diagnostic pop