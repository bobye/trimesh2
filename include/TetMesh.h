#ifndef _TETMESH_H_
#define _TETMESH_H_

#include "Vec.h"
#include "Box.h"

#include <vector>
#include "TriMesh.h"
namespace trimesh {
  

class TetMesh {
 public:

  struct Element {
    int v[4];
    
    Element() {}
    Element(const int &v0, const int &v1, const int &v2, const int &v3)
    { v[0] = v0; v[1] = v1; v[2] = v2; v[3] = v3;}
    Element(const int *v_)
    { v[0] = v_[0]; v[1] = v_[1]; v[2] = v_[2]; v[3] = v_[3];}

    int &operator[] (int i) { return v[i]; }
    operator const int * () const { return &(v[0]); }
    operator const int * () { return &(v[0]); }
    operator int * () { return &(v[0]); }
    int indexof(int v_) const
    {
      return (v[0] == v_) ? 0 :
	(v[1] == v_) ? 1 :
	(v[2] == v_) ? 2 :
	(v[3] == v_) ? 3 : -1;
    }

  };

  //
  // Constructor 
  //
  TetMesh() {};

  //
  // Member
  //

  // nodes and elements of tetrahedron
  std::vector<point> nodes;
  std::vector<Element> elements;

  // element/tetrahedron volume
  std::vector<float> tetravolumes;
  std::vector<float> pointvolumes;
  // facetarea[i][j]: the j-th facet of the i-th elements
  std::vector< std::vector<float> > facetareas;



  std::vector< std::vector<int> > neighbors;
  std::vector< std::vector<int> > adjacentelements;
  // std::vector<int> nodes_on_surface; // Usage: surface.vertices[nodes_on_surface[i]]
  TriMesh surface;



  void need_neighbors();
  void need_adjacentelements();
  void need_tetravolumes();
  void need_pointvolumes();
  void need_facetareas();


  bool write(const char *filename);

  //
  // Input and Output
  //
 protected:  
  static bool read_helper(const char *filename, TetMesh* mesh);
 public:
  static TetMesh *read(const char *filename);

  void need_elements()
  {
    if (!elements.empty())
      return;
  }

  float dihedral(int i, int a, int b) {// element i
    int c=0, d, ia,ib,ic,id;
    bool k[4];
    k[a] = k[b] = true;
    while (k[c]) ++c;
    d = c+1;
    while (k[d]) ++d;    

    ia = elements[i][a];
    ib = elements[i][b];
    ic = elements[i][c];
    id = elements[i][d];

    vec n_acd = (nodes[ia] - nodes[ic]) CROSS (nodes[ia] - nodes[id]); normalize(n_acd);
    vec n_bcd = (nodes[ib] - nodes[id]) CROSS (nodes[ib] - nodes[id]); normalize(n_bcd);

    return std::acos(n_acd DOT n_bcd);
  }

};
}

#endif /* _TETMESH_H_ */








