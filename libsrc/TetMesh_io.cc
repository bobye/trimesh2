#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cctype>
#include <cstdarg>
#include "TriMesh.h"
#include "TetMesh.h"
#include "strutil.h"

namespace trimesh {

#define dprintf TriMesh::dprintf
#define eprintf TriMesh::eprintf

  bool read_default(const char* filename, TetMesh* mesh) {
    int n, nothing;

    // read .node
    FILE* f = fopen(filename, "rb");
    if (!f) {
      eprintf("Error opening [%s] for reading: %s.\n", filename,
	      strerror(errno));
      return false;    
    }
    
    fscanf(f, "%d %d %d %d",  &n, &nothing, &nothing, &nothing);
    for (int i=0; i<n; ++i) {
      point p;
      fscanf(f, "%d %f %f %f", &nothing, &p[0], &p[1], &p[2]);
      mesh->nodes.push_back(p);
    }
    fclose(f);

    // read .ele
    f = fopen(replace_ext(filename,"ele").c_str(), "rb");
    if (!f) {
      eprintf("Error opening [%s] for reading: %s.\n", filename,
	      strerror(errno));
      return false;    
    }
    
    fscanf(f, "%d %d %d", &n, &nothing, &nothing);
    for (int i=0; i<n; ++i) {
      TetMesh::Element e;
      fscanf(f, "%d %d %d %d %d", &nothing, &e[0], &e[1], &e[2], &e[3]);
      mesh->elements.push_back(e);
    }
    fclose(f);

    // read .face
    // note for tetrahedron mesh generated from triangle mesh by tetgen
    //   ./tetgen -pq1.2Y test.off
    // the surface nodes are in first inserted to nodes array, and then
    // interior nodes are subsequently inserted thereafter.
    f = fopen(replace_ext(filename,"face").c_str(), "rb");
    if (!f) {
      eprintf("Error opening [%s] for reading: %s.\n", filename,
	      strerror(errno));
      return false;    
    }
    fscanf(f, "%d %d", &n, &nothing);
    //mesh->nodes_on_surface.resize(mesh->nodes.size(), -1);
    int count=0, idx;
    for (int i=0; i<n; ++i) {
      fscanf(f, "%d", &nothing);
      TriMesh::Face thisface;

      for (int j=0; j<3; ++j) {
	fscanf(f, "%d", &idx);
	/*
	if (mesh->nodes_on_surface[idx] == -1) {
	  mesh->nodes_on_surface[idx]=count++;
	  mesh->surface.vertices.push_back(mesh->nodes[idx]);
	}

	thisface[j] = mesh->nodes_on_surface[idx];
	*/
	
	if (idx+1 > count) count = idx +1;
	thisface[j] = idx;
      }

      while (1) {int c = fgetc(f);if (c == EOF || c == '\n') break;}
            
      mesh->surface.faces.push_back(thisface);
    }

    mesh->surface.vertices = std::vector<point> (mesh->nodes.begin(), mesh->nodes.begin() + count);
    fclose(f);
    
    
    return true;
  }


  TetMesh *TetMesh::read(const char* filename)
  {
    TetMesh *mesh = new TetMesh();
    if (read_helper(filename, mesh))
      return mesh;
    delete mesh;
    return NULL;
  }

  bool TetMesh::read_helper(const char* filename, TetMesh* mesh) {
    if (!filename || *filename == '\0')
      return false;
    bool ok = false;

    
    // Read tetgen generated format by default

    // read example.1.node
    if (ends_with(filename, ".node")) {
      ok = read_default(filename, mesh);
      goto out;
    }

out:
	if (!ok || mesh->nodes.empty()) {
		eprintf("Error reading file [%s].\n", filename);
		return false;
	}

	dprintf("Done.\n");

	return true;


  }
}
