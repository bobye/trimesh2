# trimesh2++

A C++ style adaption of trimesh2 (branching from version 2.11)

Please see newest version of original [trimesh2](http://gfx.cs.princeton.edu/proj/trimesh2/)

## ChangeLog
 * Read COFF format in TriMesh::read()
 * Add new class TetMesh
 * Add tight packed data block for vertices and normals
 * Add TriMesh::faceareas, TriMesh::edgelengths
 * Fix a bug in XForm<T>::read()
 * Fix a bug in computing TriMesh::cornelangle()
 * add Makedefs using Intel Compiler
 * name scope restriction: access classes by trimesh::
