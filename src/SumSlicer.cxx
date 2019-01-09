#include "WireCellImg/SumSlicer.h"
#include "WireCellUtil/NamedFactory.h"

#include "WireCellIface/FrameTools.h" // fixme: *still* need to move this out of iface...


WIRECELL_FACTORY(SumSlicer, WireCell::Img::SumSlicer,
                 WireCell::IFrameSlicer, WireCell::IConfigurable)

using namespace std;
using namespace WireCell;


Img::SumSlicer::SumSlicer()
    : m_tick_span(4)
    , m_tag("")
{
}

Img::SumSlicer::~SumSlicer()
{
}

WireCell::Configuration Img::SumSlicer::default_configuration() const
{
    Configuration cfg;

    // Number of ticks over which each output slice should sum
    cfg["tick_span"] = m_tick_span;

    // If given, use tagged traces.  Otherwise use all traces.
    cfg["tag"] = m_tag;

    return cfg;
}


void Img::SumSlicer::configure(const WireCell::Configuration& cfg)
{
    m_tick_span = get(cfg, "tick_span", m_tick_span);
    m_tag = get<std::string>(cfg, "tag", m_tag);
}

// just a bag of data.  fixme: could maybe be promoted to SimpleSlice under iface/
class SumSlice : public ISlice {
    channel_value_map_t m_values;
    int m_ident;
    double m_start, m_span;
public:
    SumSlice(int ident, double start, double span)
        : m_ident(ident), m_start(start), m_span(span) { }

    int ident() const { return m_ident; }
    double start() const { return m_start; }
    double span() const { return m_span; }
    channel_value_map_t values() const { return m_values; }

    // These methods are not part of the ISlice interface and may be
    // used prior to interment in the ISlice::pointer.

    void sum(channel_ident_t ident, value_t val) { m_values[ident] += val; }
};

class SumSliceFrame : public ISliceFrame {

    ISlice::vector m_slices;
    int m_ident;
    double m_time;
    
public:
    SumSliceFrame(const ISlice::vector& islices, int ident, double time)
        : m_slices(islices), m_ident(ident), m_time(time) {}

    int ident() const { return m_ident; }
    double time() const { return m_time; }

    ISlice::vector slices() const { return m_slices; }

};
bool Img::SumSlicer::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {
        return true;            // eos
    }

    const double tick = in->tick();
    const double span = tick * m_tick_span;

    // Slices will be sparse in general.  Index by a "slice bin" number
    typedef std::map<size_t, SumSlice*> slice_map_t;
    slice_map_t svcmap;

    for (auto trace : FrameTools::tagged_traces(in, m_tag)) {
        const int tbin = trace->tbin();
        const int chid = trace->channel();
        const auto& charge = trace->charge();
        const size_t nq = charge.size();
        for (size_t qind=0; qind != nq; ++qind) {
            const auto q = charge[qind];
            if (q == 0.0) {
                continue;
            }
            size_t slicebin = (tbin+qind)/m_tick_span;
            auto s = svcmap[slicebin];
            if (!s) {
                const double start = slicebin * span; // thus relative to slice frame's time.
                svcmap[slicebin] = s = new SumSlice(slicebin, start, span);
            }
            s->sum(chid, charge[qind]);
        }
    }

    std::cerr << "Img::SumSlicer found " << svcmap.size() << " slices\n";

    // intern
    ISlice::vector islices;
    for (auto sit : svcmap) {
        auto s = sit.second;
        islices.push_back(ISlice::pointer(s));
    }
    out = make_shared<SumSliceFrame>(islices, in->ident(), in->time());

    return true;
}

