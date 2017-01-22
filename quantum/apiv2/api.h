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

#define API_ALIGN 4
#define API_ENDPOINT_BROADCAST 255
#define API_MAX_SIZE 32
// If the driver automatically aligns the packet, then you can define this in your keyboard
// This would save API_MAX_SIZE bytes of memory
//#define API_DRIVER_ALIGNS

// The first two bytes of this structure can never change
// But you could potentially add more fields if the protocol version is bumped
typedef struct __attribute__((packed, aligned(API_ALIGN))) {
    uint16_t id : 15;
    uint16_t is_response : 1;
} api_packet_t;

// Note, this define has to match the api_packet_t above
#define BEGIN_MSG typedef struct __attribute__((packed, aligned(API_ALIGN))) { \
    uint16_t id : 15; \
    uint16_t is_response : 1;
#define END_MSG(name) } name;


#include "api_commands.h"
#include "api_requests.h"
#include "api_responses.h"

// You should call this until it returns true. Otherwise no requests will be sent, and no responses received
// Note that the connection is one-way, so you can still receive incoming requests without this
bool api_connect(uint8_t endpoint);
// You can also check if the endpoint is connected
bool api_is_connected(uint8_t endpoint);

// Send a request to the endpoint asynchronically
void api_send(uint8_t endpoint, uint16_t command, void* data, uint8_t size);
// Send a response to the endpoint asynchronically, you don't normally call this, instead use API_HANDLE
void api_send_response(uint8_t endpoint, uint16_t command, void* buffer, uint8_t size);
// Receive a response to the last command that has one, this will block until the response is received
// or the connection is disconnected. When there's a disconnect, NULL is returned. NOTE: You should always
// assume that there can be disconnects, unless you know that it physically can't happen. So be prepared to
// handle NULL pointers as result.
void* api_recv_response(uint8_t endpoint, uint16_t command, uint8_t size);
// Combines send and recv in a single call.
void* api_send_and_recv(uint8_t endpoint, uint16_t command, void* data, uint8_t size, uint8_t recv_size);

// Some convenience macros for the above functions. This should almost always be preferred since they do some
// other safety checks as well. Note that the command argument is the command id without the
// api_command_ prefix. The msg is a pointer to the actual data to send

#define API_SEND(endpoint, command, msg)  \
    ({req_##command* req_tmp = msg; \
    api_send(endpoint, api_command_##command, req_tmp, sizeof(req_##command));})

#define API_SEND_RESPONSE(endpoint, command, msg) \
    ({res_##command* res_tmp = msg; \
    api_send_response(endpoint, api_command_##command, res_tmp, sizeof(res_##command));})

#define API_RECV_RESPONSE(endpoint, command, msg) \
    (res_##command*)api_recv_response(endpoint, api_command_##command, sizeof(res_##command))

#define API_SEND_AND_RECV(endpoint, command, msg) \
    ({req_##command* req_tmp = msg; \
    (res_##command*)api_send_and_recv(endpoint, api_command_##command, req_tmp, sizeof(req_##command), \
    		                          sizeof(res_##command));})

// Reset the whole API system, should not be used outside of unit tests
void api_reset(void);

typedef struct {
    // This should start initiating a physical and transport level connection to the the endpoint.
    // The call should be non-blocking, and can be called multiple times until it succeeds.
    // The driver can perform the connecting in the background and return true when it succeeds.
    // Either physical or transport level connection can be skipped if the driver don't need to do that
    // Once the connection has succeeded, a reliable communication channel should be established.
    // Note: the connections are unidirectional, so even if a one way connection is not established, the
    // connection in the opposite direction doesn't have to be. If the other side need to send commands, it
    // needs to connect too. The same thing applies to disconnect, the connection could be disconnected in one
    // direction but not the other.
    bool (*connect)(uint8_t endpoint);

    // Should perform a blocking send, but doesn't need to wait for the confirmation of the other side.
    // If it can't physically send yet, it could just save the buffer for later use
    // Should return false only if the link is or gets disconnected during the send
    bool (*send)(uint8_t endpoint, void* buffer, uint8_t size);

    // Should return a buffer if data was received, it then also sets the endpoint and the size
    // If endpoint isn't API_ENDPOINT_BROADCAST, should perform a blocking read, that is ended by the
    // following conditions
    // 1. Data is received from the specified endpoint
    // 2. Data is received from another endpoint on the same driver
    // 3. The specified endpoint is disconnected
    // When the endpoint is API_ENDPOINT_BROADCAST, it should perform a non-blocking read
    // Returns NULL if there's no data to be read, or if all endpoints are disconnected
    // The returned buffer should be valid at least until the next send or recv is performed on the driver
    // You don't need to align the returned pointer, but if possible then it's better to do so. If all drivers
    // in use do that, then  you can define API_DRIVER_ALIGNS to save some memory
    void* (*recv)(uint8_t* endpoint, uint8_t* size);
}api_driver_t;

// The keyboard should implement this, you can return NULL for unsupported endpoints
// TODO: The memory usage could be slightly reduced if the api_driver_t was stored in PROGMEM
api_driver_t* api_get_driver(uint8_t endpoint);

// The keyboard should call this once per scan loop for each driver. The function will check for incoming
// packets and call the api_process_* functions below when receiving packets
void api_process_driver(api_driver_t* driver);

// QMK, the keyboard and the keymap should implement these
// They are called when new incoming packets are received, use API_HANDLE below inside a switch statement like
// this:
// api_process_qmk(uint8_t endpoint, api_packet_t* packet, uint8_t size) {
//    switch (packet->id) {
//        API_HANDLE(qmk, handle_qmk);
//    }
//}

void api_process_qmk(uint8_t endpoint, api_packet_t* packet, uint8_t size);
void api_process_keyboard(uint8_t endpoint, api_packet_t* packet, uint8_t size);
void api_process_keymap(uint8_t endpoint, api_packet_t* packet, uint8_t size);

// Use this macro to handle incoming requests inside a switch statement inside the the above process
// functions.
// The id is the command id excluding the prefix "api_command_"
// The function is a function to be called when the request is received
#define API_HANDLE(id, function) \
    case api_command_##id: \
        if (sizeof(req_##id) == size) { \
            req_##id* request = (req_##id*)(packet); \
            res_##id response; \
            function(endpoint, request, &response); \
            api_send_response(endpoint, api_command_##id, &response, sizeof(response)); \
        } \
        break;

#endif /* QUANTUM_APIV2_API_H_ */
