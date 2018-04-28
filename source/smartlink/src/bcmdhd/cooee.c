#include <string.h>

#include <cooee.h>

static cooee_param_t g_cooee_param;
static cooee_param_t g_qqcon_param = {
    .key_bytes = {
        'a', 'a', '6', 'b', 'c', '5', '1', '4', 
        '-', '6', 'e', '7', '2', '-', '4', 'a'},
    .random_bytes = {
        0x00, 0x00, 0x00, 0x00, 0x71, 0x71, 0x63, 0x6f, 
        0x6e, 0x6e, 0x65, 0x63, 0x74},
};

static cooee_result_t g_cooee_result;
static int qqcon = 0;

void cooee_get_param(void* p) {
    if (qqcon) {
        memcpy(p, (void*) &g_qqcon_param, sizeof(g_cooee_param));
    } else {
        memcpy(p, (void*) &g_cooee_param, sizeof(g_cooee_param));
    }
}

void cooee_set_result(const void* p) {
    memcpy((void*) &g_cooee_result, p, sizeof(g_cooee_result));
}

void cooee_enable_qqconnect() {
    qqcon = 1;
}

int cooee_set_key(const char* key) {
    if (strlen(key) < sizeof(g_cooee_param.key_bytes)) {
        LOGE("invalid key length: %d < %d\n", 
                strlen(key), sizeof(g_cooee_param.key_bytes));
        return -1;
    }

    memcpy(g_cooee_param.key_bytes, key, sizeof(g_cooee_param.key_bytes));
    memcpy(g_qqcon_param.key_bytes, key, sizeof(g_qqcon_param.key_bytes));

    return 0;
}

int cooee_get_sender_ip(char buff[], int buff_len) {
    char ip_text[16];
    cooee_result_t* r = &g_cooee_result;

    if (g_cooee_result.es_result.state != EASY_SETUP_STATE_DONE) {
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

int cooee_get_sender_port(uint16* port) {
    if (g_cooee_result.es_result.state != EASY_SETUP_STATE_DONE) {
        LOGE("easy setup data unavailable\n");
        return -1;
    }

    *port = g_cooee_result.host_port;

    return 0;
}
