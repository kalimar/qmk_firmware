/* Copyright 2016 Fred Sundvik
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "api.h"
#include <stddef.h>
#include <string.h>

#ifndef API_MAX_CONNECTED_ENDPOINTS
#error Please define API_MAX_CONNECTED_ENDPOINTS
#endif

#define NOT_CONNECTED API_ENDPOINT_BROADCAST

static uint8_t outgoing_connections[API_MAX_CONNECTED_ENDPOINTS] = {
    [0 ... API_MAX_CONNECTED_ENDPOINTS - 1] = NOT_CONNECTED
};

static uint8_t aligned_packet[API_MAX_SIZE] __attribute__((aligned(API_ALIGN)));

static bool s_response_sent = false;

static uint8_t* get_connection(uint8_t endpoint, uint8_t* connections) {
    for (int i=0; i < API_MAX_CONNECTED_ENDPOINTS; ++i) {
        uint8_t* c = &connections[i];
        if (*c == endpoint) {
            return c;
        }
    }
    return NULL;
}

static uint8_t* get_or_reserve_connection(uint8_t endpoint, uint8_t* connections) {
    uint8_t* free_connection = NULL;
    for (int i=0; i < API_MAX_CONNECTED_ENDPOINTS; ++i) {
        uint8_t* e = &connections[i];
        if (*e == NOT_CONNECTED && free_connection == NULL) {
            free_connection = e;
        }
        else if(*e == endpoint) {
            return e;
        }
    }
    return free_connection;
}

static void disconnect_endpoint(uint8_t endpoint, uint8_t* connections) {
    uint8_t* c = get_connection(endpoint, connections);
    if (c) {
        *c = NOT_CONNECTED;
    }
}

bool api_connect(uint8_t endpoint) {
    uint8_t* c = get_or_reserve_connection(endpoint, outgoing_connections);
    if (!c) {
        return false;
    }
    if (*c != NOT_CONNECTED) {
        return true;
    }
    *c = endpoint;
    api_driver_t* driver = api_get_driver(endpoint);
    if (driver) {
        if (driver->connect(endpoint)) {
            req_connect req;
            req.protocol_version = API_PROTOCOL_VERSION;
            res_connect* resp = API_SEND_AND_RECV(endpoint, connect, &req);
            if (resp && resp->successful) {
                return true;
            }
        }
    }
    if (*c == endpoint) {
        *c = NOT_CONNECTED;
    }
    return false;
}

bool api_is_connected(uint8_t endpoint) {
    return get_connection(endpoint, outgoing_connections) != NULL;
}

void api_reset(void) {
    for (int i=0; i < API_MAX_CONNECTED_ENDPOINTS; ++i) {
        outgoing_connections[i] = NOT_CONNECTED;
    }
    s_response_sent = false;
}

static void process_incoming_connect(uint8_t endpoint, req_connect* req, res_connect* resp) {
    resp->successful = req->protocol_version == API_PROTOCOL_VERSION;
}

static void process_internal(uint8_t endpoint, api_packet_t* packet, uint8_t size) {
    switch(packet->id) {
        API_HANDLE(connect, process_incoming_connect);
    }
}

static void add_packet(uint8_t endpoint, void* buffer, uint8_t size) {
    if (size < sizeof(api_packet_t) ) {
        res_protocol_error res;
        res.error = PROTOCOL_ERROR_INCOMING_TOO_SMALL;
        API_SEND_RESPONSE(endpoint, protocol_error, &res);
        return;
    }

    if (((uintptr_t)(buffer) % API_ALIGN) != 0) {
        #if API_DRIVER_ALIGNS
            return;
        #else
            uint8_t s = size > API_MAX_SIZE ? API_MAX_SIZE : size;
            memcpy(aligned_packet, buffer, s);
            buffer = aligned_packet;
        #endif
    }

    api_packet_t* packet = (api_packet_t*)(buffer);
    // We should not receive responses if we are not waiting for it
    if (packet->is_response) {
        res_protocol_error res;
        res.error = PROTOCOL_ERROR_UNEXPECTED_RESPONSE;
        API_SEND_RESPONSE(endpoint, protocol_error, &res);
        return;
    }
    // TODO: Should not accept any packets unless the remote is connected
    // Should also disconnect on the above errors
    // Note that connected here means the the incoming connection, not the outgoing one
    s_response_sent = false;

    switch (packet->id) {
    case api_internal_begin ... api_internal_end:
        process_internal(endpoint, packet, size);
        break;
    case api_qmk_begin ... api_qmk_end:
        api_process_qmk(endpoint, packet, size);
        break;
    case api_keyboard_begin ... api_keyboard_end:
        api_process_keyboard(endpoint, packet, size);
        break;
    case api_keymap_begin ... api_keymap_end:
        api_process_keymap(endpoint, packet, size);
        break;
    }

    if (!s_response_sent) {
        res_unhandled unhandled;
        unhandled.original_request = packet->id;
        API_SEND_RESPONSE(endpoint, unhandled, &unhandled);
    }
}

void api_send_response(uint8_t endpoint, uint16_t command, void* buffer, uint8_t size) {
    if (s_response_sent) {
        return;
    }
    api_driver_t* driver = api_get_driver(endpoint);
    if (driver) {
        api_packet_t* packet = (api_packet_t*)(buffer);
        packet->id = command;
        packet->is_response = true;
        driver->send(endpoint, packet, size);
    }
    s_response_sent = true;
}

void api_send(uint8_t endpoint, uint16_t command, void* data, uint8_t size) {
    if (size < sizeof(api_packet_t)) {
        //TODO: This is not properly tested
        disconnect_endpoint(endpoint, outgoing_connections);
        return;
    }
    api_packet_t* send_packet = (api_packet_t*)(data);
    send_packet->id = command;
    send_packet->is_response = false;
    api_driver_t* driver = api_get_driver(endpoint);
    bool connected = api_is_connected(endpoint);
    if (connected) {
        connected = driver->send(endpoint, data, size);
        // TODO: Probably need to be tested separately
        if (!connected) {
            disconnect_endpoint(endpoint, outgoing_connections);
        }
    }
}

void* api_recv_response(uint8_t endpoint, uint16_t command, uint8_t size) {
    bool connected = api_is_connected(endpoint);
    api_driver_t* driver = api_get_driver(endpoint);
    while (connected) {
        uint8_t recv_endpoint = endpoint;
        uint8_t actual_recv_size;
        api_packet_t* res = (api_packet_t*)driver->recv(&recv_endpoint, &actual_recv_size);
        if (!res)  {
            connected = false;
        }

        if (((uintptr_t)(res) % API_ALIGN) != 0) {
            #if API_DRIVER_ALIGNS
                connected = false;
            #else
                uint8_t s = actual_recv_size > API_MAX_SIZE ? API_MAX_SIZE : actual_recv_size;
                memcpy(aligned_packet, res, s);
                res = (api_packet_t*)(aligned_packet);
            #endif
        }

        if (actual_recv_size >= sizeof(api_packet_t) && res->is_response == false) {
            // TODO: This need to be unit tested
            // Now partly unit tested for connection packets
            add_packet(recv_endpoint, res, actual_recv_size);
            continue;
        }
        else if (recv_endpoint == endpoint) {
            if (actual_recv_size == size && res->id == command) {
                return res;
            }
            else if (actual_recv_size == sizeof(res_unhandled) && res->id == api_command_unhandled) {
                connected = false;
            }
            else {
                connected = false;
            }
        } else {
            // We got something unexpected from another endpoint, most likely the wrong response, so
            // disconnect if we have an open connection
            disconnect_endpoint(recv_endpoint, outgoing_connections);
        }
    }
    // Complete the disconnection
    disconnect_endpoint(endpoint, outgoing_connections);
    return NULL;
}

void* api_send_and_recv(uint8_t endpoint, uint16_t command, void* data, uint8_t size, uint8_t recv_size) {
    api_send(endpoint, command, data, size);
    return api_recv_response(endpoint, command, recv_size);
}

void api_process_driver(api_driver_t* driver) {
    uint8_t size;
    uint8_t endpoint = API_ENDPOINT_BROADCAST;
    void* res = driver->recv(&endpoint, &size);
    while (res) {
        add_packet(endpoint, res, size);
        endpoint = API_ENDPOINT_BROADCAST;
        res = driver->recv(&endpoint, &size);
    };
}
