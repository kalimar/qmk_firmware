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
using testing::DoAll;
using testing::Invoke;
using testing::WithArg;

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
class DriverMock {
public:
    DriverMock() {
        m_driver.connect =
        [](uint8_t endpoint) {
            return DriverMock::s_instance->connect(endpoint);
        };
        m_driver.send =
        [](uint8_t endpoint, void* buffer, uint8_t size) {
            return DriverMock::s_instance->send(endpoint, buffer, size);
        };
        m_driver.recv =
        [](uint8_t* endpoint, uint8_t* size) {
            return DriverMock::s_instance->recv(endpoint, size);
        };
        s_instance = this;
    }
    ~DriverMock() {
        s_instance = nullptr;
    }
    api_driver_t* get_driver() { return &m_driver; }


    MOCK_METHOD1(connect, bool (uint8_t endpoint));
    MOCK_METHOD3(send, bool (uint8_t endpoint, void* buffer, uint8_t size));
    MOCK_METHOD2(recv, void* (uint8_t* endpoint, uint8_t* size));
private:
    static DriverMock* s_instance;
    api_driver_t m_driver;
};

template<size_t id>
DriverMock<id>* DriverMock<id>::s_instance = nullptr;

class Api : public testing::Test
{
public:
    Api() {
        api_reset();
    }
};

TEST_F(Api, ConnectingToANonRegisteredEndpointFails) {
    EXPECT_FALSE(api_is_connected(3));
    GetDriverMock mock;
    EXPECT_CALL(mock, get_driver(_)).WillRepeatedly(Return(nullptr));
    EXPECT_FALSE(api_connect(3));
    EXPECT_FALSE(api_is_connected(3));
}

TEST_F(Api, ASuccessfulConnection) {
    EXPECT_FALSE(api_is_connected(3));
    GetDriverMock mock;
    DriverMock<1> driver;
    res_connect resp;
    resp.successful = 1;
    EXPECT_CALL(mock, get_driver(3)).WillRepeatedly(Return(driver.get_driver()));
    EXPECT_CALL(driver, connect(3)).WillOnce(Return(true));
    EXPECT_CALL(driver, send(3, _, _)).WillOnce(Return(true));
    EXPECT_CALL(driver, recv( _, _)).WillOnce(Return(&resp));
    EXPECT_TRUE(api_connect(3));
    EXPECT_TRUE(api_is_connected(3));
    // Another endpoint should not be connected
    EXPECT_FALSE(api_is_connected(2));
}

TEST_F(Api, AFailedConnection) {
    EXPECT_FALSE(api_is_connected(3));
    GetDriverMock mock;
    DriverMock<1> driver;
    EXPECT_CALL(mock, get_driver(3)).WillRepeatedly(Return(driver.get_driver()));
    EXPECT_CALL(driver, connect(3)).WillOnce(Return(false));
    EXPECT_FALSE(api_connect(3));
    EXPECT_FALSE(api_is_connected(3));
}

TEST_F(Api, AFailedConnectionThroughFailureDuringConnectionPacketSendAndReceive) {
    EXPECT_FALSE(api_is_connected(3));
    GetDriverMock mock;
    DriverMock<1> driver;
    EXPECT_CALL(mock, get_driver(3)).WillRepeatedly(Return(driver.get_driver()));
    EXPECT_CALL(driver, connect(3)).WillOnce(Return(true));
    EXPECT_CALL(driver, send(3, _, _)).WillOnce(Return(false));
    EXPECT_FALSE(api_connect(3));
    EXPECT_FALSE(api_is_connected(3));
}

TEST_F(Api, AFailedConnectionDueToRemoteNotAccepting) {
    EXPECT_FALSE(api_is_connected(3));
    GetDriverMock mock;
    DriverMock<1> driver;
    res_connect resp;
    resp.successful = 0;
    EXPECT_CALL(mock, get_driver(3)).WillRepeatedly(Return(driver.get_driver()));
    EXPECT_CALL(driver, connect(3)).WillOnce(Return(true));
    EXPECT_CALL(driver, send(3, _, _)).WillOnce(Return(true));
    EXPECT_CALL(driver, recv(_, _)).WillOnce(Return(&resp));
    EXPECT_FALSE(api_connect(3));
    EXPECT_FALSE(api_is_connected(3));
}

TEST_F(Api, TryingToConnectWhenAlreadyConnectedDoesNothing) {
    GetDriverMock mock;
    DriverMock<1> driver;
    res_connect resp;
    resp.successful = 1;
    EXPECT_CALL(mock, get_driver(1)).WillOnce(Return(driver.get_driver()));
    EXPECT_CALL(driver, connect(1)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(driver, send(1, _, _)).WillOnce(Return(true));
    EXPECT_CALL(driver, recv(_, _)).WillOnce(Return(&resp));
    EXPECT_TRUE(api_connect(1));
    EXPECT_TRUE(api_connect(1));
}

TEST_F(Api, ItsPossibleToConnectAfterAFailedTry) {
    GetDriverMock mock;
    DriverMock<1> driver;
    res_connect resp;
    resp.successful = 1;
    EXPECT_CALL(mock, get_driver(1)).WillRepeatedly(Return(driver.get_driver()));
    EXPECT_CALL(driver, connect(1)).Times(2).WillOnce(Return(false)).WillOnce(Return(true));
    EXPECT_FALSE(api_connect(1));
    EXPECT_CALL(driver, send(1, _, _)).WillOnce(Return(true));
    EXPECT_CALL(driver, recv(_, _)).WillOnce(Return(&resp));
    EXPECT_TRUE(api_connect(1));
}

TEST_F(Api, ConnectionFailsWhenTooManyConcurrentConnectionsAreOpened) {
    GetDriverMock mock;
    DriverMock<1> driver;
    res_connect resp;
    resp.successful = 1;
    EXPECT_CALL(mock, get_driver(_)).WillRepeatedly(Return(driver.get_driver()));
    EXPECT_CALL(driver, connect(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(driver, send(_, _, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(driver, recv(_, _)).WillRepeatedly(Return(&resp));
    for (int i = 0; i < API_MAX_CONNECTED_ENDPOINTS; i++) {
        EXPECT_TRUE(api_connect(i));
    }
    EXPECT_FALSE(api_connect(API_MAX_CONNECTED_ENDPOINTS));
    for (int i = 0; i < API_MAX_CONNECTED_ENDPOINTS; i++) {
        EXPECT_TRUE(api_is_connected(i));
    }
    EXPECT_FALSE(api_is_connected(API_MAX_CONNECTED_ENDPOINTS));
}
