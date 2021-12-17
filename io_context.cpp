#include "io_context.h"

#include <iostream>

#include "overlapped_awaitable.h"
#include <stdexcept>

io_context::io_context()
    : iocp_handle(CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0))
{
    if (iocp_handle == nullptr)
        throw std::runtime_error("CreateIoCompletionPort failed");
}

io_context::~io_context()
{
    CloseHandle(iocp_handle);
}

void io_context::wait()
{
    DWORD bytes_transferred;
    ULONG_PTR completion_key;
    OVERLAPPED* overlapped;
    BOOL res = GetQueuedCompletionStatus(iocp_handle, &bytes_transferred, &completion_key, &overlapped, INFINITE);
    if (!res)
        throw std::runtime_error("GetQueuedCompletionStatus failed");

    overlapped_awaitable* awaitable = static_cast<overlapped_awaitable*>(overlapped);
    awaitable->bytes_transferred = bytes_transferred;

    std::cerr << "completed overlapped " << overlapped << ", task " << awaitable->continuation.address() << "\n";
    if (awaitable->continuation)
        awaitable->continuation.resume();
}

