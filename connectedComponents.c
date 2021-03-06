#include <stdio.h>

#include "graph.h"
#include "connectedComponents.h"

struct ShallowGraph* getComp(struct Vertex* v, int compNumber, struct ShallowGraph* comp, struct ListPool* lp) {
	struct VertexList* e;

	v->lowPoint = compNumber;

	for (e=v->neighborhood; e!=NULL; e=e->next) {
		/* go to the vertex, if it was not visited before */
		if (e->endPoint->lowPoint == -1) {
			getComp(e->endPoint, compNumber, comp, lp);
		} 
		/* add e = (v,w) to comp, if v<w. This is to have only one 
		copy of each edge in comp */
		if (e->startPoint->number < e->endPoint->number) {
			pushEdge(comp, shallowCopyEdge(e, lp));
		}
	}
	return comp;
}

void markComp(struct Vertex* v, int compNumber) {
	struct VertexList* e;

	v->lowPoint = compNumber;

	for (e=v->neighborhood; e!=NULL; e=e->next) {
		if (e->endPoint->lowPoint == -1) {
			markComp(e->endPoint, compNumber);
		} 
	}
}

/* returns a list of shallowgraphs that each contain the edges of a connected
component of the graph (each edge is contained only once as a -> b, where a->number < b->number)
If there is an isolated vertex v in the graph, then there will be a Shallowgraph c with
c->m == 0 and c->data == i->number. */
struct ShallowGraph* getConnectedComponents(struct Graph* g, struct ShallowGraphPool* sgp) {
	int v;
	int i = 0;
	struct ShallowGraph* result = NULL;

	for (v=0; v<g->n; ++v) {
		g->vertices[v]->lowPoint = -1;
	}

	for (v=0; v<g->n; ++v) {
		if (g->vertices[v]->lowPoint == -1) {
			struct ShallowGraph* comp = getComp(g->vertices[v], i, getShallowGraph(sgp), sgp->listPool);
			comp->next = result;
			result = comp;
			++i;
		}
		/* there might be connected components consisting of a single vertex.
		in this case, we store its number in the -> data field of the shallow graph */
		if (g->vertices[v]->neighborhood == NULL) {
			result->data = v;
		}
	}
	return result;
}


/**
Returns a list of vertices who are roots of connected components. 
Result is a shallow graph. each edge->endPoint is pointing to a vertex
belonging to a unique connected component. */
struct ShallowGraph* getRepresentativeVertices(struct Graph* g, struct ShallowGraphPool* sgp) {
	int v;
	int i = 0;
	struct ShallowGraph* result = getShallowGraph(sgp);

	for (v=0; v<g->n; ++v) {
		g->vertices[v]->lowPoint = -1;
	}

	for (v=0; v<g->n; ++v) {
		if (g->vertices[v]->lowPoint == -1) {
			struct VertexList* e = getVertexList(sgp->listPool);
			pushEdge(result, e);
			e->endPoint = g->vertices[v];
			markComp(g->vertices[v], i);
			++i;
		}
	}
	return result;
}


char isConnected(struct Graph* g) {
	int v;

	/* the empty graph is connected: for all pairs of vertices (there are none) there is a path :) */
	if (g->n == 0) {
		return 1;
	}

	for (v=0; v<g->n; ++v) {
		g->vertices[v]->lowPoint = -1;
	}

	markComp(g->vertices[0], 0);

	for (v=0; v<g->n; ++v) {
		if (g->vertices[v]->lowPoint == -1) {
			return 0;
		}
	}
	return 1;
}
