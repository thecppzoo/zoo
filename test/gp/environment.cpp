#include <cstdio>
#include <iostream>
#include <string>
#include <functional>

enum Node {
    TurnLeft,
    TurnRight,
    Move,
    IFA,
    Prog2,
    Prog3
};

struct Individual {
    Node node_; // The node representing the type of instruction.
    Individual *descendants_[3]; // Array of pointers to child nodes.

    auto node() const noexcept {
        return node_;
    }

    auto descendantsStart() const noexcept {
        return const_cast<Individual **>(descendants_);
    }

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

    void initializeFood() {
        for (int r = 0; r < GridHeight; r++) {
            for (int c = 0; c < GridWidth; c++) {
                food_[r][c] = InitialFoodMatrix[r][c];
            }
        }
    }

    void turnLeft() {
        int oldX = ant_.dir.x;
        int oldY = ant_.dir.y;
        ant_.dir = Position{-oldY, oldX};
    }

    void turnRight() {
        int oldX = ant_.dir.x;
        int oldY = ant_.dir.y;
        ant_.dir = Position{oldY, -oldX};
    }

    Position aheadPosition() const {
        return ant_.pos + ant_.dir;
    }

    bool hasFood(Position p) const {
        return food_[p.y][p.x];
    }

    bool consumeFoodAt(Position p) {
        if (hasFood(p)) {
            food_[p.y][p.x] = false;
            return true;
        }
        return false;
    }

    bool foodAhead() const {
        return hasFood(aheadPosition());
    }

    void moveForward() {
        ant_.pos = aheadPosition();
    }

    bool consumeFood() {
        return consumeFoodAt(ant_.pos);
    }

    bool atEnd() const {
        return MaxSteps <= steps_;
    }

    Environment() : steps_(0) {
        ant_.pos = Position{0, 0};
        ant_.dir = Position{1, 0};
        initializeFood();
    }
};

void artificialAntExecution(Environment &e, Individual &ind) {
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


