#include <string.h>

#include <neeze.h>

static neeze_param_t g_neeze_param;
static neeze_result_t g_neeze_result;

void neeze_get_param(void* p) {
    memcpy(p, &g_neeze_param, sizeof(g_neeze_param));
}

void neeze_set_result(const void* p) {
    memcpy(&g_neeze_result, p, sizeof(g_neeze_result));
}

int neeze_set_key(const char* key) {
    if (strlen(key) < sizeof(g_neeze_param.key_bytes)) {
        LOGE("invalid key length: %d < %d\n", 
                strlen(key), sizeof(g_neeze_param.key_bytes));
        return -1;
    }

    memcpy(g_neeze_param.key_bytes, key, sizeof(g_neeze_param.key_bytes));

    return 0;
}

int neeze_get_sender_ip(char buff[], int buff_len) {
    char ip_text[16];
    neeze_result_t* r = &g_neeze_result;

    if (g_neeze_result.es_result.state != EASY_SETUP_STATE_DONE) {
        LOGE("easy setup data unavailable\n");
        return -1;
    }

    if (r->host_ip_address.version != COOEE_IPV4) {
        return -1;
    }

    int ip = r->host_ip_address.ip.v4;
    snprintf(ip_text, sizeof(ip_text), "%d.%d.%d.%d",
            (ip>>24)&0xff,
            (ip>>16)&0xff,
            (ip>>8)&0xff,
            (ip>>0)&0xff);
    ip_text[16-1] = 0;

    if ((size_t) buff_len < strlen(ip_text)+1) {
        LOGE("insufficient buffer provided: %d < %d\n", buff_len, strlen(ip_text)+1);
        return -1;
    }

    strcpy(buff, ip_text);

    return 0;
}

int neeze_get_sender_port(uint16* port) {
    if (g_neeze_result.es_result.state != EASY_SETUP_STATE_DONE) {
        LOGE("easy setup data unavailable\n");
        return -1;
    }

    *port = g_neeze_result.host_port;

    return 0;
}
