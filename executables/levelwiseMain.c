#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "../graph.h"
#include "../searchTree.h"
#include "../listSpanningTrees.h"
#include "../upperBoundsForSpanningTrees.h"
#include "../subtreeIsomorphism.h"
#include "../levelwiseTreePatternMining.h"
#include "levelwiseMain.h" 



/**
 * Print --help message
 */
int printHelp() {
	FILE* helpFile = fopen("executables/levelwiseMainHelp.txt", "r");
	if (helpFile != NULL) {
		int c = EOF;
		while ((c = fgetc(helpFile)) != EOF) {
			fputc(c, stdout);
		}
		fclose(helpFile);
		return EXIT_SUCCESS;
	} else {
		fprintf(stderr, "Could not read helpfile\n");
		return EXIT_FAILURE;
	}
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
		struct ListPool *lp = createListPool(10000);
		struct VertexPool *vp = createVertexPool(10000);
		struct ShallowGraphPool *sgp = createShallowGraphPool(1000, lp);
		struct GraphPool *gp = createGraphPool(100, vp, lp);

		/* user input handling variables */
		int param;

		/* init params to default values*/
		char debugInfo = 1;
		double fraction = 0.3;
		int threshold = 1000;
		int maxPatternSize = 20;
		int minEdgeID = 100;
		int nGraphs = 1000; //TODO replace by autoset size of db
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
		int (*embeddingOperator)(struct ShallowGraph*, struct Graph**, double, int, int, int**, struct Vertex**, struct GraphPool*);

		/* user input handling */
		for (param=2; param<argc; param+=2) {
			char known = 0;
			if ((strcmp(argv[param], "--help") == 0) || (strcmp(argv[param], "-h") == 0)) {
				printHelp();
				return EXIT_SUCCESS;
			}
			if (strcmp(argv[param], "-nGraphs") == 0) {
				sscanf(argv[param+1], "%i", &nGraphs);
				known = 1;
			}
			if (strcmp(argv[param], "-frequency") == 0) {
				sscanf(argv[param+1], "%i", &threshold);
				known = 1;
			}
			if (strcmp(argv[param], "-fraction") == 0) {
				sscanf(argv[param+1], "%lf", &fraction);
				known = 1;
			}
			if (strcmp(argv[param], "-patternSize") == 0) {
				sscanf(argv[param+1], "%i", &maxPatternSize);
				known = 1;
			}
			if (known == 0) {
				fprintf(stderr, "Unknown parameter: %s\n", argv[param]);
				return EXIT_FAILURE;
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

		initPruning(nGraphs);

		/* choose the embedding operator for frequent pattern mining */
		if ((fraction <= 0) || (fraction > 1)) {
			embeddingOperator = &checkIfSubIsoCompatible;
		} else {
			embeddingOperator = &checkIfImportantSubIso;
		}

		/* find frequent single vertices and frequent edges */
		/* set lowest id of any edge pattern to a number large enough to don't have collisions */
		frequentEdges->lowPoint = minEdgeID;
		getVertexAndEdgeHistograms(inputFileName, frequentVertices, frequentEdges, countFile, gp, sgp);
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
			scanDBNoCache(inputFileName, candidateSet, refinements, pointers, refinementSize, threshold, nGraphs, fraction, countFile, gp, sgp, embeddingOperator);

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
