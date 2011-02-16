// Begin License:
// Copyright (C) 2006-2008 Tobias Sargeant (tobias.sargeant@gmail.com).
// All rights reserved.
//
// This file is part of the Carve CSG Library (http://carve-csg.com/)
//
// This file may be used under the terms of the GNU General Public
// License version 2.0 as published by the Free Software Foundation
// and appearing in the file LICENSE.GPL2 included in the packaging of
// this file.
//
// This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
// INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE.
// End:


#pragma once

#include <carve/carve.hpp>

#include <carve/geom.hpp>
#include <carve/geom3d.hpp>
#include <carve/tag.hpp>
#include <carve/djset.hpp>

#include <carve/aabb.hpp>

#include <iostream>

namespace carve {
  namespace poly {
    class Polyhedron;
  }

  namespace mesh {


    template<unsigned ndim> class Edge;
    template<unsigned ndim> class Face;
    template<unsigned ndim> class Mesh;
    template<unsigned ndim> class MeshSet;



    // A Vertex may participate in several meshes. If the Mesh belongs
    // to a MeshSet, then the vertices come from the vertex_storage
    // member of the MeshSet. This allows one to construct one or more
    // Meshes out of sets of connected faces (possibly using vertices
    // from a variety of MeshSets, and other storage), then create a
    // MeshSet from the Mesh(es), causing the vertices to be
    // collected, cloned and repointed into the MeshSet.

    // Normally, in a half-edge structure, Vertex would have a member
    // pointing to an incident edge, allowing the enumeration of
    // adjacent faces and edges. Because we want to support vertex
    // sharing between Meshes and groups of Faces, this is made more
    // complex. If Vertex contained a list of incident edges, one from
    // each disjoint face set, then this could be done (with the
    // caveat that you'd need to pass in a Mesh pointer to the
    // adjacency queries). However, it seems that this would
    // unavoidably complicate the process of incorporating or removing
    // a vertex into an edge.

    // In most cases it is expected that a vertex will be arrived at
    // via an edge or face in the mesh (implicit or explicit) of
    // interest, so not storing this information will not hurt,
    // overly.
    template<unsigned ndim>
    class Vertex : public tagable {
    public:
      typedef carve::geom::vector<ndim> vector_t;
      typedef MeshSet<ndim> owner_t;
      typedef carve::geom::aabb<ndim> aabb_t;

      carve::geom::vector<ndim> v;

      Vertex(const vector_t &_v) : tagable(), v(_v) {
      }

      Vertex() : tagable(), v() {
      }

      aabb_t getAABB() const {
        return aabb_t(v, carve::geom::vector<ndim>::ZERO());
      }
    };



    struct hash_vertex_pair {
      template<unsigned ndim>
      size_t operator()(const std::pair<Vertex<ndim> *, Vertex<ndim> *> &pair) const {
        size_t r = (size_t)pair.first;
        size_t s = (size_t)pair.second;
        return r ^ ((s >> 16) | (s << 16));
      }
      template<unsigned ndim>
      size_t operator()(const std::pair<const Vertex<ndim> *, const Vertex<ndim> *> &pair) const {
        size_t r = (size_t)pair.first;
        size_t s = (size_t)pair.second;
        return r ^ ((s >> 16) | (s << 16));
      }
    };



    namespace detail {
      template<typename list_t> struct circlist_iter_t;
      template<typename list_t> struct fwd_circlist_iter_t;

      template<typename iter_t, typename mapping_t> struct mapped_iter_t;

      template<unsigned ndim> struct edge_vertex_mapping;
      template<unsigned ndim> struct const_edge_vertex_mapping;
    }



    // The half-edge structure proper (Edge) is maintained by Face
    // instances. Together with Face instances, the half-edge
    // structure defines a simple mesh (either one or two faces
    // incident on each edge).
    template<unsigned ndim>
    class Edge : public tagable {
    public:
      typedef Vertex<ndim> vertex_t;
      typedef Face<ndim> face_t;

      typedef detail::circlist_iter_t<Edge<ndim> > iter_t;
      typedef detail::circlist_iter_t<const Edge<ndim> > const_iter_t;

      typedef detail::fwd_circlist_iter_t<Edge<ndim> > fwd_iter_t;
      typedef detail::fwd_circlist_iter_t<const Edge<ndim> > const_fwd_iter_t;

      typedef detail::mapped_iter_t<iter_t, detail::edge_vertex_mapping<ndim> > vert_iter_t;
      typedef detail::mapped_iter_t<const_iter_t, detail::const_edge_vertex_mapping<ndim> > const_vert_iter_t;

      typedef detail::mapped_iter_t<fwd_iter_t, detail::edge_vertex_mapping<ndim> > fwd_vert_iter_t;
      typedef detail::mapped_iter_t<const_fwd_iter_t, detail::const_edge_vertex_mapping<ndim> > const_fwd_vert_iter_t;

      vertex_t *vert;
      face_t *face;
      Edge *prev, *next, *rev;

      // Remove this edge from its containing edge loop. disconnect
      // rev links. The rev links of the previous edge also change, as
      // its successor vertex changes.
      void remove();

      // Insert this edge into a loop before other. If edge was
      // already in a loop, it needs to be removed first.
      void insertBefore(Edge *other);

      // Insert this edge into a loop after other. If edge was
      // already in a loop, it needs to be removed first.
      void insertAfter(Edge *other);

      iter_t iter();
      const_iter_t iter() const;

      fwd_iter_t begin();
      const_fwd_iter_t begin() const;

      fwd_iter_t end();
      const_fwd_iter_t end() const;

      vert_iter_t viter();
      const_vert_iter_t viter() const;

      fwd_vert_iter_t vbegin();
      const_fwd_vert_iter_t vbegin() const;

      fwd_vert_iter_t vend();
      const_fwd_vert_iter_t vend() const;

      size_t loopSize() const;

      vertex_t *v1() { return vert; }
      vertex_t *v2() { return next->vert; }

      const vertex_t *v1() const { return vert; }
      const vertex_t *v2() const { return next->vert; }

      Edge *perimNext() const {
        if (rev) return NULL;
        Edge *e = next;
        while(e->rev) {
          e = e->rev->next;
        }
        return e;
      }

      Edge *perimPrev() const {
        if (rev) return NULL;
        Edge *e = prev;
        while(e->rev) {
          e = e->rev->prev;
        }
        return e;
      }

      Edge(vertex_t *_vert, face_t *_face);

      ~Edge();
    };



    // A Face contains a pointer to the beginning of the half-edge
    // circular list that defines its boundary.
    template<unsigned ndim>
    class Face : public tagable {
    public:
      typedef Vertex<ndim> vertex_t;
      typedef Edge<ndim> edge_t;
      typedef Mesh<ndim> mesh_t;

      typedef typename Vertex<ndim>::vector_t vector_t;
      typedef carve::geom::aabb<ndim> aabb_t;
      typedef carve::geom::plane<ndim> plane_t;
      typedef carve::geom::vector<2> (*project_t)(const vector_t &);
      typedef vector_t (*unproject_t)(const carve::geom::vector<2> &, const plane_t &);

      struct vector_mapping {
        typedef typename vertex_t::vector_t value_type;

        value_type operator()(const carve::geom::vector<ndim> &v) const { return v; }
        value_type operator()(const carve::geom::vector<ndim> *v) const { return *v; }
        value_type operator()(const Edge<ndim> &e) const { return e.vert->v; }
        value_type operator()(const Edge<ndim> *e) const { return e->vert->v; }
        value_type operator()(const Vertex<ndim> &v) const { return v.v; }
        value_type operator()(const Vertex<ndim> *v) const { return v->v; }
      };

      struct projection_mapping {
        typedef carve::geom::vector<2> value_type;
        project_t proj;
        projection_mapping(project_t _proj) : proj(_proj) { }
        value_type operator()(const carve::geom::vector<ndim> &v) const { return proj(v); }
        value_type operator()(const carve::geom::vector<ndim> *v) const { return proj(*v); }
        value_type operator()(const Edge<ndim> &e) const { return proj(e.vert->v); }
        value_type operator()(const Edge<ndim> *e) const { return proj(e->vert->v); }
        value_type operator()(const Vertex<ndim> &v) const { return proj(v.v); }
        value_type operator()(const Vertex<ndim> *v) const { return proj(v->v); }
      };

      edge_t *edge;
      size_t n_edges;
      mesh_t *mesh;
      size_t id;

      plane_t plane;
      project_t project;
      unproject_t unproject;

    private:
      Face &operator=(const Face &other);

    protected:
      Face(const Face &other) :
        edge(NULL), n_edges(other.n_edges), mesh(NULL), id(other.id),
        plane(other.plane), project(other.project), unproject(other.unproject) {
      }

    public:
      project_t getProjector(bool positive_facing, int axis);
      unproject_t getUnprojector(bool positive_facing, int axis);

      aabb_t getAABB() const;

      bool recalc();

      void clearEdges();

      // build an edge loop in forward orientation from an iterator pair
      template<typename iter_t>
      void loopFwd(iter_t vbegin, iter_t vend);

      // build an edge loop in reverse orientation from an iterator pair
      template<typename iter_t>
      void loopRev(iter_t vbegin, iter_t vend);

      // initialize a face from an ordered list of vertices.
      template<typename iter_t>
      void init(iter_t begin, iter_t end);

      // initialization of a triangular face.
      void init(vertex_t *a, vertex_t *b, vertex_t *c);

      // initialization of a quad face.
      void init(vertex_t *a, vertex_t *b, vertex_t *c, vertex_t *d);

      void getVertices(std::vector<const vertex_t *> &verts) const;
      void getProjectedVertices(std::vector<carve::geom::vector<2> > &verts) const;

      size_t nVertices() const {
        return n_edges;
      }

      size_t nEdges() const {
        return n_edges;
      }

      vector_t centroid() const {
        vector_t v;
        edge_t *e = edge;
        do {
          v += e->vert->v;
          e = e->next;
        } while(e != edge);
        v /= n_edges;
        return v;
      }
      
      Face(vertex_t *a, vertex_t *b, vertex_t *c) : edge(NULL), n_edges(0), mesh(NULL) {
        init(a, b, c);
        recalc();
      }

      Face(vertex_t *a, vertex_t *b, vertex_t *c, vertex_t *d) : edge(NULL), n_edges(0), mesh(NULL) {
        init(a, b, c, d);
        recalc();
      }

      template<typename iter_t>
      Face(iter_t begin, iter_t end) : edge(NULL), n_edges(0), mesh(NULL) {
        init(begin, end);
        recalc();
      }

      Face *clone(const vertex_t *old_base, vertex_t *new_base, std::unordered_map<const edge_t *, edge_t *> &edge_map) const {
        Face *r = new Face(*this);

        edge_t *e = edge;
        edge_t *r_p = NULL;
        edge_t *r_e;
        do {
          r_e = new edge_t(e->vert - old_base + new_base, r);
          edge_map[e] = r_e;
          if (r_p) {
            r_p->next = r_e;
            r_e->prev = r_p;
          } else {
            r->edge = r_e;
          }
          r_p = r_e;
          e = e->next;
        } while (e != edge);
        r_e->next = r->edge;
        r->edge->prev = r_e;

        return r;
      }

      ~Face() {
        clearEdges();
      }
    };



    namespace detail {
      class FaceStitcher {
        typedef Vertex<3> vertex_t;
        typedef Edge<3> edge_t;
        typedef Face<3> face_t;

        typedef std::pair<const vertex_t *, const vertex_t *> vpair_t;
        typedef std::list<edge_t *> edgelist_t;
        typedef std::unordered_map<vpair_t, edgelist_t, carve::mesh::hash_vertex_pair> edge_map_t;
        typedef std::unordered_map<const vertex_t *, std::set<const vertex_t *> > edge_graph_t;

        edge_map_t edges;
        edge_map_t complex_edges;

        carve::djset::djset face_groups;
        std::vector<bool> is_open;

        edge_graph_t edge_graph;

        struct EdgeOrderData {
          size_t group_id;
          bool is_reversed;
          carve::geom::vector<3> face_dir;
          edge_t *edge;

          EdgeOrderData(edge_t *_edge, size_t _group_id, bool _is_reversed) :
            group_id(_group_id),
            is_reversed(_is_reversed) {
            if (is_reversed) {
              face_dir = -(_edge->face->plane.N);
            } else {
              face_dir =  (_edge->face->plane.N);
            }
            edge = _edge;
          }

          struct TestGroups {
            size_t fwd, rev;

            TestGroups(size_t _fwd, size_t _rev) : fwd(_fwd), rev(_rev) {
            }

            bool operator()(const EdgeOrderData &eo) const {
              return eo.group_id == (eo.is_reversed ? rev : fwd);
            }
          };

          struct Cmp {
            carve::geom::vector<3> edge_dir;
            carve::geom::vector<3> base_dir;

            Cmp(const carve::geom::vector<3> &_edge_dir,
                const carve::geom::vector<3> &_base_dir) :
              edge_dir(_edge_dir),
              base_dir(_base_dir) {
            }
            bool operator()(const EdgeOrderData &a, const EdgeOrderData &b) const {
              int v = carve::geom3d::compareAngles(edge_dir, base_dir, a.face_dir, b.face_dir);
              double da = carve::geom3d::antiClockwiseAngle(base_dir, a.face_dir, edge_dir);
              double db = carve::geom3d::antiClockwiseAngle(base_dir, b.face_dir, edge_dir);
              int v0 = v;
              v = 0;
              if (da < db) v = -1;
              if (db < da) v = +1;
              if (v0 != v) {
                std::cerr << "v0= " << v0 << " v= " << v << " da= " << da << " db= " << db << "  " << edge_dir << " " << base_dir << " " << a.face_dir << b.face_dir << std::endl;
              }
              if (v < 0) return true;
              if (v == 0) {
                if (a.is_reversed && !b.is_reversed) return true;
                if (a.is_reversed == b.is_reversed) {
                  return a.group_id < b.group_id;
                }
              }
              return false;
            }
          };
        };

        template<typename iter_t>
        void initEdges(iter_t begin, iter_t end) {
          size_t c = 0;
          for (iter_t i = begin; i != end; ++i) {
            face_t *face = *i;
            CARVE_ASSERT(face->mesh == NULL); // for the moment, can only insert a face into a mesh once.

            face->id = c++;
            edge_t *e = face->edge;
            do {
              edges[vpair_t(e->v1(), e->v2())].push_back(e);
              e = e->next;
              if (e->rev) { e->rev->rev = NULL; e->rev = NULL; }
            } while (e != face->edge);
          }

          face_groups.init(c);
          is_open.clear();
          is_open.resize(c, false);
        }

        void extractConnectedEdges(std::vector<const vertex_t *>::iterator begin,
                                   std::vector<const vertex_t *>::iterator end,
                                   std::vector<std::vector<Edge<3> *> > &efwd,
                                   std::vector<std::vector<Edge<3> *> > &erev);

        size_t faceGroupID(const Face<3> *face);
        size_t faceGroupID(const Edge<3> *edge);

        void resolveOpenEdges();

        void fuseEdges(std::vector<Edge<3> *> &fwd,
                       std::vector<Edge<3> *> &rev);

        void joinGroups(std::vector<std::vector<Edge<3> *> > &efwd,
                        std::vector<std::vector<Edge<3> *> > &erev,
                        size_t fwd_grp,
                        size_t rev_grp);

        void matchOrderedEdges(const std::vector<std::vector<EdgeOrderData> >::iterator begin,
                               const std::vector<std::vector<EdgeOrderData> >::iterator end,
                               std::vector<std::vector<Edge<3> *> > &efwd,
                               std::vector<std::vector<Edge<3> *> > &erev);

        void reorder(std::vector<EdgeOrderData> &ordering, size_t fwd_grp);

        void orderForwardAndReverseEdges(std::vector<std::vector<Edge<3> *> > &efwd,
                                         std::vector<std::vector<Edge<3> *> > &erev,
                                         std::vector<std::vector<EdgeOrderData> > &result);

        void edgeIncidentGroups(const vpair_t &e,
                                const edge_map_t &all_edges,
                                std::pair<std::set<size_t>, std::set<size_t> > &groups);

        void buildEdgeGraph(const edge_map_t &all_edges);
        void extractPath(std::vector<const vertex_t *> &path);
        void removePath(const std::vector<const vertex_t *> &path);
        void matchSimpleEdges();
        void construct();

        template<typename iter_t>
        void build(iter_t begin, iter_t end, std::vector<Mesh<3> *> &meshes) {
          // work out what set each face belongs to, and then construct
          // mesh instances for each set of faces.
          std::vector<size_t> index_set;
          std::vector<size_t> set_size;
          face_groups.get_index_to_set(index_set, set_size);

          std::vector<std::vector<face_t *> > mesh_faces;
          mesh_faces.resize(set_size.size());
          for (size_t i = 0; i < set_size.size(); ++i) {
            mesh_faces[i].reserve(set_size[i]);
          }
      
          for (iter_t i = begin; i != end; ++i) {
            face_t *face = *i;
            mesh_faces[index_set[face->id]].push_back(face);
          }

          meshes.clear();
          meshes.reserve(mesh_faces.size());
          for (size_t i = 0; i < mesh_faces.size(); ++i) {
            meshes.push_back(new Mesh<3>(mesh_faces[i]));
          }
        }

      public:
        template<typename iter_t>
        void create(iter_t begin, iter_t end, std::vector<Mesh<3> *> &meshes) {
          initEdges(begin, end);
          construct();
          build(begin, end, meshes);
        }
      };
    }

    // A Mesh is a connected set of faces. It may be open (some edges
    // have NULL rev members), or closed. On destruction, a Mesh
    // should free its Faces (which will in turn free Edges, but not
    // Vertices).  A Mesh is edge-connected, which is to say that each
    // face in the mesh shares an edge with at least one other face in
    // the mesh. Touching at a vertex is not sufficient. This means
    // that the perimeter of an open mesh visits each vertex no more
    // than once.
    template<unsigned ndim>
    class Mesh {
    public:
      typedef Vertex<ndim> vertex_t;
      typedef Edge<ndim> edge_t;
      typedef Face<ndim> face_t;
      typedef carve::geom::aabb<ndim> aabb_t;
      typedef MeshSet<ndim> meshset_t;

      std::vector<face_t *> faces;
      std::vector<edge_t *> open_edges;
      std::vector<edge_t *> closed_edges;
      bool is_negative;
      meshset_t *meshset;

    protected:
      Mesh(std::vector<face_t *> &_faces,
           std::vector<edge_t *> &_open_edges,
           std::vector<edge_t *> &_closed_edges,
           bool _is_negative) {
        std::swap(faces, _faces);
        std::swap(open_edges, _open_edges);
        std::swap(closed_edges, _closed_edges);
        is_negative = _is_negative;
        meshset = NULL;
        
        for (size_t i = 0; i < faces.size(); ++i) {
          faces[i]->mesh = this;
        }
      }

    public:
      Mesh(std::vector<face_t *> &_faces);

      ~Mesh();

      template<typename iter_t>
      static void create(iter_t begin, iter_t end, std::vector<Mesh<ndim> *> &meshes);

      aabb_t getAABB() const {
        aabb_t result;
        if (faces.size()) {
          result = faces[0]->getAABB();
          for (size_t i = 1; i < faces.size(); ++i) {
            result.unionAABB(faces[i]->getAABB());
          }
        }
        return result;
      }

      bool isClosed() const {
        return open_edges.size() == 0;
      }

      bool isNegative() const {
        return is_negative;
      }

      struct IsClosed {
        bool operator()(const Mesh &mesh) const { return mesh.isClosed(); }
        bool operator()(const Mesh *mesh) const { return mesh->isClosed(); }
      };

      Mesh *clone(const vertex_t *old_base, vertex_t *new_base) const {
        std::vector<face_t *> r_faces;
        std::vector<edge_t *> r_open_edges;
        std::vector<edge_t *> r_closed_edges;
        std::unordered_map<const edge_t *, edge_t *> edge_map;

        r_faces.reserve(faces.size());
        r_open_edges.reserve(r_open_edges.size());
        r_closed_edges.reserve(r_closed_edges.size());

        for (size_t i = 0; i < faces.size(); ++i) {
          r_faces.push_back(faces[i]->clone(old_base, new_base, edge_map));
        }
        for (size_t i = 0; i < closed_edges.size(); ++i) {
          r_closed_edges.push_back(edge_map[closed_edges[i]]);
          r_closed_edges.back()->rev = edge_map[closed_edges[i]->rev];
        }
        for (size_t i = 0; i < open_edges.size(); ++i) {
          r_open_edges.push_back(edge_map[open_edges[i]]);
        }

        return new Mesh(r_faces, r_open_edges, r_closed_edges, is_negative);
      }
    };

    template<unsigned ndim>
    template<typename iter_t>
    void Mesh<ndim>::create(iter_t begin, iter_t end, std::vector<Mesh<ndim> *> &meshes) {
      meshes.clear();
    }

    template<>
    template<typename iter_t>
    void Mesh<3>::create(iter_t begin, iter_t end, std::vector<Mesh<3> *> &meshes) {
      detail::FaceStitcher().create(begin, end, meshes);
    }



    // A MeshSet manages vertex storage, and a collection of meshes.
    // It should be easy to turn a vertex pointer into its index in
    // its MeshSet vertex_storage.
    template<unsigned ndim>
    class MeshSet {
    public:
      typedef Vertex<ndim> vertex_t;
      typedef Edge<ndim> edge_t;
      typedef Face<ndim> face_t;
      typedef Mesh<ndim> mesh_t;
      typedef carve::geom::aabb<ndim> aabb_t;

      std::vector<vertex_t> vertex_storage;
      std::vector<mesh_t *> meshes;

    public:
      struct FaceIter : public std::iterator<std::random_access_iterator_tag, face_t *> {
        typedef std::iterator<std::random_access_iterator_tag, face_t *> super;
        const MeshSet<ndim> *obj;
        size_t mesh, face;

        FaceIter(const MeshSet<ndim> *_obj, size_t _mesh, size_t _face) : obj(_obj), mesh(_mesh), face(_face) {
        }

        void fwd(size_t n) {
          if (mesh < obj->meshes.size()) {
            face += n;
            while (face >= obj->meshes[mesh]->faces.size()) {
              face -= obj->meshes[mesh++]->faces.size();
              if (mesh == obj->meshes.size()) { face = 0; break; }
            }
          }
        }

        void rev(size_t n) {
          while (n > face) {
            n -= face;
            if (mesh == 0) { face = 0; return; }
            face = obj->meshes[--mesh]->faces.size() - 1;
          }
          face -= n;
        }

        void adv(int n) {
          if (n > 0) {
            fwd((size_t)n);
          } else if (n < 0) {
            rev((size_t)-n);
          }
        }

        FaceIter operator++(int) { FaceIter tmp = *this; fwd(1); return tmp; }
        FaceIter operator+(int v) { FaceIter tmp = *this; adv(v); return tmp; }
        FaceIter &operator++() { fwd(1); return *this; }
        FaceIter &operator+=(int v) { adv(v); return *this; }

        FaceIter operator--(int) { FaceIter tmp = *this; rev(1); return tmp; }
        FaceIter operator-(int v) { FaceIter tmp = *this; adv(-v); return tmp; }
        FaceIter &operator--() { rev(1); return *this; }
        FaceIter &operator-=(int v) { adv(-v); return *this; }

        typename super::difference_type operator-(const FaceIter &other) const {
          CARVE_ASSERT(obj == other.obj);
          if (mesh == other.mesh) return face - other.face;

          size_t m = 0;
          for (size_t i = std::min(mesh, other.mesh) + 1; i < std::max(mesh, other.mesh); ++i) {
            m += obj->meshes[i]->faces.size();
          }

          if (mesh < other.mesh) {
            return -(obj->meshes[mesh]->faces.size() - face +
                     m +
                     other.face);
          } else {
            return +(obj->meshes[other.mesh]->faces.size() - other.face +
                     m +
                     face);
          }
        }

        bool operator==(const FaceIter &other) const {
          return obj == other.obj && mesh == other.mesh && face == other.face;
        }
        bool operator!=(const FaceIter &other) const {
          return !(*this == other);
        }
        bool operator<(const FaceIter &other) const {
          CARVE_ASSERT(obj == other.obj);
          return mesh < other.mesh || (mesh == other.mesh && face < other.face);
        }
        bool operator>(const FaceIter &other) const {
          return other < *this;
        }
        bool operator<=(const FaceIter &other) const {
          return !(other < *this);
        }
        bool operator>=(const FaceIter &other) const {
          return !(*this < other);
        }

        face_t *operator*() const {
          return obj->meshes[mesh]->faces[face];
        }
      };

      FaceIter faceBegin() { return FaceIter(this, 0, 0); }
      FaceIter faceEnd() { return FaceIter(this, meshes.size(), 0); }

      MeshSet(const std::vector<typename vertex_t::vector_t> &points,
              size_t n_faces,
              const std::vector<int> &face_indices) {
        vertex_storage.reserve(points.size());
        std::vector<face_t *> faces;
        faces.reserve(n_faces);
        for (size_t i = 0; i < points.size(); ++i) {
          vertex_storage.push_back(vertex_t(points[i]));
        }

        std::vector<carve::mesh::Vertex<3> *> v;
        size_t p = 0;
        for (size_t i = 0; i < n_faces; ++i) {
          const size_t N = face_indices[p++];
          v.clear();
          v.reserve(N);
          for (size_t j = 0; j < N; ++j) {
            v.push_back(&vertex_storage[face_indices[p++]]);
          }
          faces.push_back(new carve::mesh::Face<3>(v.begin(), v.end()));
        }
        CARVE_ASSERT(p == face_indices.size());
        carve::mesh::Mesh<3>::create(faces.begin(), faces.end(), meshes);
      }

      aabb_t getAABB() const {
        aabb_t result;
        if (meshes.size()) {
          result = meshes[0]->getAABB();
          for (size_t i = 1; i < meshes.size(); ++i) {
            result.unionAABB(meshes[i]->getAABB());
          }
        }
        return result;
      }

      MeshSet(std::vector<vertex_t> &_vertex_storage,
              std::vector<mesh_t *> &_meshes) {
        vertex_storage.swap(_vertex_storage);
        meshes.swap(_meshes);

        for (size_t i = 0; i < meshes.size(); ++i) {
          meshes[i]->meshset = this;
        }
      }

      // This constructor consolidates and rewrites vertex pointers in
      // each mesh, repointing them to local storage.
      MeshSet(std::vector<mesh_t *> &_meshes) {
        meshes.swap(_meshes);
        std::unordered_map<vertex_t *, size_t> vert_idx;

        for (size_t m = 0; m < meshes.size(); ++m) {
          mesh_t *mesh = meshes[m];
          CARVE_ASSERT(mesh->meshset == NULL);
          mesh->meshset = this;
          for (size_t f = 0; f < mesh->faces.size(); ++f) {
            face_t *face = mesh->faces[f];
            edge_t *edge = face->edge;
            do {
              vert_idx[edge->vert] = 0;
              edge = edge->next;
            } while (edge != face->edge);
          }
        }

        vertex_storage.reserve(vert_idx.size());
        for (typename std::unordered_map<vertex_t *, size_t>::iterator i = vert_idx.begin(); i != vert_idx.end(); ++i) {
          (*i).second = vertex_storage.size();
          vertex_storage.push_back(*(*i).first);
        }

        for (size_t m = 0; m < meshes.size(); ++m) {
          mesh_t *mesh = meshes[m];
          for (size_t f = 0; f < mesh->faces.size(); ++f) {
            face_t *face = mesh->faces[f];
            edge_t *edge = face->edge;
            do {
              size_t i = vert_idx[edge->vert];
              edge->vert = &vertex_storage[i];
              edge = edge->next;
            } while (edge != face->edge);
          }
        }
      }

      MeshSet *clone() const {
        std::vector<vertex_t> r_vertex_storage = vertex_storage;
        std::vector<mesh_t *> r_meshes;
        for (size_t i = 0; i < meshes.size(); ++i) {
          r_meshes.push_back(meshes[i]->clone(&vertex_storage[0], &r_vertex_storage[0]));
        }
        return new MeshSet(r_vertex_storage, r_meshes);
      }

      ~MeshSet() {
        for (size_t i = 0; i < meshes.size(); ++i) {
          delete meshes[i];
        }
      }
    };

  }
  mesh::MeshSet<3> *meshFromPolyhedron(const poly::Polyhedron *, int manifold_id);
  poly::Polyhedron *polyhedronFromMesh(const mesh::MeshSet<3> *, int manifold_id);
};

#include <carve/mesh_impl.hpp>
