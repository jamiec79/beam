#pragma once
#include "reactor.h"
#include "config.h"
#include "bufferchain.h"
#include "reactor.h"

namespace beam { namespace io {

class TcpStream : protected Reactor::Object {
public:
    using Ptr = std::shared_ptr<TcpStream>;

    // TODO hide library-specifics
    // errors<=>description platform-independent
    static const int END_OF_STREAM = UV_EOF;
    static const int WRITE_BUFFER_OVERFLOW = -100500;

    // TODO consider 2 more options for read buffer if it appears cheaper to avoid
    // double copying:
    // 1) call back with sub-chunk of shared memory
    // 2) embed deserializer (protocol-specific) object into stream

    // what==0 on new data
    using Callback = std::function<void(int what, void* data, size_t size)>;

    struct State {
        uint64_t received=0;
        uint64_t sent=0;
        size_t unsent=0; // == _writeBuffer.size()
    };

    // Sets callback and enables reading from the stream if callback is not empty
    // returns false if stream disconnected
    bool enable_read(const Callback& callback);

    bool disable_read();

    // TODO: iovecs[], shared buffers to immutable memory
    // deprecated
    size_t try_write(const void* data, size_t size);

    bool write(const void* data, size_t size) {
        return write(SharedBuffer(data, size));
    }

    bool write(const SharedBuffer& buf);

    bool write(const BufferChain& buf);

    bool is_connected() const;

    void close();

    const State& state() const {
        return _state;
    }

    Address peer_address() const;

    int get_last_error() const { return _lastError; }

private:
    friend class TcpServer;
    friend class Reactor;

    TcpStream() = default;

    size_t try_write(const IOVec* buf);

    // sends async write request
    bool send_write_request();

    // returns status code
    int accepted(uv_handle_t* acceptor);

    void connected(uv_stream_t* handle);

    std::vector<char> _readBuffer;
    BufferChain _writeBuffer;
    Callback _callback;
    State _state;
    uv_write_t _writeRequest;
    bool _writeRequestSent=false;
    int _lastError=0;
};

}} //namespaces
