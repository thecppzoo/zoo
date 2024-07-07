#include "TightPolicy.h"

using Visit = void(*)();

struct TightVisitable;

struct TightVisitPolicy: TightPolicy {
    using MemoryLayout = TightVisitable;
    using Visit = ::Visit;
};

using VisitableAnyTight = zoo::AnyContainer<TightVisitPolicy>;

std::string to_string(VisitableAnyTight &v);

struct TightVisitable;

inline Fallback *fallback(const VisitableAnyTight &);

using VAT = VisitableAnyTight;

struct TightVisitable: Tight {
    using Stringifier = void *(*)(const VAT &, void *);
    using Numerifier = void *(*)(const VAT &, void *);

    static std::string *stringify(Tight t) {
        return zoo::anyContainerCast<std::string>(fallback(t.code.pointer));
    }

    Visit *visits() {
        static Visit rv[] = {
            (Visit)(Stringifier)(
                [](const VAT &ta, void *result) -> void * {
                    std::string *r = static_cast<std::string *>(result);
                    auto t = *ta.container();
                    if(t.isInteger()) {
                        *r = std::to_string(long{t.code.integer});
                        return r;
                    }
                    if(t.isString()) {
                        *r = t.code.string;
                        return r;
                    }
                    if(t.isPointer()) {
                        *r = *stringify(t);
                        return r;
                    }
                    return nullptr;
            }), (Visit)(Numerifier)(
                [](const VAT &ta, void *result) -> void * {
                    auto &r = *static_cast<long *>(result);
                    auto t = *ta.container();
                    if(t.isInteger()) {
                        r = t.code.integer;
                        return &r;
                    }
                    if(t.isString()) {
                        std::string tmp{t.code.string};
                        try {
                            r = stol(tmp);
                        } catch(std::invalid_argument &) {
                            return nullptr;
                        }
                        return &r;
                    }
                    if(t.isPointer()) {
                        auto f = fallback(ta);
                        if(typeid(std::string) != f->type()) {
                            return nullptr;
                        }
                        auto &str = *stringify(t);
                        try {
                            r = stol(str);
                        } catch(std::invalid_argument &) {
                            return nullptr;
                        }
                        return &r;
                    }
                    return nullptr;
        })};
        return rv;
    }
};

Fallback *fallback(const VisitableAnyTight &t) {
    return fallback(t.container()->code.pointer);
}

#include <iostream>

int main(int argc, const char *argv[]) {
    VAT hello{std::string{"Hello World!"}};
    std::string response;
    auto v = (TightVisitable::Stringifier)visits(hello)[0];
    auto success = v(hello, &response);
    if(success) { std::cout << response << std::endl; }
    else { return 1; }
    VAT numb{std::string{"1973"}};
    auto nv = (TightVisitable::Numerifier)visits(numb)[1];
    long longNumber;
    success = nv(numb, &longNumber);
    if(!success) { return 2; }
    std::cout << longNumber << std::endl;
    return 0;
}

