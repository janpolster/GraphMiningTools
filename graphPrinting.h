#ifndef GRAPH_PRINTING_H
#define GRAPH_PRINTING_H

void printGraphEdgesOfTwoGraphs(char* name, struct Graph *g, struct Graph* h);
char diffGraphs(char* name, struct Graph *g, struct Graph* h);
void printVertexList(struct VertexList *f);
void printGraphEdges(struct Graph *g);
void printGraph(struct Graph* g);
int printShallowGraphCount(struct ShallowGraph* g, char silent);
void printShallowGraph(struct ShallowGraph* g);


#endif /* GRAPH_PRINTING_H */