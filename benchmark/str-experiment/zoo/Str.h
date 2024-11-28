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

inline void noOp(...) {}
#define printf noOp
#define fprintf noOp

/// \brief Ah
///
/// Design:
/// To minimize allocations, this type will attempt to hold the chars
/// of the string locally.
/// To maximize space efficiency, the encoding of whether the string
/// is local or on the heap will occur at the last byte, least
/// significant bits of the local storage.
/// the local storage doubles as the encoding of a heap pointer, for
/// strings that can not be held locally
/// This uses Andrei Alexandrescu's idea of storing the size of the
/// string as the remaining local capacity, in this way, a 0 local
/// capacity also encodes the null byte for c_str(), as reported by
/// Nicholas Omrod at CPPCon 2016.
/// In case the string is too long for local allocation, the
/// pointer in the heap will be encoded locally in the last word
/// of the representation.
/// \tparam StorageModel Indicates the size of the local storage
/// and its alignment
/// For large strings (allocated in the heap):
/// In the last void * worth of local storage:
/// +--------- Big Endian diagram ------------------------
/// | Most significant part of Pointer | BytesForSizeEncoding
/// +--------- Little Endian actual representation
/// For local strings:
/// Little Endian diagram
/// +-----------------------------------------------------
/// | characters... | remaining count
/// +------------------------------
/// The last byte looks like this:
/// For heap encoding: number of bytes of the size in the allocation,
/// the value is in the interval [1, 8], this is represented with 4
/// bits in which the bit for
/// For local encoding there is the need for log2(Size) bits to express
/// the size of the local string (either as size or remaining)
/// And we need one more bit to encode local (zero) or heap(1)
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
    alignas(sizeof(StorageModel)) char buffer_[sizeof(StorageModel)];

    auto &lastByte() const { return buffer_[Size - 1]; }
    auto &lastByte() { return buffer_[Size - 1]; }
    auto lastPtr() const { return buffer_ + Size - sizeof(char *); }
    auto lastPtr() { return buffer_ + Size - sizeof(char *); }
    auto allocationPtr() const {
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

    Str() {
        buffer_[0] = '\0';
        lastByte() = Size - 1;
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

            uintptr_t littleEndian;
            memcpy(&littleEndian, &onHeap, sizeof(char *));
            auto bigEndian = __builtin_bswap64(littleEndian);
            memcpy(lastPtr(), &bigEndian, sizeof(char *));
            // We encode the range [1..8] as [0..7]
            auto code = (bytesForSize - 1) | OnHeapIndicatorFlag;
            fprintf(
                stderr,
                "LB before: %x, code %x\n",
                (unsigned char)lastByte(),
                (unsigned char) code
            );
            
            lastByte() |= code;
            fprintf(stderr, "LB after: %x\n", (unsigned char)lastByte());
            printf("In ctor: lastByte(): %02x\n", (unsigned char)lastByte());
            printf("length: %lu, bytesForSize: %lu\n", length, bytesForSize);
            printf("OnHeapIndicatorFlag: %02x\n", OnHeapIndicatorFlag);
            
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
            
            //assert('\0' == ptrToStr[length]);

            printf("Size: %lu\n", Size);
            printf("BitsToEncodeLocalLength: %lu\n", BitsToEncodeLocalLength);
            printf("CodeSize: %lu\n", CodeSize);
            printf("HeapAlignmentRequirement: %lu\n", HeapAlignmentRequirement);
        }
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

    template<std::size_t L>
    Str(const char (&in)[L]): Str(static_cast<const char *>(in), L - 1) {}


    const char *c_str() noexcept {
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

    auto onHeap() const noexcept {
        return bool(OnHeapIndicatorFlag & lastByte());
    }
};

} // closes zoo
