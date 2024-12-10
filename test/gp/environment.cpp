#include <cstdio>

enum Node {
    TurnLeft,
    TurnRight,
    Move,
    IFA,
    Prog2,
    Prog3
};

struct Individual {
    Node node_; ///< The node representing the type of instruction.
    Individual *descendants_[3]; ///< Array of pointers to child nodes.

    /// Returns the current node type.
    auto node() const noexcept {
        return node_;
    }

    /// Returns a pointer to the first descendant in the array.
    auto descendantsStart() const noexcept {
        return const_cast<Individual **>(descendants_);
    }

    /// Advances the pointer to the next descendant in the array.
    /// @param current Pointer to the current descendant.
    /// @return Pointer to the next descendant.
    auto next(Individual **current) const noexcept {
        return const_cast<Individual **>(current + 1);
    }
};

auto descendantCount(Node n) {
    switch (n) {
    case IFA:
        return 1;
    case Prog2:
        return 2;
    case Prog3:
        return 3;
    default:
        return 0;
    }
}

/// Represents the environment, including a trail of food and the ant's state.
/// The environment tracks the ant's position, orientation, and the distribution of food.
/// The ant moves on a grid with wrap-around boundaries, and tries to consume all the food
/// placed according to the Santa Fe trail pattern.
struct Environment {
    static constexpr auto
        GridWidth = 32,
        GridHeight = 32,
        MaxSteps = 600;

    struct Position {
        int x, y;
        auto operator+(Position other) const {
            return Position{
                (x + other.x + GridWidth) % GridWidth,
                (y + other.y + GridHeight) % GridHeight
            };
        }
    };

    struct Ant {
        Position pos;
        Position dir;
    };

    Ant ant_;
    bool food_[GridHeight][GridWidth];
    int steps_;

    constexpr static inline bool InitialFoodMatrix[32][32] = {
        #define F true
        #define _ false
        {_,F,F,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
        {_,_,_,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
        {_,_,_,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,F,F,F,_,_,_,_},
        {_,_,_,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_,F,_,_},
        {_,_,_,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_,F,_,_},
        {_,_,_,F,F,F,F,_,F,F,F,F,F,_,_,_,_,_,_,_,_,F,F,_,_,_,_,_,_,_,_,_},
        {_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,F,_,_},
        {_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,_,_,_,_},
        {_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,_,_,_,_},
        {_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,_,F,_,_},
        {_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,_,_,_,_},
        {_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
        {_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,F,_,_},
        {_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,_,_,_,_},
        {_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,F,_,_,_,_,_,F,F,F,_,_,_},
        {_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_,_,F,_,_,_,_,_,_,_,_},
        {_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
        {_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
        {_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,F,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_},
        {_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,F,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_},
        {_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
        {_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
        {_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_,_},
        {_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,_},
        {_,_,_,F,F,_,_,F,F,F,F,F,_,_,_,_,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
        {_,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
        {_,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
        {_,F,_,_,_,_,_,_,F,F,F,F,F,F,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
        {_,F,_,_,_,_,_,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
        {_,_,_,_,_,_,_,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
        {_,_,F,F,F,F,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_},
        {_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_}
        #undef F
        #undef _
    };



    auto initializeFood() {
        for (auto r = 0; r < GridHeight; r = r + 1) {
            for (auto c = 0; c < GridWidth; c = c + 1) {
                food_[r][c] = InitialFoodMatrix[r][c];
            }
        }
    }

    auto turnLeft() {
        auto oldX = ant_.dir.x;
        auto oldY = ant_.dir.y;
        ant_.dir = Position{-oldY, oldX};
    }

    auto turnRight() {
        auto oldX = ant_.dir.x;
        auto oldY = ant_.dir.y;
        ant_.dir = Position{oldY, -oldX};
    }

    auto aheadPosition() const {
        return ant_.pos + ant_.dir;
    }

    auto hasFood(Position p) const {
        return food_[p.y][p.x];
    }

    auto consumeFoodAt(Position p) {
        if (hasFood(p)) {
            food_[p.y][p.x] = false;
            return true;
        }
        return false;
    }

    auto foodAhead() {
        return hasFood(aheadPosition());
    }

    auto moveForward() {
        ant_.pos = aheadPosition();
    }

    auto consumeFood() {
        return consumeFoodAt(ant_.pos);
    }

    auto atEnd() const {
        return MaxSteps <= steps_;
    }

    Environment() : steps_(decltype(0){}) {
        ant_.pos = Position{decltype(0){}, decltype(0){}};
        ant_.dir = Position{decltype(1){}, decltype(0){}};
        initializeFood();
    }
};

auto artificialAntExecution(Environment &e, Individual &ind) {
    if (e.atEnd()) {
        return;
    }

    switch (ind.node()) {
    case TurnLeft:
        e.turnLeft();
        ++e.steps_;
        break;

    case TurnRight:
        e.turnRight();
        ++e.steps_;
        break;

    case Move:
        e.moveForward();
        e.consumeFood();
        ++e.steps_;
        break;

    case IFA:
        if (e.foodAhead()) {
            auto descendant = ind.descendantsStart();
            artificialAntExecution(e, **descendant);
        }
        break;

    case Prog2: {
        auto descendant = ind.descendantsStart();
        artificialAntExecution(e, **descendant);
        descendant = ind.next(descendant);
        artificialAntExecution(e, **descendant);
        break;
    }

    case Prog3: {
        auto descendant = ind.descendantsStart();
        artificialAntExecution(e, **descendant);
        descendant = ind.next(descendant);
        artificialAntExecution(e, **descendant);
        descendant = ind.next(descendant);
        artificialAntExecution(e, **descendant);
        break;
    }
    }
}

#include <iostream>

void displayEnvironment(const Environment &env) {
    static constexpr auto
        EMPTY_CELL = '.',
        FOOD_CELL = 'F',
        ANT_UP = '^',
        ANT_RIGHT = '>',
        ANT_DOWN = 'v',
        ANT_LEFT = '<';

    // Select the character representing the ant's direction.
    char antChar;
    if (env.ant_.dir.x == 0 && env.ant_.dir.y == -1) antChar = ANT_UP;
    else if (env.ant_.dir.x == 1 && env.ant_.dir.y == 0) antChar = ANT_RIGHT;
    else if (env.ant_.dir.x == 0 && env.ant_.dir.y == 1) antChar = ANT_DOWN;
    else if (env.ant_.dir.x == -1 && env.ant_.dir.y == 0) antChar = ANT_LEFT;
    else antChar = '?'; // Fallback for invalid direction.

    // Display the grid row by row.
    for (int y = 0; y < Environment::GridHeight; ++y) {
        for (int x = 0; x < Environment::GridWidth; ++x) {
            if (x == env.ant_.pos.x && y == env.ant_.pos.y) {
                // Ant's position.
                std::cout << antChar;
            } else if (env.food_[y][x]) {
                // Food pellet.
                std::cout << FOOD_CELL;
            } else {
                // Empty cell.
                std::cout << EMPTY_CELL;
            }
        }
        std::cout << '\n';
    }
}



auto eval(Environment &env, Individual &ind) {
    auto initialFoodCount = decltype(0){};
    for (auto r = decltype(0){}; r < Environment::GridHeight; r = r + 1) {
        for (auto c = decltype(0){}; c < Environment::GridWidth; c = c + 1) {
            if (env.food_[r][c]) {
                initialFoodCount = initialFoodCount + 1;
            }
        }
    }

    artificialAntExecution(env, ind);

    auto remainingFoodCount = decltype(0){};
    for (auto r = 0; r < Environment::GridHeight; r = r + 1) {
        for (auto c = 0; c < Environment::GridWidth; c = c + 1) {
            if (env.food_[r][c]) {
                remainingFoodCount = remainingFoodCount + 1;
            }
        }
    }

    auto consumed = (initialFoodCount - remainingFoodCount);
    return (consumed + decltype(0.0f){});
}


#include <string>
#include <functional>

/// \brief Preorder traversal of an individual's tree structure.
///
/// @tparam NodeProcessing A callable that takes a `Node` and processes it
/// @param individual The root of the tree to traverse.
/// @param fun The processor of nodes, for example, what converts a node to its label.
/// @return The result of the traversal, for example, the string representation of the tree
template<typename NodeProcessing>
auto traverse(Individual &individual, NodeProcessing fun) {
    auto result = fun(individual.node_); // Get the label for the current node.

    // Check the number of descendants for the current node.
    auto count = descendantCount(individual.node_);
    if(0 == count) {
        return result; // Early return for terminals, no descendants to process.
    }

    result += "(";

    // Factor out of the main loop the first descendant as a special case
    // because it does not add print ", "
    auto descendant = individual.descendantsStart();
    result += traverse(*descendant, fun);

    while(--count) { // Predecrement because one descendant was processed
        result += ", ";
        descendant = individual.next(descendant); // Advance to the next descendant.
        result += traverse(*descendant, fun);
    }

    result += ")";

    return result;
}
