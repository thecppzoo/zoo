#include "zoo/gp/ArtificialAntEnvironment.h"
#include "zoo/gp/ArtificialAntLanguage.h"

#include "zoo/gp/Individual.h"

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

