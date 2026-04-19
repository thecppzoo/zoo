## Example of subtyping as subclassing: Typical Serialization in C++

```c++
#include <sstream>

// Infrastructure:
namespace user {

struct TypeRegistry {
    template<typename T>
    std::uint64_t id() const;
};

extern TypeRegistry g_registry;

struct ISerialize {
    virtual std::ostream &serialize(std::ostream &) const;
    virtual std::size_t length() const;
};

template<typename T>
struct SerializeWrapper: ISerialize {
    T value_;
    virtual std::ostream &serialize(std::ostream &to) const override {
        return
            to << g_registry.id<T>() << ':' << value_;
    }
    virtual std::size_t length() const {
        std::ostringstream temporary;
        serialize(temporary);
        return temporary.str().length();
    }
};

}
```
