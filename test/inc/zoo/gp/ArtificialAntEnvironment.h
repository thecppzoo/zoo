#ifndef ZOO_GP_ENVIRONMENT_H
#define ZOO_GP_ENVIRONMENT_H

#include "ArtificialAntLanguage.h"

#include "zoo/gp/Individual.h"

#include <array>

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
        Position pos, dir;
    };

    Ant ant_;
    bool food_[GridHeight][GridWidth];
    int
        steps_,
        thinking_,
        eaten_;

    constexpr static inline bool InitialFoodMatrix[GridHeight][GridWidth] = {
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
        ++steps_;
    }

    void turnRight() {
        int oldX = ant_.dir.x;
        int oldY = ant_.dir.y;
        ant_.dir = Position{oldY, -oldX};
        ++steps_;
    }

    Position aheadPosition() const {
        return ant_.pos + ant_.dir;
    }

    bool hasFood(Position p) const {
        return food_[p.y][p.x];
    }

    bool foodAhead() const {
        return hasFood(aheadPosition());
    }

    void moveForward() {
        ant_.pos = aheadPosition();
        if(hasFood(ant_.pos)) {
            ++eaten_;
            food_[ant_.pos.y][ant_.pos.x] = false;
        }
        ++steps_;
    }

    bool atEnd() const {
        return (MaxSteps * 16) <= (this->thinking_ + this->steps_ * 16);
        // return MaxSteps <= steps_;
    }

    ArtificialAntEnvironment() : steps_(0), thinking_(0), eaten_(0) {
        ant_.pos = Position{0, 0};
        ant_.dir = Position{1, 0};
        initializeFood();
    }
};

using ImplementationArray = const std::array<void *, 6>;
extern ImplementationArray implementationArray;

using AAEvaluationFunction =
    void(*)(
        ArtificialAntEnvironment &,
        zoo::WeightedPreorder<ArtificialAnt> &,
        ImplementationArray &
    );

using EvaluationRecursiveFunction =
    void (*)(
        ArtificialAntEnvironment &,
        zoo::WeightedPreorder<ArtificialAnt> &,
        void *
    );

void artificialAntEvaluation(
    ArtificialAntEnvironment &e, zoo::WeightedPreorder<ArtificialAnt> &i
);
void recursiveEvaluationFrontEnd(
    ArtificialAntEnvironment &e,
    zoo::WeightedPreorder<ArtificialAnt> &i,
    void *
);

#endif
