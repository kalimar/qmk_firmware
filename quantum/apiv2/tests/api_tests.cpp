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
using testing::MatcherCast;
using testing::Pointee;
using testing::Field;
using testing::AllOf;
using testing::PrintToString;
using testing::InSequence;

enum test_commands {
    api_command_qmk = api_qmk_begin + 100,
    api_command_keyboard = api_keyboard_begin + 100,
    api_command_keymap = api_keymap_begin + 100,
};

BEGIN_MSG
    uint32_t request;
END_MSG(req_qmk);

BEGIN_MSG
    uint32_t response;
END_MSG(res_qmk);

BEGIN_MSG
END_MSG(req_keyboard)

BEGIN_MSG
END_MSG(res_keyboard)

BEGIN_MSG
END_MSG(req_keymap)

BEGIN_MSG
END_MSG(res_keymap)

MATCHER(CommandIsResponse, "The command is a response") {
    return arg->is_response;
}

MATCHER(CommandIsRequest, "The command is a request") {
    return !arg->is_response;
}

MATCHER_P(CommandIs, id, std::string("The command is " + PrintToString(id))) {
    return arg->id == id;
}

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

class ProcessApiMock {
public:
    ProcessApiMock() {
        s_instance = this;
    }

    ~ProcessApiMock() {
        s_instance = nullptr;
    }
    MOCK_METHOD3(api_process_qmk, void (uint8_t endpoint, api_packet_t* packet, uint8_t size));
    MOCK_METHOD3(api_process_keyboard, void (uint8_t endpoint, api_packet_t* packet, uint8_t size));
    MOCK_METHOD3(api_process_keymap, void (uint8_t endpoint, api_packet_t* packet, uint8_t size));
    static ProcessApiMock* s_instance;
};

ProcessApiMock* ProcessApiMock::s_instance = nullptr;

void api_process_qmk(uint8_t endpoint, api_packet_t* packet, uint8_t size) {
    if (ProcessApiMock::s_instance) {
        ProcessApiMock::s_instance->api_process_qmk(endpoint, packet, size);
    }
}

void api_process_keyboard(uint8_t endpoint, api_packet_t* packet, uint8_t size) {
    if (ProcessApiMock::s_instance) {
        ProcessApiMock::s_instance->api_process_keyboard(endpoint, packet, size);
    }
}

void api_process_keymap(uint8_t endpoint, api_packet_t* packet, uint8_t size) {
    if (ProcessApiMock::s_instance) {
        ProcessApiMock::s_instance->api_process_keymap(endpoint, packet, size);
    }
}

class Api : public testing::Test
{
public:
    Api() {
        api_reset();
    }

    GetDriverMock get_driver;
    DriverMock<1> driver1;
};

TEST_F(Api, ConnectingToANonRegisteredEndpointFails) {
    EXPECT_FALSE(api_is_connected(3));
    EXPECT_CALL(get_driver, get_driver(_))
        .WillRepeatedly(Return(nullptr));
    EXPECT_FALSE(api_connect(3));
    EXPECT_FALSE(api_is_connected(3));
}

TEST_F(Api, ASuccessfulConnection) {
    EXPECT_FALSE(api_is_connected(3));
    res_connect resp;
    resp.is_response = 1;
    resp.id = api_command_connect;
    resp.successful = 1;
    EXPECT_CALL(get_driver, get_driver(3))
        .WillRepeatedly(Return(driver1.get_driver()));
    EXPECT_CALL(driver1, connect(3))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1,
        send(3,
            MatcherCast<void*>(MatcherCast<req_connect*>(AllOf(
                Field(&req_connect::protocol_version, API_PROTOCOL_VERSION),
                CommandIsRequest(),
                CommandIs(api_command_connect)
            ))),
            sizeof(req_connect))
        )
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(3), _))
        .Times(1)
        .WillOnce(Invoke(
            [&resp](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 3;
                *size = sizeof(resp);
                return &resp;
            }
        ));
    EXPECT_TRUE(api_connect(3));
    EXPECT_TRUE(api_is_connected(3));
    // Another endpoint should not be connected
    EXPECT_FALSE(api_is_connected(2));
}

TEST_F(Api, AFailedConnection) {
    EXPECT_FALSE(api_is_connected(3));
    EXPECT_CALL(get_driver, get_driver(3))
        .WillRepeatedly(Return(driver1.get_driver()));
    EXPECT_CALL(driver1, connect(3))
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_FALSE(api_connect(3));
    EXPECT_FALSE(api_is_connected(3));
}

TEST_F(Api, AFailedConnectionThroughFailureDuringConnectionPacketSend) {
    EXPECT_FALSE(api_is_connected(3));
    EXPECT_CALL(get_driver, get_driver(3))
        .WillRepeatedly(Return(driver1.get_driver()));
    EXPECT_CALL(driver1, connect(3))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, send(3, _, _))
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_FALSE(api_connect(3));
    EXPECT_FALSE(api_is_connected(3));
}

TEST_F(Api, AFailedConnectionDueToRemoteNotAccepting) {
    EXPECT_FALSE(api_is_connected(3));
    res_connect resp;
    resp.is_response = 1;
    resp.id = api_command_connect;
    resp.successful = 0;
    EXPECT_CALL(get_driver, get_driver(3))
        .WillRepeatedly(Return(driver1.get_driver()));
    EXPECT_CALL(driver1, connect(3))
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, send(3, _, _))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(_, _))
        .Times(1)
        .WillOnce(Invoke(
            [&resp](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 3;
                *size = sizeof(resp);
                return &resp;
            }
        ));
    EXPECT_FALSE(api_connect(3));
    EXPECT_FALSE(api_is_connected(3));
}

TEST_F(Api, AConnectionFailsWhenTheWrongTypeOfResponseIsReceived) {
    // Use the same type of packet so that we know if the id really is checked
    res_connect resp;
    resp.is_response = 1;
    resp.id = 0xDEAD;
    resp.is_response = 1;
    resp.successful = 1;
    EXPECT_CALL(get_driver, get_driver(3))
        .WillRepeatedly(Return(driver1.get_driver()));
    EXPECT_CALL(driver1, connect(3))
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, send(3, _, _))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(_, _))
        .Times(1)
        .WillOnce(Invoke(
            [&resp](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 3;
                *size = sizeof(resp);
                return &resp;
            }
        ));
    EXPECT_FALSE(api_connect(3));
    EXPECT_FALSE(api_is_connected(3));
}

TEST_F(Api, AConnectionFailsWhenTheResponseHasTheWrongSize) {
    // Use the same type of packet so that we know if the id really is checked
    res_connect resp;
    resp.is_response = 1;
    resp.id = api_command_connect;
    resp.is_response = 1;
    resp.successful = 1;
    EXPECT_CALL(get_driver, get_driver(3))
        .WillRepeatedly(Return(driver1.get_driver()));
    EXPECT_CALL(driver1, connect(3))
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, send(3, _, _))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(_, _))
        .Times(1).WillOnce(Invoke(
            [&resp](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 3;
                *size = sizeof(resp) - 1;
                return &resp;
            }
        ));
    EXPECT_FALSE(api_connect(3));
    EXPECT_FALSE(api_is_connected(3));
}

TEST_F(Api, AConnectionFailsWhenTheresADisconnectWaitingForResponse) {
    EXPECT_CALL(get_driver, get_driver(3))
        .WillRepeatedly(Return(driver1.get_driver()));
    EXPECT_CALL(driver1, connect(3))
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, send(3, _, _))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(_, _))
        .Times(1)
        .WillOnce(Return(nullptr));
    EXPECT_FALSE(api_connect(3));
    EXPECT_FALSE(api_is_connected(3));
}

TEST_F(Api, ReceivingAResponseFromAnUnrelatedEndpointDoesNothing) {
    res_connect resp;
    resp.is_response = 1;
    resp.id = api_command_connect;
    resp.successful = 1;
    EXPECT_CALL(get_driver, get_driver(3))
        .WillRepeatedly(Return(driver1.get_driver()));
    EXPECT_CALL(driver1, connect(3))
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, send(3, _, _))
        .Times(1)
        .WillOnce(Return(true));

    EXPECT_CALL(driver1, recv(_, _))
        .Times(2)
        .WillOnce(Invoke(
            [&resp](uint8_t* endpoint, uint8_t* size) {
                *size = sizeof(resp);
                *endpoint = 1;
                return &resp;
            }
        ))
        .WillOnce(Invoke(
            [&resp](uint8_t* endpoint, uint8_t* size) {
                *size = sizeof(resp);
                *endpoint = 3;
                return &resp;
            }
        ));
    EXPECT_TRUE(api_connect(3));
    EXPECT_TRUE(api_is_connected(3));
}

TEST_F(Api, AnotherEndpointIsDisconnectedWhenRecievingUnexpectedConnectionResponseFromItDuringConnect) {
    res_connect resp;
    resp.is_response = 1;
    resp.id = api_command_connect;
    resp.successful = 1;
    EXPECT_CALL(get_driver, get_driver(_))
        .WillRepeatedly(Return(driver1.get_driver()));
    EXPECT_CALL(driver1, connect(_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(driver1, send(_, _, _))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(1), _))
        .Times(1)
        .WillOnce(Invoke(
            [&resp](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 1;
                *size = sizeof(resp);
                return &resp;
            }
        ));
    EXPECT_TRUE(api_connect(1));
    EXPECT_TRUE(api_is_connected(1));

    EXPECT_CALL(driver1, recv( _, _))
        .Times(2)
        .WillOnce(Invoke(
            [&resp](uint8_t* endpoint, uint8_t* size) {
                *size = sizeof(resp);
                *endpoint = 1;
                return &resp;
            }
        ))
        .WillOnce(Invoke(
            [&resp](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 2;
                return &resp;
            }
        ));
    EXPECT_TRUE(api_connect(2));
    EXPECT_TRUE(api_is_connected(2));
    // The original endpoint should be disconnected
    EXPECT_FALSE(api_is_connected(1));
}

TEST_F(Api, AnotherEndpointIsDisconnectedWhenRecievingUnexpectedGeneralResponseFromItDuringConnect) {
    res_connect resp;
    resp.is_response = 1;
    resp.id = api_command_connect;
    resp.successful = 1;

    res_connect general_response;
    general_response.is_response = 1;
    general_response.id = 0xDEAD;
    general_response.successful = 1;
    EXPECT_CALL(get_driver, get_driver(_))
        .WillRepeatedly(Return(driver1.get_driver()));
    EXPECT_CALL(driver1, connect(_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(driver1, send(_, _, _))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(1), _))
        .Times(1)
        .WillOnce(Invoke(
            [&resp](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 1;
                *size = sizeof(resp);
                return &resp;
            }
        ));
    EXPECT_TRUE(api_connect(1));
    EXPECT_TRUE(api_is_connected(1));

    EXPECT_CALL(driver1, recv( _, _))
        .Times(2)
        .WillOnce(Invoke(
            [&general_response](uint8_t* endpoint, uint8_t* size) {
                *size = sizeof(general_response);
                *endpoint = 1;
                return &general_response;
            }
        ))
        .WillOnce(Invoke(
            [&resp](uint8_t* endpoint, uint8_t* size) {
                *size = sizeof(resp);
                *endpoint = 2;
                return &resp;
            }
        ));
    EXPECT_TRUE(api_connect(2));
    EXPECT_TRUE(api_is_connected(2));
    // The original endpoint should be disconnected
    EXPECT_FALSE(api_is_connected(1));
}

TEST_F(Api, TryingToConnectWhenAlreadyConnectedDoesNothing) {
    res_connect resp;
    resp.is_response = 1;
    resp.id = api_command_connect;
    resp.successful = 1;
    EXPECT_CALL(get_driver, get_driver(1))
        .WillRepeatedly(Return(driver1.get_driver()));
    EXPECT_CALL(driver1, connect(1))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, send(1, _, _))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(_, _))
        .Times(1)
        .WillOnce(Invoke(
            [&resp](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 1;
                *size = sizeof(resp);
                return &resp;
            }
        ));
    EXPECT_TRUE(api_connect(1));
    EXPECT_TRUE(api_connect(1));
}

TEST_F(Api, ItsPossibleToConnectAfterAFailedTry) {
    res_connect resp;
    resp.is_response = 1;
    resp.id = api_command_connect;
    resp.successful = 1;
    EXPECT_CALL(get_driver, get_driver(1))
        .WillRepeatedly(Return(driver1.get_driver()));
    EXPECT_CALL(driver1, connect(1))
        .Times(2)
        .WillOnce(Return(false))
        .WillOnce(Return(true));
    EXPECT_FALSE(api_connect(1));
    EXPECT_CALL(driver1, send(1, _, _))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(_, _))
        .Times(1)
        .WillOnce(Invoke(
            [&resp](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 1;
                *size = sizeof(resp);
                return &resp;
            }
        ));
    EXPECT_TRUE(api_connect(1));
}

TEST_F(Api, ConnectionFailsWhenTooManyConcurrentConnectionsAreOpened) {
    res_connect resp;
    resp.is_response = 1;
    resp.id = api_command_connect;
    resp.successful = 1;
    EXPECT_CALL(get_driver, get_driver(_))
        .WillRepeatedly(Return(driver1.get_driver()));
    EXPECT_CALL(driver1, connect(_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(driver1, send(_, _, _))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(driver1, recv(_, _))
        .WillRepeatedly(Invoke(
            [&resp](uint8_t* endpoint, uint8_t* size) {
                *size = sizeof(resp);
                return &resp;
            }
        ));
    for (int i = 0; i < API_MAX_CONNECTED_ENDPOINTS; i++) {
        EXPECT_TRUE(api_connect(i));
    }
    EXPECT_FALSE(api_connect(API_MAX_CONNECTED_ENDPOINTS));
    for (int i = 0; i < API_MAX_CONNECTED_ENDPOINTS; i++) {
        EXPECT_TRUE(api_is_connected(i));
    }
    EXPECT_FALSE(api_is_connected(API_MAX_CONNECTED_ENDPOINTS));
}

TEST_F(Api, AnIncommingConnectionWithTheCorrectVersionIsAccepted) {
    req_connect req;
    req.id = api_command_connect;
    req.is_response = 0;
    req.protocol_version = API_PROTOCOL_VERSION;
    EXPECT_CALL(get_driver, get_driver(5))
        .WillRepeatedly(Return(driver1.get_driver()));
    EXPECT_CALL(driver1,
        send(5,
            MatcherCast<void*>(MatcherCast<res_connect*>(AllOf(
                Field(&res_connect::successful, 1),
                CommandIsResponse(),
                CommandIs(api_command_connect)
            ))),
            sizeof(res_connect)))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(API_ENDPOINT_BROADCAST), _))
        .Times(2)
        .WillOnce(Invoke(
            [&req](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 5;
                *size = sizeof(req);
                return &req;
            }
        ))
        .WillOnce(Return(nullptr));

    api_process_driver(driver1.get_driver());
}

TEST_F(Api, AnIncommingConnectionWithTheWrongVersionIsNotAccepted) {
    req_connect req;
    req.id = api_command_connect;
    req.is_response = 0;
    req.protocol_version = 0xDEAD;
    EXPECT_CALL(get_driver, get_driver(5))
        .WillRepeatedly(Return(driver1.get_driver()));
    EXPECT_CALL(driver1,
        send(5,
            MatcherCast<void*>(MatcherCast<res_connect*>(AllOf(
                Field(&res_connect::successful, 0),
                CommandIsResponse(),
                CommandIs(api_command_connect)
            ))),
            sizeof(res_connect)))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(API_ENDPOINT_BROADCAST), _))
        .Times(2).
        WillOnce(Invoke(
            [&req](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 5;
                *size = sizeof(req);
                return &req;
            }
        )).
        WillOnce(Return(nullptr));
    api_process_driver(driver1.get_driver());
}

TEST_F(Api, AnIncommingConnectionFromTheSameEndpointCanBeAcceptedDuringConnect) {
    res_connect resp;
    resp.is_response = true;
    resp.id = api_command_connect;
    resp.successful = 1;

    req_connect req;
    req.is_response = false;
    req.id = api_command_connect;
    req.protocol_version = API_PROTOCOL_VERSION;
    EXPECT_CALL(get_driver, get_driver(1))
        .WillRepeatedly(Return(driver1.get_driver()));
    InSequence s;
    // Connect the endpoint
    EXPECT_CALL(driver1, connect(1))
        .Times(1)
        .WillOnce(Return(true));
    // Send the request
    EXPECT_CALL(driver1,
        send(1,
            MatcherCast<void*>(MatcherCast<req_connect*>(AllOf(
                CommandIsRequest(),
                CommandIs(api_command_connect)
            ))),
            sizeof(req_connect))
        )
        .Times(1).WillOnce(Return(true));
    // Receive the connection request from the remote, before we have received our response
    EXPECT_CALL(driver1, recv(_, _))
        .Times(1)
        .WillOnce(Invoke(
            [&req](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 1;
                *size = sizeof(req);
                return &req;
            }
        ));
    // Send the response
    EXPECT_CALL(driver1,
        send(1,
            MatcherCast<void*>(MatcherCast<res_connect*>(AllOf(
                CommandIsResponse(),
                CommandIs(api_command_connect)
            ))),
            sizeof(res_connect))
        )
        .Times(1);
    // Receive the response for our outgoing request
    EXPECT_CALL(driver1, recv(_, _))
        .Times(1)
        .WillOnce(Invoke(
            [&resp](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 1;
                *size = sizeof(resp);
                return &resp;
            }
        ));
    EXPECT_TRUE(api_connect(1));
}

TEST_F(Api, AnIncommingConnectionFromAnotherEndpointCanBeAcceptedDuringConnect) {
    res_connect resp;
    resp.is_response = true;
    resp.id = api_command_connect;
    resp.successful = 1;

    req_connect req;
    req.is_response = false;
    req.id = api_command_connect;
    req.protocol_version = API_PROTOCOL_VERSION;
    EXPECT_CALL(get_driver, get_driver(_))
        .WillRepeatedly(Return(driver1.get_driver()));
    InSequence s;
    // Connect the endpoint
    EXPECT_CALL(driver1, connect(1))
        .Times(1)
        .WillOnce(Return(true));
    // Send the request
    EXPECT_CALL(driver1,
        send(1,
            MatcherCast<void*>(MatcherCast<req_connect*>(AllOf(
                CommandIsRequest(),
                CommandIs(api_command_connect)
            ))),
            sizeof(req_connect))
        )
        .Times(1)
        .WillOnce(Return(true));
    // Receive the connection request from the remote, before we have received our response
    EXPECT_CALL(driver1, recv(_, _))
        .Times(1)
        .WillOnce(Invoke(
            [&req](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 2;
                *size = sizeof(req);
                return &req;
            }
        ));
    // Send the response
    EXPECT_CALL(driver1,
        send(2,
            MatcherCast<void*>(MatcherCast<res_connect*>(AllOf(
                CommandIsResponse(),
                CommandIs(api_command_connect)
            ))),
            sizeof(res_connect))
        )
        .Times(1);
    // Receive the response for our outgoing request
    EXPECT_CALL(driver1, recv(_, _))
        .Times(1)
        .WillOnce(Invoke(
            [&resp](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 1;
                *size = sizeof(resp);
                return &resp;
            }
        ));
    EXPECT_TRUE(api_connect(1));
}

class ConnectedApi : public Api
{
public:
    ConnectedApi() {
        connect_endpoint(1);
    }

    void connect_endpoint(uint8_t endpoint) {
        res_connect resp;
        resp.is_response = 1;
        resp.id = api_command_connect;
        resp.successful = 1;
        EXPECT_CALL(get_driver, get_driver(endpoint))
            .WillRepeatedly(Return(driver1.get_driver()));
        EXPECT_CALL(driver1, connect(endpoint))
            .Times(1)
            .WillOnce(Return(true));
        EXPECT_CALL(driver1,
            send(endpoint,
                MatcherCast<void*>(MatcherCast<req_connect*>(AllOf(
                    Field(&req_connect::protocol_version, API_PROTOCOL_VERSION),
                    CommandIsRequest(),
                    CommandIs(api_command_connect)
                ))),
                sizeof(req_connect))
            )
            .WillOnce(Return(true));
        EXPECT_CALL(driver1, recv(Pointee(endpoint), _))
            .Times(1)
            .WillOnce(Invoke(
                [&resp, ep=endpoint](uint8_t* endpoint, uint8_t* size) {
                    *endpoint = ep;
                    *size = sizeof(resp);
                    return &resp;
                }
            ));
        EXPECT_TRUE(api_connect(endpoint));
        EXPECT_TRUE(api_is_connected(endpoint));
    }
};

TEST_F(ConnectedApi, SuccessfulSendAndReceive) {
    req_qmk request;
    request.request = 37;

    res_qmk response;
    response.id = api_command_qmk;
    response.is_response = true;
    response.response = 12;

    EXPECT_CALL(driver1,
        send(1,
            MatcherCast<void*>(MatcherCast<req_qmk*>(AllOf(
                Field(&req_qmk::request, 37),
                CommandIsRequest(),
                CommandIs(api_command_qmk)
            ))),
            sizeof(req_qmk))
        )
        .Times(1).WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(1), _))
        .Times(1)
        .WillOnce(Invoke(
            [&response](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 1;
                *size = sizeof(response);
                return &response;
            }
        ));
    res_qmk* received_resp = API_SEND_AND_RECV(1, qmk, &request);
    ASSERT_NE(received_resp, nullptr);
    EXPECT_EQ(12, received_resp->response);
    EXPECT_TRUE(api_is_connected(1));
}

TEST_F(ConnectedApi, AFailedSendReturnsNullAndDisconnects) {
    req_qmk request;
    request.request = 37;

    EXPECT_CALL(driver1, send(1, _, _))
        .Times(1)
        .WillOnce(Return(false));
    auto* received_resp = API_SEND_AND_RECV(1, qmk, &request);
    ASSERT_EQ(received_resp, nullptr);
    ASSERT_FALSE(api_is_connected(1));
}

TEST_F(ConnectedApi, AFailedReceiveOfResponseReturnsNullAndDisconnects) {
    req_qmk request;
    request.request = 37;

    EXPECT_CALL(driver1, send(1, _, _))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(1), _))
        .Times(1)
        .WillOnce(Return(nullptr));
    auto* received_resp = API_SEND_AND_RECV(1, qmk, &request);
    ASSERT_EQ(received_resp, nullptr);
    ASSERT_FALSE(api_is_connected(1));
}

TEST_F(ConnectedApi, AFailedReceiveOfResponseWithAnyEndpointReturnsNullAndDisconnects) {
    req_qmk request;
    request.request = 37;

    EXPECT_CALL(driver1, send(1, _, _))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(1), _))
        .Times(1)
        .WillOnce(Invoke(
            [](uint8_t* endpoint, uint8_t* size) {
                // Set the endpoint to something else, to make sure that it's still handled correctly
                // Even if the driver doesn't set it
                *endpoint = 27;
                return nullptr;
            }
        ));
    auto* received_resp = API_SEND_AND_RECV(1, qmk, &request);
    ASSERT_EQ(received_resp, nullptr);
    ASSERT_FALSE(api_is_connected(1));
}


TEST_F(Api, ASendFailsForADisconnectedEndpoint) {
    EXPECT_CALL(get_driver, get_driver(1))
        .WillRepeatedly(Return(driver1.get_driver()));
    EXPECT_CALL(driver1, connect(1))
        .Times(0);
    EXPECT_CALL(driver1, send(1, _, _))
        .Times(0);
    req_qmk request;
    request.request = 37;
    auto* resp = API_SEND_AND_RECV(1, qmk, &request);
    EXPECT_EQ(resp, nullptr);
}

TEST_F(ConnectedApi, TryingToSendATooSmallPacketReturnsNullAndDisconnects) {
    uint8_t request;

    EXPECT_CALL(driver1, send(1, _, _))
        .Times(0);
    auto* res = api_send_and_recv(1, api_command_qmk, &request, 1, sizeof(res_qmk));
    EXPECT_EQ(res, nullptr);
    EXPECT_FALSE(api_is_connected(1));
}

TEST_F(ConnectedApi, ReceivingAResponseWithTheWrongIdFailsAndDisconnects) {
    req_qmk request;
    request.request = 37;

    res_qmk response;
    response.id = 0xDEAD;
    response.is_response = true;
    response.response = 12;

    EXPECT_CALL(driver1, send(1, _, _))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(1), _))
        .Times(1)
        .WillOnce(Invoke(
            [&response](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 1;
                *size = sizeof(response);
                return &response;
            }
        ));
    auto* received_resp = API_SEND_AND_RECV(1, qmk, &request);
    EXPECT_EQ(received_resp, nullptr);
    EXPECT_FALSE(api_is_connected(1));
}

TEST_F(ConnectedApi, ReceivingAResponseWithTheWrongSizeFailsAndDisconnects) {
    req_qmk request;
    request.request = 37;

    res_qmk response;
    response.id = api_command_qmk;
    response.is_response = true;
    response.response = 12;

    EXPECT_CALL(driver1, send(1, _, _))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(1), _))
        .Times(1)
        .WillOnce(Invoke(
            [&response](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 1;
                *size = sizeof(response) + 3;
                return &response;
            }
        ));
    auto* received_resp = API_SEND_AND_RECV(1, qmk, &request);
    EXPECT_EQ(received_resp, nullptr);
    EXPECT_FALSE(api_is_connected(1));
}

TEST_F(ConnectedApi, ReceivingAResponseFromTheWrongEndpointWillDisconnectItButTheOriginalCanStillSuccceed) {
    req_qmk request;
    request.request = 37;

    res_qmk response;
    response.id = api_command_qmk;
    response.is_response = true;
    response.response = 12;

    connect_endpoint(4);

    EXPECT_CALL(driver1, send(1, _, _)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(1), _))
        .Times(2)
        .WillOnce(Invoke(
            [&response](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 4;
                *size = sizeof(response);
                return &response;
            }
        ))
        .WillOnce(Invoke(
            [&response](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 1;
                *size = sizeof(response);
                return &response;
            }
        ));
    auto* received_resp = API_SEND_AND_RECV(1, qmk, &request);
    EXPECT_NE(received_resp, nullptr);
    EXPECT_TRUE(api_is_connected(1));
    EXPECT_FALSE(api_is_connected(4));
}

TEST_F(ConnectedApi, AnUhandledRequestsReturnsNullToTheSenderAndDisconnects) {
    req_qmk request;
    request.request = 37;

    res_unhandled unhandled;
    unhandled.id = api_command_unhandled;
    unhandled.is_response = true;
    unhandled.original_request = api_command_qmk;

    EXPECT_CALL(driver1,
        send(1,
            MatcherCast<void*>(MatcherCast<req_qmk*>(AllOf(
                Field(&req_qmk::request, 37),
                CommandIsRequest(),
                CommandIs(api_command_qmk)
            ))),
            sizeof(req_qmk)
        ))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(1), _))
        .Times(1)
        .WillOnce(Invoke(
            [&unhandled](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 1;
                *size = sizeof(unhandled);
                return &unhandled;
            }
        ));
    auto* received_resp = API_SEND_AND_RECV(1, qmk, &request);
    ASSERT_EQ(received_resp, nullptr);
    EXPECT_FALSE(api_is_connected(1));
}

TEST_F(ConnectedApi, AnIncomingConnectionRequestFromTheSameEndpointIsAcceptedDuringSend) {
    req_qmk request;
    request.request = 37;

    req_connect con_req;
    con_req.is_response = false;
    con_req.id = api_command_connect;
    con_req.protocol_version = API_PROTOCOL_VERSION;

    res_qmk response;
    response.id = api_command_qmk;
    response.is_response = true;
    response.response = 12;

    InSequence s;
    EXPECT_CALL(driver1, send(1, _, _))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(1), _))
        .WillOnce(Invoke(
            [&con_req](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 1;
                *size = sizeof(con_req);
                return &con_req;
            }
        ));
    EXPECT_CALL(driver1,
        send(1,
            MatcherCast<void*>(MatcherCast<res_connect*>(AllOf(
                CommandIsResponse(),
                CommandIs(api_command_connect)
            ))),
            sizeof(res_connect))
        )
        .Times(1);
    EXPECT_CALL(driver1, recv(Pointee(1), _))
        .WillOnce(Invoke(
            [&response](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 1;
                *size = sizeof(response);
                return &response;
            }
        ));
    auto* received_resp = API_SEND_AND_RECV(1, qmk, &request);
    EXPECT_NE(received_resp, nullptr);
}

TEST_F(ConnectedApi, AnIncomingConnectionRequestFromADifferentEndpointIsAcceptedDuringSend) {
    req_qmk request;
    request.request = 37;

    req_connect con_req;
    con_req.is_response = false;
    con_req.id = api_command_connect;
    con_req.protocol_version = API_PROTOCOL_VERSION;

    res_qmk response;
    response.id = api_command_qmk;
    response.is_response = true;
    response.response = 12;

    EXPECT_CALL(get_driver, get_driver(7))
        .WillRepeatedly(Return(driver1.get_driver()));

    InSequence s;
    EXPECT_CALL(driver1, send(1, _, _))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(1), _))
        .WillOnce(Invoke(
            [&con_req](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 7;
                *size = sizeof(con_req);
                return &con_req;
            }
        ));
    EXPECT_CALL(driver1,
        send(7,
            MatcherCast<void*>(MatcherCast<res_connect*>(AllOf(
                CommandIsResponse(),
                CommandIs(api_command_connect)
            ))),
            sizeof(res_connect)
        ))
        .Times(1);
    EXPECT_CALL(driver1, recv(Pointee(1), _))
        .WillOnce(Invoke(
            [&response](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 1;
                *size = sizeof(response);
                return &response;
            }
        ));
    auto* received_resp = API_SEND_AND_RECV(1, qmk, &request);
    EXPECT_NE(received_resp, nullptr);
}

TEST_F(Api, AnAcceptedIncomingQMKRequestReturnsTheCorrectResponse) {
    ProcessApiMock process;
    req_qmk request;
    request.id = api_command_qmk;
    request.is_response = false;
    request.request = 12;

    EXPECT_CALL(get_driver, get_driver(4))
        .WillRepeatedly(Return(driver1.get_driver()));

    auto handle_qmk = [](uint8_t endpoint, req_qmk* req, res_qmk* res) {
        res->response = 42;
    };

    EXPECT_CALL(process, api_process_qmk(4, reinterpret_cast<api_packet_t*>(&request), sizeof(request)))
        .WillOnce(Invoke(
            [handle_qmk](uint8_t endpoint, api_packet_t* packet, uint8_t size) {
                switch (packet->id) {
                    API_HANDLE(qmk, handle_qmk);
                }
            })
        );
    EXPECT_CALL(process, api_process_keyboard(_, _, _))
        .Times(0);
    EXPECT_CALL(process, api_process_keymap(_, _, _))
        .Times(0);
    EXPECT_CALL(driver1,
        send(4,
            MatcherCast<void*>(MatcherCast<res_qmk*>(AllOf(
                CommandIsResponse(),
                CommandIs(api_command_qmk),
                Field(&res_qmk::response, 42)
            ))),
            sizeof(res_qmk)
         ))
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(API_ENDPOINT_BROADCAST), _))
        .Times(2).WillOnce(Invoke(
            [&request](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 4;
                *size = sizeof(request);
                return &request;
            }
        ))
        .WillOnce(Return(nullptr));
     api_process_driver(driver1.get_driver());
}

TEST_F(Api, AnAcceptedIncomingKeyboardRequestReturnsTheCorrectResponse) {
    ProcessApiMock process;
    req_keyboard request;
    request.id = api_command_keyboard;
    request.is_response = false;

    EXPECT_CALL(get_driver, get_driver(4))
        .WillRepeatedly(Return(driver1.get_driver()));

    auto handle_keyboard = [](uint8_t endpoint, req_keyboard* req, res_keyboard* res) {
    };

    EXPECT_CALL(process, api_process_keyboard(4, reinterpret_cast<api_packet_t*>(&request), sizeof(request)))
        .WillOnce(Invoke(
            [handle_keyboard](uint8_t endpoint, api_packet_t* packet, uint8_t size) {
                switch (packet->id) {
                    API_HANDLE(keyboard, handle_keyboard);
                }
            }
        ));
    EXPECT_CALL(process, api_process_qmk(_, _, _))
        .Times(0);
    EXPECT_CALL(process, api_process_keymap(_, _, _))
        .Times(0);
    EXPECT_CALL(driver1,
        send(4,
            MatcherCast<void*>(MatcherCast<res_keyboard*>(AllOf(
                CommandIsResponse(),
                CommandIs(api_command_keyboard)
            ))),
            sizeof(res_keyboard)
         ))
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(API_ENDPOINT_BROADCAST), _))
        .Times(2)
        .WillOnce(Invoke(
            [&request](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 4;
                *size = sizeof(request);
                return &request;
            }
        ))
        .WillOnce(Return(nullptr));
     api_process_driver(driver1.get_driver());
}

TEST_F(Api, AnAcceptedIncomingKeymapRequestReturnsTheCorrectResponse) {
    ProcessApiMock process;
    req_keyboard request;
    request.id = api_command_keymap;
    request.is_response = false;

    EXPECT_CALL(get_driver, get_driver(4)).WillRepeatedly(Return(driver1.get_driver()));

    auto handle_keymap = [](uint8_t endpoint, req_keymap* req, res_keymap* res) {
    };

    EXPECT_CALL(process, api_process_keymap(4, reinterpret_cast<api_packet_t*>(&request), sizeof(request)))
        .WillOnce(Invoke(
            [handle_keymap](uint8_t endpoint, api_packet_t* packet, uint8_t size) {
                switch (packet->id) {
                    API_HANDLE(keymap, handle_keymap);
                }
            }
        ));
    EXPECT_CALL(process, api_process_qmk(_, _, _))
        .Times(0);
    EXPECT_CALL(process, api_process_keyboard(_, _, _))
        .Times(0);
    EXPECT_CALL(driver1,
        send(4,
            MatcherCast<void*>(MatcherCast<res_keymap*>(AllOf(
                CommandIsResponse(),
                CommandIs(api_command_keymap)
            ))),
            sizeof(res_keymap)
         ))
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(API_ENDPOINT_BROADCAST), _))
        .Times(2)
        .WillOnce(Invoke(
            [&request](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 4;
                *size = sizeof(request);
                return &request;
            }
        ))
        .WillOnce(Return(nullptr));
     api_process_driver(driver1.get_driver());
}

TEST_F(Api, AnUnhandledIncomingQMKRequestReturnsUnhandled) {
    ProcessApiMock process;
    req_qmk request;
    request.id = api_command_qmk;
    request.is_response = false;
    request.request = 12;

    EXPECT_CALL(get_driver, get_driver(4))
        .WillRepeatedly(Return(driver1.get_driver()));

    EXPECT_CALL(process, api_process_qmk(4, reinterpret_cast<api_packet_t*>(&request), sizeof(request)))
        .WillOnce(Invoke(
            [](uint8_t endpoint, api_packet_t* packet, uint8_t size) {
            })
        );
    EXPECT_CALL(driver1,
        send(4,
            MatcherCast<void*>(MatcherCast<res_unhandled*>(AllOf(
                CommandIsResponse(),
                CommandIs(api_command_unhandled),
                Field(&res_unhandled::original_request, api_command_qmk)
            ))),
            sizeof(res_unhandled)
         ))
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(API_ENDPOINT_BROADCAST), _))
        .Times(2)
        .WillOnce(Invoke(
            [&request](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 4;
                *size = sizeof(request);
                return &request;
            }
        ))
        .WillOnce(Return(nullptr));
     api_process_driver(driver1.get_driver());
}

TEST_F(Api, AResponseIsOnlySentOnceEvenIfCalledTwice) {
    ProcessApiMock process;
    req_qmk request;
    request.id = api_command_qmk;
    request.is_response = false;
    request.request = 12;

    EXPECT_CALL(get_driver, get_driver(4)).WillRepeatedly(Return(driver1.get_driver()));

    auto handle_qmk = [](uint8_t endpoint, req_qmk* req, res_qmk* res) {
        res->response = 42;
        API_SEND_RESPONSE(endpoint, qmk, res);
    };

    EXPECT_CALL(process, api_process_qmk(4, reinterpret_cast<api_packet_t*>(&request), sizeof(request)))
        .WillOnce(Invoke(
            [handle_qmk](uint8_t endpoint, api_packet_t* packet, uint8_t size) {
                switch (packet->id) {
                    API_HANDLE(qmk, handle_qmk);
                }
            }
        ));
    EXPECT_CALL(driver1,
        send(4,
            MatcherCast<void*>(MatcherCast<res_qmk*>(AllOf(
                CommandIsResponse(),
                CommandIs(api_command_qmk),
                Field(&res_qmk::response, 42)
            ))),
            sizeof(res_qmk)
         ))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(API_ENDPOINT_BROADCAST), _))
        .Times(2)
        .WillOnce(Invoke(
            [&request](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 4;
                *size = sizeof(request);
                return &request;
            }
        ))
        .WillOnce(Return(nullptr));
     api_process_driver(driver1.get_driver());
}

TEST_F(Api, ATooSmallIncomingPacketReturnsProtocolError) {
    ProcessApiMock process;
    uint8_t buffer[1];

    EXPECT_CALL(get_driver, get_driver(4))
        .WillRepeatedly(Return(driver1.get_driver()));

    EXPECT_CALL(process, api_process_qmk(_,_,_))
        .Times(0);
    EXPECT_CALL(driver1,
        send(4,
            MatcherCast<void*>(MatcherCast<res_protocol_error*>(AllOf(
                CommandIsResponse(),
                CommandIs(api_command_protocol_error),
                Field(&res_protocol_error::error, PROTOCOL_ERROR_INCOMING_TOO_SMALL)
            ))),
            sizeof(res_protocol_error)
         ))
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(API_ENDPOINT_BROADCAST), _))
        .Times(2)
        .WillOnce(Invoke(
            [&buffer](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 4;
                *size = 1;
                return buffer;
            }
        ))
        .WillOnce(Return(nullptr));
     api_process_driver(driver1.get_driver());
}

TEST_F(Api, AnUnexpectedResponseReturnsProtocolError) {
    ProcessApiMock process;
    res_qmk res;

    EXPECT_CALL(get_driver, get_driver(4))
        .WillRepeatedly(Return(driver1.get_driver()));

    EXPECT_CALL(process, api_process_qmk(_,_,_))
        .Times(0);
    EXPECT_CALL(driver1,
        send(4,
            MatcherCast<void*>(MatcherCast<res_protocol_error*>(AllOf(
                CommandIsResponse(),
                CommandIs(api_command_protocol_error),
                Field(&res_protocol_error::error, PROTOCOL_ERROR_UNEXPECTED_RESPONSE)
            ))),
            sizeof(res_protocol_error)
         ))
        .WillOnce(Return(true));
    EXPECT_CALL(driver1, recv(Pointee(API_ENDPOINT_BROADCAST), _))
        .Times(2)
        .WillOnce(Invoke(
            [&res](uint8_t* endpoint, uint8_t* size) {
                *endpoint = 4;
                *size = sizeof(res);
                return &res;
            }
        ))
        .WillOnce(Return(nullptr));
     api_process_driver(driver1.get_driver());
}

// TODO: Add tests for other requests during connect
