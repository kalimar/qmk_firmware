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
    API_QMK_BEGIN=0,
    // Don't add anything before this line
    API_CONNECT,
    // Don't add anything after this here, define another enum in your keyboard or keymap instead
    API_QMK_END = 8191,
    API_KEYBOARD_BEGIN = 8192,
    API_KEYBOARD_END = 16383,
    API_KEYMAP_BEGIN = 16384,
    API_KEYMAP_END = 24575,
};


#endif /* QUANTUM_APIV2_API_COMMANDS_H_ */
