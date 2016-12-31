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

#ifndef QUANTUM_APIV2_API_H_
#define QUANTUM_APIV2_API_H_

#include <stdint.h>
#include <stdbool.h>
#define BEGIN_MSG typedef struct __attribute((packed, aligned(4))) { \
    uint16_t id : 15; \
    uint16_t is_response : 1;

#define END_MSG(name) } name;
#include "api_commands.h"
#include "api_requests.h"
#include "api_responses.h"

bool api_connect(uint8_t endpoint);
bool api_is_connected(uint8_t endpoint);
void api_reset(void);

typedef struct {
    // * This should start initiating a physical and transport level connection to the the endpoint.
    // * The call should be non-blocking, and can be called multiple times until it succeeds.
    // * The driver can perform the connecting in the background and return true when it succeeds.
    // * Either physical or transport level connection can be skipped if the driver don't need to do that
    // * Once the connection has succeeded, a reliable communication channel should be established.
    // * Note, you are only allowed to connect to endpoints with a lower id than yourself, this to prevent
    //   some race conditions in the connection protocol
    bool (*connect)(uint8_t endpoint);

    // * Should perform a blocking send, but doesn't need to wait for the confirmation of the other side.
    // * If it can't physically send yet, it could just save the buffer for later use
    // * Should return false only if the link is or gets disconnected during the send
    bool (*send)(uint8_t endpoint, void* buffer, uint8_t size);

    // * Should return a buffer if data was received, it then also sets the endpoint and the size
    // * If endpoint is specified, should perform a blocking read, that is ended by the following condtions
    //   1. Data is received from the specified endpoint
    //   2. Data is received from another endpoint on the same driver
    //   3. The specified endpoint is disconnected
    // * When no endpoint is specified, it should perform a non-blocking read
    // * Returns NULL if there's no data to be read, or if the specified endpoint is disconnected
    // * The returned buffer should be valid at least until the next send or recv is performed on the driver
    void* (*recv)(uint8_t* endpoint, uint8_t* size);
}api_driver_t;

// The keyboard should implement this, you can return NULL for unsupported endpoints
api_driver_t* api_get_driver(uint8_t endpoint);


#endif /* QUANTUM_APIV2_API_H_ */
