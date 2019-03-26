/** BlobTools
 */  
#ifndef WIRECELL_IMG_BLOBTOOLS
#define WIRECELL_IMG_BLOBTOOLS

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>

#include "WireCellIface/IAnodePlane.h"
#include "WireCellIface/IBlobSet.h"


namespace WireCell {
    namespace Img {

        namespace chan_wire_blob {

            // Each node identifies a Channel, a Wire or a Blob
            struct node_t {
                char ntype;      // 'c' for channel, 'w' wire, 'b' blob.
                int index;       // b's index or a channel/wire ident.
                float value;     // c's charge value, o.w. zero.
            };

        
            typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, node_t> graph_t;
            typedef boost::graph_traits<graph_t>::vertex_descriptor vertex_t;

            // Fill channel-wire-blob graph.  Return number of blobs
            size_t fill(graph_t& graph, IAnodePlane::pointer anode,
                        const IBlobSet::vector& blobsets);

        }

    }  // Img

}  // WireCell


#endif /* WIRECELL_IMG_BLOBTOOLS */
