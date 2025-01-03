#ifndef ZOO_GP_TO_STRING_H
#define ZOO_GP_TO_STRING_H

#include <string>
#include <assert.h>

namespace zoo {

template<typename Language>
struct IndividualStringifier {
    using L = Language;
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
            assert(unsigned(head) < size(Language::ArgumentCount));
            thy->str += Language::Tokens[head];
            if(head < Language::TerminalsCount) {
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

template<typename L>
std::string flat(const char *p, int size) {
    std::string rv = "[";
    rv += std::to_string(size);
    if(0 < size) {
        rv += ' ';
        rv += L::Tokens[*p++];
    }
    for(int i = 1; i < size; ++i) {
        rv += ", ";
        rv += L::Tokens[*p++];
    }
    rv += " ]";
    return rv;
}

}
#endif
