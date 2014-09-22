#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>

#include "../graph.h"
#include "../searchTree.h"
#include "../loading.h"
#include "../listSpanningTrees.h"
#include "../upperBoundsForSpanningTrees.h"
#include "../subtreeIsomorphism.h"
#include "../graphPrinting.h"
#include "../treeCenter.h"
#include "../connectedComponents.h"
#include "../cs_Tree.h"
#include "../wilsonsAlgorithm.h"

char DEBUG_INFO = 1;

/**
 * Print --help message
 */
void printHelp() {
	printf("This is The TreePatternKernel\n");
	printf("implemented by Pascal Welke 2014\n\n\n");
	printf("usage: tpk F [parameterList]\n\n");
	printf("    without parameters: display this help screen\n\n");
	printf("    F: (required) use F as graph database\n\n");
	printf("    -bound d: choose maximum number of spanning trees\n"
		   "        of a graph for which you are willing to compute\n"
		   "        the kernel (default 1000)\n\n");
	printf("    -k v: number of spanning trees you want to sample\n");
	printf("    -output O: write output to stdout (default p)\n"
		   "        w: for all graphs with less than bound spts\n"
		   "            sample k spts using Wilsons algorithm\n"
		   "        l: for all graphs with less than bound spts\n"
		   "            sample k spts using explicit enumeration\n"
		   "        p print the spanning tree patterns of all graphs with\n"
		   "            less than filter spanning trees\n");
	printf("    -limit N: process the first N graphs in F (default: process all)\n");
	printf("    -min M process graphs starting from Mth instance (default 0)\n\n");
	printf("    -h | --help: display this help\n\n");
}


/**
 * Input handling, parsing of database and call of opk feature extraction method.
 */
int main(int argc, char** argv) {
	if ((argc < 2) || (strcmp(argv[1], "--help") == 0) || (strcmp(argv[1], "-h") == 0)) {
		printHelp();
		return EXIT_FAILURE;
	} else {

		/* create object pools */
		struct ListPool *lp = createListPool(10000);
		struct VertexPool *vp = createVertexPool(10000);
		struct ShallowGraphPool *sgp = createShallowGraphPool(1000, lp);
		struct GraphPool *gp = createGraphPool(100, vp, lp);

		/* pointer to the current graph which is returned by the input iterator */
		struct Graph* g = NULL;

		/* user input handling variables */
		char outputOption = 0;
		int param;
		long int depth = 1000;
		int k = 1;

		/* i counts the number of graphs read */
		int i = 0;

		/* graph delimiter */
		int maxGraphs = -1;
		int minGraph = 0;

		long int avgTrees = 0;

		/* user input handling */
		for (param=2; param<argc; param+=2) {
			char known = 0;
			if ((strcmp(argv[param], "--help") == 0) || (strcmp(argv[param], "-h") == 0)) {
				printHelp();
				return EXIT_SUCCESS;
			}
			if (strcmp(argv[param], "-limit") == 0) {
				sscanf(argv[param+1], "%i", &maxGraphs);
				known = 1;
			}
			if (strcmp(argv[param], "-output") == 0) {
				outputOption = argv[param+1][0];
				known = 1;
			}
			if (strcmp(argv[param], "-bound") == 0) {
				sscanf(argv[param+1], "%li", &depth);
				if (depth < 1) {
					depth = LONG_MAX / 2;
				}
				known = 1;
			}
			if (strcmp(argv[param], "-k") == 0) {
				sscanf(argv[param+1], "%i", &k);
				known = 1;
			}
			if (strcmp(argv[param], "-min") == 0) {
				sscanf(argv[param+1], "%i", &minGraph);
				known = 1;
			}
			if (!known) {
				fprintf(stderr, "Unknown parameter: %s\n",argv[param]);
				return(EXIT_FAILURE);
			}
		}

		if (outputOption == 0) {
			outputOption = 'p';
		}

		/* try to load a file */
		createFileIterator(argv[1], gp);

		/* iterate over all graphs in the database */
		while (((i < maxGraphs) || (maxGraphs == -1)) && (g = iterateFile(&aids99VertexLabel, &aids99EdgeLabel))) {
		
			/* if there was an error reading some graph the returned n will be -1 */
			if (g->n > 0) {
				if (i >= minGraph) {
					switch (outputOption) {
						case 'w':
						if (isConnected(g)) {
							/* getGoodEstimate returns an upper bound on the number of spanning
							trees in g, or -1 if there was an overflow of long ints while computing */
							struct Vertex* searchTree = getVertex(gp->vertexPool);
							long upperBound = getGoodEstimate(g, sgp, gp);
							if ((upperBound < depth) && (upperBound != -1)) {
								int j;
								for (j=0; j<k; ++j) {
									struct ShallowGraph* treeEdges = randomSpanningTreeAsShallowGraph(g, sgp);
									struct Graph* tree = shallowGraphToGraph(treeEdges, gp);

									/* assumes that tree is a tree */
									struct ShallowGraph* cString = canonicalStringOfTree(tree, sgp);
									addToSearchTree(searchTree, cString, gp, sgp);

									/* garbage collection */
									dumpShallowGraphCycle(sgp, treeEdges);
									dumpGraph(gp, tree);
								}
							}
							/* output tree patterns represented as canonical strings */
							printf("# %i %i\n", g->number, searchTree->d);
							printStringsInSearchTree(searchTree, stdout, sgp);
							fflush(stdout);

							avgTrees += searchTree->number;
							dumpSearchTree(gp, searchTree);
						}
						break;

						case 'l':
						if (isConnected(g)) {
							/* getGoodEstimate returns an upper bound on the number of spanning
							trees in g, or -1 if there was an overflow of long ints while computing */
							struct Vertex* searchTree = getVertex(gp->vertexPool);
							long upperBound = getGoodEstimate(g, sgp, gp);
							if ((upperBound < depth) && (upperBound != -1)) {
								struct ShallowGraph* trees = listSpanningTrees(g, sgp, gp);
								struct ShallowGraph* idx;

								int j = 0;
								struct ShallowGraph** array;
								int nTrees;

								/* find number of listed trees, put them in array */
								for (idx=trees; idx; idx=idx->next) ++j;
								nTrees = j;
								array = malloc(nTrees * sizeof(struct ShallowGraph*));
								j = 0;
								for (idx=trees; idx; idx=idx->next) {	
									array[j] = idx;
									++j;
								}

								/* sample k trees uniformly at random */
								for (j=0; j<nTrees; ++j) {
									struct Graph* tree;
									struct ShallowGraph* cString;
									int rnd = rand() % nTrees;

									tree = shallowGraphToGraph(array[rnd], gp);

									/* assumes that tree is a tree */
									cString = canonicalStringOfTree(tree, sgp);
									addToSearchTree(searchTree, cString, gp, sgp);

									/* garbage collection */
									dumpGraph(gp, tree);
								}
								dumpShallowGraphCycle(sgp, trees);
							}

							/* output tree patterns represented as canonical strings */
							printf("# %i %i\n", g->number, searchTree->d);
							printStringsInSearchTree(searchTree, stdout, sgp);
							fflush(stdout);

							avgTrees += searchTree->number;
							dumpSearchTree(gp, searchTree);
						}
						break;

						case 'p':
						if (isConnected(g)) {
							/* getGoodEstimate returns an upper bound on the number of spanning
							trees in g, or -1 if there was an overflow of long ints while computing */
							struct Vertex* searchTree = getVertex(gp->vertexPool);
							long upperBound = getGoodEstimate(g, sgp, gp);
							if ((upperBound < depth) && (upperBound != -1)) {
								struct ShallowGraph* trees = listSpanningTrees(g, sgp, gp);
								struct ShallowGraph* idx;
								for (idx=trees; idx; idx=idx->next) {	
									struct Graph* tree = shallowGraphToGraph(idx, gp);

									/* assumes that tree is a tree */
									struct ShallowGraph* cString = canonicalStringOfTree(tree, sgp);
									addToSearchTree(searchTree, cString, gp, sgp);

									/* garbage collection */
									dumpGraph(gp, tree);
								}
								dumpShallowGraphCycle(sgp, trees);
							} else {
								/* if there are more than depth spanning trees, we sample depth of them uniformly at random */
								int j;
								for (j=0; j<depth; ++j) {
									struct ShallowGraph* treeEdges = randomSpanningTreeAsShallowGraph(g, sgp);
									struct Graph* tree = shallowGraphToGraph(treeEdges, gp);

									/* assumes that tree is a tree */
									struct ShallowGraph* cString = canonicalStringOfTree(tree, sgp);
									addToSearchTree(searchTree, cString, gp, sgp);

									/* garbage collection */
									dumpShallowGraphCycle(sgp, treeEdges);
									dumpGraph(gp, tree);
								}
							}
							/* output tree patterns represented as canonical strings */
							printf("# %i %i\n", g->number, searchTree->d);
							printStringsInSearchTree(searchTree, stdout, sgp);
							fflush(stdout);

							avgTrees += searchTree->number;
							dumpSearchTree(gp, searchTree);
						}
						break;
					}
				}		

				/***** do not alter ****/

				++i;
				/* garbage collection */
				dumpGraph(gp, g);

			} else {
				/* TODO should be handled by dumpgraph */
				free(g);
			}
		}

		fprintf(stderr, "avgTrees = %f\n", avgTrees / (double)i);

		/* global garbage collection */
		destroyFileIterator();
		freeGraphPool(gp);
		freeShallowGraphPool(sgp);
		freeListPool(lp);
		freeVertexPool(vp);

		return EXIT_SUCCESS;
	}
}
