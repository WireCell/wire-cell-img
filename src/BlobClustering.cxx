#include "WireCellImg/BlobClustering.h"
#include "WireCellUtil/RayClustering.h"

#include "WireCellUtil/NamedFactory.h"

#include <boost/graph/connected_components.hpp>
#include <boost/graph/graphviz.hpp>

#include <iostream>

WIRECELL_FACTORY(BlobClustering, WireCell::Img::BlobClustering,
                 WireCell::IClustering, WireCell::IConfigurable)

using namespace WireCell;


Img::BlobClustering::BlobClustering()
    : m_spans(1.0), m_last_bs(nullptr)
{
}
Img::BlobClustering::~BlobClustering()
{
}

void Img::BlobClustering::configure(const WireCell::Configuration& cfg)
{
    m_spans = get(cfg, "spans", m_spans);
}

WireCell::Configuration Img::BlobClustering::default_configuration() const
{
    Configuration cfg;
    // A number multiplied to the span of the current slice when
    // determining it a gap exists between the next slice.  Default is
    // 1.0.  Eg, if set to 2.0 then a single missing slice won't be
    // grounds for considering a gap in the cluster.  A number less
    // than 1.0 will cause each "cluster" to consist of only one blob.
    cfg["spans"] = m_spans;
    return cfg;
}

void Img::BlobClustering::flush(output_queue& clusters)
{
    {
        auto e = edges(m_graph);
        if (e.first == e.second) {
            return;                 // empty
        }
    }

    std::unordered_map<vertex_t, int> subclusters;
    size_t num = boost::connected_components(m_graph, boost::make_assoc_property_map(subclusters));
    std::cerr << "BlobClustering: found " << num << " clusters at " << m_last_bs->ident() << std::endl;

    for (auto& p : subclusters) {
        vertex_t blobident = p.first;
        int clustnum = p.second;
        std::cerr << "CLUSTER: " << clustnum << ", BLOB: " << blobident << std::endl;
    }

    // fixme:
    // for (auto sg : subgraph(m_graph)) {
    //     auto scluster = new SimpleCluster(....);
    //     for (auto edge : sg) {
    //         auto iedge = convert(edge...);
    //         scluster->add_edge(iedge);
    //     }
    //     auto icluster = ICluster::pointer(scluster);
    //     clusters.push_back(icluster);
    // }


    // boost::write_graphviz(std::cout, m_graph,
    //                       [&] (auto& out, auto v) {
    //                           out << "[label=\"" << m_graph[v].iblob->ident() << "\"]";
    //                       },
    //                       [&] (auto& out, auto e) {
    //                           out << "[label=\"\"]";
    //                       });
    // std::cout << std::flush;

    m_graph.clear();
    m_ident2vertex.clear();
    //m_vertex2iblob.clear();
}

void Img::BlobClustering::intern(const input_pointer& newbs)
{
    m_last_bs = newbs;
}

bool Img::BlobClustering::judge_gap(const input_pointer& newbs)
{
    auto nslice = newbs->slice();
    auto oslice = m_last_bs->slice();

    const double dt = nslice->start() - oslice->start();
    const double epsilon = 1*units::ns;
    return std::abs(dt - m_spans*oslice->span()) > epsilon;
}

Img::BlobClustering::vertex_t Img::BlobClustering::vertex(const IBlob::pointer& iblob)
{
    // this is a little painful!

    int ident = iblob->ident();
    auto it = m_ident2vertex.find(ident);
    if (it == m_ident2vertex.end()) {
        auto vtx = boost::add_vertex(node_t{iblob}, m_graph);
        //auto vtx = boost::add_vertex(m_graph);
        //m_vertex2iblob[vtx] = iblob;
        m_ident2vertex[ident] = vtx;
        return vtx;
    }
    return it->second;
}

bool Img::BlobClustering::graph_bs(const input_pointer& newbs)
{
    if (!m_last_bs) {
        // need to wait for next one to do anything
        return false;
    }
    if (judge_gap(newbs)) {
        // nothing to do, but pass on that we hit a gap
        return true;
    }

    IBlob::vector iblobs1 = newbs->blobs();
    IBlob::vector iblobs2 = m_last_bs->blobs();

    RayGrid::blobs_t blobs1 = newbs->shapes();
    RayGrid::blobs_t blobs2 = m_last_bs->shapes();

    const auto beg1 = blobs1.begin();
    const auto beg2 = blobs2.begin();

    auto assoc = [&](RayGrid::blobref_t& a, RayGrid::blobref_t& b) {
                     int an = a - beg1;
                     int bn = b - beg2;

                     vertex_t av = vertex(iblobs1[an]);
                     vertex_t bv = vertex(iblobs2[bn]);

                     boost::add_edge(av, bv, m_graph);
                 };

    RayGrid::associate(blobs1, blobs2, assoc);

    return false;
}

bool Img::BlobClustering::operator()(const input_pointer& blobset, output_queue& clusters)
{
    if (!blobset) {             // eos
        flush(clusters);
        clusters.push_back(nullptr); // forward eos
        return true;
    }


    bool gap = graph_bs(blobset);
    if (gap) {
        flush(clusters);
        // note: flush fast to keep memory usage in this component
        // down and because in an MT job, downstream components might
        // benefit to start consuming clusters ASAP.  We do NOT want
        // to intern() the new blob set BEFORE a flush if there is a
        // gap because newbs is needed for next time and fush clears
        // the cache.
    }

    intern(blobset);

    return true;
}

