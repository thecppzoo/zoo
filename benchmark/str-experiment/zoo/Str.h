#include <memory>
#include <string.h> // memcpy
#include <new> // aligned allocations
//#include <experimental/bit>
#include <assert.h>

namespace zoo {

template<typename T>
constexpr auto Log2Floor(T value) {
    return sizeof(unsigned long long) * 8 - 1 -
        //std::countl_zero(value);
        __builtin_clzll(value);
}

template<typename T>
// assumes 1 < value!
constexpr auto Log2Ceiling(T value) {
    return 1 + Log2Floor(value - 1);
}

template<typename T>
constexpr auto Log256Celing(T value) {
    return (7 + Log2Ceiling(value)) / 8;
}

inline void noOp(...) {}
#define printf noOp
#define fprintf noOp

/// \brief String controller type optimized for minimum
/// size and configurability
///
/// Design:
/// This type attempts to use a minimum size fully usable string type
/// compatible with the API of std::string. The extreme case is when
/// sizeof(Str<void *>) == sizeof(void *). This type is a template that
/// takes a type argument to serve as the *prototype* for the local
/// buffer. If you select "void *" as the prototype, the local buffer and
/// the size of Str<void *> will have size and alignment of void *. If you
/// select "void *[2]" (array of two pointers), then there will be 2 void
/// pointers worth of space (16 bytes) locally.
///
/// To maximize space efficiency, the encoding of whether the string is
/// local or on the heap will occur at the last byte, least significant
/// bits of the local storage. The space corresponding to the last pointer
/// worth of local storage doubles as the encoding of a heap pointer for
/// strings that cannot be held locally.
///
/// This uses Andrei Alexandrescu's idea of storing the size of the string
/// as the remaining local capacity. In this way, a 0 local capacity also
/// encodes the null byte for c_str(), as reported by Nicholas Omrod at
/// CPPCon 2016. In case the string is too long for local allocation, the
/// pointer in the heap will be encoded locally in the last word of the
/// representation.
///
/// For an application expecting large strings, the optimal size of the
/// local storage would be that of a "char *". Since most strings would be
/// large, there would be few chances of storing them in the local buffer.
/// You could use "char *" or "void *" for the storage prototype.
///
/// \tparam StorageModel Indicates the size of the local storage and its
/// alignment.
///
/// For large strings (allocated in the heap):
/// In the last void * worth of local storage:
/// +---------------- Big Endian diagram -----------------------------+
/// | Most significant part of Pointer |  Last Byte, rob 4 lower bits |
/// +-----------------------------------------------------------------+
/// Since we're robbing 4 bits from the last byte, this implies the pointers
/// are rounded up to 16 bytes.
///
/// For local strings:
/// +------- Little Endian Diagram -----+
/// | characters... | last byte         |
/// +-----------------------------------+
///
/// The last byte looks like this:
/// For heap encoding: the number of bytes required to encode the size of
/// the string, or mathematically, the logarithm base 256 of the string's
/// size. This value will be in the interval [1, 8], encoded with a shifted
/// range of [0, 7], allowing us to use 3 bits to encode the length. An
/// extra bit flags whether the local buffer stores a pointer to a heap
/// string (1) or if it is represented in the local buffer (0).
///
/// Because the storage model can be of arbitrary size, we cannot assume
/// the lowest available bit will encode 8. It will be the lowest bit the
/// local buffer size allows. For a storage model of void *[4], the heap
/// flag encodes for 32 as a number. For Str<void *>, the bit encodes for 8.
/// This strategy forces allocations to align to the ceiling of
/// log2(sizeof(StorageModel)).
///
/// Local strings:
/// The last byte serves as the COUNT OF REMAINING BYTES, including the null
/// character terminator. Hence, Str<void *> can hold up to seven bytes
/// + null terminator locally. Str<void *[8]> allows 8*8 = 64 characters
/// locally (including the null).
///
/// Heap-allocated strings:
/// As explained, the lower three bits indicate the number of bytes needed
/// to encode the string's length, shifted by 1. The memory referenced by
/// the encoded pointer starts with however many bytes are needed to encode
/// the length, followed by the actual bytes of the string.
///
/// The Case For A Custom Allocator:
/// A custom allocator is considered for integration with the StorageModel
/// for Str. The considerations are:
/// * The size of the StorageModel can be set to the size of char *, the
///   theoretical optimal size of the StorageModel.
/// * The remaining sizes can be divided into arenas (with increasing limit
///   sizes) to minimize waste. Regular (constexpr) steps (e.g., 1 between
///   8-15, 2 between 16-23, etc.) can be trialed.
/// * An arena should 'steal' a slot from an arena whose base size is a
///   multiple of the original. This forces us to maintain metadata on the
///   size and load factor for each arena.
/// * The previous point aims to minimize hits on the general allocator.
template<typename StorageModel>
struct Str {
    constexpr static auto
        Size = sizeof(StorageModel), // remaining count is Size - used
        BitsToEncodeLocalLength = Log2Ceiling(Size - 1),
        CodeSize = BitsToEncodeLocalLength + 1,
        HeapAlignmentRequirement = std::size_t(1) << CodeSize;
    ;
    constexpr static char OnHeapIndicatorFlag = 1 << BitsToEncodeLocalLength;
    static_assert(sizeof(void *) <= Size);
    alignas(alignof(StorageModel)) char buffer_[sizeof(StorageModel)];

    auto lastByte() const noexcept { return buffer_[Size - 1]; }
    auto &lastByte() noexcept { return buffer_[Size - 1]; }
    auto codePtr() const noexcept { return buffer_ + Size - sizeof(char *); }
    auto codePtr() noexcept { return buffer_ + Size - sizeof(char *); }
    auto allocationPtr() const noexcept {
        uintptr_t codeThatContainsAPointer;
        memcpy(&codeThatContainsAPointer, codePtr(), sizeof(char *));
        assert(onHeap());
        auto asLittleEndian = __builtin_bswap64(codeThatContainsAPointer);
        auto bytesForSizeMinus1 = asLittleEndian & 7;
        auto codeRemoved =
            asLittleEndian ^ OnHeapIndicatorFlag ^ bytesForSizeMinus1;

        char *rv;
        memcpy(&rv, &codeRemoved, sizeof(char *));
        return rv;
    }

    auto encodePointer(char numberOfBytes, const char *ptr) {
        uintptr_t code;
        memcpy(&code, &ptr, sizeof(char *));
        static_assert(8 == sizeof(char *));
        if(8 < numberOfBytes) {
            __builtin_unreachable();
        }
        return
            __builtin_bswap64(code | (numberOfBytes - 1) | OnHeapIndicatorFlag);
    }

    auto size() const noexcept {
        if(onHeap()) {
            auto countOfBytesForEncodingSize = 1 + (lastByte() & 7);
            const char *ptr = allocationPtr();
            std::remove_const_t<decltype(Size)> length = 0;
            memcpy(&length, ptr, countOfBytesForEncodingSize);
            return length - 1;
        }
        else {
            return Size - lastByte() - 1;
        }
    }

    auto onHeap() const noexcept {
        return bool(OnHeapIndicatorFlag & lastByte());
    }

    Str(const char *source, std::size_t length) {
        if(length <= Size) {
            lastByte() = Size - length;
            memcpy(buffer_, source, length);
        } else {
            auto bytesForSize = (Log2Ceiling(length) + 7) >> 3;
            auto onHeap =
                new(std::align_val_t(HeapAlignmentRequirement))
                char[bytesForSize + length];
            fprintf(stderr, "onHeap: %p\n", onHeap);
            
            assert(onHeap);
            // assumes little endian here!
            memcpy(onHeap, &length, bytesForSize);
            memcpy(onHeap + bytesForSize, source, length);

            auto code = encodePointer(bytesForSize, onHeap);
            auto addressOfLastPointerWorthOfSpace = codePtr();
            memcpy(addressOfLastPointerWorthOfSpace, &code, sizeof(char *));
            // We encode the range [1..8] as [0..7]

            printf("buffer_ : %p:\n", buffer_);
            for (auto i = 0; i < Size; i++) {
                printf("%02x ", (unsigned char)buffer_[i]);
            }
            printf("\n");
            auto ptrToStr = allocationPtr() + bytesForSize;
            printf("%p \n", ptrToStr);
            printf("\n");

            for (auto i = 0; i <= length; i++) {
                printf("%c ", (unsigned char)ptrToStr[i]);
            }
            printf("\n");

            printf("Size: %lu\n", Size);
            printf("BitsToEncodeLocalLength: %lu\n", BitsToEncodeLocalLength);
            printf("CodeSize: %lu\n", CodeSize);
            printf("HeapAlignmentRequirement: %lu\n", HeapAlignmentRequirement);
        }
    }

    template<std::size_t L>
    Str(const char (&in)[L]) noexcept(L <= Size):
        Str(static_cast<const char *>(in), L - 1)
    {}

    Str() noexcept {
        buffer_[0] = '\0';
        lastByte() = Size - 1;
    }

    Str(const Str &model): Str(model.c_str(), model.size() + 1) {}

    Str(Str &&donor) noexcept {
        memcpy(buffer_, donor.buffer_, sizeof(this->buffer_));
        donor.lastByte() = ~OnHeapIndicatorFlag;
    }

    Str &operator=(const Str &model) {
        if(!model.onHeap()) {
            if(onHeap()) { delete[] allocationPtr(); }
            else {
                if(this == &model) { return *this; }
            }
            new(this) Str(model);
            return *this;
        }
        // The model is on the heap
        auto modelSize = model.size();
        if(size() < modelSize) {
            if(onHeap()) { this->~Str(); }
            new(this) Str(model);
        } else {
            if(this == &model) { return *this; }
            // assert we are on the heap too
            auto modelBytesForSize = 1 + (model.lastByte() & 7);
            auto myPtr = allocationPtr();
            auto modelPtr = model.allocationPtr();
            memcpy(myPtr, modelPtr, modelBytesForSize + modelSize + 1);
            encodePointer(modelBytesForSize, myPtr);
        }
        return *this;
    };

    Str &operator=(Str &&donor) noexcept {
        Str temporary(std::move(donor));
        this->~Str();
        return *new(this) Str(std::move(temporary));
    }

    ~Str() {
        if(onHeap()) { delete[] allocationPtr(); }
    }

    const char *c_str() const noexcept {
        if (!onHeap()) {
            return buffer_;
        }

        // the three least-significant bits of code contain the size of length,
        // encoded as a range [0..7] that maps to a range [1..8] in which each
        // the unit is a byte
        auto byteWithEncoding = lastByte();
        auto bytesForLength = 1 + (byteWithEncoding & 7);

        auto location = allocationPtr();
        auto rv = location + bytesForLength;
        fprintf(stderr, "Returning %s\n", rv);
        return rv;
    }
};

} // closes zoo
