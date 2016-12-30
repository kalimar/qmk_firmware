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

#ifndef API_MAX_CONNECTED_ENDPOINTS
#error Please define API_MAX_CONNECTED_ENDPOINTS
#endif

typedef struct {
    uint8_t endpoint;
    bool is_valid;
}connected_endpoint_t;

static connected_endpoint_t connected_endpoints[API_MAX_CONNECTED_ENDPOINTS] = {
    [0 ... API_MAX_CONNECTED_ENDPOINTS - 1] = {.is_valid = false }
};

connected_endpoint_t* get_endpoint(uint8_t endpoint) {
    for (int i=0; i < API_MAX_CONNECTED_ENDPOINTS; ++i) {
        connected_endpoint_t* e = &connected_endpoints[i];
        if (e->is_valid && e->endpoint == endpoint) {
            return e;
        }
    }
    return NULL;
}

connected_endpoint_t* get_or_create_endpoint(uint8_t endpoint) {
    connected_endpoint_t* free_endpoint = NULL;
    for (int i=0; i < API_MAX_CONNECTED_ENDPOINTS; ++i) {
        connected_endpoint_t* e = &connected_endpoints[i];
        if (!e->is_valid && free_endpoint == NULL) {
            free_endpoint = e;
        }
        else if(e->is_valid && e->endpoint == endpoint) {
            return e;
        }
    }
    if (free_endpoint) {
        free_endpoint->endpoint = endpoint;
    }
    return free_endpoint;
}


bool api_connect(uint8_t endpoint) {
    connected_endpoint_t* e = get_or_create_endpoint(endpoint);
    if (!e) {
        return false;
    }
    if (e->is_valid) {
        return true;
    }
    api_driver_t* driver = api_get_driver(endpoint);
    if (driver) {
        bool connected =  driver->connect(endpoint);
        e->is_valid = connected;
        return connected;
    }
    else {
        return false;
    }
}

bool api_is_connected(uint8_t endpoint) {
    return get_endpoint(endpoint) != NULL;
}

void api_reset(void) {
    for (int i=0; i < API_MAX_CONNECTED_ENDPOINTS; ++i) {
        connected_endpoint_t* e = &connected_endpoints[i];
        e->is_valid = false;
    }
}
