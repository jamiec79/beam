#pragma once
#include "logger.h"
#include <tuple>

#define COMBINE1(X,Y) X##Y
#define COMBINE(X,Y) COMBINE1(X,Y)
#define NUM_ARGS(...) std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value

#define CHECKPOINT_CREATE(SIZE) \
    beam::CheckpointData<SIZE> COMBINE(checkpointData,__LINE__); \
    beam::Checkpoint COMBINE(checkpoint,__LINE__)(COMBINE(checkpointData,__LINE__).items, SIZE, __FILE__, __LINE__, __FUNCTION__)

#define CHECKPOINT_ADD() if (currentCheckpoint) *currentCheckpoint

#define CHECKPOINT(...) \
    beam::CheckpointData<NUM_ARGS(__VA_ARGS__)> COMBINE(checkpointData,__LINE__); \
    beam::Checkpoint COMBINE(checkpoint,__LINE__)(COMBINE(checkpointData,__LINE__).items, NUM_ARGS(__VA_ARGS__), __FILE__, __LINE__, __FUNCTION__); \
    COMBINE(checkpoint,__LINE__).push(__VA_ARGS__)

namespace beam {
    
namespace detail {
    union Data {
        const void* ptr;
        const char* cstr;
        uint64_t value;
    };
    
    typedef void (*flush_fn)(LogMessage& m, Data* d);
    
    template<class T> void flush_pointer(LogMessage& m, Data* d) {
        T* t = (T*)d->ptr;
        m << t << ' ';
    }
    
    template<class T> void flush_value(LogMessage& m, Data* d) {
        T* t = (T*)d;
        m << t << ' ';
    }
    
    void flush_cstr(LogMessage& m, Data* d) {
        m << d->cstr << ' ';
    }

    struct CheckpointItem {
        flush_fn fn;
        Data data;
    };    
    
} //namespace

template <size_t MAX_ITEMS> struct CheckpointData {
    detail::CheckpointItem items[MAX_ITEMS];
};

class Checkpoint {
    From _from;
    detail::CheckpointItem* _items;
    detail::CheckpointItem* _ptr;
    size_t _maxItems;
    Checkpoint* _next;
    Checkpoint* _prev;
    
public:
    Checkpoint(detail::CheckpointItem* items, size_t maxItems, const char* file, int line, const char* function);
    
    void flush();
    
    ~Checkpoint();
    
    template <typename T, typename = std::enable_if<std::is_arithmetic<T>::value>>
    Checkpoint& operator<<(T value) {
        assert(_ptr < _items + _maxItems);
        _ptr->fn = detail::flush_value<T>;
        (T&)(_ptr->data.value) = value;
        ++_ptr;
        return *this;
    }
    
    template <typename T>
    Checkpoint& operator<<(T* ptr) {
        assert(_ptr < _items + _maxItems);
        if constexpr (std::is_same<T*, const char*>::value) {
            _ptr->fn = detail::flush_cstr;
        } else {
            _ptr->fn = detail::flush_pointer<T>;
        }
        _ptr->data.ptr = ptr;
        ++_ptr;
        return *this;
    }
    
    void push() {}
    
    template <typename T> void push(T a) {
        *this << a;
    }
    
    template <typename T, typename... Args> void push(T a, Args... args) {
        push(a);
        push(args...);
    }
};
    
} //namespace