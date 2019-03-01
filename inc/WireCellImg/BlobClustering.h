/** Cluster blobs.

    This takes a stream of IBlobSets and produces a stream of IClusters.

    It assumes each blob set represents all blobs found in one time
    slice and that blob sets are delivered in time order.  Gaps in
    time between blob sets will be determined by the ident of the
    slice from the blob set.  Clusters will not span a gap.  Likewise,
    when an EOS is encountered, all clusters are flushed to the output
    queue.

    Clustering is performed by constructing a graph and calculating
    the connected subgraphs.

    Note, that input blob sets and blobs are held between calls (via
    their shared pointers).

 */

#ifndef WIRECELLIMG_BLOBCLUSTERING
#define WIRECELLIMG_BLOBCLUSTERING

#include "WireCellIface/IClustering.h"
#include "WireCellIface/IConfigurable.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>

#include <unordered_map>

namespace WireCell {
    namespace Img {

        class BlobClustering : public IClustering, public IConfigurable{
        public:
            BlobClustering();
            virtual ~BlobClustering();

            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

            virtual bool operator()(const input_pointer& blobset, output_queue& clusters) ;

        private:
            // User may configure how many slices can go missing
            // before breaking the clusters.  Default is 1.0, slices
            // must be adjacent in time or clusters will flush.
            double m_spans;

            // Mostly to access previous slice time to judge gap
            IBlobSet::pointer m_last_bs;

            struct node_t {
                IBlob::pointer iblob;
            };

            // The graph type.  Hold blob  as vertex property.
            typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, node_t> graph_t;
            typedef boost::graph_traits<graph_t>::vertex_descriptor vertex_t;
            std::unordered_map<int, vertex_t> m_ident2vertex;
            //std::unordered_map<vertex_t, IBlob::pointer> m_vertex2iblob;
            // boost graphs are such unfriendly things
            
            // Find and maybe add vertex in graph corresponding to blob.
            vertex_t vertex(const IBlob::pointer& iblob);
            

            graph_t m_graph;
            // internal methods

            // flush graph to output queue
            void flush(output_queue& clusters);

            // Add the newbs to the graph.  Return true if a flush is needed (eg, because of a gap)
            bool graph_bs(const input_pointer& newbs);

            // return true if a gap exists between the slice of newbs and the last bs.
            bool judge_gap(const input_pointer& newbs);

            // Blob set must be kept, this saves them.
            void intern(const input_pointer& newbs);

        };
    }
}

#endif
