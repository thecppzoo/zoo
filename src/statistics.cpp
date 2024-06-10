#ifdef TRACE_TIGHT
#include <vector>
#include <string>
#include <iostream>
#include "TightPolicy.h"

std::vector<std::string> constructionTrace;

void constructedWith(const char *how, const std::string *s) {
    constructionTrace.push_back(how);
    constructionTrace.push_back(*s);
}

void constructedWith(const char *how, const char *buffer, std::size_t length) {
    std::string arg{buffer, buffer + length};
    constructedWith(how, &arg);
}

template<typename T>
void makeRefAnn(const T *driver, const void *space, const void *vptr) {
    std::cout << "Referential made for " << typeid(*driver).name() <<
        '@' << driver << " Handling space at " << space << " pointing to " << vptr << std::endl;
}

#define ANNOTATE_MAKE_REFERENTIAL(...) makeRefAnn(__VA_ARGS__);
#define TIGHT_CONSTRUCTOR_ANNOTATION(...) constructedWith(__VA_ARGS__);
#endif

#include "TightPolicy.h"

int stringInternalBufferSize() {
    std::string s{"1"};
    for(;;) {
        auto data = s.data();
        std::string other{std::move(s)};
        if(data == other.data()) {
            // this size first moved the buffer
            return other.size();
        }
        s = other + '/';
    }
}

template<typename Iterator>
long occupied(Iterator begin, Iterator end) {
    static auto sibs = stringInternalBufferSize();
    constexpr auto cut = 1l << 32;
    long rv = 0;
    for(auto cursor = begin; cursor != end; ++cursor) {
        if(!cursor->container()->isPointer()) {
            ++rv;
            continue;
        }
        if(typeid(std::string) != cursor->type()) { throw; }
        rv += cut + 3; // allocates an AnyContainer<ConverterPolicy>
        auto str = tightCast<std::string>(*cursor);
        auto strS = str.size();
        if(sibs <= strS) {
            // the string requires allocating a buffer
            rv += cut + (strS + 7)/8;
        }
    }
    return rv;
}

#include <vector>
#include <istream>
#include <cctype>

#include <iostream>

void deconstruct(const TightAny &ta) {
    auto cont = ta.container();
    if(cont->isPointer()) {
        void *ptr = cont->code.pointer;
        std::cout << "Is pointer: " << ptr << std::endl;
        auto f = fallback(cont->code.pointer);
        auto innerC = f->container();
        std::cout << typeid(*innerC).name() << " at " << innerC << std::endl;
        auto driver = innerC->driver();
        std::cout << typeid(*driver).name() << " at " << driver << std::endl;
        auto &tid = driver->type();
        if(typeid(std::string) == tid) {
            void *reference = innerC->m_space;
            std::string *strPtr = *reinterpret_cast<std::string **>(reference);
            std::cout << "Contains std::string ptr at " << reference << std::endl;
            std::cout << *strPtr << std::endl;
            //auto valPtr = zoo::anyContainerCast<std::string>(f);
            //if(valPtr != str)
        }
    }
}

template<typename Container>
void parse(Container &container, std::istream &input) {
    using namespace std;
    char buffer[256];
    int count = 0;
    while(input >> noskipws >> buffer[count]) {
        auto c = buffer[count];
        if(isspace(c)) {
            if(0 == count) { continue; }
            std::string orig{buffer, buffer + count};
            TightAny a{orig};
            container.push_back(a);
            count = 0;
            continue;
        }
        if(isalpha(c)) {
            if(255 == count) { throw; }
            ++count;
            continue;
        }
        if(count) {
            std::string orig{buffer, buffer + count};
            TightAny a{orig};
            container.push_back(a);
        }
        count = 0;
    }
}

#include <fstream>

int main(int argc, const char *argv[]) {
    std::vector<TightAny> anies;
    std::ifstream in{"input"};
    parse(anies, in);
    std::cout << "Parsed " << anies.size() << std::endl;
    auto hum = occupied(anies.begin(), anies.end());
    constexpr auto cut = 1l << 32;
    std::cout << "Allocations: " << hum / cut << " @" << 8*(hum % cut) << std::endl;
    std::cout << "String internal buffer size: " << stringInternalBufferSize()
        << std::endl;
    /*for(auto &v: anies) {
        std::cout << tightCast<std::string>(v) << std::endl;
    }*/
	return 0;
}
