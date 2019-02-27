#include "WireCellImg/GridTiling.h"

#include "WireCellUtil/RayTiling.h"
#include "WireCellUtil/NamedFactory.h"

WIRECELL_FACTORY(GridTiling, WireCell::Img::GridTiling,
                 WireCell::ITiling, WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::RayGrid;


Img::GridTiling::GridTiling()
{
}

Img::GridTiling::~GridTiling()
{

}

void Img::GridTiling::configure(const WireCell::Configuration& cfg)
{
    m_anode = Factory::find_tn<IAnodePlane>(cfg["anode"].asString());
    m_face = m_anode->face(cfg["face"].asInt());
}

WireCell::Configuration Img::GridTiling::default_configuration() const
{
    Configuration cfg;
    cfg["anode"] = "";          // user must set
    cfg["face"] = 0;            // the face number to focus on
    return cfg;
}


class SimpleBlob : public IBlob {
public:
    SimpleBlob(int ident, float value, float uncertainty, const RayGrid::Blob& shape)
        : m_ident(ident), m_value(value), m_uncertainty(uncertainty), m_shape(shape) { }
    virtual ~SimpleBlob(){}
        
    int ident() const { return m_ident; }
    
    float value() const { return m_value; }
    
    float uncertainty() const { return m_uncertainty; }
    
    const RayGrid::Blob& shape() const { return m_shape; }

private:
    int m_ident;
    float m_value;
    float m_uncertainty;
    RayGrid::Blob m_shape;
};



class SimpleBlobSet : public IBlobSet {
public:
    SimpleBlobSet(int ident, int face, const ISlice::pointer& slice)
        : m_ident(ident), m_face(face), m_slice(slice) { }
    virtual ~SimpleBlobSet()  {}

    virtual int ident() const { return m_ident; }
    virtual int face() const { return m_face; }

    virtual ISlice::pointer slice() const { return m_slice; }

    virtual IBlob::vector blobs() const { return m_blobs; }

    int m_ident, m_face;
    ISlice::pointer m_slice;
    IBlob::vector m_blobs;
};


bool Img::GridTiling::operator()(const input_pointer& slice, output_pointer& out)
{
    out = nullptr;
    if (!slice) {
        return true;            // eos
    }

    const int nlayers = m_face->nplanes()+2;
    std::vector< std::vector<Activity::value_t> > measures(nlayers);
    measures[0].push_back(1);   // assume first to layers in RayGrid::Coordinates
    measures[1].push_back(1);   // are for horiz/vert bounds

    const auto face = m_face->ident();
    auto chvs = slice->activity();
    if (chvs.empty()) {
        std::cerr << "GridTiling: face: "<<face<<", slice:" << slice->ident() << " no activity\n";
        return true;
    }

    const int nactivities = slice->activity().size();
    int total_activity=0;
    std::cerr << "GridTiling: face:"<<face<<", slice:" << slice->ident()
              << " t=" << slice->start()/units::ms << "ms + "
              << slice->span()/units::us << "us with "
              << nactivities << " activitie channels\n";
    if (nactivities < m_face->nplanes()) {
        // must have at least one activity per plane
        return true;
    }

    for (const auto& chv : slice->activity()) {
        std::cerr << "GridTiling: active chid=" << chv.first->ident()
                  << ", chind=" << chv.first->index()
                  << ", wpid=" << chv.first->planeid()
                  << ", val=" << chv.second
                  << " " << chv.first->wires().size() << " wires"
                  << "\n";
        for (const auto& wire : chv.first->wires()) {
            if (wire->planeid().face() != face) {
                std::cerr << "GridTiling:\tignoring wire "
                          << wire->ident()
                          << " seg " << wire->segment()
                          << " of " << wire->planeid()
                          << " not in face " << face << std::endl;
                continue;
            }
            const int pit_ind = wire->index();
            const int layer = 2 + wire->planeid().index();
            std::cerr << "GridTiling:\tusing wire "
                      << wire->ident()
                      << " seg " << wire->segment()
                      << " of " << wire->planeid()
                      << " in face " << face
                      << " L" << layer << " pit_ind=" << pit_ind
                      << std::endl;
            auto& m = measures[layer];
            if (pit_ind < 0) {
                std::cerr << "GridTiling:\twire with negative pitch index: " << pit_ind
                          << " in wire plane " << wire->planeid() 
                          << std::endl;
                continue;
            }
            if ((int)m.size() <= pit_ind) {
                m.resize(pit_ind+1, 0.0);
            }
            m[pit_ind] += 1.0;
            ++total_activity;
        }
    }

    if (!total_activity) {
        std::cerr << "GridTiling: slice " << slice->ident() << " no activity\n";
        // fixme: need to send empty IBlob else we create EOS
        return true;
    }
    size_t nactive_layers = 0;
    for (const auto& blah : measures) {
        if (blah.empty()) { continue; }
        ++nactive_layers;
    }
    if (nactive_layers != measures.size()) {
        std::cerr << "GridTiling: only " << nactive_layers
                  << " active layers out of " << measures.size() << " measures spanning n ray bins:";
        for (const auto& m : measures) {
            std::cerr << " " << m.size();
        }
        std::cerr << "\n";
        // fixme: need to send empty IBlob else we create EOS
        return true;
    }
    std::cerr << "GridTiling: accepted slice " << slice->ident() << " "<< total_activity << " hit wires\n";

    activities_t activities;
    for (int layer = 0; layer<nlayers; ++layer) {
        auto& m = measures[layer];
        Activity activity(layer, {m.begin(), m.end()});
        std::cerr << "GridTiling:\t" << activity << std::endl;
        activities.push_back(activity);
    }

    auto blobs = make_blobs(m_face->raygrid(), activities);
    // std::cerr << "GridTiling: slice " << slice->ident() << " found "<<blobs.size()<<" blobs\n";
    
    const int sbs_ident = slice->ident();
    SimpleBlobSet* sbs = new SimpleBlobSet(sbs_ident, m_face->ident(), slice);

    int blob_ident = 0;
    const float blob_value = 0.0; // tiling doesn't consider particular charge
    for (const auto& blob : blobs) {
        SimpleBlob* sb = new SimpleBlob(blob_ident, blob_value, 0.0, blob);
        sbs->m_blobs.push_back(IBlob::pointer(sb));
    }

    out = IBlobSet::pointer(sbs);
    return true;
}

