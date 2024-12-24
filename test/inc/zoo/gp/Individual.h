#ifndef ZOO_GP_INDIVIDUAL_H
#define ZOO_GP_INDIVIDUAL_H

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

    WeightedPreorder(char *p): space_(p) {}

    struct Proxy {
        WeightedPreorder wp;
        Proxy(char *p): wp(p) {}
        WeightedPreorder &operator*() { return wp; }
        WeightedPreorder &operator->() { return wp; }
    };

    Proxy descendantsStart() { return {space_ + 3}; }
    
    Proxy next(Proxy p) {
        uint16_t currentLength;
        memcpy(&currentLength, p.wp.space_, 2);
        return {p.wp.space_ + 3 * currentLength};
    }

    typename L::TokenEnum node() {
        return typename L::TokenEnum{space_[2]};
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
