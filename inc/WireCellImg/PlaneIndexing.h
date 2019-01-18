#ifndef WIRECELLIMG_PLANEINDEXING
#define WIRECELLIMG_PLANEINDEXING

#include "WireCellUtil/Point.h"
#include "WireCellUtil/ObjectArray2d.h"
#include "WireCellIface/IWire.h"

#include <boost/multi_array.hpp>

#include <unordered_map>
#include <vector>
#include <tuple>

namespace WireCell {
    namespace Img {

        // Bundle up per-plane, per-plane-pair and per-plane-trio parameters.
        class PlaneIndexing {
        public:
            // wire-in-plane index.
            typedef size_t wip_index_t;
            typedef std::pair<wip_index_t, wip_index_t> wip_pair_t;

            // identify a plane by its index in the vector given to the constructor.
            typedef size_t plane_index_t;

            // A pair of wires
            typedef std::pair<IWire::pointer, IWire::pointer> wire_pair_t;

            // A vector of a pair of wires
            typedef std::vector<wire_pair_t> wire_pair_vector_t;

            // A pair and triple of plane indices.
            typedef std::pair<plane_index_t, plane_index_t> plane_pair_t;
            typedef std::tuple<plane_index_t, plane_index_t, plane_index_t> plane_triple_t;

            // Used for 3-plane coefficients tensor type.
            typedef boost::multi_array<double, 3> tensor_t;
            
            // A 1D array of vectors.
            typedef std::vector<Vector> vector_array1d_t;
            // A 2D array of vectors.
            typedef ObjectArray2d<Vector> vector_array2d_t;
            // Note, the vectors are inhernetly 2D (projected into the
            // plane).  Some improvements could be made to use 2D
            // Vectors instead of 3D ones.  A 2D version of Point.h
            // could be written in support of this.

            // Construct a PlaneIndexing with a vector of pairs of IWires from
            // each plane.  The pair is expected to be wire 0 and wire 1 from
            // the plane.  The normal_axis says which Cartesian axis is normal
            // to the wire planes.  The index of planes in this input vector
            // and wire-in-plane indices are then used to perform fast
            // geometry calculations.
            PlaneIndexing(const wire_pair_vector_t& zero_one_wires, size_t normal_axis=0);

            // Return the zero crossing point for two planes.
            Vector zero_crossing(plane_pair_t plane) const;

            // Return the crossing point for a pair of planes an a paire of wires, one from each plane.
            Vector wire_crossing(const plane_pair_t& plane, const wip_pair_t& wips) const;

            // Return 3-way pitch location for two wires.  The ind tuple
            // should number the planes involved.  First two plane indicices
            // correspond to the two given wire-in-plane indices.  The pitch
            // location returned is measured in the third plane.
            double pitch_location(const plane_triple_t& ind, wip_index_t w1ind, wip_index_t w2ind) const;

        private:

            size_t m_nplanes;

            // Pitch magnitude for each plane
            std::vector<double> m_pitch_mag;

            // The unit vector in the pitch direction for each plane
            vector_array1d_t m_pitch_dir;

            // A point (center point) on wire 0 of each plane.
            vector_array1d_t m_center;

            // Zero-wires crossing points indexed by plane index pairs.
            // Symmetric array, diagonal is invalid.
            vector_array2d_t m_zero_crossing;
    
            // Element (l,m) holds a relative vector which jumps along wires
            // of plane l between crossing points of neighboring wires of
            // plane m.  Not symmectric, but diagonal is invalid.
            vector_array2d_t m_wire_jump;

            // See README.org for detail.
            tensor_t m_a, m_b, m_c;
        };



    }
}

#endif
