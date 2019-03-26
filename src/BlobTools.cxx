#include "WireCellImg/BlobTools.h"

using namespace WireCell;
using namespace WireCell::Img::chan_wire_blob;


// little helper for loading wires into graph without duplicating.
struct wire_node_adder_t {
    graph_t& g;
    std::unordered_map<int, vertex_t> wid2vtx;
    vertex_t operator()(const IWire::pointer iwire) {
        const int wid = iwire->ident();
        auto it = wid2vtx.find(wid);
        if (it != wid2vtx.end()) {
            return it->second;
        }
        vertex_t wnode = boost::add_vertex(g);
        wid2vtx[wid] = wnode;
        g[wnode].ntype = 'w';
        g[wnode].index = wid;
        g[wnode].value = 0.0;
        return wnode;
    }
};

// Fill channel-wire-blob graph.
size_t fill(graph_t& graph, IAnodePlane::pointer anode,
            const IBlobSet::vector& blobsets)
{
    wire_node_adder_t wire_node_adder{graph};

    // Load in channel vertices from slice and channel<-->wire edges
    auto islice = blobsets[0]->slice();
    for (const auto& chv : islice->activity()) {
        auto ich = chv.first;
        auto chid = ich->ident();
        vertex_t chnode = boost::add_vertex(graph);
        graph[chnode].ntype = 'c';
        graph[chnode].index = chid;
        graph[chnode].value = chv.second;

        for (const auto& iwire : ich->wires()) {
            vertex_t wnode = wire_node_adder(iwire);
            add_edge(chnode, wnode, graph);
        }
    }

    // Load in blob vertices and blob <--> wire edges
    

    int blob_index = 0;
    for (auto& bs: blobsets) {
        int face = bs->face();
        auto iface = anode->face(face);
        auto wire_planes = iface->planes();
        for (const auto& iblob : bs->blobs()) {
            vertex_t bnode = boost::add_vertex(graph);
            graph[bnode].ntype = 'b';
            graph[bnode].index = blob_index;
            graph[bnode].value = 0.0;
            ++blob_index;

            const auto& shape = iblob->shape();
            for (const auto& strip : shape.strips()) {
                // FIXME: need a way to encode this convention!
                int iplane = strip.layer - 2;
                if (iplane < 0) {
                    continue; 
                }
                const auto& wires = wire_planes[iplane]->wires();
                for (int wip=strip.bounds.first; wip < strip.bounds.second; ++wip) {
                    auto iwire = wires[wip];
                    vertex_t wnode = wire_node_adder(iwire);
                    boost::add_edge(bnode, wnode, graph);
                }
            }
        }
    }

    return blob_index;
}

