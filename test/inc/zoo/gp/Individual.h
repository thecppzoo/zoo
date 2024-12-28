#ifndef ZOO_GP_INDIVIDUAL_H
#define ZOO_GP_INDIVIDUAL_H

#include <string.h>
#include <string>
#include <assert.h>
#include <stack>

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

    auto destination = output;

    auto writeWeight = [&]() {
        // assumes little endian
        // memcpy would allow unaligned copying of the bytes
        // on the platforms that support it,
        // a "reinterpret_cast" might not respect necessary
        // alignment in some platforms
        auto &ft = frames.top();
        memcpy(ft.destination, &ft.weight, sizeof(uint16_t));
    };

    for(;;) {
        auto current = *input++;

        *destination++ = current;
        auto argumentCount = L::ArgumentCount[current];
        
        if(0 < argumentCount) {
            // There are argumentCount descendants to convert,
            // create a new frame for them
            frames.push({0, unsigned(argumentCount), destination});
        } else {
            constexpr uint16_t one = 1;
            memcpy(destination, &one, 2);
            if(frames.empty()) { return; }
        }
        destination += 2;
        auto ft = &frames.top();
        ++ft->weight;
        while(0 == ft->remainingSiblings--) {
            // No more siblings, thus completed the frame
            writeWeight();
            auto written = frames.top().weight;
            frames.pop();
            if(frames.empty()) { return; }
            ft = &frames.top();
            ft->weight += written;
        }
    }
}

template<typename L>
struct WeightedPreorder {
    char *space_ = nullptr;

    WeightedPreorder(char *p): space_(p) {}

    struct Proxy {
        WeightedPreorder wp;
        Proxy(char *p): wp(p) {}
        WeightedPreorder &operator*() { return wp; }
        WeightedPreorder *operator->() { return &wp; }
    };

    Proxy descendantsStart() { return {space_ + 3}; }
    
    Proxy next(Proxy p) {
        uint16_t currentLength;
        memcpy(&currentLength, p.wp.space_ + 1, 2);
        return {p.wp.space_ + 3 * currentLength};
    }

    typename L::TokenEnum node() {
        return typename L::TokenEnum{space_[0]};
    }
};

/// \details The environment tracks the fitness internally during the execution.
template<
    typename EvaluationFunction,
    typename Environment,
    typename Ind,
    typename ImplementationArray
>
void evaluate(Environment &env, Ind &individual, ImplementationArray &ia) {
    if(env.atEnd()) { return; }
    auto evalFunction =
        reinterpret_cast<EvaluationFunction>(ia[individual.node()]);
    evalFunction(env, individual, ia);
}

} // namespace zoo

#endif
