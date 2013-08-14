#ifndef SEARCH_TREE_H_
#define SEARCH_TREE_H_

/**
 * Used to store results of mergeSearchTree
 */
struct compInfo{
	int id;
	int count;
	int depth;
};
struct compInfo* getResultVector(int n);
int compInfoComparison(const void* e1, const void* e2);

struct Vertex* buildSearchTree(struct ShallowGraph* strings, struct GraphPool* gp, struct ShallowGraphPool* sgp);
char addStringToSearchTree(struct Vertex* root, struct VertexList* edge, struct GraphPool* p);
void dumpSearchTree(struct GraphPool* p, struct Vertex* root);
void printSearchTree(struct Vertex* root, int level);
void printStringsInSearchTree(struct Vertex* root, FILE* stream, struct ShallowGraphPool* sgp);
int streamBuildSearchTree(FILE* stream, struct Vertex* root, int bufferSize, struct GraphPool* gp, struct ShallowGraphPool* sgp);
char containsString(struct Vertex* root, struct ShallowGraph* string);


struct Vertex* addToSearchTree(struct Vertex* root, struct ShallowGraph* strings, struct GraphPool* gp, struct ShallowGraphPool* sgp);
void mergeSearchTrees(struct Vertex* globalTree, struct Vertex* localTree, int divisor, struct compInfo* results, int* pos, struct Vertex* trueRoot, int depth, struct GraphPool* p);
void shallowMergeSearchTrees(struct Vertex* globalTree, struct Vertex* localTree, int divisor, struct compInfo* results, int* pos, struct Vertex* trueRoot, int depth, struct GraphPool* p);
void resetToUnique(struct Vertex* root);

#endif