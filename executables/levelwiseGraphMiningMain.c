#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../lwm_embeddingOperators.h"
#include "../levelwiseGraphMining.h"
#include "levelwiseGraphMiningMain.h"

const char DEBUG_INFO = 1;


/**
 * Print --help message
 */
int printHelp() {
#include "levelwiseGraphMiningHelp.help"
	unsigned char* help = executables_levelwiseGraphMiningHelp_txt;
	int len = executables_levelwiseGraphMiningHelp_txt_len;
	if (help != NULL) {
		int i=0;
		for (i=0; i<len; ++i) {
			fputc(help[i], stdout);
		}
		return EXIT_SUCCESS;
	} else {
		fprintf(stderr, "Could not read help file\n");
		return EXIT_FAILURE;
	}
}


/**
 * Input handling, parsing of database and call of opk feature extraction method.
 */
int main(int argc, char** argv) {

	/* object pools */
	struct ListPool *lp;
	struct VertexPool *vp;
	struct ShallowGraphPool *sgp;
	struct GraphPool *gp;

	/* user input handling variables */
	int threshold = 1000;
	unsigned int maxPatternSize = 20;

	// init random generator according to current time
	srand(time(NULL));

	// initializator for the mining
	size_t (*initMining)(size_t, double, struct Vertex**, struct SubtreeIsoDataStoreList**, struct ShallowGraph**, void**, FILE*, FILE*, FILE*, struct GraphPool*, struct ShallowGraphPool*) = &initBFSBase;

	// mining strategy
	void (*miningStrategy)(size_t, size_t, size_t, struct Vertex*, struct SubtreeIsoDataStoreList*, struct ShallowGraph*, struct SubtreeIsoDataStore (*)(struct SubtreeIsoDataStore, struct Graph*, double, struct GraphPool*, struct ShallowGraphPool*), double, FILE*, FILE*, FILE*, struct GraphPool*, struct ShallowGraphPool*) = &iterativeBFSMain;

	// garbage collector after mining
	void (*garbageCollector)(void** y, struct GraphPool* gp, struct ShallowGraphPool* sgp) = &garbageCollectBFSBase;

	// embedding operator
	struct SubtreeIsoDataStore (*embeddingOperator)(struct SubtreeIsoDataStore, struct Graph*, double, struct GraphPool*, struct ShallowGraphPool*) = &subtreeCheckOperator;

	// other
	double importance = 0.5;
	char* patternFile = NULL;

	/* parse command line arguments */
	int arg;
	int seed;
	const char* validArgs = "ht:p:m:o:e:i:r:";
	for (arg=getopt(argc, argv, validArgs); arg!=-1; arg=getopt(argc, argv, validArgs)) {
		switch (arg) {
		case 'h':
			printHelp();
			return EXIT_SUCCESS;
			break;
		case 't':
			if (sscanf(optarg, "%i", &threshold) != 1) {
				fprintf(stderr, "value must be integer, is: %s\n", optarg);
				return EXIT_FAILURE;
			}
			break;
		case 'r':
			if (sscanf(optarg, "%i", &seed) != 1) {
				fprintf(stderr, "value must be integer, is: %s\n", optarg);
				return EXIT_FAILURE;
			} else {
				srand(seed);
			}
			break;
		case 'p':
			if (sscanf(optarg, "%u", &maxPatternSize) != 1) {
				fprintf(stderr, "value must be integer, is: %s\n", optarg);
				return EXIT_FAILURE;
			}
			break;
		case 'm':
//			if (strcmp(optarg, "dfs") == 0) {
//				miningStrategy = &iterativeDFSMain;
//				break;
//			}
			if (strcmp(optarg, "bfs") == 0) {
				miningStrategy = &iterativeBFSMain;
				break;
			}
			fprintf(stderr, "Unknown mining technique: %s\n", optarg);
			return EXIT_FAILURE;
		case 'e':
			// operators for forest transaction databases
			if (strcmp(optarg, "subtree") == 0) {
				initMining = &initIterativeBFSForForestDB;
				embeddingOperator = &subtreeCheckOperator;
				garbageCollector = &garbageCollectIterativeBFSForForestDB;
				break;
			}
			if (strcmp(optarg, "iterative") == 0) {
				initMining = &initIterativeBFSForForestDB;
				embeddingOperator = &iterativeSubtreeCheckOperator;
				garbageCollector = &garbageCollectIterativeBFSForForestDB;
				break;
			}
			if (strcmp(optarg, "relImp") == 0) {
				initMining = &initIterativeBFSForForestDB;
				embeddingOperator = &relativeImportanceOperator;
				garbageCollector = &garbageCollectIterativeBFSForForestDB;
				break;
			}
			if (strcmp(optarg, "absImp") == 0) {
				initMining = &initIterativeBFSForForestDB;
				embeddingOperator = &absoluteImportanceOperator;
				garbageCollector = &garbageCollectIterativeBFSForForestDB;
				break;
			}
			if (strcmp(optarg, "andOr") == 0) {
				initMining = &initIterativeBFSForForestDB;
				embeddingOperator = &andorEmbeddingOperator;
				garbageCollector = &garbageCollectIterativeBFSForForestDB;
				break;
			}
			// operators for arbitrary graph transaction databases
			if (strcmp(optarg, "exactGlobalTreeEnumeration") == 0) {
				initMining = &initIterativeBFSForAllGlobalTreeEnumerationExactMining;
				embeddingOperator = &subtreeCheckOperator;
				if ((int)importance <= 0) {
					importance = 1;
				}
				garbageCollector = &garbageCollectIterativeBFSForForestDB;
				break;
			}
			if (strcmp(optarg, "probabilisticTreeSampling") == 0) {
				initMining = &initIterativeBFSForSampledProbabilisticTree;
				embeddingOperator = &subtreeCheckOperator;
				if ((int)importance <= 0) {
					importance = 1;
				}
				garbageCollector = &garbageCollectIterativeBFSForForestDB;
				break;
			}
			if (strcmp(optarg, "localEasy") == 0) {
				initMining = &initIterativeBFSForExactLocalEasy;
				embeddingOperator = &localEasySubtreeCheckOperator;
				garbageCollector = &garbageCollectIterativeBFSForLocalEasy;
				break;
			}
			if (strcmp(optarg, "localEasySampling") == 0) {
				initMining = &initIterativeBFSForSampledLocalEasy;
				embeddingOperator = &localEasySubtreeCheckOperator;
				if ((int)importance <= 0) {
					importance = 1;
				}
				garbageCollector = &garbageCollectIterativeBFSForLocalEasy;
				break;
			}
			if (strcmp(optarg, "localEasySamplingSlow") == 0) {
				initMining = &initIterativeBFSForSampledLocalEasySlow;
				embeddingOperator = &noniterativeLocalEasySamplingSubtreeCheckOperatorIndependent;
				if ((int)importance <= 0) {
					importance = 1;
				}
				garbageCollector = &garbageCollectIterativeBFSForLocalEasy;
				break;
			}
			if (strcmp(optarg, "localEasySamplingIndependent") == 0) {
				initMining = &initIterativeBFSForSampledLocalEasySlow;
				embeddingOperator = &noniterativeLocalEasySamplingSubtreeCheckOperatorIndependent;
				if ((int)importance <= 0) {
					importance = 1;
				}
				garbageCollector = &garbageCollectIterativeBFSForLocalEasy;
				break;
			}
			if (strcmp(optarg, "localEasyResampling") == 0) {
				initMining = &initIterativeBFSForSampledLocalEasy;
				embeddingOperator = &localEasySubtreeCheckOperatorWithResampling;
				if ((int)importance <= 0) {
					importance = 1;
				}
				garbageCollector = &garbageCollectIterativeBFSForLocalEasy;
				break;
			}
			fprintf(stderr, "Unknown embedding operator: %s\n", optarg);
			return EXIT_FAILURE;
		case 'i':
			if (sscanf(optarg, "%lf", &importance) != 1) {
				fprintf(stderr, "value must be float, is: %s\n", optarg);
				return EXIT_FAILURE;
			}
			if (importance <= 0) {
				fprintf(stderr, "value must be larger than zero but is %lf\n", importance);
			}
			break;
		case 'o':
			patternFile = optarg;
			break;
		case '?':
			return EXIT_FAILURE;
			break;
		}
	}

	/* init object pools */
	lp = createListPool(10000);
	vp = createVertexPool(10000);
	sgp = createShallowGraphPool(1000, lp);
	gp = createGraphPool(100, vp, lp);

	/* initialize the stream to read graphs from
   check if there is a filename present in the command line arguments
   if so, open the file, if not, read from stdin */
	if (optind < argc) {
		char* filename = argv[optind];
		/* if the present filename is not '-' then init a file iterator for that file name */
		if (strcmp(filename, "-") != 0) {
			createFileIterator(filename, gp);
		} else {
			createStdinIterator(gp);
		}
	} else {
		createStdinIterator(gp);
	}

	// start frequent subgraph mining

	FILE* featureStream = stdout;
	FILE* patternStream = stdout;
	FILE* logStream = stderr;

	if (patternFile != NULL) {
		patternStream = fopen(patternFile, "w");
		fprintf(logStream, "Write patterns to file: %s\n", patternFile);
	}

	struct Vertex* initialFrequentPatterns = NULL;
	struct SubtreeIsoDataStoreList* supportSets = NULL;
	struct ShallowGraph* extensionEdgeList = NULL;
	void* dataStructures = NULL;

	size_t initialPatternSize = initMining(threshold, importance,
				//output
				&initialFrequentPatterns,
				&supportSets,
				&extensionEdgeList,
				&dataStructures,
				// printing
				featureStream,
				patternStream,
				logStream,
				// pools
				gp,
				sgp);

	miningStrategy(initialPatternSize, maxPatternSize, threshold, initialFrequentPatterns, supportSets, extensionEdgeList, embeddingOperator, importance,
			//printing
			featureStream, patternStream, logStream,
			//pools
			gp, sgp);

	garbageCollector(dataStructures, gp, sgp);

	destroyFileIterator(); // graphs are in memory now

	if (patternFile != NULL) {
		fclose(patternStream);
	}

	/* global garbage collection */
	freeGraphPool(gp);
	freeShallowGraphPool(sgp);
	freeListPool(lp);
	freeVertexPool(vp);

	return EXIT_SUCCESS;
}
