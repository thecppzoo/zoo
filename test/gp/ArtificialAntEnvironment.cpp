#include "zoo/gp/ArtificialAntEnvironment.h"
#include "zoo/gp/ArtificialAntLanguage.h"

template<typename IType>
void artificialAntExecution(Environment &e, IType &ind) {
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


