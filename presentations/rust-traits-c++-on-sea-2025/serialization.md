## Example of subtyping as subclassing: Typical Serialization in C++

```c++
#include <ostringstream>

// Infrastructure:
namespace user {

struct ISerialize {
    virtual std::ostream &serialize(std::ostream &) const;
    virtual std::size_t length() const;
};

template<typename T>
struct SerializeWrapper: ISerialize {
    T value_;
    virtual std::ostream &serialize(std::ostream &to) const override {
        return to << type_index(typeid(T)) << ':' << value_;
    }
    virtual std::size_t length() const {
        std::ostream temporary;
        serialize(temporary);
        return temporary.str().length();
    }
};

```
