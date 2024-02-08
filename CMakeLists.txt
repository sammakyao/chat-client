cmake_minimum_required(VERSION 3.23)
project(chat_system C)

set(CMAKE_C_STANDARD 11)

add_executable(chat_server server.c)
add_executable(chat_client client.c)
