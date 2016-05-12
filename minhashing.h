/*
 * minhashing.h
 *
 *  Created on: May 12, 2016
 *      Author: pascal
 */

#ifndef MINHASHING_H_
#define MINHASHING_H_

#include <stdlib.h>
#include <malloc.h>

#include "graph.h"
#include "searchTree.h"
#include "cs_Tree.h"
#include "cs_Parsing.h"
#include "listComponents.h"

// PERMUTATIONS
int* getRandomPermutation(int n);

// POSET PERMUTATION SHRINK
int posetPermutationMark(int* permutation, int n, struct Graph* F);
int* posetPermutationShrink(int* permutation, int n, int shrunkSize);

// PREPROCESSING
struct Graph* buildTreePosetFromGraphDB(struct Graph** db, int nGraphs, struct GraphPool* gp, struct ShallowGraphPool* sgp);

#endif /* MINHASHING_H_ */