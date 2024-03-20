#! /usr/bin/bash

COMPILER=$1

shift

${COMPILER} -D__include_directive__='#include' -E -CC $*
