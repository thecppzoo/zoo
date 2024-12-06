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

/// \brief Ah
///
/// Design:
/// To minimize allocations, this type has a configurqble internal
/// buffer --of size N * sizeof(char *)-- that will hold the carac-
/// ters of a string short enough to fit in it.
/// To maximize space efficiency, the encoding of whether the string
/// is local or on the heap will occur at the last byte, least
/// significant bits of the local storage.
/// The local storage doubles as the encoding of a heap pointer, for
/// strings that can not be held locally.
/// This uses Andrei Alexandrescu's idea of storing the size of the
/// string as the remaining local capacity, in this way, a 0 local
/// capacity also encodes the null byte for c_str(), as reported by
/// Nicholas Omrod at CPPCon 2016.
/// In case the string is too long for local allocation, the
/// pointer in the heap will be encoded locally in the last word
/// of the representation.
/// Thus the optimal size of the local buffer is of sizeof(char *)
/// since it well either used as local storage or as pointer to the
/// heap.
/// \tparam StorageModel Indicates the size of the local storage
/// and its alignment.
/// For large strings (allocated in the heap):
/// In the last void * worth of local storage:
/// +---------------- Big Endian diagram -----------------------------+
/// | Most significant part of Pointer |  Last Byte, rob 4 lower bits |
/// +-----------------------------------------------------------------+
/// Since we're robbing 4 bits from the last byte, this implies the 
/// pointers are rounded to 16 bytes.
/// For local strings:
/// +------- Little Endian Diagam -----+
/// | characters... | last byte        |
/// +----------------------------------+
/// The last byte looks like this:
/// For heap encoding: number of bytes of the size in the allocation,
/// the value is in the interval [1, 8], this is represented with a 
/// shifted range of [0, 7] which allow us to use 3 bits to encode the
/// number of bytes used to encode the length of the string. A fourth
/// bit is used to flag whether the local buffer stores a pointer to a
/// heap allocated string (1) or if it is represented in the local bu=
/// ffer (0). In the heap encoding the binary number represented in such
/// bits, precedes the actual string payload.
/// For local encoding: the last bytes take the form of the null ter-
/// minator.
///
/// The Case For A Custom Allocator.
/// A custom allocator was being considered for integration with the
/// StorageModel String. The considerations we see are:
/// * The size of the StorageModel can be set to size of char *, the
///   theoretical optimal size of the StorageModel String.
/// * The rest of the sizes can be divided in arenas (with increasing
///   limit sizes) to minimize the waste. We can assign some regular
///   (constexpr) step to these (say 1 between 8-15, 2 between 16-23,
///   and so on. that will depend on trial an error).
/// * An arena should be able to 'steal' a slot from an arena that has 
///   a base size that is a multiple of the original. This will force
///   us to maintain metadata of the size and load factor for each
///   arena.
/// * A goal that the previous point tries to achieve is to minimize 
///   the number of hits on the general allocator.
/// * The design has rely on touching only metadata. with just a visit
///   to the arena to mark the buffer as taken.
///   
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

    auto &lastByte() const noexcept { return buffer_[Size - 1]; }
    auto &lastByte() noexcept { return buffer_[Size - 1]; }
    auto lastPtr() const noexcept { return buffer_ + Size - sizeof(char *); }
    auto lastPtr() noexcept { return buffer_ + Size - sizeof(char *); }
    auto allocationPtr() const noexcept {
        uintptr_t codeThatContainsAPointer;
        memcpy(&codeThatContainsAPointer, lastPtr(), sizeof(char *));
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
            auto addressOfLastPointerWorthOfSpace = lastPtr();
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
