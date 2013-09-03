#include <malloc.h>
#include "graph.h"
#include "bipartiteMatching.h"

int*** _cube = NULL;
int _cubeX = 0;
int _cubeY = 0;
char _cubeInUse = 0;

/** Utility data structure destructor */
void _freeCube(int*** cube, int x, int y) {
	int i, j;
	for (i=0; i<x; ++i) {
		if (cube[i] != NULL) {
			for (j=0; j<y; ++j) {
				if (cube[i][j] != NULL) {
					free(cube[i][j]);
				}
			}
			free(cube[i]);
		}
	}
	free(cube);
}

void dumpCube() {
	_freeCube(_cube, _cubeX, _cubeY);
}


/** Utility data structure creator.
Cube will store, what is called S in the paper. */
int*** createCube(int x, int y) {
	if (!_cubeInUse) {
		int i, j;
		if ((x > _cubeX) || (y > _cubeY)) {
			if (_cube != NULL) {
				_freeCube(_cube, _cubeX, _cubeY);
			}
			_cubeX = x;
			_cubeY = y;
			if ((_cube = malloc(x * sizeof(int**)))) {
				for (i=0; i<x; ++i) {
					_cube[i] = malloc(y * sizeof(int*));
					if (_cube[i] != NULL) {
						for (j=0; j<y; ++j) {
							_cube[i][j] = NULL;
						}
					} else {
						for (j=0; j<i; ++j) {
							free(_cube[i]);
						}
						free(_cube);
						return NULL;
					}
				}
			} else {
				return NULL;
			}
			_cubeInUse = 1;
			return _cube;
		} else {
			_cubeInUse = 1;
			return _cube;
		}
	} else {
		return NULL;
	}
}



/** Utility data structure destructor */
void freeCube(int*** cube, int x, int y) {
	int i, j;
	for (i=0; i<x; ++i) {
		if (cube[i] != NULL) {
			for (j=0; j<y; ++j) {
				if (cube[i][j] != NULL) {
					free(cube[i][j]);
					cube[i][j] = NULL;
				}
			}
		}
	}
	_cubeInUse = 0;
}



/** Print a single entry in the cube */
void printS(int*** S, int v, int u) {
	int i;
	printf("S(%i, %i)={", v, u);
	if (S[v][u]) {
		for (i=1; i<S[v][u][0]-1; ++i) {
			printf("%i, ", S[v][u][i]);
		}
		printf("%i}\n", S[v][u][S[v][u][0]-1]);
	} else {
		printf("}\n");
	}
	fflush(stdout);
}


/**
 * Print some information about a ShallowGraph
 */
void printStrangeMatching(struct ShallowGraph* g) {
	
	struct ShallowGraph* index = g;
	struct VertexList* e;
	do {
		if (index) {
			printf("matching ");
			for (e=index->edges; e; e=e->next) {
				printf("(%i, %i) ", e->startPoint->lowPoint, e->endPoint->lowPoint);
			}
			printf("\n");
		} else {
			/* if index is NULL, the input pointed to a list and not to a cycle */
			break;
		}
	} while (index != g);
}


/**
Find all leaves of g that are not equal to r.

The method returns an int array leaves. leaves[0] contains the length of
leaves, i.e. number of leaves plus one.
Subsequent positions of leaves contain the vertex numbers of leaves in ascending order. 
The method sets the ->d members of leaf vertices in g to 1 all other to 0.
*/
int* findLeaves(struct Graph* g, int root) {
	int nLeaves = 0;
	int* leaves;
	int v;

	for (v=0; v<g->n; ++v) {
		if (v != root) {
			if (isLeaf(g->vertices[v])) {
				++nLeaves;
				g->vertices[v]->d = 1;
			} else {
				g->vertices[v]->d = 0;
			}	
		}
	}
	leaves = malloc((nLeaves+1) * sizeof(int));
	leaves[0] = nLeaves + 1;
	nLeaves = 0;
	for (v=0; v<g->n; ++v) {
		if (v != root) {
			if (isLeaf(g->vertices[v])) {
				leaves[nLeaves+1] = v;
				++nLeaves;	
			}
		}
	}
	return leaves;
}


int dfs(struct Vertex* v, int value) {
	struct VertexList* e;

	/* to make this method save for graphs that are not trees */
	v->visited = -2;

	for (e=v->neighborhood; e!=NULL; e=e->next) {
		if (e->endPoint->visited == -1) {
			value = 1 + dfs(e->endPoint, value);
		}
	}
	v->visited = value;
	return value;
}


/**
Compute a dfs order or postorder on g starting at vertex root.
The ->visited members of vertices are set to the position they have in the order 
(starting with 0). Vertices that cannot be reached from root get ->visited = -1
The method returns an array of length g->n where position i contains the vertex number 
of the ith vertex in the order. 
*/
int* getPostorder(struct Graph* g, int root) {
	int i;
	int* order = malloc(g->n * sizeof(int));
	for (i=0; i<g->n; ++i) {
		g->vertices[i]->visited = -1;
		order[i] = -1;
	}
	dfs(g->vertices[root], 0);
	for (i=0; i<g->n; ++i) {
		if (g->vertices[i]->visited != -1) {
			order[g->vertices[i]->visited] = i;
		} else {
			/* should never happen if g is a tree */
			fprintf(stderr, "Vertex %i was not visited by dfs.\nThis can not happen, if g is a tree.\n", i);
		}
	}
	return order;
}


/* vertices of g have their ->visited values set to the postorder. Thus, 
children of v are vertices u that are neighbors of v and have u->visited < v->visited */
struct Graph* makeBipartiteInstance(struct Graph* g, int v, struct Graph* h, int u, int*** S, struct GraphPool* gp) {
	struct Graph* B;
	int i, j;

	int sizeofX = degree(h->vertices[u]);
	int sizeofY = degree(g->vertices[v]);
	struct VertexList* e;

 	/* construct bipartite graph B(v,u) */ 
	B = createGraph(sizeofX + sizeofY, gp);
	/* store size of first partitioning set */
	B->number = sizeofX;

	/* add vertex numbers of original vertices to ->lowPoint of each vertex in B */
	i = 0;
	for (e=h->vertices[u]->neighborhood; e!=NULL; e=e->next) {
		B->vertices[i]->lowPoint = e->endPoint->number;
		++i;
	}
	for (e=g->vertices[v]->neighborhood; e!=NULL; e=e->next) {
		B->vertices[i]->lowPoint = e->endPoint->number;
		++i;
	}

	/* add edge (x,y) if u in S(y,x) */
	for (i=0; i<sizeofX; ++i) {
		for (j=sizeofX; j<B->n; ++j) {
			int x = B->vertices[i]->lowPoint;
			int y = B->vertices[j]->lowPoint;
			int k;

			/* y has to be a child of v */
			if (g->vertices[y]->visited < g->vertices[v]->visited) {

				if (S[y][x] != NULL) {
					for (k=1; k<S[y][x][0]; ++k) {
						if (S[y][x][k] == u) {
							addResidualEdges(B->vertices[i], B->vertices[j], gp->listPool);
							++B->m;
						}

					}
				}
			}
		}
	} 

	return B;
}


struct ShallowGraph* removeVertexFromBipartiteInstance(struct Graph* B, int v, struct ShallowGraphPool* sgp) {
	struct ShallowGraph* temp = getShallowGraph(sgp);
	struct VertexList* e;
	struct VertexList* f;
	struct VertexList* g;	
	int w;


	/* mark edges that will be removed */
	for (e=B->vertices[v]->neighborhood; e!=NULL; e=e->next) {
		e->used = 1;
		((struct VertexList*)e->label)->used = 1;
	}

	/* remove edges from v */
	for (e=B->vertices[v]->neighborhood; e!=NULL; e=f) {
		f = e->next;
		appendEdge(temp, e);
	}
	B->vertices[v]->neighborhood = NULL;

	/* remove residual edges */
	for (w=B->number; w<B->n; ++w) {
		f = NULL;
		g = NULL;
		/* partition edges */
		for (e=B->vertices[w]->neighborhood; e!=NULL; e=B->vertices[w]->neighborhood) {
			B->vertices[w]->neighborhood = e->next;
			if (e->used == 1) {
				e->next = f;
				f = e;
			} else {
				e->next = g;
				g = e;
			}
		}
		/* set neighborhood to unused, append used to temp */
		B->vertices[w]->neighborhood = g;
		while (f!=NULL) {
			e = f;
			f = f->next;
			appendEdge(temp, e);
		}
	}
	return temp;
}

void addVertexToBipartiteInstance(struct ShallowGraph* temp) {
	struct VertexList* e;

	for (e=popEdge(temp); e!=NULL; e=popEdge(temp)) {
		e->used = 0;
		addEdge(e->startPoint, e);
	}
}


/**
Labeled Subtree isomorphism check.  

Implements the version of subtree isomorphism algorithm described by

Ron Shamir, Dekel Tsur [1999]: Faster Subtree Isomorphism. Section 2 

in its original unlabeled version.
*/
char subtreeCheck(struct Graph* g, struct Graph* h, struct GraphPool* gp, struct ShallowGraphPool* sgp) {
	/* iterators */
	int u, v;

	struct Vertex* r = g->vertices[0];
	int*** S = createCube(g->n, h->n);
	int* postorder = getPostorder(g, r->number);


	/* init the S(v,u) for v and u leaves */
	int* gLeaves = findLeaves(g, 0);
	/* h is not rooted, thus every vertex with one neighbor is a leaf */
	int* hLeaves = findLeaves(h, -1);
	for (v=1; v<gLeaves[0]; ++v) {
		for (u=1; u<hLeaves[0]; ++u) {
			S[gLeaves[v]][hLeaves[u]] = malloc(2 * sizeof(int));
			/* 'header' of array stores its length */
			S[gLeaves[v]][hLeaves[u]][0] = 2;
			/* the number of the unique neighbor of u in h*/
			S[gLeaves[v]][hLeaves[u]][1] = h->vertices[hLeaves[u]]->neighborhood->endPoint->number;
		}
	}
	/* garbage collection for init */
	free(gLeaves);
	free(hLeaves);
	gLeaves = NULL;
	hLeaves = NULL;

	for (v=0; v<g->n; ++v) {
		struct Vertex* current = g->vertices[postorder[v]];
		int currentDegree = degree(current);
		if (!isLeaf(current) || (current->number == r->number)) {
			for (u=0; u<h->n; ++u) {
				int i;
				int degU = degree(h->vertices[u]);
				if (degU <= currentDegree + 1) {
					struct Graph* B = makeBipartiteInstance(g, postorder[v], h, u, S, gp);
					int* matchings = malloc((degU + 1) * sizeof(int));

					matchings[0] = bipartiteMatchingFastAndDirty(B, gp);

					if (matchings[0] == degU) {
						free(postorder);
						free(matchings);
						freeCube(S, g->n, h->n);
						dumpGraph(gp, B);
						return 1;
					} else {
						matchings[0] = degU + 1;
					}

					for (i=0; i<B->number; ++i) {
						struct ShallowGraph* storage = removeVertexFromBipartiteInstance(B, i, sgp);
						initBipartite(B);
						matchings[i+1] = bipartiteMatchingFastAndDirty(B, gp);

						if (matchings[i+1] == degU - 1) {
							matchings[i+1] = B->vertices[i]->lowPoint;
						} else {
							matchings[i+1] = -1;
						}

						addVertexToBipartiteInstance(storage);
						dumpShallowGraph(sgp, storage);
					}
					S[current->number][u] = matchings;

					dumpGraph(gp, B);
				}
			}		
		}
	}

	/* garbage collection */
	free(postorder);
	freeCube(S, g->n, h->n);

	return 0;
}
