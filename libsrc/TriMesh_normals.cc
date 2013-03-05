/*
Szymon Rusinkiewicz
Princeton University

TriMesh_normals.cc
Compute per-vertex normals for TriMeshes

For meshes, uses average of per-face normals, weighted according to:
  Max, N.
  "Weights for Computing Vertex Normals from Facet Normals,"
  Journal of Graphics Tools, Vol. 4, No. 2, 1999.

For raw point clouds, fits plane to k nearest neighbors.
*/

#include "TriMesh.h"
#include "KDtree.h"
#include "lineqn.h"

namespace trimesh {

void TriMesh::allocate_data_tightpacked()
// allocate memory for tightly packed data
{
  int nv = vertices.size();
  vertices_tightpacked = new float[3*nv];

#pragma omp parallel for
  for (int i=0; i<nv; ++i) {
    vertices_tightpacked[3*i]   = vertices[i][0];
    vertices_tightpacked[3*i+1] = vertices[i][1];
    vertices_tightpacked[3*i+2] = vertices[i][2];
  }

  normals_tightpacked  = (float*) malloc( 3*nv*sizeof(float));
  recompute_normals_tightpacked();
}

void TriMesh::recompute_normals_tightpacked()
// recompute normals and stored in tightly packed data
{
	// Nothing to do if we already have normals
	int nv = vertices.size();

	dprintf("Recomputing normals... ");
	for (int i=0; i< 3*nv; ++i) normals_tightpacked[i] = 0.f;
	// TODO: direct handling of grids
	need_faces();
	{
		// Compute from faces
		int nf = faces.size();
		  
#pragma omp parallel for
		for (int i = 0; i < nf; i++) {
		  const int i0 = faces[i][0] * 3;
		  const int i1 = faces[i][1] * 3;
		  const int i2 = faces[i][2] * 3;
		  const float* p0 = vertices_tightpacked + i0;
		  const float* p1 = vertices_tightpacked + i1;
		  const float* p2 = vertices_tightpacked + i2;
		  float a[3] = {p0[0] - p1[0], p0[1] - p1[1], p0[2] - p1[2]};
		  float b[3] = {p1[0] - p2[0], p1[1] - p2[1], p1[2] - p2[2]};
		  float c[3] = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
				    
		  float l2a = std::sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]), l2b = std::sqrt(b[0]*b[0] + b[1]*b[1] + b[2]*b[2]), l2c = std::sqrt(c[0]*c[0] + c[1]*c[1] + c[2]*c[2]);
		  
		  if (!l2a || !l2b || !l2c)
		    continue;
		  float facenormal[3] = {a[1]*b[2] - a[2]*b[1], a[2]*b[0] - a[0]*b[2], a[0]*b[1] - a[1]*b[0]};
		  normals_tightpacked[i0    ] += facenormal[0] * (1.0f / (l2a * l2c));
		  normals_tightpacked[i0 + 1] += facenormal[1] * (1.0f / (l2a * l2c));
		  normals_tightpacked[i0 + 2] += facenormal[2] * (1.0f / (l2a * l2c));
		  
		  normals_tightpacked[i1    ] += facenormal[0] * (1.0f / (l2b * l2a));
		  normals_tightpacked[i1 + 1] += facenormal[1] * (1.0f / (l2b * l2a));
		  normals_tightpacked[i1 + 2] += facenormal[2] * (1.0f / (l2b * l2a));
		  
		  normals_tightpacked[i2    ] += facenormal[0] * (1.0f / (l2c * l2b));
		  normals_tightpacked[i2 + 1] += facenormal[1] * (1.0f / (l2c * l2b));
		  normals_tightpacked[i2 + 2] += facenormal[2] * (1.0f / (l2c * l2b));
		}
	} 

	// Make them all unit-length
#pragma omp parallel for
        for (int i = 0; i < 3*nv; i+=3) {
	  float n = std::sqrt(normals_tightpacked[i]*normals_tightpacked[i] + normals_tightpacked[i+1]*normals_tightpacked[i+1] + normals_tightpacked[i+2]*normals_tightpacked[i+2]);
	  normals_tightpacked[i  ] /=n;
	  normals_tightpacked[i+1] /=n;
	  normals_tightpacked[i+2] /=n;
	}
	dprintf("Done.\n");
}
// Compute per-vertex normals
void TriMesh::need_normals()
{
	// Nothing to do if we already have normals
	int nv = vertices.size();
	if (int(normals.size()) == nv)
		return;

	dprintf("Computing normals... ");
	normals.clear();
	normals.resize(nv);

	// TODO: direct handling of grids
	if (!tstrips.empty()) {
		// Compute from tstrips
		const int *t = &tstrips[0], *end = t + tstrips.size();
		while (likely(t < end)) {
			int striplen = *t - 2;
			t += 3;
			bool flip = false;
			for (int i = 0; i < striplen; i++, t++, flip = !flip) {
				const point &p0 = vertices[*(t-2)];
				const point &p1 = vertices[*(t-1)];
				const point &p2 = vertices[* t   ];
				vec a = p0-p1, b = p1-p2, c = p2-p0;
				float l2a = len2(a), l2b = len2(b), l2c = len2(c);
				if (!l2a || !l2b || !l2c)
					continue;
				vec facenormal = flip ? (b CROSS a) : (a CROSS b);
				normals[*(t-2)] += facenormal * (1.0f / (l2a * l2c));
				normals[*(t-1)] += facenormal * (1.0f / (l2b * l2a));
				normals[* t   ] += facenormal * (1.0f / (l2c * l2b));
			}
		}
	} else if (need_faces(), !faces.empty()) {
		// Compute from faces
		int nf = faces.size();
#pragma omp parallel for
		for (int i = 0; i < nf; i++) {
			const point &p0 = vertices[faces[i][0]];
			const point &p1 = vertices[faces[i][1]];
			const point &p2 = vertices[faces[i][2]];
			vec a = p0-p1, b = p1-p2, c = p2-p0;
			float l2a = len2(a), l2b = len2(b), l2c = len2(c);
			if (!l2a || !l2b || !l2c)
				continue;
			vec facenormal = a CROSS b;
			normals[faces[i][0]] += facenormal * (1.0f / (l2a * l2c));
			normals[faces[i][1]] += facenormal * (1.0f / (l2b * l2a));
			normals[faces[i][2]] += facenormal * (1.0f / (l2c * l2b));
		}
	} else {
		// Find normals of a point cloud
		const int k = 6;
		const vec ref(0, 0, 1);
		KDtree kd(vertices);
#pragma omp parallel for
		for (int i = 0; i < nv; i++) {
			std::vector<const float *> knn;
			kd.find_k_closest_to_pt(knn, k, vertices[i]);
			int actual_k = knn.size();
			if (actual_k < 3) {
				dprintf("Warning: not enough points for vertex %d\n", i);
				normals[i] = ref;
				continue;
			}
			// Compute covariance
			float C[3][3] = { {0,0,0}, {0,0,0}, {0,0,0} };
			// The below loop starts at 1, since element 0     
			// is just vertices[i] itself 
			for (int j = 1; j < actual_k; j++) {
				vec d = point(knn[j]) - vertices[i];
				for (int l = 0; l < 3; l++)
					for (int m = 0; m < 3; m++)
						C[l][m] += d[l] * d[m];
			}
			float e[3];
			eigdc<float,3>(C, e);
			normals[i] = vec(C[0][0], C[1][0], C[2][0]);
			if ((normals[i] DOT ref) < 0.0f)
				normals[i] = -normals[i];
		}
	}

	// Make them all unit-length
#pragma omp parallel for
	for (int i = 0; i < nv; i++)
		normalize(normals[i]);

	dprintf("Done.\n");
}

} // end namespace trimesh
