# Copyright 2016 Fred Sundvik
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

api_SRC := $(API2_PATH)/tests/api_tests.cpp $(API2_SRC)
api_DEFS := $(API2_DEFS) -DAPI_MAX_CONNECTED_ENDPOINTS=8
api_INC := $(API2_PATH)