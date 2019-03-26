#include "WireCellImg/BlobSolver.h"
#include "WireCellImg/BlobTools.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Exceptions.h"
#include "WireCellUtil/Ress.h"
#include "WireCellIface/SimpleBlob.h"

#include <iostream>

WIRECELL_FACTORY(BlobSolver, WireCell::Img::BlobSolver,
                 WireCell::IBlobSetPipeline, WireCell::IConfigurable)


using namespace WireCell;


Img::BlobSolver::BlobSolver()
{
}

Img::BlobSolver::~BlobSolver()
{
}

void Img::BlobSolver::configure(const WireCell::Configuration& cfg)
{
    m_anode = Factory::find_tn<IAnodePlane>(cfg["anode"].asString());
}

WireCell::Configuration Img::BlobSolver::default_configuration() const
{
    Configuration cfg;
    cfg["anode"] = "";          // user must set
    return cfg;
}

// In pointer is a 2-vector if IBlobSet.  Each set spans one one face
// in the same time slice.
bool Img::BlobSolver::operator()(const input_pointer& in, output_pointer& out)
{
    // Solve M=G*B for B, vector of blob charge, M measurement of
    // channel groups, G matrix of channel grouping.

    // Form G and M by looping over IBlobs in the input IBlobSets.
    // Follow like NaiveStriper but different.

    // Make a graph which connects channels to wires and then for each
    // blob, connect each wire in the blob to its neighbor at on
    // higher pitch.  Must take care to identify wire uniquely in the
    // context of 3 (MB) or 6 (for pD/DUNE) logical wire planes.  Then
    // ask graph for all connected subgraphs.  The wires or each
    // subgraph represent a "merged wire" (in WCP-speak) and the sum
    // of its channels represent one element in the M vector.  The
    // blobs caught up populate a row in the G matrix.

    using namespace WireCell::Img::chan_wire_blob;
    graph_t graph;

    const IBlobSet::vector& blobsets = *in;
    const size_t nblobs = fill(graph, m_anode, blobsets);


    // Heavy lifting to define strips (aka "merged wires") and their
    // associated blobs.  This is done asking for all connected
    // component subgraphs and then for each find the nodes that are
    // of type "blob" or "channel".
    std::unordered_map<vertex_t, int> subclusters;
    size_t num = boost::connected_components(graph, boost::make_assoc_property_map(subclusters));
    std::cerr << "Img::BlobSolver: found " << num << " stripes\n";

    // rows = "strips" (aka "merged wires"), cols = "which blobs in a given strip"
    const size_t nstrips = subclusters.size();


    // Now solving part.

    // Blobs may come in already with some solved charge, use that as initial values.
    Ress::vector_t initial = Ress::vector_t::Zero(nstrips);
    int blob_index = 0;
    for (const auto& bs : blobsets) {
        for (const auto& iblob: bs->blobs()) {
            initial(blob_index) = iblob->value();
        }
        ++blob_index;
    }

    Ress::matrix_t geometry = Ress::matrix_t::Zero(nstrips, nblobs);
    Ress::vector_t measured = Ress::vector_t::Zero(nstrips);

    for (auto& vci : subclusters) {
        vertex_t vtx = vci.first;
        char ntype = graph[vtx].ntype;
        const size_t strip_ind = vci.second;
        if (ntype == 'c') {     // channel
            measured(strip_ind) += graph[vtx].value;
        }
        else if (ntype == 'b') { // blob
            const size_t blob_ind = graph[vtx].index;
            geometry(strip_ind, blob_ind) = 1.0;
        }
        // else the vertex is a wire and we don't care about the connective tissue
    }    

    // M=G*B.
    // The blob vector spans the blob_index, 0..., nblobs-1
    // The G matrix has nclusters rows and nblobs columns.
    // The M matrix is ordered channel value
    // fixme: how to add measurement uncertainty?
    Ress::Params params;
    params.model = Ress::lasso;
    std::cerr << "BlobSolver: lambda=" << params.lambda << std::endl;
    Ress::vector_t solved = Ress::solve(geometry, measured, params, initial);


    // Finally, pack output, adding blob charge from solution
    
    auto vp = new IBlobSet::vector();
    {
        int blob_index = 0;
        for (auto& bs: *in) {
            SimpleBlobSet* sbs = new SimpleBlobSet(bs->ident(), bs->face(), bs->slice());
            for (const auto& iblob : bs->blobs()) {
                float blob_value = (float)solved(blob_index);
                float blob_uncert = 0.0; // fixme: where does this come from?
                SimpleBlob* sb = new SimpleBlob(blob_index++, blob_value, blob_uncert, iblob->shape());
                sbs->m_blobs.push_back(IBlob::pointer(sb));
            }
            vp->push_back(IBlobSet::pointer(sbs));
        }        
    }
    out = IBlobSet::shared_vector(vp);

    return true;
}




