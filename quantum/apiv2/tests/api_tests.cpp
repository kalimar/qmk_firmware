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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

extern "C" {
#include "api.h"
}

using testing::_;
using testing::Return;

class GetDriverMock {
public:
    GetDriverMock() { s_instance = this; }
    ~GetDriverMock() { s_instance = nullptr; }
    MOCK_METHOD1(get_driver, api_driver_t* (uint8_t));

    static GetDriverMock* s_instance;
};

GetDriverMock* GetDriverMock::s_instance = nullptr;

api_driver_t* api_get_driver(uint8_t endpoint) {
    if (GetDriverMock::s_instance) {
        return GetDriverMock::s_instance->get_driver(endpoint);
    }
    else {
        return nullptr;
    }
}


template<size_t id>
class DriverMock : public api_driver_t {
public:
    DriverMock() {
        api_driver_t::connect = [](uint8_t endpoint) {
            if (DriverMock::s_instance) {
                return DriverMock::s_instance->connect(endpoint);
            } else {
                return false;
            }
        };
        s_instance = this;
    }
    ~DriverMock() {
        s_instance = nullptr;
    }
    MOCK_METHOD1(connect, bool (uint8_t endpoint));
private:
    static DriverMock* s_instance;
};

template<size_t id>
DriverMock<id>* DriverMock<id>::s_instance = nullptr;

TEST(Api, ConnectingToANonRegisteredEndpointFails) {
    GetDriverMock mock;
    EXPECT_CALL(mock, get_driver(_)).WillRepeatedly(Return(nullptr));
    EXPECT_FALSE(api_connect(3));
}

TEST(Api, ASuccessfulConnection) {
    GetDriverMock mock;
    DriverMock<1> driver;
    EXPECT_CALL(mock, get_driver(3)).WillRepeatedly(Return(&driver));
    EXPECT_CALL(driver, connect(3)).WillOnce(Return(true));
    EXPECT_TRUE(api_connect(3));
}

TEST(Api, AFailedConnection) {
    GetDriverMock mock;
    DriverMock<1> driver;
    EXPECT_CALL(mock, get_driver(1)).WillRepeatedly(Return(&driver));
    EXPECT_CALL(driver, connect(1)).WillOnce(Return(false));
    EXPECT_FALSE(api_connect(1));
}
