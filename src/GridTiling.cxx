#include "WireCellImg/GridTiling.h"

#include "WireCellUtil/RayTiling.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellIface/SimpleBlob.h"

WIRECELL_FACTORY(GridTiling, WireCell::Img::GridTiling,
                 WireCell::ITiling, WireCell::IConfigurable)

using namespace WireCell;
using namespace WireCell::RayGrid;


Img::GridTiling::GridTiling()
    : m_blobs_seen(0)
    , l(Log::logger("img"))
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



bool Img::GridTiling::operator()(const input_pointer& slice, output_pointer& out)
{
    out = nullptr;
    if (!slice) {
        m_blobs_seen = 0;
        SPDLOG_LOGGER_TRACE(l, "GridTiling: EOS");
        return true;            // eos
    }

    const int sbs_ident = slice->ident();
    SimpleBlobSet* sbs = new SimpleBlobSet(sbs_ident, slice);
    out = IBlobSet::pointer(sbs);

    const int nlayers = m_face->nplanes()+2;
    std::vector< std::vector<Activity::value_t> > measures(nlayers);
    measures[0].push_back(1);   // assume first to layers in RayGrid::Coordinates
    measures[1].push_back(1);   // are for horiz/vert bounds

    const auto face = m_face->ident();
    auto chvs = slice->activity();
    if (chvs.empty()) {
        SPDLOG_LOGGER_TRACE(l,"GridTiling: face:{} slice:{} no activity", face,  slice->ident());
        return true;
    }

    const int nactivities = slice->activity().size();
    int total_activity=0;
    if (nactivities < m_face->nplanes()) {
        SPDLOG_LOGGER_TRACE(l,"GridTiling: too few activities given");
        return true;
    }

    for (const auto& chv : slice->activity()) {
        for (const auto& wire : chv.first->wires()) {
            if (wire->planeid().face() != face) {
                continue;
            }
            const int pit_ind = wire->index();
            const int layer = 2 + wire->planeid().index();
            auto& m = measures[layer];
            if (pit_ind < 0) {
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
        SPDLOG_LOGGER_TRACE(l,"GridTiling: {} no activity", slice->ident());
        // fixme: need to send empty IBlob else we create EOS
        return true;
    }
    size_t nactive_layers = 0;
    for (const auto& blah : measures) {
        if (blah.empty()) { continue; }
        ++nactive_layers;
    }
    if (nactive_layers != measures.size()) {
        return true;
    }

    activities_t activities;
    for (int layer = 0; layer<nlayers; ++layer) {
        auto& m = measures[layer];
        Activity activity(layer, {m.begin(), m.end()});
        activities.push_back(activity);
    }

    auto blobs = make_blobs(m_face->raygrid(), activities);

    const float blob_value = 0.0; // tiling doesn't consider particular charge
    for (const auto& blob : blobs) {
        SimpleBlob* sb = new SimpleBlob(m_blobs_seen++, blob_value, 0.0, blob, slice, m_face);
        sbs->m_blobs.push_back(IBlob::pointer(sb));
    }
    SPDLOG_LOGGER_TRACE(l,"GridTiling: found {} blobs in slice {}", sbs->m_blobs.size(), slice->ident());

    return true;
}

