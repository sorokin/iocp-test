#pragma once
#include <winsock2.h>
#include <windows.h>

struct io_context
{
    io_context();
    ~io_context();

    io_context(io_context const& other) = delete;
    io_context& operator=(io_context const& other) = delete;

    void wait();

private:
    HANDLE iocp_handle;

    friend struct tcp_socket;
};
