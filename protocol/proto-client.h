#ifndef ACMS_SERVER_PROTO_SERVER_H
#define ACMS_SERVER_PROTO_SERVER_H
#define APP_MSG_NUM 2

#include "ap_data.h"
#include "app_sv_handlers.h"
#include "app_cl_handlers.h"
#include "../server/socket.h"
#include "mqueue.h"

size_t pl_size[APP_MSG_NUM];

void init_handlers() {
    pl_size[0] = sizeof(p_auth);
    pl_size[1] = sizeof(p_auth_resp);
}

int prepare_packet(int cmd, char *sender, g_msg *msg, void* data, size_t size) {
    msg->command = cmd;
    sprintf(msg->sender, "%s", sender);
    memcpy(&msg->payload, data, size);
    return 0;
}

int prepare_packet_id(int cmd, char *sender, g_msg *msg, void* data, size_t size) {
    static int last_id;
    msg->command = cmd;
    msg->id = last_id++;
    sprintf(msg->sender, "%s", sender);
    memcpy(&msg->payload, data, size);
    return 0;
}


int log_msg(g_msg *message)
{
    printf("[ACMS][MSG:%d] => Type: %d || Sender %s \n", message->id, message->command, message->sender);
}


int client_message_handler(socket_peer * peer, g_msg* msg) {
    log_msg(msg);
    switch (msg->command) {
        case P_AUTH_RESP: {
            handle_p_auth_resp(msg);
//            char out_buffer [60] = "Suck";
            if (mq_send(*peer->qd_client, (const char*) msg, sizeof(g_msg), 0) == -1)
                perror ("Sender: Not able to send message to client");
            else
                printf("[ACMS][MQ] Sended: %s\n", (char*)&msg);
            break;
        }
    }
};






#endif //ACMS_SERVER_PROTO_SERVER_H
