#ifndef __NEEZE_H__
#define __NEEZE_H__

#include <common.h>
#include <easy_setup.h>

#define NEEZE_KEY_STRING_LEN (16)

typedef struct {
    uint8 key_bytes[NEEZE_KEY_STRING_LEN];
} neeze_param_t;

typedef struct {
    easy_setup_result_t es_result;
    ip_address_t host_ip_address;      /* setup client's ip address */
    uint16 host_port;            /* setup client's port */
} neeze_result_t;

void neeze_get_param(void* p);
void neeze_set_result(const void* p);

int neeze_set_key(const char* key);
int neeze_get_sender_ip(char buff[], int buff_len);
int neeze_get_sender_port(uint16* port);

#endif /* __NEEZE_H__ */
