#pragma once
#include <coroutine>
#include <winsock2.h>
#include <windows.h>

struct overlapped_awaitable : OVERLAPPED
{
    overlapped_awaitable();

    overlapped_awaitable(overlapped_awaitable const& other) = delete;
    overlapped_awaitable& operator=(overlapped_awaitable const& other) = delete;

    void await_suspend(std::coroutine_handle<> c) noexcept;

    size_t bytes_transferred;
    std::coroutine_handle<> continuation;
};
