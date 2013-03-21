#include "TetMesh.h"

namespace trimesh {
  void TetMesh::need_tetravolumes() 
  {
    
    int en = elements.size();
    if (tetravolumes.size() == en) return;
    else tetravolumes.resize(en);

#pragma omp parallel for
    for (int i=0; i<en; ++i) {
      vec v1 = nodes[elements[i][1]] - nodes[elements[i][0]],
	v2 = nodes[elements[i][2]] - nodes[elements[i][0]],
	v3 = nodes[elements[i][3]] - nodes[elements[i][0]];

      tetravolumes[i] = std::fabs((v1 CROSS v2) DOT v3)/6.;
    }
  }

  void TetMesh::need_facetareas()
  {
    int en = elements.size();
    if (facetareas.size() == en) return;
    else facetareas.resize(en);


#pragma omp parallel for
    for (int i=0; i<en; ++i) {
      for (int j=0; j<4; ++j) {
	vec v1 = nodes[elements[i][(j+2)%4]] - nodes[elements[i][(j+1)%4]],
	  v2 = nodes[elements[i][(j+3)%4]] - nodes[elements[i][(j+1)%4]];
	vec norm = v1 CROSS v2;
	facetareas[i].push_back(std::fabs(norm DOT norm)/2.);
      }
    }
  }
}
