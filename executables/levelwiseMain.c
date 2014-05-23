#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "../graph.h"
#include "../searchTree.h"
#include "../listSpanningTrees.h"
#include "../upperBoundsForSpanningTrees.h"
#include "../subtreeIsomorphism.h"
#include "../levelwiseMining.h"
#include "levelwiseMain.h" 



/**
 * Print --help message
 */
void printHelp() {
	printf("This is a Levelwise Algorithm for TreePatterns\n");
	printf("implemented by Pascal Welke 2013\n\n\n");
	printf("usage: lwm F [parameterList]\n\n");
	printf("    without parameters: display this help screen\n\n");
	printf("    F: (required) use F as graph database\n\n");
	printf("    -nGraphs N: maximum number of transaction graphs you\n");
	printf("        want to process (default 10000)\n\n");
	printf("    -frequency T: absolute frequency threshold for frequent\n");
	printf("        subtree patterns (e.g. 32 if a pattern has to occur\n");
	printf("        in at least 32 transactions to be frequent) (default 1000)\n\n");
	printf("    -min M: process graphs starting from Mth instance (default 0)\n\n");
	printf("    -patternSize P: maximum number of edges in frequent patterns\n");
	printf("        (default 20)\n\n");
	printf("    -h | --help: display this help\n\n");
}


/**
 Main method of the TreePatternKernel levelwise pattern generation algorithm.
 It will use a database of spanning trees generated by the preprocessing 
 algorithm accompanying it.
 */
int main(int argc, char** argv) {
	if ((argc < 2) || (strcmp(argv[1], "--help") == 0) || (strcmp(argv[1], "-h") == 0)) {
		printHelp();
		return EXIT_FAILURE;
	} else {

		/* create object pools */
		struct ListPool *lp = createListPool(1);
		struct VertexPool *vp = createVertexPool(1);
		struct ShallowGraphPool *sgp = createShallowGraphPool(1, lp);
		struct GraphPool *gp = createGraphPool(1, vp, lp);

		/* user input handling variables */
		int param;

		/* init params to default values*/
		char debugInfo = 1;
		int minGraph = 0;
		int maxGraph = 10000;
		int threshold = 1000;
		int maxPatternSize = 20;
		int minEdgeID = 100;
		char* inputFileName = argv[1];
		char countFileName[500];
		char featureFileName[500];
		char patternFileName[500];	
		char suffix[200];
		FILE* featureFile;
		FILE* countFile;
		FILE* patternFile;
		struct Vertex* frequentPatterns;
		struct Vertex* frequentVertices = getVertex(vp);
		struct Vertex* frequentEdges = getVertex(vp);
		struct ShallowGraph* extensionEdges;
		int patternSize;

		/* user input handling */
		for (param=2; param<argc; param+=2) {
			if ((strcmp(argv[param], "--help") == 0) || (strcmp(argv[param], "-h") == 0)) {
				printHelp();
				return EXIT_SUCCESS;
			}
			if (strcmp(argv[param], "-nGraphs") == 0) {
				sscanf(argv[param+1], "%i", &maxGraph);
			}
			if (strcmp(argv[param], "-frequency") == 0) {
				sscanf(argv[param+1], "%i", &threshold);
			}
			if (strcmp(argv[param], "-min") == 0) {
				sscanf(argv[param+1], "%i", &minGraph);
			}
			if (strcmp(argv[param], "-patternSize") == 0) {
				sscanf(argv[param+1], "%i", &maxPatternSize);
			}
		}

		/* open output files */
		sprintf(suffix, "_ps%i_f%i.counts", maxPatternSize, threshold);
		strcpy(countFileName, inputFileName);
		strcat(countFileName, suffix);
 		countFile = fopen(countFileName, "w");
 		
 		sprintf(suffix, "_ps%i_f%i.features", maxPatternSize, threshold);
		strcpy(featureFileName, inputFileName);
		strcat(featureFileName, suffix);
		featureFile = fopen(featureFileName, "w");
		
		sprintf(suffix, "_ps%i_f%i.patterns", maxPatternSize, threshold);
		strcpy(patternFileName, inputFileName);
		strcat(patternFileName, suffix);
 		patternFile = fopen(patternFileName, "w");

		initPruning(maxGraph);

		/* find frequent single vertices and frequent edges */
		/* set lowest id of any edge pattern to a number large enough to don't have collisions */
		frequentEdges->lowPoint = minEdgeID;
		getVertexAndEdgeHistogramsP(inputFileName, minGraph, maxGraph, frequentVertices, frequentEdges, countFile, gp, sgp);
		filterSearchTreeP(frequentVertices, threshold, frequentVertices, featureFile, gp);
		filterSearchTreeP(frequentEdges, threshold, frequentEdges, featureFile, gp);


		/* print first two levels to patternfile */
		fprintf(patternFile, "patterns size 0\n");
		printStringsInSearchTree(frequentVertices, patternFile, sgp); 
		fprintf(patternFile, "patterns size 1\n");
		printStringsInSearchTree(frequentEdges, patternFile, sgp); 
		if (debugInfo) { fprintf(stderr, "Computation of level 0 and 1 done\n"); }

		/* convert frequentEdges to ShallowGraph */
		extensionEdges = edgeSearchTree2ShallowGraph(frequentEdges, gp, sgp);	

		for (frequentPatterns = frequentEdges, patternSize = 2; (frequentPatterns->d > 0) && (patternSize < maxPatternSize); ++patternSize) {
			int i;
			struct ShallowGraph* prefix = getShallowGraph(sgp);
			struct Vertex* candidateSet;
			struct Vertex** pointers;
			struct Graph** refinements;
			int refinementSize;
			
			candidateSet = generateCandidateSet(frequentPatterns, extensionEdges, gp, sgp);
			setLowPoints(candidateSet);
			refinementSize = candidateSet->d;
			pointers = malloc(refinementSize * sizeof(struct Vertex*));
			refinements = malloc(refinementSize * sizeof(struct Graph*));

			makeGraphsAndPointers(candidateSet, candidateSet, refinements, pointers, 0, prefix, gp, sgp); 
			scanDB(inputFileName, candidateSet, refinements, pointers, refinementSize, minGraph, maxGraph, threshold, countFile, gp, sgp);

			/* threshold + 1 as candidateSet contains each candidate once, already */
			filterSearchTreeP(candidateSet, threshold + 1, candidateSet, featureFile, gp);

			fprintf(patternFile, "patterns size %i\n", patternSize);
			printStringsInSearchTree(candidateSet, patternFile, sgp); 

			if (debugInfo) { fprintf(stderr, "Computation of level %i done\n", patternSize); }

			/* garbage collection */
			dumpSearchTree(gp, frequentPatterns);
			dumpShallowGraph(sgp, prefix);
			free(pointers);
			for (i=0; i<refinementSize; ++i) {
				dumpGraph(gp, refinements[i]);
			}
			free(refinements);
			frequentPatterns = candidateSet;

			/* flush the output */
			fflush(featureFile);
			fflush(countFile);
			fflush(patternFile);
		}

		/* garbage collection */
		dumpCube();
		freePruning();
		freeFrequentEdgeShallowGraph(gp, sgp, extensionEdges);
		dumpSearchTree(gp, frequentVertices);
		dumpSearchTree(gp, frequentPatterns);
		fclose(featureFile);
		fclose(countFile);
		fclose(patternFile);

		freeGraphPool(gp);
		freeShallowGraphPool(sgp);
		freeListPool(lp);
		freeVertexPool(vp);

		return EXIT_SUCCESS;
	}
}
