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

#ifndef QUANTUM_APIV2_API_COMMANDS_H_
#define QUANTUM_APIV2_API_COMMANDS_H_

enum api_commands {
    api_internal_begin=0,
    api_command_connect,
    api_command_unhandled, // Only valid as a response, returned when the remote doesn't handle the request
    api_internal_end=15,
    api_qmk_begin=16,
    // Don't add anything before this line
    // Don't add anything after this here, define another enum in your keyboard or keymap instead
    api_qmk_end = 8191,
    api_keyboard_begin = 8192,
    api_keyboard_end = 16383,
    api_keymap_begin = 16384,
    api_keymap_end = 24575,
};


#endif /* QUANTUM_APIV2_API_COMMANDS_H_ */
