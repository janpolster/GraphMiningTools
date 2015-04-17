#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <getopt.h>

#include "../graph.h"
#include "../searchTree.h"
#include "../listSpanningTrees.h"
#include "../upperBoundsForSpanningTrees.h"
#include "../subtreeIsomorphism.h"
#include "../levelwiseTreePatternMining.h"
#include "../bloomFilter.h"
#include "levelwiseTreesetMiningMain.h" 



/**
 * Print --help message
 */
int printHelp() {
	FILE* helpFile = fopen("executables/levelwiseTreesetMiningMainHelp.txt", "r");
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

	/* object pools */
	struct ListPool *lp = NULL;
	struct VertexPool *vp = NULL;
	struct ShallowGraphPool *sgp = NULL;
	struct GraphPool *gp = NULL;

	/* init params to default values*/
	char debugInfo = 0;
	char onlyCountClasses = 0;
	double fraction = 0.3;
	int threshold = 1000;
	int maxPatternSize = 20;
	char* inputFileName = NULL; // mandatory argument

	int minEdgeID = 100;
	int nGraphs = 1000; // arbitrary positive value used for initializing the pruning array. Will be reset to correct value by getVertexAndEdgeHistograms 
	int (*embeddingOperator)(struct ShallowGraph*, struct Graph**, double, int, int, int**, struct Vertex**, struct GraphPool*) = &checkIfSubIsoCompatible;
	
	/* automatically set variables */
	// io
	char* outputPrefix = NULL;
	char countFileName[500];
	char featureFileName[500];
	char patternFileName[500];	
	char suffix[200];
	FILE* featureFile = NULL;
	FILE* countFile = NULL;
	FILE* patternFile = NULL;
	// frequent patterns
	struct Vertex* frequentPatterns = NULL;
	struct Vertex* frequentVertices = NULL;
	struct Vertex* frequentEdges = NULL;
	struct ShallowGraph* extensionEdges = NULL;
	// counter
	int patternSize = NULL;

	/* parse command line arguments */
	int arg;
	const char* validArgs = "hvt:i:p:o:e:c";
	for (arg=getopt(argc, argv, validArgs); arg!=-1; arg=getopt(argc, argv, validArgs)) {
		switch (arg) {
		case 'h':
			printHelp();
			return EXIT_SUCCESS;
		case 't':
			if (sscanf(optarg, "%i", &threshold) != 1) {
				fprintf(stderr, "threshold must be integer, is: %s\n", optarg);
				return EXIT_FAILURE;
			}
			break;
		case 'i':
			if (sscanf(optarg, "%lf", &fraction) != 1) {
				fprintf(stderr, "fraction must be a float, is: %s\n", optarg);
				return EXIT_FAILURE;
			}
			break;
		case 'p':
			if (sscanf(optarg, "%i", &maxPatternSize) != 1) {
				fprintf(stderr, "maxPatternSize must be integer, is: %s\n", optarg);
				return EXIT_FAILURE;
			}
			break;
		case 'v':
			debugInfo = 1;
			break;
		case 'o':
			outputPrefix = optarg;
			break;
		case 'e':
			if (strcmp(optarg, "existence") == 0) {
				embeddingOperator = &checkIfSubIsoCompatible;
				break;
			}
			if (strcmp(optarg, "importance") == 0) {
				embeddingOperator = &checkIfImportantSubIso;
				break;
			}
			fprintf(stderr, "Unknown embedding operator: %s\n", optarg);
			return EXIT_FAILURE;
		case 'c':
			onlyCountClasses = 1;
			break;
		case '?':
			return EXIT_FAILURE;
			break;
		}
	}	

	/* get the input file name */
	if (optind < argc) {
		inputFileName = argv[optind];
		outputPrefix = !outputPrefix ? inputFileName : outputPrefix;
	} else {
		fprintf(stderr, "No input file name given.\n");
		return EXIT_FAILURE;
	}

	/* init object pools */
	lp = createListPool(10000);
	vp = createVertexPool(10000);
	sgp = createShallowGraphPool(1000, lp);
	gp = createGraphPool(100, vp, lp);

	if (onlyCountClasses) {		
		FILE* stream = fopen(inputFileName, "r");
		int bufferSize = 20;
		int number = 0;
		int count = 0;
		struct ShallowGraph* patterns;

		while ((patterns = streamReadPatternsAndTheirNumber(stream, bufferSize, &number, &count, sgp))) {
			printf("%i %i\n", number, count);
			dumpShallowGraphCycle(sgp, patterns);	
		}

		fclose(stream);
		freeGraphPool(gp);
		freeShallowGraphPool(sgp);
		freeListPool(lp);
		freeVertexPool(vp);
		return EXIT_SUCCESS;
	}


	/* open output files */
	sprintf(suffix, "_ps%i_f%i.counts", maxPatternSize, threshold);
	strcpy(countFileName, outputPrefix);
	strcat(countFileName, suffix);
	countFile = fopen(countFileName, "w");
		
	sprintf(suffix, "_ps%i_f%i.features", maxPatternSize, threshold);
	strcpy(featureFileName, outputPrefix);
	strcat(featureFileName, suffix);
	featureFile = fopen(featureFileName, "w");
	
	sprintf(suffix, "_ps%i_f%i.patterns", maxPatternSize, threshold);
	strcpy(patternFileName, outputPrefix);
	strcat(patternFileName, suffix);
	patternFile = fopen(patternFileName, "w");

	initPruning(nGraphs);

	/* find frequent single vertices and frequent edges */
	/* set lowest id of any edge pattern to a number large enough to don't have collisions */
	frequentVertices = getVertex(vp);
	frequentEdges = getVertex(vp);
	frequentEdges->lowPoint = minEdgeID;
	nGraphs = getVertexAndEdgeHistograms(inputFileName, frequentVertices, frequentEdges, countFile, gp, sgp);
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
