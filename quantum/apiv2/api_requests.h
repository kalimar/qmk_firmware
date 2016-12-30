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

#ifndef QUANTUM_APIV2_API_REQUESTS_H_
#define QUANTUM_APIV2_API_REQUESTS_H_

// The requests and responses should always be defined as
// typedef struct __attribute((packed, aligned(4))) {
//     fields
// }; req_name
// Where name should match the command id, in commands.h
// for respones use res_name instead of req_name

// You are only allowed to define the following field types
// uint8_t
// int8_t
// uint16_t
// int16_t
// uint32_t
// int32_t
// uint64_t
// int64_t
// float
// And arrays of those

// All data types are stored in little_ending, if you are on a big endian platform, you need to do manual
// conversion.

// Specifically you are not allowed to use char, int, unsigned int and so on, as their sizes are not well
// defined. For char it's additionally not defined if they are unsigned or signed

// Floats are allowed but not doubles, as on AVR for example double is exactly the same as float, that is 32
// bits. The floating format should be 32-bit IEEE 754. If you are using some exotic platform that doesn't
// natively support that, you need to convert it before storing and using.

// Standard bools are not allowed either, as their size is not defined, and typically the 1 byte that they
// take is too much if you have many bools. Instead use a bit field, if you have less than or equal to 8
// booleans, use an uint8_t bitfield. 16 or less, uint16_t and so on.
// An example
// uint8_t boolean1 : 1;
// uint8_t boolean2 : 1;

// You can use bitfields for compression as well, but you cannot mix data types, as at least Microsoft Visual
// C++ derives from the c standard. So the following is allowed
// uint8_t value1 : 3;
// uint8_t value2 : 4;
// But not
// uint8_t value1 : 3;
// int8_t value2 : 1;
// If your really need different datastructures, use casting in two steps
// uint8_t temp = value2;
// int8_t signed_value = (int8_t)temp;

// You can use nested structs, but those have to be packed as well.

// Unions are also well defined, as long as any nested structs are packed.

// Finally you can of course use uint8_t buffers and manual serialization, but that's not recommended


typedef struct __attribute((packed, aligned(4))) {
    uint8_t protocol_version;
} req_connect;

#endif /* QUANTUM_APIV2_API_REQUESTS_H_ */
