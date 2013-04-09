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

  bool read_tetgen(const char* filename, TetMesh* mesh) {
    int n, nothing;

    // read .node
    std::vector<point> nodes_cpy;
    std::vector<TetMesh::Element> elements_cpy;

    FILE* f = fopen(filename, "rb");
    if (!f) {
      eprintf("Error opening [%s] for reading: %s.\n", filename,
	      strerror(errno));
      return false;    
    }
    
    fscanf(f, "%d %d %d %d",  &n, &nothing, &nothing, &nothing);
    dprintf("#nodes: %d\t", n);
    for (int i=0; i<n; ++i) {
      point p;
      fscanf(f, "%d %f %f %f", &nothing, &p[0], &p[1], &p[2]);
      nodes_cpy.push_back(p);
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
    dprintf("#elements: %d\t",n);
    for (int i=0; i<n; ++i) {
      TetMesh::Element e;
      fscanf(f, "%d %d %d %d %d", &nothing, &e[0], &e[1], &e[2], &e[3]);
      elements_cpy.push_back(e);
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
    dprintf("#faces: %d\n",n);
    std::vector<int> nodes_on_surface(nodes_cpy.size(), -1);
    int count=0, idx;
    for (int i=0; i<n; ++i) {
      fscanf(f, "%d", &nothing);
      TriMesh::Face thisface;

      for (int j=0; j<3; ++j) {
	fscanf(f, "%d", &idx);

	if (nodes_on_surface[idx] == -1) {
	  nodes_on_surface[idx]=count++;
	  mesh->surface.vertices.push_back(nodes_cpy[idx]);
	}

	thisface[j] = nodes_on_surface[idx];

	/*	
		if (idx+1 > count) count = idx +1;
		thisface[j] = idx;
	*/
      }

      while (1) {int c = fgetc(f);if (c == EOF || c == '\n') break;}

      //face flip
      std::swap(thisface[0], thisface[2]);
            
      mesh->surface.faces.push_back(thisface);
    }
    dprintf("#nodes on boundary surface: %d\n", count);
    
    for (int i=0; i<nodes_cpy.size(); ++i)
      if (nodes_on_surface[i] == -1) nodes_on_surface[i] = count ++;

    mesh->nodes.resize(nodes_cpy.size());
    for (int i=0; i<nodes_cpy.size(); ++i)
      mesh->nodes[nodes_on_surface[i]] = nodes_cpy[i];

    mesh->elements.resize(elements_cpy.size());
    for (int i=0; i<elements_cpy.size(); ++i) {
      mesh->elements[i][0] = nodes_on_surface[elements_cpy[i][0]];
      mesh->elements[i][1] = nodes_on_surface[elements_cpy[i][1]];
      mesh->elements[i][2] = nodes_on_surface[elements_cpy[i][2]];
      mesh->elements[i][3] = nodes_on_surface[elements_cpy[i][3]];
    }
    //    mesh->surface.vertices = std::vector<point> (mesh->nodes.begin(), mesh->nodes.begin() + count);

    fclose(f);
    
    f = fopen("default_remap.txt","wb");
    for (int i=0; i<nodes_cpy.size(); ++i) 
      fprintf(f, "%d\n", nodes_on_surface[i]);
    fclose(f);

    return true;
  }

  bool write_tetgen(TetMesh* mesh, const char* filename) {
    FILE *f = NULL;

    // write .node
    if (strcmp(filename, "-") == 0) {
      f = stdout;
      filename = "standard output";
    } else {
      f = fopen(filename, "wb");
      if (!f) {
	eprintf("Error opening [%s] for writing: %s.\n", filename,
		strerror(errno));
	return false;
      }
    }


    fprintf(f,"%4d   3   0   0\n", (int) mesh->nodes.size());
    for (int i = 0; i<mesh->nodes.size(); ++i) {
      fprintf(f,"%4d    %.10f    %.10f    %.10f\n", i, 
	      mesh->nodes[i][0], mesh->nodes[i][1], mesh->nodes[i][2]);
    }

    fclose(f);
    
    // write .ele
    f = fopen(replace_ext(filename, "ele").c_str(), "wb");
    fprintf(f,"%4d    4    0\n", (int) mesh->elements.size());
    for (int i = 0; i<mesh->elements.size(); ++i) {
      fprintf(f, "%4d    %4d    %4d    %4d    %4d\n", i, 
	   mesh->elements[i][0], mesh->elements[i][1], mesh->elements[i][2], mesh->elements[i][3]);
    }
    fclose(f);

    // write .face
    f = fopen(replace_ext(filename, "face").c_str(), "wb");
    fprintf(f,"%4d    0\n", (int) mesh->surface.faces.size());
    for (int i = 0; i<mesh->surface.faces.size(); ++i) {
      // remember face flip
      fprintf(f, "%4d    %4d    %4d    %4d\n", i,
	      mesh->surface.faces[i][2], mesh->surface.faces[i][1], mesh->surface.faces[i][0]);
    }
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

    
    // Read tetgen generated format 

    // read example.1.node
    if (ends_with(filename, ".node")) {
      ok = read_tetgen(filename, mesh);
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

  bool TetMesh::write(const char *filename) {
    if (!filename || *filename == '\0') {
      eprintf("Can't write to empty filename.\n");
      return false;
    }

    if (nodes.empty()) {
      eprintf("Empty mesh - nothing to write.\n");
      return false;
    }

    enum { TETGEN } filetype;

    
    if (ends_with(filename, ".node"))
      filetype = TETGEN;


    dprintf("Writing %s... ", filename);

    bool ok = false;
    switch (filetype) {
    case TETGEN:
      ok = write_tetgen(this, filename);
      break;
    default:
      break;
    }

    if (!ok) {
      eprintf("Error writing file [%s].\n", filename);
      return false;
    }

    dprintf("Done.\n");
    return true;

  }
  
}
