#include "connection.h"

static lq_connection_t g_connection = {0};

void lq_connection_init(int max) {
    // +1 for server_sockfd listen
    g_connection.n_max = max+1;
}

lq_connection_t *lq_get_connection() {
    return &g_connection;
}

void lq_connection_create(lq_event_t *event) {
    LQ_DEBUG("connection_create. n_current[%d], n_max[%d]", g_connection.n_current, g_connection.n_max);
    lq_connection_data_t *connection_data = LQ_MALLOC(sizeof(lq_connection_data_t));
    if (connection_data == NULL) {
        LQ_WARNING("LQ_MALLOC() failed.");
        return;
    }

    connection_data->fd = event->fd;
    connection_data->event = event;
    connection_data->last_active = lq_time();

    // add to head
    connection_data->next = g_connection.head;
    connection_data->prev = NULL;
    if (g_connection.head != NULL) {
        g_connection.head->prev = connection_data;
    }
    g_connection.head = connection_data;
    if (g_connection.tail == NULL) {
        g_connection.tail = connection_data;
    }

    event->connection = connection_data;

    ++g_connection.n_current;
    ++g_connection.n_connected;

    connection_data->will_close = 0;
    if (g_connection.n_current > g_connection.n_max) {
        LQ_DEBUG("too many connections. will close.");
        connection_data->will_close = 1;
    }
}

void lq_connection_active(lq_event_t *event) {
    LQ_DEBUG("connection_active. fd[%d]", event->fd);
    lq_connection_data_t *connection_data = event->connection;
    connection_data->last_active = lq_time();
    if (connection_data->prev == NULL) {
        // now is head
        LQ_DEBUG("connection is head already.");
        return;
    }

    connection_data->prev->next = connection_data->next;
    if (connection_data->next != NULL) {
        connection_data->next->prev = connection_data->prev;
    } else {
        g_connection.tail = connection_data->prev;
    }
    connection_data->prev = NULL;
    g_connection.head->prev = connection_data;
    connection_data->next = g_connection.head;
    g_connection.head = connection_data;
}

void lq_connection_close(lq_event_t *event) {
    LQ_DEBUG("connection_close. fd[%d]", event->fd);
    lq_connection_data_t *connection_data = event->connection;
    if (connection_data->prev != NULL) {
        connection_data->prev->next = connection_data->next;
        if (connection_data->next != NULL) {
            connection_data->next->prev = connection_data->prev;
        } else {
            g_connection.tail = connection_data->prev;
            connection_data->prev->next = NULL;
        }
    } else {
        g_connection.head = connection_data->next;
        if (connection_data->next != NULL) {
            connection_data->next->prev = NULL;
        } else {
            g_connection.tail = NULL;
        }
    }

    LQ_FREE(connection_data);
    event->connection = NULL;
    --g_connection.n_current;
    ++g_connection.n_close;
}



















/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
