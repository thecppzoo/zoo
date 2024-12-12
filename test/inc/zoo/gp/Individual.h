#ifndef ZOO_GP_INDIVIDUAL_H
#define ZOO_GP_INDIVIDUAL_H

#include <string>
#include <assert.h>

namespace zoo {

template<typename Language>
struct IndividualStringifier {
    const char *inorderRepresentation;

    struct Conversion {
        std::string str;

        const char *convert(const char *source) {
            return converter(this, source, (void *)&converter);
        }

        static const char *converter(
            Conversion *thy,
            const char *source,
            void *ptrToConverter
        ) {
            auto recursion =
                reinterpret_cast<
                    const char *(*)(Conversion *, const char *, void *)
                >(ptrToConverter);
            auto head = *source++;
            assert(unsigned(head) < size(ArtificialAnt::ArgumentCount));
            thy->str += Language::Tokens[head];
            if(head < TerminalsCount<Language>()) {
                return source;
            }
            thy->str += "(";
            source = recursion(thy, source, ptrToConverter);
            for(
                auto remainingCount = Language::ArgumentCount[head] - 1;
                remainingCount--;
            ) {
                thy->str += ", ";
                source = recursion(thy, source, ptrToConverter);
            }
            thy->str += ")";
            return source;
        }
    };
};

template<typename Language>
std::string to_string(const IndividualStringifier<Language> &i) {
    typename IndividualStringifier<Language>::Conversion c;
    c.convert(i.inorderRepresentation);
    return c.str;
};

struct ConversionFrame {
    unsigned weight, remainingSiblings;
    char *destination;
};

template<typename L>
auto conversionToWeightedElement(char *output, const char *input) {
    std::stack<ConversionFrame> frames;

    auto writeWeight = [&](unsigned w) {
        // assumes little endian
        // memcpy would allow unaligned copying of the bytes
        // on the platforms that support it,
        // a "reinterpret_cast" might not respect necessary
        // alignment in some platforms
        memcpy(frames.top().destination, &w, sizeof(uint16_t));
        output += 2;
    };

    frames.push({0, 1, output});
    assert(0 < L::ArgumentCount[*input]);

    for(;;) {
        auto &top = frames.top();
        if(0 == top.remainingSiblings--) {
            // No more siblings, thus completed the frame
            writeWeight(top.weight);
            frames.pop();
            if(frames.empty()) { break; }
            continue;
        }
        frames.top().weight += 1;
        auto current = *input++;
        auto destination = frames.top().destination;

        *frames.top().destination++ = current;
        auto argumentCount = L::ArgumentCount[current];
        if(0 < argumentCount) {
            // There are argumentCount descendants to convert,
            // create a new frame for them
            frames.push({0, unsigned(argumentCount), destination + 2});
        }
    }
}

template<typename L>
struct WeightedPreorder {
    char *space_ = nullptr;

    WeightedPreorder(const char *p, size_t count) {
        auto space = new char[count * 3];
        conversionToWeightedElement<L>(space, p);
        space_ = space;
    }

    ~WeightedPreorder() { delete[] space_; }

    char *descendantsStart() { return space_ + 3; }
    
    auto next(char *ptr) {
        uint16_t currentLength;
        memcpy(&currentLength, ptr, 2);
        return ptr + 3 * currentLength;
    }
};

}

#endif
