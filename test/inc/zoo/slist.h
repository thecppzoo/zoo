#ifndef ZOO_SLIST
#define ZOO_SLIST

#ifndef SIMPLIFY_PREPROCESSING
#include <utility>
#endif

namespace zoo {

template<typename T>
struct SList {
    struct Box {
        T element_;
        Box *next_ = nullptr;

        template<typename... Initializers>
        Box(Initializers &&...ins):
            element_{std::forward<Initializers>(ins)...}
        {}
    };

    struct Iterator {
        Box *value_;

        Iterator operator++(int) noexcept {
            auto rv = *this; value_ = value_->next_; return rv;
        }

        Iterator &operator++() noexcept {
            value_ = value_->next_;
            return *this;
        }

        T &operator*() const noexcept { return value_->element_; }
        T *operator->() const noexcept { return &*this; }

        bool operator==(Iterator o) const {
            return value_ == o.value_;
        }

        bool operator!=(Iterator o) const { return not(*this == o); }
    };

    Box *head_ = nullptr, **tail= &head_;

    Iterator begin() const { return {head_}; }
    Iterator end() const { return {nullptr}; }

    void push_back(const T &t) { *tail = new Box{t}; tail = &(*tail)->next_; }
};

}

#endif
