#include "WireCellImg/ClusterSink.h"
#include <boost/graph/graphviz.hpp>

#include "WireCellUtil/String.h"
#include "WireCellUtil/NamedFactory.h"

#include <fstream>
#include <sstream>
#include <functional>

WIRECELL_FACTORY(ClusterSink, WireCell::Img::ClusterSink,
                 WireCell::IClusterSink, WireCell::IConfigurable)

using namespace WireCell;


Img::ClusterSink::ClusterSink()
    : m_filename("")
{
}

Img::ClusterSink::~ClusterSink()
{
}

void Img::ClusterSink::configure(const WireCell::Configuration& cfg)
{
    m_filename = get(cfg, "filename", m_filename);
}

WireCell::Configuration Img::ClusterSink::default_configuration() const
{
    WireCell::Configuration cfg;
    cfg["filename"] = m_filename;
    return cfg;
}

typedef std::vector< std::function< std::string(const cluster_node_t& ptr) > > stringers_t;

std::string size_stringer(const cluster_node_t& n)
{
    size_t sp = std::get<size_t>(n.ptr);    
    std::stringstream ss;
    ss << n.code() << ":" << sp;
    return ss.str();
}

template <typename Type>
std::string scalar_stringer(const cluster_node_t& n)
{
    typename Type::pointer sp = std::get<typename Type::pointer>(n.ptr);
    std::stringstream ss;
    ss << n.code() << ":" << sp->ident();
    return ss.str();
}
template <typename Type>
std::string vector_stringer(const cluster_node_t& n)
{
    typename Type::shared_vector sv = std::get<typename Type::shared_vector>(n.ptr);
    std::stringstream ss;
    ss << n.code() << "#" << sv->size();
    return ss.str();
}

static std::string asstring(const cluster_node_t& n)
{
    // cwbsm
    stringers_t ss{
        size_stringer,
        scalar_stringer<IChannel>,
        scalar_stringer<IWire>,
        scalar_stringer<IBlob>,
        scalar_stringer<ISlice>,
        vector_stringer<IChannel>
    };
    const size_t ind = n.ptr.index();
    return ss[ind](n);
}

struct label_writer_t {
    const cluster_graph_t& g;
    template<class vdesc_t>
    void operator()(std::ostream& out, const vdesc_t v) const {
        out << "[label=\"" << asstring(g[v]) << "\"]";
    }
};

bool Img::ClusterSink::operator()(const ICluster::pointer& cluster)
{
    if (!cluster) {
        return true;
    }
    
    std::string fname = m_filename;
    if (fname.empty()) {
        return true;
    }

    if (m_filename.find("%") != std::string::npos) {
        fname = String::format(m_filename, cluster->ident());
    }
    std::ofstream out(fname.c_str());
    std::cerr << "Writing graphviz to " << fname << "\n";
    boost::write_graphviz(out, cluster->graph(), label_writer_t{cluster->graph()});

    return true;
}

