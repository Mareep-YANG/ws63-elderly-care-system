#include "stdbool.h"
#include "bits/alltypes.h"

// Maintain mapping between SLE conn_id and server board (P/E)
#ifndef AGENT_MODULE_MAIN_CONN_MAP_H
#define AGENT_MODULE_MAIN_CONN_MAP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char server_type;     /* 'P' or 'E' */
    uint16_t conn_id;     /* 0xFFFF if invalid */
} server_conn_t;

/* Two servers at most: P and E */
#define SERVER_CONN_MAX 2

extern server_conn_t g_server_conn_map[SERVER_CONN_MAX];

/*
 * Update mapping. If server_type already exists, its conn_id will be updated.
 */
void set_conn_id(char server_type, uint16_t conn_id);

/*
 * Get conn_id by server_type. Returns 0xFFFF when not found.
 */
uint16_t get_conn_id(char server_type);

#ifdef __cplusplus
}
#endif

#endif // AGENT_MODULE_MAIN_CONN_MAP_H