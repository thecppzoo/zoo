#include "zoo/gp/ArtificialAntEnvironment.h"
#include "zoo/gp/ArtificialAntLanguage.h"

#include "zoo/gp/Individual.h"

using Env = ArtificialAntEnvironment;
using Indi = zoo::WeightedPreorder<ArtificialAnt>;

using Stack = std::stack<Indi *>;
using EF = void (*)(Stack &, Env &e);

void evalTurnLeft(
    ArtificialAntEnvironment &env,
    zoo::WeightedPreorder<ArtificialAnt> &,
    ImplementationArray &
) {
    env.turnLeft();
    ++env.steps_;
}

void evalTurnRight(
    ArtificialAntEnvironment &env,
    zoo::WeightedPreorder<ArtificialAnt> &,
    ImplementationArray &
) {
    env.turnRight();
    ++env.steps_;
}

void evalMove(
    ArtificialAntEnvironment &env,
    zoo::WeightedPreorder<ArtificialAnt> &,
    ImplementationArray &
) {
    env.moveForward();
    env.consumeFood();
    ++env.steps_;
}

void evalIFA(
    ArtificialAntEnvironment &env,
    zoo::WeightedPreorder<ArtificialAnt> &ind,
    ImplementationArray &ia
) {
    if (env.foodAhead()) {
        auto descendant = ind.descendantsStart();
        zoo::evaluate<AAEvaluationFunction>(env, *descendant, ia);
    }
}

void evalProg2(
    ArtificialAntEnvironment &env,
    zoo::WeightedPreorder<ArtificialAnt> &ind,
    ImplementationArray &ia
) {
    auto descendant = ind.descendantsStart();
    zoo::evaluate<AAEvaluationFunction>(env, *descendant, ia);
    descendant = ind.next(descendant);
    zoo::evaluate<AAEvaluationFunction>(env, *descendant, ia);
}

void evalProg3(
    ArtificialAntEnvironment &env,
    zoo::WeightedPreorder<ArtificialAnt> &ind,
    ImplementationArray &ia
) {
    auto descendant = ind.descendantsStart();
    zoo::evaluate<AAEvaluationFunction>(env, *descendant, ia);
    descendant = ind.next(descendant);
    zoo::evaluate<AAEvaluationFunction>(env, *descendant, ia);
    descendant = ind.next(descendant);
    zoo::evaluate<AAEvaluationFunction>(env, *descendant, ia);
}

ImplementationArray implementationArray = {
    reinterpret_cast<void *>(evalTurnLeft),
    reinterpret_cast<void *>(evalTurnRight),
    reinterpret_cast<void *>(evalMove),
    reinterpret_cast<void *>(evalIFA),
    reinterpret_cast<void *>(evalProg2),
    reinterpret_cast<void *>(evalProg3)
};

struct StackFrame {
    decltype(std::declval<Indi &>().descendantsStart()) proxy;
    int descendantsCount;
};

template<typename>
struct Tr;
//Tr<decltype(&StackFrame::proxy)> error;

#if 0
void evaluation(Env &e, Indi &i) {
    std::stack<StackFrame> stack;
    auto current = &i;
    auto processNode = [&](auto recurrence) {
        auto node = current->node();
        switch(node) {
            case TurnRight:
                e.turnRight();
                break;
            case TurnLeft:
                e.turnLeft();
                break;
            case Move:
                e.moveForward();
                break;
            case IFA:
                stack.push({current->descendantsStart(), 0});
                recurrence(recurrence);
                break;
            case Prog2:
                stack.push({current->descendantsStart(), 1});
                break;
            case Prog3:
                stack.push({current->descendantsStart(), 2});
                break;
        }
    };
    for(;;) {
        auto node = current->node();
        switch(node) {
            case TurnRight:
                e.turnRight();
                break;
            case TurnLeft:
                e.turnLeft();
                break;
            case Move:
                e.moveForward();
                break;
            case IFA:
                if(e.foodAhead()) {
                    stack.push({current->descendantsStart(), 0});
                    goto processTop;
                }
                continue; // nothing to do
            case Prog2:
                stack.push({current->descendantsStart(), 1});
                break;
            case Prog3:
                stack.push({current->descendantsStart(), 2});
                break;
        }
        if(stack.empty() || e.atEnd()) { return; }
        
        if(0 == stack.top().descendantsCount) {
            do {
                stack.pop();
                if(stack.empty()) { return; }
            } while(0 == stack.top().descendantsCount);
        }
        {
            auto &top = stack.top();
            --top.descendantsCount;
            top.proxy = top.proxy->next(top.proxy);
        }
        processTop:
        current = &*stack.top().proxy;
    }
}
#endif

void eee(Env &e, Indi &i, void *recursion) {
    auto rf = reinterpret_cast<void(*)(Env &, Indi &, void *)>(recursion);
    auto node = i.node();
    switch(node) {
        case TurnRight:
            e.turnRight();
            break;
        case TurnLeft:
            e.turnLeft();
            break;
        case Move:
            e.moveForward();
            break;
        case IFA:
            if(e.foodAhead()) {
                rf(e, *i.descendantsStart(), recursion);
            }
            break;
        case Prog2: {
            auto p = i.descendantsStart();
            rf(e, *p, recursion);
            p = i.next(p);
            rf(e, *p, recursion);
        } break;
        case Prog3: {
            auto p = i.descendantsStart();
            rf(e, *p, recursion);
            p = i.next(p);
            rf(e, *p, recursion);
            rf(e, *i.next(p), recursion);
        } break;
    }
}

void artificialAntEvaluation(Env &e, Indi &i) {
    eee(e, i, reinterpret_cast<void *>(eee));
}
