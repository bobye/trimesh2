#include "TriMesh.h"
#include "TetMesh.h"
#include <algorithm>

namespace trimesh {
  // Find the direct neighbors of each vertex
#define dprintf TriMesh::dprintf
#define eprintf TriMesh::eprintf

void TetMesh::need_neighbors()
{
	if (!neighbors.empty())
		return;

	need_elements();

	dprintf("Finding vertex neighbors... ");
	int nn = nodes.size(), ne = elements.size();

	std::vector<int> numneighbors(nn);
	for (int i = 0; i < ne; i++) {
		numneighbors[elements[i][0]]++;
		numneighbors[elements[i][1]]++;
		numneighbors[elements[i][2]]++;
		numneighbors[elements[i][3]]++;
	}

	neighbors.resize(nn);
	for (int i = 0; i < nn; i++)
	  neighbors[i].reserve(numneighbors[i]+3); // Slop for boundaries

	for (int i = 0; i < ne; i++) {
	  for (int j = 0; j < 4; j++) {
	    std::vector<int> &me = neighbors[elements[i][j]];
	    int n1 = elements[i][(j+1)%4];
	    int n2 = elements[i][(j+2)%4];
	    int n3 = elements[i][(j+3)%4];
	    if (find(me.begin(), me.end(), n1) == me.end())
	      me.push_back(n1);
	    if (find(me.begin(), me.end(), n2) == me.end())
	      me.push_back(n2);
	    if (find(me.begin(), me.end(), n3) == me.end())
	      me.push_back(n3);
	  }
	}

	dprintf("Done.\n");
}

// Find the faces touching each vertex
void TetMesh::need_adjacentelements()
{
	if (!adjacentelements.empty())
		return;

	need_elements();

	dprintf("Finding vertex to triangle maps... ");
	int nn = nodes.size(), ne = elements.size();

	std::vector<int> numadjacentelements(nn);
	for (int i = 0; i < ne; i++) {
		numadjacentelements[elements[i][0]]++;
		numadjacentelements[elements[i][1]]++;
		numadjacentelements[elements[i][2]]++;
	}

	adjacentelements.resize(nodes.size());
	for (int i = 0; i < nn; i++)
	  adjacentelements[i].reserve(numadjacentelements[i]);
	
	for (int i = 0; i < ne; i++) {
	  for (int j = 0; j < 4; j++)
	    adjacentelements[elements[i][j]].push_back(i);
	}

	dprintf("Done.\n");
}


}
