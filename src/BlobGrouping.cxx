#include "WireCellImg/BlobGrouping.h"

#include "WireCellIface/SimpleCluster.h"

#include "WireCellUtil/NamedFactory.h"

#include <boost/graph/connected_components.hpp>

WIRECELL_FACTORY(BlobGrouping, WireCell::Img::BlobGrouping,
                 WireCell::IClusterFilter, WireCell::IConfigurable)

using namespace WireCell;

typedef std::unordered_map<WirePlaneLayer_t, cluster_indexed_graph_t> layer_graphs_t;

Img::BlobGrouping::BlobGrouping()
{
}

Img::BlobGrouping::~BlobGrouping()
{
}

void Img::BlobGrouping::configure(const WireCell::Configuration& cfg)
{
}

WireCell::Configuration Img::BlobGrouping::default_configuration() const
{
    WireCell::Configuration cfg;
    return cfg;
}



static
void fill_blob(layer_graphs_t& lgs,
               const cluster_indexed_graph_t& grind_in,
               IBlob::pointer iblob)
{
    for (auto wvtx : grind_in.neighbors(iblob)) {
        if (wvtx.code() != 'w') {
            continue;
        }
        auto iwire = std::get<IWire::pointer>(wvtx.ptr);
        auto layer = iwire->planeid().layer();
        auto& lg = lgs[layer];

        for (auto cvtx : grind_in.neighbors(iwire)) {
            if (cvtx.code() != 'c') {
                continue;
            }
            auto ich = std::get<IChannel::pointer>(cvtx.ptr);
            lg.edge(iblob, ich);
        }
    }
}


static
void fill_slice(cluster_indexed_graph_t& grind_out,
                const cluster_indexed_graph_t& grind_in,
                ISlice::pointer islice)
{
    layer_graphs_t lgs;

    for (auto other : grind_in.neighbors(islice)) {
        if (other.code() != 'b') {
            continue;
        }
        IBlob::pointer iblob = std::get<IBlob::pointer>(other.ptr);
        fill_blob(lgs, grind_in, iblob);
    }
    
    for (auto lgit : lgs) {

        auto& lgrind = lgit.second;
        auto groups = lgrind.groups();
        for (auto& group : groups) {
            IBlob::vector blobs;
            IChannel::vector chans;
            for (auto& v : group.second) {
                if (v.code() == 'b') {
                    blobs.push_back(std::get<IBlob::pointer>(v.ptr));
                    continue;
                }
                if (v.code() == 'c') {
                    chans.push_back(std::get<IChannel::pointer>(v.ptr));
                    continue;
                }
            }
            if (blobs.empty() or chans.empty()) {
                continue;                
            }
            IChannel::shared_vector ichv = std::make_shared<IChannel::vector>(chans.begin(), chans.end());
            for (auto& iblob : blobs) {
                grind_out.edge(iblob, ichv);
                grind_out.edge(islice, iblob);
            }
        }
    }
}


bool Img::BlobGrouping::operator()(const input_pointer& in, output_pointer& out)
{
    if (!in) {
        out = nullptr;
        return true;
    }
    
    cluster_indexed_graph_t grind_out, grind_in(in->graph());

    for (auto islice : oftype<ISlice::pointer>(grind_in)) {
        fill_slice(grind_out, grind_in, islice);
    }

    // don't lose b-b edges
    for (auto iblob : oftype<IBlob::pointer>(grind_in)) {
        if (!grind_out.has(iblob)) {
            continue;
        }
        for (auto nblob : grind_in.neighbors(iblob)) {
            if (nblob.code() != 'b') {
                continue;
            }
            if (!grind_out.has(nblob)) {
                continue;
            }
            // this adds both ways but only first "wins" due to setS edges.
            grind_out.edge(iblob, nblob); 
        }
    }
    
    out = std::make_shared<SimpleCluster>(grind_out.graph());    
    return true;
}

