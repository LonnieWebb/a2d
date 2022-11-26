#ifndef A2D_FE_MESH_H
#define A2D_FE_MESH_H

#include <algorithm>
#include <set>

#include "a2dmatops2d.h"
#include "a2dmatops3d.h"
#include "a2dobjs.h"
#include "multiphysics/feelementtypes.h"
#include "sparse/sparse_matrix.h"
#include "sparse/sparse_symbolic.h"

namespace A2D {

/**
 * @brief Mesh connecivity class for 3D meshes composed of tetrahedral,
 * hexahedral, wedge and pyramid elements
 */
class MeshConnectivity3D {
 public:
  static constexpr index_t NO_LABEL = std::numeric_limits<index_t>::max();

  // Use the definitions from the element types
  using ET = ElementTypes;

  template <typename I>
  MeshConnectivity3D(I nverts, I ntets, I* tets, I nhex, I* hex, I nwedge,
                     I* wedge, I npyrmd, I* pyrmd)
      : nverts(nverts),
        ntets(ntets),
        nhex(nhex),
        nwedge(nwedge),
        npyrmd(npyrmd) {
    // Allocate space for the element -> vert connectivity
    tet_verts = new index_t[ET::TET_VERTS * ntets];
    hex_verts = new index_t[ET::HEX_VERTS * nhex];
    wedge_verts = new index_t[ET::WEDGE_VERTS * nwedge];
    pyrmd_verts = new index_t[ET::PYRMD_VERTS * npyrmd];

    // vert -> element connectivity
    vert_element_ptr = NULL;
    vert_elements = NULL;

    // element -> face connectivity
    tet_faces = NULL;
    hex_faces = NULL;
    wedge_faces = NULL;
    pyrmd_faces = NULL;
    tri_face_elements = NULL;
    quad_face_elements = NULL;

    // element -> edge connectivity
    tet_edges = NULL;
    hex_edges = NULL;
    wedge_edges = NULL;
    pyrmd_edges = NULL;

    // Count the total number of elements
    nelems = ntets + nhex + nwedge + npyrmd;

    // Set the connectivity: element -> verts
    for (index_t i = 0; i < ET::TET_VERTS * ntets; i++) {
      tet_verts[i] = tets[i];
    }
    for (index_t i = 0; i < ET::HEX_VERTS * nhex; i++) {
      hex_verts[i] = hex[i];
    }
    for (index_t i = 0; i < ET::WEDGE_VERTS * nwedge; i++) {
      wedge_verts[i] = wedge[i];
    }
    for (index_t i = 0; i < ET::PYRMD_VERTS * npyrmd; i++) {
      pyrmd_verts[i] = pyrmd[i];
    }

    init_vert_element_data();
    init_face_data();
    init_edge_data();
  }
  ~MeshConnectivity3D() {
    if (tet_verts) {
      delete[] tet_verts;
    }
    if (hex_verts) {
      delete[] hex_verts;
    }
    if (wedge_verts) {
      delete[] wedge_verts;
    }
    if (pyrmd_verts) {
      delete[] pyrmd_verts;
    }

    if (vert_element_ptr) {
      delete[] vert_element_ptr;
    }
    if (vert_elements) {
      delete[] vert_elements;
    }

    if (tet_faces) {
      delete[] tet_faces;
    }
    if (hex_faces) {
      delete[] hex_faces;
    }
    if (wedge_faces) {
      delete[] wedge_faces;
    }
    if (pyrmd_faces) {
      delete[] pyrmd_faces;
    }
    if (tri_face_elements) {
      delete[] tri_face_elements;
    }
    if (quad_face_elements) {
      delete[] quad_face_elements;
    }

    if (tet_edges) {
      delete[] tet_edges;
    }
    if (hex_edges) {
      delete[] hex_edges;
    }
    if (wedge_edges) {
      delete[] wedge_edges;
    }
    if (pyrmd_edges) {
      delete[] pyrmd_edges;
    }
  }

  // Get global element counts
  index_t get_num_elements() { return nelems; }
  index_t get_num_faces() { return nfaces; }
  index_t get_num_edges() { return nedges; }
  index_t get_num_verts() { return nverts; }

  /**
   * @brief Get the element faces array
   *
   * @param elem The element index
   * @param faces The face indicies associated with the element
   * @return The number of faces
   */
  index_t get_element_faces(index_t elem, const index_t* faces[]) {
    if (elem < ntets) {
      *faces = &tet_faces[ET::TET_FACES * elem];
      return ET::TET_FACES;
    } else if (elem < ntets + nhex) {
      elem = elem - ntets;
      *faces = &hex_faces[ET::HEX_FACES * elem];
      return ET::HEX_FACES;
    } else if (elem < ntets + nhex + nwedge) {
      elem = elem - ntets - nhex;
      *faces = &wedge_faces[ET::WEDGE_FACES * elem];
      return ET::WEDGE_FACES;
    } else if (elem < ntets + nhex + nwedge + npyrmd) {
      elem = elem - ntets - nhex - nwedge;
      *faces = &pyrmd_faces[ET::PYRMD_FACES * elem];
      return ET::PYRMD_FACES;
    }
    *faces = NULL;
    return 0;
  }

  /**
   * @brief Get the element verts
   *
   * @param elem The element index
   * @param verts The vertex indices
   * @return The number of vertices
   */
  index_t get_element_verts(index_t elem, const index_t* verts[]) {
    if (elem < ntets) {
      *verts = &tet_verts[ET::TET_VERTS * elem];
      return ET::TET_VERTS;
    } else if (elem < ntets + nhex) {
      elem = elem - ntets;
      *verts = &hex_verts[ET::HEX_VERTS * elem];
      return ET::HEX_VERTS;
    } else if (elem < ntets + nhex + nwedge) {
      elem = elem - ntets - nhex;
      *verts = &wedge_verts[ET::WEDGE_VERTS * elem];
      return ET::WEDGE_VERTS;
    } else if (elem < ntets + nhex + nwedge + npyrmd) {
      elem = elem - ntets - nhex - nwedge;
      *verts = &pyrmd_verts[ET::PYRMD_VERTS * elem];
      return ET::PYRMD_VERTS;
    }
    *verts = NULL;
    return 0;
  }

  /**
   * @brief Get the adjacent elements from the vert index
   *
   * @param vert The index of the vertex
   * @param elems Array of the adjacent element indices
   * @return The number of elements
   */
  index_t get_adjacent_elements_from_vert(const index_t vert,
                                          const index_t* elems[]) {
    if (vert < nverts) {
      *elems = &vert_elements[vert_element_ptr[vert]];
      return vert_element_ptr[vert + 1] - vert_element_ptr[vert];
    }
    elems = NULL;
    return 0;
  }

  /**
   * @brief Get the indices of the face in its local order
   *
   * @param elem The element index
   * @param f The local face index
   * @param verts The vertices
   * @return The number of vertices
   */
  index_t get_element_face_verts(index_t elem, index_t f, index_t verts[]) {
    if (elem < ntets) {
      verts[1] = tet_verts[ET::TET_VERTS * elem + ET::TET_FACE_VERTS[f][1]];
      verts[2] = tet_verts[ET::TET_VERTS * elem + ET::TET_FACE_VERTS[f][2]];
      verts[0] = tet_verts[ET::TET_VERTS * elem + ET::TET_FACE_VERTS[f][0]];
      return 3;
    } else if (elem < ntets + nhex) {
      elem = elem - ntets;
      verts[0] = hex_verts[ET::HEX_VERTS * elem + ET::HEX_FACE_VERTS[f][0]];
      verts[1] = hex_verts[ET::HEX_VERTS * elem + ET::HEX_FACE_VERTS[f][1]];
      verts[2] = hex_verts[ET::HEX_VERTS * elem + ET::HEX_FACE_VERTS[f][2]];
      verts[3] = hex_verts[ET::HEX_VERTS * elem + ET::HEX_FACE_VERTS[f][3]];
      return 4;
    } else if (elem < ntets + nhex + nwedge) {
      elem = elem - ntets - nhex;

      if (f < ET::WEDGE_TRI_FACES) {
        verts[0] = wedge_verts[ET::WEDGE_VERTS * elem +
                               ET::WEDGE_TRI_FACE_VERTS[f][0]];
        verts[1] = wedge_verts[ET::WEDGE_VERTS * elem +
                               ET::WEDGE_TRI_FACE_VERTS[f][1]];
        verts[2] = wedge_verts[ET::WEDGE_VERTS * elem +
                               ET::WEDGE_TRI_FACE_VERTS[f][2]];
        return 3;
      } else {
        verts[0] =
            wedge_verts[ET::WEDGE_VERTS * elem +
                        ET::WEDGE_QUAD_FACE_VERTS[f - ET::WEDGE_TRI_FACES][0]];
        verts[1] =
            wedge_verts[ET::WEDGE_VERTS * elem +
                        ET::WEDGE_QUAD_FACE_VERTS[f - ET::WEDGE_TRI_FACES][1]];
        verts[2] =
            wedge_verts[ET::WEDGE_VERTS * elem +
                        ET::WEDGE_QUAD_FACE_VERTS[f - ET::WEDGE_TRI_FACES][2]];
        verts[3] =
            wedge_verts[ET::WEDGE_VERTS * elem +
                        ET::WEDGE_QUAD_FACE_VERTS[f - ET::WEDGE_TRI_FACES][3]];
        return 4;
      }
    } else if (elem < ntets + nhex + nwedge + npyrmd) {
      elem = elem - ntets - nhex - nwedge;

      if (f < ET::PYRMD_TRI_FACES) {
        verts[0] = pyrmd_verts[ET::PYRMD_VERTS * elem +
                               ET::PYRMD_TRI_FACE_VERTS[f][0]];
        verts[1] = pyrmd_verts[ET::PYRMD_VERTS * elem +
                               ET::PYRMD_TRI_FACE_VERTS[f][1]];
        verts[2] = pyrmd_verts[ET::PYRMD_VERTS * elem +
                               ET::PYRMD_TRI_FACE_VERTS[f][2]];
        return 3;
      } else {
        verts[0] =
            pyrmd_verts[ET::PYRMD_VERTS * elem + ET::PYRMD_QUAD_FACE_VERTS[0]];
        verts[1] =
            pyrmd_verts[ET::PYRMD_VERTS * elem + ET::PYRMD_QUAD_FACE_VERTS[1]];
        verts[2] =
            pyrmd_verts[ET::PYRMD_VERTS * elem + ET::PYRMD_QUAD_FACE_VERTS[2]];
        verts[3] =
            pyrmd_verts[ET::PYRMD_VERTS * elem + ET::PYRMD_QUAD_FACE_VERTS[3]];
        return 4;
      }
    }
    return 0;
  }

  /**
   * @brief Get the face element vertices in a sorted order
   *
   * This call will return the vertices in the same order regardless of the
   * orientation of the face. This is used to compare faces.
   *
   * @param elem The element index
   * @param f The local face index
   * @param verts The vertices
   * @return The number of vertices
   */
  index_t get_element_global_face_verts(index_t elem, index_t local_face,
                                        index_t verts[]) {
    index_t t[4];
    index_t n = get_element_face_verts(elem, local_face, t);

    if (n == 3) {
      // Find the smallest vertex index
      if (t[0] < t[1] && t[0] < t[2]) {  // t[0] is the smallest
        if (t[1] < t[2]) {
          verts[0] = t[0];
          verts[1] = t[1];
          verts[2] = t[2];
        } else {  // Reverse the order
          verts[0] = t[0];
          verts[1] = t[2];
          verts[2] = t[1];
        }
      } else if (t[1] < t[0] && t[1] < t[2]) {  // t[1] is the smallest
        if (t[2] < t[0]) {
          verts[0] = t[1];
          verts[1] = t[2];
          verts[2] = t[0];
        } else {  // Reverse the order
          verts[0] = t[1];
          verts[1] = t[0];
          verts[2] = t[2];
        }
      } else {  // t[2] is the smallest
        if (t[0] < t[1]) {
          verts[0] = t[2];
          verts[1] = t[0];
          verts[2] = t[1];
        } else {  // Reverse the order
          verts[0] = t[2];
          verts[1] = t[1];
          verts[2] = t[0];
        }
      }

      return 3;
    } else {                                            // n == 4
      if (t[0] < t[1] && t[0] < t[2] && t[0] < t[3]) {  // t[0] is smallest
        if (t[1] < t[3]) {
          verts[0] = t[0];
          verts[1] = t[1];
          verts[2] = t[2];
          verts[3] = t[3];
        } else {
          verts[0] = t[0];
          verts[1] = t[3];
          verts[2] = t[2];
          verts[3] = t[1];
        }
      } else if (t[1] < t[0] && t[1] < t[2] &&
                 t[1] < t[3]) {  // t[1] is smallest
        if (t[2] < t[0]) {
          verts[0] = t[1];
          verts[1] = t[2];
          verts[2] = t[3];
          verts[3] = t[0];
        } else {
          verts[0] = t[1];
          verts[1] = t[0];
          verts[2] = t[3];
          verts[3] = t[2];
        }
      } else if (t[2] < t[0] && t[2] < t[1] &&
                 t[2] < t[3]) {  // t[2] is smallest
        if (t[3] < t[1]) {
          verts[0] = t[2];
          verts[1] = t[3];
          verts[2] = t[0];
          verts[3] = t[1];
        } else {
          verts[0] = t[2];
          verts[1] = t[1];
          verts[2] = t[0];
          verts[3] = t[3];
        }
      } else {  // t[3] is smallest
        if (t[0] < t[2]) {
          verts[0] = t[3];
          verts[1] = t[0];
          verts[2] = t[1];
          verts[3] = t[2];
        } else {
          verts[0] = t[3];
          verts[1] = t[2];
          verts[2] = t[1];
          verts[3] = t[0];
        }
      }

      return 4;
    }
  }

  /**
   * @brief Check for equality between two faces
   *
   * @return True if the faces match, false otherwise
   */
  bool global_face_equality(index_t na, const index_t a[], index_t nb,
                            const index_t b[]) {
    if (na == nb) {
      if (na == 3) {
        if (a[0] == b[0] && a[1] == b[1] && a[2] == b[2]) {
          return true;
        }
      } else {
        if (a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3]) {
          return true;
        }
      }
    }
    return false;
  }

  /**
   * @brief Check for equality between two edges
   *
   * @return True if the edges match, false otherwise
   */
  bool global_edge_equality(const index_t a[], const index_t b[]) {
    if ((a[0] == b[0] && a[1] == b[1]) || (a[0] == b[1] && a[1] == b[0])) {
      return true;
    }
    return false;
  }

  /**
   * @brief Get the face elements associated with the given face
   *
   * @param face Global face index
   * @param e1 Returned value of the first element
   * @param e2 Returned value of the second element (if it exists)
   * @return Boolean if this face is on the boundary
   */
  bool get_face_elements(index_t face, index_t* e1, index_t* e2) {
    if (face < ntri_faces) {
      *e1 = tri_face_elements[2 * face];
      *e2 = tri_face_elements[2 * face + 1];
      return (tri_face_elements[2 * face + 1] == NO_LABEL);
    } else {
      face = face - ntri_faces;
      *e1 = quad_face_elements[2 * face];
      *e2 = quad_face_elements[2 * face + 1];
      return (quad_face_elements[2 * face + 1] == NO_LABEL);
    }
  }

  /**
   * @brief Get the element edges
   *
   * @param elem The element index
   * @param edges The edge indices
   * @return The number of edges for this element
   */
  index_t get_element_edges(index_t elem, const index_t* edges[]) {
    if (elem < ntets) {
      *edges = &tet_edges[ET::TET_EDGES * elem];
      return ET::TET_EDGES;
    } else if (elem < ntets + nhex) {
      elem = elem - ntets;
      *edges = &hex_edges[ET::HEX_EDGES * elem];
      return ET::HEX_EDGES;
    } else if (elem < ntets + nhex + nwedge) {
      elem = elem - ntets - nhex;
      *edges = &wedge_edges[ET::WEDGE_EDGES * elem];
      return ET::WEDGE_EDGES;
    } else if (elem < ntets + nhex + nwedge + npyrmd) {
      elem = elem - ntets - nhex - nwedge;
      *edges = &pyrmd_edges[ET::PYRMD_EDGES * elem];
      return ET::PYRMD_EDGES;
    }
    return 0;
  }

  /**
   * @brief Get the vertices of the associated element edge
   *
   * This returns the edge oriented by the local direction
   *
   * @param elem The element index
   * @param edge The local element edge index
   * @param verts The global vertices, ordered in
   */
  void get_element_edge_verts(index_t elem, index_t edge, index_t verts[]) {
    if (elem < ntets) {
      verts[0] = tet_verts[ET::TET_VERTS * elem + ET::TET_EDGE_VERTS[edge][0]];
      verts[1] = tet_verts[ET::TET_VERTS * elem + ET::TET_EDGE_VERTS[edge][1]];
    } else if (elem < ntets + nhex) {
      elem = elem - ntets;
      verts[0] = hex_verts[ET::HEX_VERTS * elem + ET::HEX_EDGE_VERTS[edge][0]];
      verts[1] = hex_verts[ET::HEX_VERTS * elem + ET::HEX_EDGE_VERTS[edge][1]];
    } else if (elem < ntets + nhex + nwedge) {
      elem = elem - ntets - nhex;
      verts[0] =
          wedge_verts[ET::WEDGE_VERTS * elem + ET::WEDGE_EDGE_VERTS[edge][0]];
      verts[1] =
          wedge_verts[ET::WEDGE_VERTS * elem + ET::WEDGE_EDGE_VERTS[edge][1]];
    } else if (elem < ntets + nhex + nwedge + npyrmd) {
      elem = elem - ntets - nhex - nwedge;
      verts[0] =
          pyrmd_verts[ET::PYRMD_VERTS * elem + ET::PYRMD_EDGE_VERTS[edge][0]];
      verts[1] =
          pyrmd_verts[ET::PYRMD_VERTS * elem + ET::PYRMD_EDGE_VERTS[edge][1]];
    }
  }

  /**
   * @brief Label the verts, edges and faces that touch the list of vertices
   *
   * @param nv The number of input vertices
   * @param verts The vertex numbers
   * @param vert_labels An array of length nverts
   * @param edge_labels An array of length nedges
   * @param face_labels An array of length nfaces
   */
  template <typename IdxType>
  void get_labels_from_verts(const index_t nv, const IdxType verts[],
                             index_t vert_labels[], index_t edge_labels[],
                             index_t face_labels[]) {
    std::fill(vert_labels, vert_labels + nverts, NO_LABEL);
    std::fill(edge_labels, edge_labels + nedges, NO_LABEL);
    std::fill(face_labels, face_labels + nfaces, NO_LABEL);

    // Label the vertices
    for (index_t i = 0; i < nv; i++) {
      if (verts[i] < nverts) {
        vert_labels[verts[i]] = i;
      }
    }

    // Loop over elements and label the edges and faces
    for (index_t elem = 0; elem < nelems; elem++) {
      // Check whether the edges are labeled
      const index_t* elem_edges;
      index_t ne = get_element_edges(elem, &elem_edges);
      for (index_t e = 0; e < ne; e++) {
        index_t v[2];
        get_element_edge_verts(elem, e, v);

        // Both verts of this element edge are labeled - label the edge
        if (vert_labels[v[0]] != NO_LABEL && vert_labels[v[1]] != NO_LABEL) {
          edge_labels[elem_edges[e]] = elem_edges[e];
        }
      }

      const index_t* elem_faces;
      index_t nf = get_element_faces(elem, &elem_faces);
      for (index_t f = 0; f < nf; f++) {
        index_t fv[4];  // Face vertices
        index_t nfv = get_element_face_verts(elem, f, fv);

        // Check if all the face vertices are labeled
        if (nfv == 3) {
          if (vert_labels[fv[0]] != NO_LABEL &&
              vert_labels[fv[1]] != NO_LABEL &&
              vert_labels[fv[2]] != NO_LABEL) {
            face_labels[elem_faces[f]] = elem_faces[f];
          }
        } else if (nfv == 4) {
          if (vert_labels[fv[0]] != NO_LABEL &&
              vert_labels[fv[1]] != NO_LABEL &&
              vert_labels[fv[2]] != NO_LABEL &&
              vert_labels[fv[3]] != NO_LABEL) {
            face_labels[elem_faces[f]] = elem_faces[f];
          }
        }
      }
    }
  }

 private:
  /**
   * @brief Initialize the data for the connection between vertices and
   * the elements
   *
   * Given a vertex index, this enables retrieval of all elements that
   * reference that vertex.
   */
  void init_vert_element_data() {
    // Construct vert -> element data
    vert_element_ptr = new index_t[nverts + 1];
    std::fill(vert_element_ptr, vert_element_ptr + nverts + 1, 0);

    for (index_t elem = 0; elem < nelems; elem++) {
      const index_t* verts;
      index_t nv = get_element_verts(elem, &verts);
      for (index_t i = 0; i < nv; i++) {
        vert_element_ptr[verts[i] + 1]++;
      }
    }

    for (index_t i = 0; i < nverts; i++) {
      vert_element_ptr[i + 1] += vert_element_ptr[i];
    }

    vert_elements = new index_t[vert_element_ptr[nverts]];

    for (index_t elem = 0; elem < nelems; elem++) {
      const index_t* verts;
      index_t nv = get_element_verts(elem, &verts);
      for (index_t i = 0; i < nv; i++) {
        vert_elements[vert_element_ptr[verts[i]]] = elem;
        vert_element_ptr[verts[i]]++;
      }
    }

    for (index_t i = nverts; i > 0; i--) {
      vert_element_ptr[i] = vert_element_ptr[i - 1];
    }
    vert_element_ptr[0] = 0;
  }

  /**
   * @brief Get a non-constant element faces array
   *
   * @param elem The element index
   * @param faces The face indicies associated with the element
   * @return The number of faces
   */
  index_t get_element_faces(index_t elem, index_t* faces[]) {
    if (elem < ntets) {
      *faces = &tet_faces[ET::TET_FACES * elem];
      return ET::TET_FACES;
    } else if (elem < ntets + nhex) {
      elem = elem - ntets;
      *faces = &hex_faces[ET::HEX_FACES * elem];
      return ET::HEX_FACES;
    } else if (elem < ntets + nhex + nwedge) {
      elem = elem - ntets - nhex;
      *faces = &wedge_faces[ET::WEDGE_FACES * elem];
      return ET::WEDGE_FACES;
    } else if (elem < ntets + nhex + nwedge + npyrmd) {
      elem = elem - ntets - nhex - nwedge;
      *faces = &pyrmd_faces[ET::PYRMD_FACES * elem];
      return ET::PYRMD_FACES;
    }
    *faces = NULL;
    return 0;
  }

  /**
   * @brief Get the element edges
   *
   * @param elem The element index
   * @param edges The edge indices
   * @return The number of edges for this element
   */
  index_t get_element_edges(index_t elem, index_t* edges[]) {
    if (elem < ntets) {
      *edges = &tet_edges[ET::TET_EDGES * elem];
      return ET::TET_EDGES;
    } else if (elem < ntets + nhex) {
      elem = elem - ntets;
      *edges = &hex_edges[ET::HEX_EDGES * elem];
      return ET::HEX_EDGES;
    } else if (elem < ntets + nhex + nwedge) {
      elem = elem - ntets - nhex;
      *edges = &wedge_edges[ET::WEDGE_EDGES * elem];
      return ET::WEDGE_EDGES;
    } else if (elem < ntets + nhex + nwedge + npyrmd) {
      elem = elem - ntets - nhex - nwedge;
      *edges = &pyrmd_edges[ET::PYRMD_EDGES * elem];
      return ET::PYRMD_EDGES;
    }
    return 0;
  }

  /**
   * @brief Initialize data associated with the face information
   *
   * This code uniquely orders the faces assocaited with each element
   */
  void init_face_data() {
    if (tet_faces) {
      delete[] tet_faces;
    }
    if (hex_faces) {
      delete[] hex_faces;
    }
    if (wedge_faces) {
      delete[] wedge_faces;
    }
    if (pyrmd_faces) {
      delete[] pyrmd_faces;
    }

    // Allocate the face data
    tet_faces = new index_t[ET::TET_FACES * ntets];
    hex_faces = new index_t[ET::HEX_FACES * nhex];
    wedge_faces = new index_t[ET::WEDGE_FACES * nwedge];
    pyrmd_faces = new index_t[ET::PYRMD_FACES * npyrmd];

    // Fill the face arrays with NO_LABEL
    std::fill(tet_faces, tet_faces + ET::TET_FACES * ntets, NO_LABEL);
    std::fill(hex_faces, hex_faces + ET::HEX_FACES * nhex, NO_LABEL);
    std::fill(wedge_faces, wedge_faces + ET::WEDGE_FACES * nwedge, NO_LABEL);
    std::fill(pyrmd_faces, pyrmd_faces + ET::PYRMD_FACES * npyrmd, NO_LABEL);

    // Prepare to count and number the number of faces. This keeps track of
    // separate triangle and quadrilateral face counts
    ntri_faces = 0;
    nquad_faces = 0;
    nfaces = 0;

    for (index_t elem = 0; elem < nelems; elem++) {
      // Loop over the elements of this face
      index_t* faces;
      const index_t nf = get_element_faces(elem, &faces);

      for (index_t face = 0; face < nf; face++) {
        if (faces[face] == NO_LABEL) {
          // Get the unique set of verts corresponding to this face
          index_t face_verts[4];
          index_t nface_verts =
              get_element_global_face_verts(elem, face, face_verts);

          bool face_located = false;

          // For all adjacent elements check if there is a match
          for (index_t m = 0; m < nface_verts; m++) {
            const index_t* adj_elems;
            const index_t nadj_elems =
                get_adjacent_elements_from_vert(face_verts[m], &adj_elems);

            // Loop over all adjacent elements that are not the current
            // element
            for (index_t k = 0; k < nadj_elems; k++) {
              if (adj_elems[k] != elem) {
                // Loop over the elements of this face
                index_t* adj_faces;
                const index_t adj_nf =
                    get_element_faces(adj_elems[k], &adj_faces);

                for (index_t adj_face = 0; adj_face < adj_nf; adj_face++) {
                  // Get the unique set of vertices corresponding to this
                  // face
                  index_t adj_face_verts[4];
                  index_t adj_nface_verts = get_element_global_face_verts(
                      adj_elems[k], adj_face, adj_face_verts);

                  if (adj_faces[adj_face] == NO_LABEL &&
                      global_face_equality(nface_verts, face_verts,
                                           adj_nface_verts, adj_face_verts)) {
                    if (nface_verts == 3) {
                      adj_faces[adj_face] = ntri_faces;
                      faces[face] = ntri_faces;
                      ntri_faces++;
                    } else {
                      adj_faces[adj_face] = nquad_faces;
                      faces[face] = nquad_faces;
                      nquad_faces++;
                    }

                    face_located = true;
                    break;
                  }
                }

                if (face_located) {
                  break;
                }
              }
            }

            if (face_located) {
              break;
            }
          }

          // No adjacent face was found. This is a boundary face
          if (!face_located) {
            if (nface_verts == 3) {
              faces[face] = ntri_faces;
              ntri_faces++;
            } else {
              faces[face] = nquad_faces;
              nquad_faces++;
            }

            // Add this face to the boundary face list???
          }
        }
      }
    }

    // Sum up the total number of faces
    nfaces = ntri_faces + nquad_faces;

    // Set the face indices associated with the two adjacent elements. At
    // this point the face indices stored are with respect to separate lists
    // of triangular and quadrilateral faces. We order the triangular faces
    // first, so we have to add the total number of triangular faces to each
    // quadrilateral face index to get the global index.
    tri_face_elements = new index_t[2 * ntri_faces];
    std::fill(tri_face_elements, tri_face_elements + 2 * ntri_faces, NO_LABEL);

    quad_face_elements = new index_t[2 * nquad_faces];
    std::fill(quad_face_elements, quad_face_elements + 2 * nquad_faces,
              NO_LABEL);

    for (index_t elem = 0; elem < nelems; elem++) {
      // Loop over the elements of this face
      index_t* faces;
      const index_t nf = get_element_faces(elem, &faces);

      for (index_t face = 0; face < nf; face++) {
        index_t face_verts[4];
        index_t nface_verts =
            get_element_global_face_verts(elem, face, face_verts);

        if (nface_verts == 3) {
          if (tri_face_elements[2 * faces[face]] == NO_LABEL) {
            tri_face_elements[2 * faces[face]] = elem;
          } else if (tri_face_elements[2 * faces[face] + 1] == NO_LABEL) {
            tri_face_elements[2 * faces[face] + 1] = elem;
          }
        } else {
          if (quad_face_elements[2 * faces[face]] == NO_LABEL) {
            quad_face_elements[2 * faces[face]] = elem;
          } else if (quad_face_elements[2 * faces[face] + 1] == NO_LABEL) {
            quad_face_elements[2 * faces[face] + 1] = elem;
          }

          // Reset the face index into the global face index - add the
          // number of triangle faces. Now the faces will index from a
          // global face number.
          faces[face] += ntri_faces;
        }
      }
    }
  }

  /**
   * @brief Initialize and order the edge information.
   *
   * This relies on the face connectivity data - so that must be initialized
   * first.
   */
  void init_edge_data() {
    nedges = 0;

    if (tet_edges) {
      delete[] tet_edges;
    }
    if (hex_edges) {
      delete[] hex_edges;
    }
    if (wedge_edges) {
      delete[] wedge_edges;
    }
    if (pyrmd_edges) {
      delete[] pyrmd_edges;
    }

    tet_edges = new index_t[ET::TET_EDGES * ntets];
    hex_edges = new index_t[ET::HEX_EDGES * nhex];
    wedge_edges = new index_t[ET::WEDGE_EDGES * nwedge];
    pyrmd_edges = new index_t[ET::PYRMD_EDGES * npyrmd];

    // Fill the face arrays with NO_LABEL
    std::fill(tet_edges, tet_edges + ET::TET_EDGES * ntets, NO_LABEL);
    std::fill(hex_edges, hex_edges + ET::HEX_EDGES * nhex, NO_LABEL);
    std::fill(wedge_edges, wedge_edges + ET::WEDGE_EDGES * nwedge, NO_LABEL);
    std::fill(pyrmd_edges, pyrmd_edges + ET::PYRMD_EDGES * npyrmd, NO_LABEL);

    for (index_t elem = 0; elem < nelems; elem++) {
      // Get the number of element edges
      index_t* elem_edges;
      index_t ne = get_element_edges(elem, &elem_edges);

      for (index_t e0 = 0; e0 < ne; e0++) {
        if (elem_edges[e0] == NO_LABEL) {
          index_t edge_num = nedges;
          nedges++;

          elem_edges[e0] = edge_num;

          // Find adjacent elements with this edge
          index_t v0[2];
          get_element_edge_verts(elem, e0, v0);

          // Loop over all adjacent elements to find touching edges
          const index_t* adj_elems;
          const index_t nadj_elems =
              get_adjacent_elements_from_vert(v0[0], &adj_elems);

          // Loop back over the elements and edges and set the global
          //   numbering
          for (index_t i = 0; i < nadj_elems; i++) {
            if (adj_elems[i] != elem) {
              index_t* adj_elem_edges;
              index_t adj_ne = get_element_edges(adj_elems[i], &adj_elem_edges);

              // Check for a matching vertex
              for (index_t e1 = 0; e1 < adj_ne; e1++) {
                index_t v1[2];
                get_element_edge_verts(adj_elems[i], e1, v1);

                // Check if we have a match
                if (global_edge_equality(v0, v1)) {
                  adj_elem_edges[e1] = edge_num;
                }
              }
            }
          }
        }
      }
    }
  }

  // Input counts of the verts, tet, hex, wedge and pyramid elements
  index_t nverts;
  index_t ntets, nhex, nwedge, npyrmd;

  // Derived count information
  index_t nelems;  // Total number of elements
  index_t ntri_faces, nquad_faces;
  index_t nfaces;
  index_t nedges;

  // Element -> vert connectivity
  index_t* tet_verts;
  index_t* hex_verts;
  index_t* wedge_verts;
  index_t* pyrmd_verts;

  // Global data for pointing from verts to elements
  index_t* vert_element_ptr;
  index_t* vert_elements;

  // Element -> face connectivity
  index_t* tet_faces;
  index_t* hex_faces;
  index_t* wedge_faces;
  index_t* pyrmd_faces;

  // Face -> element connectivity
  index_t* tri_face_elements;
  index_t* quad_face_elements;

  // Element -> edge connectivity
  index_t* tet_edges;
  index_t* hex_edges;
  index_t* wedge_edges;
  index_t* pyrmd_edges;
};

template <index_t degree, class HOrderBasis, class Basis>
class HexProjection {
 public:
  static const index_t order = degree + 1;

  index_t get_num_projection_elements() const {
    return (order - 1) * (order - 1) * (order - 1);
  }

  void get_projection_index(index_t n, index_t index[]) {
    const index_t i = n % degree;
    const index_t j = (n % (degree * degree)) / degree;
    const index_t k = n / (degree * degree);

    if constexpr (Basis::nbasis > 0) {
      get_projection_index_<0>(i, j, k, index);
    }
  }

  void get_projection_signs(index_t n, int signs[]) {
    const index_t i = n % degree;
    const index_t j = (n % (degree * degree)) / degree;
    const index_t k = n / (degree * degree);

    if constexpr (Basis::nbasis > 0) {
      get_projection_signs_<0>(i, j, k, signs);
    }
  }

 private:
  template <index_t basis>
  void get_projection_index_(index_t i, index_t j, index_t k, index_t index[]) {
    if (Basis::template get_basis_type<basis>() == H1) {
      get_h1_index<basis>(i, j, k, index);
    } else if (Basis::template get_basis_type<basis>() == HCURL) {
    } else if (Basis::template get_basis_type<basis>() == HDIV) {
      get_hdiv_index<basis>(i, j, k, index);
    } else if (Basis::template get_basis_type<basis>() == L2) {
      get_l2_index<basis>(i, j, k, index);
    }

    if constexpr (basis + 1 < Basis::nbasis) {
      get_projection_index_<basis + 1>(i, j, k, index);
    }
  }
  template <index_t basis>
  void get_projection_signs_(index_t i, index_t j, index_t k, int signs[]) {
    if (Basis::template get_basis_type<basis>() == H1) {
      get_h1_signs<basis>(i, j, k, signs);
    } else if (Basis::template get_basis_type<basis>() == HCURL) {
    } else if (Basis::template get_basis_type<basis>() == HDIV) {
      get_hdiv_signs<basis>(i, j, k, signs);
    } else if (Basis::template get_basis_type<basis>() == L2) {
      get_l2_signs<basis>(i, j, k, signs);
    }

    if constexpr (basis + 1 < Basis::nbasis) {
      get_projection_signs_<basis + 1>(i, j, k, signs);
    }
  }

  template <index_t basis>
  void get_h1_index(const index_t i, const index_t j, const index_t k,
                    index_t dof[]) {
    const index_t stride = Basis::template get_basis_stride<basis>();
    const index_t lo_offset = Basis::template get_dof_offset<basis>();
    const index_t ho_offset = HOrderBasis::template get_dof_offset<basis>();

    for (index_t kk = 0; kk < 2; kk++) {
      for (index_t jj = 0; jj < 2; jj++) {
        for (index_t ii = 0; ii < 2; ii++) {
          for (index_t n = 0; n < stride; n++) {
            dof[lo_offset + n + stride * (ii + 2 * (jj + 2 * kk))] =
                ho_offset + n +
                stride * ((i + ii) + order * ((j + jj) + order * (k + kk)));
          }
        }
      }
    }
  }

  template <index_t basis>
  void get_hdiv_index(const index_t i, const index_t j, const index_t k,
                      index_t dof[]) {
    const index_t stride = Basis::template get_basis_stride<basis>();
    const index_t lo_offset = Basis::template get_dof_offset<basis>();
    const index_t ho_offset = HOrderBasis::template get_dof_offset<basis>();

    dof[lo_offset] = ho_offset + i + order * (j + (order - 1) * k);
    dof[lo_offset + 1] = ho_offset + i + 1 + order * (j + (order - 1) * k);

    dof[lo_offset + 2] = ho_offset + i + (order - 1) * (j + (order - 1) * k) +
                         order * (order - 1) * (order - 1);
    dof[lo_offset + 3] = ho_offset + i +
                         (order - 1) * (j + 1 + (order - 1) * k) +
                         order * (order - 1) * (order - 1);

    dof[lo_offset + 4] = ho_offset + i + (order - 1) * (j + (order - 1) * k) +
                         2 * order * (order - 1) * (order - 1);
    dof[lo_offset + 5] = ho_offset + i +
                         (order - 1) * (j + (order - 1) * (k + 1)) +
                         2 * order * (order - 1) * (order - 1);
  }

  template <index_t basis>
  void get_l2_index(const index_t i, const index_t j, const index_t k,
                    index_t dof[]) {
    const index_t stride = Basis::template get_basis_stride<basis>();
    const index_t lo_offset = Basis::template get_dof_offset<basis>();
    const index_t ho_offset = HOrderBasis::template get_dof_offset<basis>();

    for (index_t n = 0; n < stride; n++) {
      dof[lo_offset + n] =
          ho_offset + n + stride * (i + (order - 1) * (j + (order - 1) * k));
    }
  }

  template <index_t basis>
  void get_h1_signs(const index_t i, const index_t j, const index_t k,
                    int signs[]) {
    const index_t stride = Basis::template get_basis_stride<basis>();
    const index_t lo_offset = Basis::template get_dof_offset<basis>();
    for (index_t n = 0; n < 8 * stride; n++) {
      signs[lo_offset + n] = 1;
    }
  }

  template <index_t basis>
  void get_hdiv_signs(const index_t i, const index_t j, const index_t k,
                      int signs[]) {
    const index_t stride = Basis::template get_basis_stride<basis>();
    const index_t lo_offset = Basis::template get_dof_offset<basis>();

    if (i == 0) {
      signs[lo_offset] = 1;
    } else {
      signs[lo_offset] = -1;
    }
    signs[lo_offset + 1] = 1;

    if (j == 0) {
      signs[lo_offset + 2] = 1;
    } else {
      signs[lo_offset + 2] = -1;
    }
    signs[lo_offset + 3] = 1;

    if (k == 0) {
      signs[lo_offset + 4] = 1;
    } else {
      signs[lo_offset + 4] = -1;
    }
    signs[lo_offset + 5] = 1;
  }

  template <index_t basis>
  void get_l2_signs(const index_t i, const index_t j, const index_t k,
                    int signs[]) {
    const index_t stride = Basis::template get_basis_stride<basis>();
    const index_t lo_offset = Basis::template get_dof_offset<basis>();
    for (index_t n = 0; n < stride; n++) {
      signs[lo_offset + n] = 1;
    }
  }
};

/*
  ElementMesh - Map from an element to the global to element local degrees
  of freedom
*/
template <class Basis>
class ElementMesh {
 public:
  using ET = ElementTypes;

  /// @brief  Constant expression indicating an un-ordered node
  static constexpr index_t NO_INDEX = std::numeric_limits<index_t>::max();

  // Number of degrees of freedom for each element
  static const index_t ndof_per_element = Basis::ndof;

  ElementMesh(MeshConnectivity3D& conn)
      : nelems(conn.get_num_elements()), num_dof(0) {
    // Count up the number of degrees of freedom
    element_dof = new index_t[nelems * ndof_per_element];
    element_sign = new int[nelems * ndof_per_element];
    std::fill(element_dof, element_dof + nelems * ndof_per_element, NO_INDEX);

    // Perform a sweep of the elements
    std::vector<index_t> ids(nelems, NO_INDEX), stack(nelems);

    index_t start = 0, end = 1, level = 0;
    stack[0] = 0;
    ids[0] = level;

    while (start < end) {
      index_t next = end;
      level++;

      for (index_t i = start; i < end; i++) {
        // Loop over
        const index_t* faces;
        index_t nf = conn.get_element_faces(stack[i], &faces);

        for (index_t j = 0; j < nf; j++) {
          // Look at the adjacent elements
          index_t e1, e2;
          conn.get_face_elements(faces[j], &e1, &e2);

          if (e1 < nelems && ids[e1] == NO_INDEX) {
            ids[e1] = level;
            stack[next] = e1;
            next++;
          }
          if (e2 < nelems && ids[e2] == NO_INDEX) {
            ids[e2] = level;
            stack[next] = e2;
            next++;
          }
        }
      }

      start = end;
      end = next;
    }

    std::vector<index_t> face_owners(conn.get_num_faces(), NO_INDEX);
    std::vector<index_t> edge_owners(conn.get_num_edges(), NO_INDEX);
    std::vector<index_t> vert_owners(conn.get_num_verts(), NO_INDEX);

    index_t dof_counter = 0;
    index_t dof[ndof_per_element];

    for (index_t basis = 0; basis < Basis::nbasis; basis++) {
      for (index_t counter = nelems; counter > 0; counter--) {
        index_t elem = stack[counter - 1];

        index_t* elem_dof = &element_dof[elem * ndof_per_element];
        int* elem_sign = &element_sign[elem * ndof_per_element];

        // The volume DOF are always owned by the element - no need to check
        // for the element that owns them
        index_t ndof = Basis::get_entity_ndof(basis, ET::VOLUME, 0);

        for (index_t i = 0; i < ndof; i++, dof_counter++) {
          dof[i] = dof_counter;
        }
        Basis::set_entity_dof(basis, ET::VOLUME, 0, 0, dof, elem_dof);
        Basis::set_entity_signs(basis, ET::VOLUME, 0, 0, elem_sign);

        // Order the faces
        const index_t* faces;
        index_t nf = conn.get_element_faces(elem, &faces);
        for (index_t index = 0; index < nf; index++) {
          index_t face = faces[index];
          index_t orient = 0;
          if (face_owners[face] == NO_INDEX || face_owners[face] == elem) {
            face_owners[face] = elem;

            ndof = Basis::get_entity_ndof(basis, ET::FACE, index);
            for (index_t i = 0; i < ndof; i++, dof_counter++) {
              dof[i] = dof_counter;
            }
          } else {
            index_t owner_elem = face_owners[face];
            const index_t* owner_faces;
            index_t nf_owner = conn.get_element_faces(owner_elem, &owner_faces);

            for (index_t i = 0; i < nf_owner; i++) {
              if (owner_faces[i] == face) {
                index_t ref[4], verts[4];
                index_t nverts =
                    conn.get_element_face_verts(owner_elem, i, ref);
                conn.get_element_face_verts(elem, index, verts);

                Basis::get_entity_dof(
                    basis, ET::FACE, i,
                    &element_dof[owner_elem * ndof_per_element], dof);

                if (nverts == 4) {
                  orient = ET::get_quad_face_orientation(ref, verts);
                }
                break;
              }
            }
          }

          Basis::set_entity_dof(basis, ET::FACE, index, orient, dof, elem_dof);
          Basis::set_entity_signs(basis, ET::FACE, index, orient, elem_sign);
        }

        // Order the edges
        const index_t* edges;
        index_t ne = conn.get_element_edges(elem, &edges);
        for (index_t index = 0; index < ne; index++) {
          index_t edge = edges[index];
          index_t orient = 0;
          if (edge_owners[edge] == NO_INDEX || edge_owners[edge] == elem) {
            edge_owners[edge] = elem;

            ndof = Basis::get_entity_ndof(basis, ET::EDGE, index);
            for (index_t i = 0; i < ndof; i++, dof_counter++) {
              dof[i] = dof_counter;
            }
          } else {
            index_t owner_elem = edge_owners[edge];
            const index_t* owner_edges;
            index_t ne_owner = conn.get_element_edges(owner_elem, &owner_edges);

            for (index_t i = 0; i < ne_owner; i++) {
              if (owner_edges[i] == edge) {
                index_t ref[2], verts[2];
                conn.get_element_edge_verts(owner_elem, i, ref);
                conn.get_element_edge_verts(elem, index, verts);

                Basis::get_entity_dof(
                    basis, ET::EDGE, i,
                    &element_dof[owner_elem * ndof_per_element], dof);

                if (ref[0] == verts[1] && ref[1] == verts[0]) {
                  orient = 1;
                }
                break;
              }
            }
          }

          Basis::set_entity_dof(basis, ET::EDGE, index, orient, dof, elem_dof);
          Basis::set_entity_signs(basis, ET::EDGE, index, orient, elem_sign);
        }

        // Order the vertices
        const index_t* verts;
        index_t nv = conn.get_element_verts(elem, &verts);
        for (index_t index = 0; index < nv; index++) {
          index_t vert = verts[index];
          index_t orient = 0;
          if (vert_owners[vert] == NO_INDEX || vert_owners[vert] == elem) {
            vert_owners[vert] = elem;

            ndof = Basis::get_entity_ndof(basis, ET::VERTEX, index);
            for (index_t i = 0; i < ndof; i++, dof_counter++) {
              dof[i] = dof_counter;
            }
          } else {
            index_t owner_elem = vert_owners[vert];
            const index_t* owner_verts;
            index_t nv_owner = conn.get_element_verts(owner_elem, &owner_verts);

            for (index_t i = 0; i < nv_owner; i++) {
              if (owner_verts[i] == vert) {
                Basis::get_entity_dof(
                    basis, ET::VERTEX, i,
                    &element_dof[owner_elem * ndof_per_element], dof);
                break;
              }
            }
          }

          Basis::set_entity_dof(basis, ET::VERTEX, index, orient, dof,
                                elem_dof);
          Basis::set_entity_signs(basis, ET::VERTEX, index, orient, elem_sign);
        }
      }
    }

    // Set the number of degrees of freedom
    num_dof = dof_counter;
  }

  template <class HOrderBasis, class ElemProj>
  ElementMesh(ElementMesh<HOrderBasis>& mesh, ElemProj& proj)
      : nelems(proj.get_num_projection_elements() * mesh.get_num_elements()),
        num_dof(mesh.get_num_dof()) {
    element_dof = new index_t[nelems * ndof_per_element];
    element_sign = new int[nelems * ndof_per_element];

    // Loop over all the high-order elements and generate the low-order
    // representation
    for (index_t j = 0; j < mesh.get_num_elements(); j++) {
      // Get the signs associated wtih the high-order mesh
      const index_t* horder_dof;
      const int* horder_signs;
      mesh.get_element_dof(j, &horder_dof);
      mesh.get_element_signs(j, &horder_signs);

      for (index_t i = 0; i < proj.get_num_projection_elements(); i++) {
        index_t elem = i + j * proj.get_num_projection_elements();

        // Index into the high-order element
        index_t index[ndof_per_element];
        proj.get_projection_index(i, index);

        // Sign - indicating any orientation flip
        int signs[ndof_per_element];
        proj.get_projection_signs(i, signs);

        for (index_t k = 0; k < ndof_per_element; k++) {
          element_sign[ndof_per_element * elem + k] =
              signs[k] * horder_signs[index[k]];
          element_dof[ndof_per_element * elem + k] = horder_dof[index[k]];
        }
      }
    }
  }

  index_t get_num_elements() { return nelems; }
  index_t get_num_dof() { return num_dof; }

  template <index_t basis>
  int get_global_dof_sign(index_t elem, index_t index) {
    return element_sign[ndof_per_element * elem +
                        Basis::template get_dof_offset<basis>() + index];
    return 1;
  }

  template <index_t basis>
  index_t get_global_dof(index_t elem, index_t index) {
    return element_dof[ndof_per_element * elem +
                       Basis::template get_dof_offset<basis>() + index];
  }

  // Get the degrees of freedom associated with this element
  void get_element_dof(const index_t elem, const index_t* dof[]) {
    *dof = &element_dof[ndof_per_element * elem];
  }

  // Get the signs associated with the degrees of freedom
  void get_element_signs(const index_t elem, const int* signs[]) {
    *signs = &element_sign[ndof_per_element * elem];
  }

  template <typename T, index_t M>
  std::shared_ptr<BSRMat<index_t, T, M, M>> create_block_matrix() {
    std::set<std::pair<index_t, index_t>> node_set;

    for (index_t i = 0; i < nelems; i++) {
      index_t dof_reduced[Basis::ndof];
      index_t n = 0;
      for (index_t j1 = 0; j1 < Basis::ndof; j1++, n++) {
        index_t row = element_dof[i * Basis::ndof + j1] / M;
        while (j1 + 1 < Basis::ndof &&
               row == (element_dof[i * Basis::ndof + j1 + 1] / M)) {
          j1++;
        }
        dof_reduced[n] = row;
      }

      for (index_t j1 = 0; j1 < n; j1++) {
        for (index_t j2 = 0; j2 < n; j2++) {
          node_set.insert(std::make_pair(dof_reduced[j1], dof_reduced[j2]));
        }
      }
    }

    index_t nrows = num_dof / M;
    if (num_dof % M > 0) {
      nrows += 1;
    }

    // Find the number of nodes referenced by other nodes
    std::vector<index_t> rowp(nrows + 1);

    typename std::set<std::pair<index_t, index_t>>::iterator it;
    for (it = node_set.begin(); it != node_set.end(); it++) {
      rowp[it->first + 1] += 1;
    }

    // Set the pointer into the rows
    rowp[0] = 0;
    for (index_t i = 0; i < nrows; i++) {
      rowp[i + 1] += rowp[i];
    }

    index_t nnz = rowp[nrows];
    std::vector<index_t> cols(nnz);

    for (it = node_set.begin(); it != node_set.end(); it++) {
      cols[rowp[it->first]] = it->second;
      rowp[it->first]++;
    }

    // Reset the pointer into the nodes
    for (index_t i = nrows; i > 0; i--) {
      rowp[i] = rowp[i - 1];
    }
    rowp[0] = 0;

    // Sort the cols array
    SortCSRData(nrows, rowp, cols);

    return std::make_shared<BSRMat<index_t, T, M, M>>(nrows, nrows, nnz, rowp,
                                                      cols);
  }

 private:
  index_t nelems;   // Total number of elements
  index_t num_dof;  // Total number of degrees of freedom

  // Store the degrees of freedom for each element and the element sign
  index_t* element_dof;
  int* element_sign;
};

template <class Basis>
class BoundaryCondition {
 public:
  // Use the definitions from the element types
  using ET = ElementTypes;

  /**
   * @brief Construct a new Boundary Condition object by constraining the
   * degrees of freedom from the vertices, edges and faces that touch the
   * specified degrees of freedom
   *
   * @param conn The connectivity
   * @param mesh The mesh object with the ordered degrees of freedom
   * @param basis_select The degrees of freedom indices selected for each basis
   * @param ndof Output number of degrees of freedom
   * @param dof Output degrees of freedom
   */
  template <typename IdxType>
  BoundaryCondition(MeshConnectivity3D& conn, ElementMesh<Basis>& mesh,
                    const index_t basis_select[], index_t nv,
                    const IdxType verts[]) {
    index_t nelems = conn.get_num_elements();
    index_t nverts = conn.get_num_verts();
    index_t nedges = conn.get_num_edges();
    index_t nfaces = conn.get_num_faces();
    index_t* vert_labels = new index_t[nverts];
    index_t* edge_labels = new index_t[nedges];
    index_t* face_labels = new index_t[nfaces];

    conn.get_labels_from_verts(nv, verts, vert_labels, edge_labels,
                               face_labels);

    index_t num_dof = mesh.get_num_dof();
    index_t boundary_dof_counter = 0;
    index_t* boundary_dof = new index_t[num_dof];

    for (index_t elem = 0; elem < nelems; elem++) {
      index_t dof[Basis::ndof];
      const index_t* elem_dof;
      mesh.get_element_dof(elem, &elem_dof);

      // Scan over the faces
      const index_t* elem_faces;
      index_t nf = conn.get_element_faces(elem, &elem_faces);
      for (index_t f = 0; f < nf; f++) {
        if (face_labels[elem_faces[f]] != MeshConnectivity3D::NO_LABEL) {
          for (index_t basis = 0; basis < Basis::nbasis; basis++) {
            if (basis_select[basis]) {
              index_t nface_dof = Basis::get_entity_ndof(basis, ET::FACE, f);
              Basis::get_entity_dof(basis, ET::FACE, f, elem_dof, dof);

              for (index_t k = 0; k < nface_dof; k++) {
                boundary_dof[boundary_dof_counter] = dof[k];
                boundary_dof_counter++;
              }
            }
          }

          face_labels[elem_faces[f]] = MeshConnectivity3D::NO_LABEL;
        }
      }

      // Scan over the edges
      const index_t* elem_edges;
      index_t ne = conn.get_element_faces(elem, &elem_edges);
      for (index_t e = 0; e < ne; e++) {
        if (edge_labels[elem_edges[e]] != MeshConnectivity3D::NO_LABEL) {
          for (index_t basis = 0; basis < Basis::nbasis; basis++) {
            if (basis_select[basis]) {
              index_t nedge_dof = Basis::get_entity_ndof(basis, ET::EDGE, e);
              Basis::get_entity_dof(basis, ET::EDGE, e, elem_dof, dof);

              for (index_t k = 0; k < nedge_dof; k++) {
                boundary_dof[boundary_dof_counter] = dof[k];
                boundary_dof_counter++;
              }
            }
          }

          edge_labels[elem_edges[e]] = MeshConnectivity3D::NO_LABEL;
        }
      }

      // Scan over the vertices
      const index_t* elem_verts;
      index_t nv = conn.get_element_verts(elem, &elem_verts);
      for (index_t v = 0; v < nv; v++) {
        if (vert_labels[elem_verts[v]] != MeshConnectivity3D::NO_LABEL) {
          for (index_t basis = 0; basis < Basis::nbasis; basis++) {
            if (basis_select[basis]) {
              index_t nvert_dof = Basis::get_entity_ndof(basis, ET::VERTEX, v);
              Basis::get_entity_dof(basis, ET::VERTEX, v, elem_dof, dof);

              for (index_t k = 0; k < nvert_dof; k++) {
                boundary_dof[boundary_dof_counter] = dof[k];
                boundary_dof_counter++;
              }
            }
          }

          vert_labels[elem_verts[v]] = MeshConnectivity3D::NO_LABEL;
        }
      }
    }

    delete[] vert_labels;
    delete[] edge_labels;
    delete[] face_labels;

    ndof = boundary_dof_counter;
    dof = new index_t[ndof];
    for (index_t i = 0; i < ndof; i++) {
      dof[i] = boundary_dof[i];
    }

    // Free the boundary_dof array
    delete[] boundary_dof;
  }

  index_t get_bcs(const index_t* bcs[]) {
    if (bcs) {
      *bcs = dof;
    }
    return ndof;
  }

  template <class VecType, typename T>
  void set_bcs(VecType& vec, T value) {
    for (index_t i = 0; i < ndof; i++) {
      vec[dof[i]] = value;
    }
  }

 private:
  index_t ndof;
  index_t* dof;
};

}  // namespace A2D

#endif  // A2D_FE_MESH_H
