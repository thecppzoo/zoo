#ifndef ZOO_GP_STRIP_H
#define ZOO_GP_STRIP_H

#include <string>
#include <stack>

namespace zoo {

struct Strip {
    std::string payload;
};

/// Crosses two subtrees from two parent trees into a new tree.
///
/// \param strip The destination string where the crossed result will be appended.
///        It is resized internally to accommodate the result.
/// \param q The first parent tree represented as a character array.
/// \param qNdx The index within `q` of the root of the subtree to cross.
/// \param p The second parent tree represented as a character array.
/// \param pNdx The index within `p` of the root of the subtree to cross.
/// \param subTreeSize A function that computes the size of a subtree given its root.
void cross(
    std::string &strip,
    const char *q, int qNdx, const char *p, int pNdx,
    int (*subTreeSize)(const char *) noexcept
);

template<typename Language>
auto treeSize(const char *tree) noexcept {
    auto base = tree;
    std::stack<int> remainingSiblings;

    auto siblings = 1;

    for(;;) {
        auto root = *tree++;
        auto count = Language::ArgumentCount[root];
        if(0 == count) {
            while(--siblings == 0) {
                if(remainingSiblings.empty()) { return tree - base; }
                siblings = remainingSiblings.top();
                remainingSiblings.pop();
            }
            continue;
        }
        if(1 == count) {
            continue;
        }
        remainingSiblings.push(siblings);
        siblings = count;
    }
}

}
#endif
