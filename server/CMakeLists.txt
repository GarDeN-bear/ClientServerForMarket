CMAKE_MINIMUM_REQUIRED(VERSION 3.21)

SET(configs ${CMAKE_SOURCE_DIR}/configs)

INCLUDE_DIRECTORIES(${configs})
INCLUDE_DIRECTORIES(session)
INCLUDE_DIRECTORIES(trading_exchange)

ADD_EXECUTABLE(server 
    main.cpp
    trading_exchange_server.cpp
    session/session.cpp
    trading_exchange/trading_exchange_client.cpp
)

FIND_PACKAGE(Boost 1.40 COMPONENTS system REQUIRED)
FIND_PACKAGE(Threads REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

SET(CMAKE_CXX_STANDARD 17)

TARGET_LINK_LIBRARIES(server PRIVATE Threads::Threads ${Boost_LIBRARIES})