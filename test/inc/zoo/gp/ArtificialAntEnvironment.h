#ifndef ZOO_GP_ENVIRONMENT_H
#define ZOO_GP_ENVIRONMENT_H

struct ArtificialAntEnvironment {
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

    ArtificialAntEnvironment() : steps_(0) {
        ant_.pos = Position{0, 0};
        ant_.dir = Position{1, 0};
        initializeFood();
    }
};

template<typename IType>
void artificialAntExecution(ArtificialAntEnvironment &e, IType &ind);

#endif
