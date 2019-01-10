#include "WireCellImg/SumSlice.h"
#include "WireCellUtil/NamedFactory.h"

#include "WireCellIface/FrameTools.h" // fixme: *still* need to move this out of iface...


WIRECELL_FACTORY(SumSlicer, WireCell::Img::SumSlicer,
                 WireCell::IFrameSlicer, WireCell::IConfigurable)

WIRECELL_FACTORY(SumSlices, WireCell::Img::SumSlices,
                 WireCell::IFrameSlices, WireCell::IConfigurable)

using namespace std;
using namespace WireCell;


Img::SumSliceBase::SumSliceBase() : m_tick_span(4) , m_tag("") { }
Img::SumSliceBase::~SumSliceBase() { }
Img::SumSlicer::~SumSlicer() { }
Img::SumSlices::~SumSlices() { }

WireCell::Configuration Img::SumSliceBase::default_configuration() const
{
    Configuration cfg;

    // Name of an IAnodePlane from which we can resolve channel ident to IChannel
    cfg["anode"] = "";

    // Number of ticks over which each output slice should sum
    cfg["tick_span"] = m_tick_span;

    // If given, use tagged traces.  Otherwise use all traces.
    cfg["tag"] = m_tag;

    return cfg;
}


void Img::SumSliceBase::configure(const WireCell::Configuration& cfg)
{
    m_anode = Factory::find_tn<IAnodePlane>(cfg["anode"].asString()); // throws
    m_tick_span = get(cfg, "tick_span", m_tick_span);
    m_tag = get<std::string>(cfg, "tag", m_tag);
}

// just a bag of data.  fixme: could maybe be promoted to SimpleSlice under iface/
namespace WireCell {
    namespace Img {
        class SumSlice : public ISlice {
            IFrame::pointer m_frame;
            map_t m_activity;
            int m_ident;
            double m_start, m_span;
        public:
            SumSlice(const IFrame::pointer& frame, int ident, double start, double span)
                : m_frame(frame), m_ident(ident), m_start(start), m_span(span) { }

            IFrame::pointer frame() const {return m_frame;}

            int ident() const { return m_ident; }
            double start() const { return m_start; }
            double span() const { return m_span; }
            map_t activity() const { return m_activity; }

            // These methods are not part of the ISlice interface and may be
            // used prior to interment in the ISlice::pointer.

            void sum(const IChannel::pointer& ch, value_t val) { m_activity[ch] += val; }
        };

        // simple collection
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
    }
}

void Img::SumSliceBase::slice(const IFrame::pointer& in, slice_map_t& svcmap)
{
    const double tick = in->tick();
    const double span = tick * m_tick_span;

    for (auto trace : FrameTools::tagged_traces(in, m_tag)) {
        const int tbin = trace->tbin();
        const int chid = trace->channel();
        IChannel::pointer ich = m_anode->channel(chid);
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
                svcmap[slicebin] = s = new Img::SumSlice(in, slicebin, start, span);
            }
            s->sum(ich, q);
        }
    }
}


bool Img::SumSlicer::operator()(const input_pointer& in, output_pointer& out)
{
    out = nullptr;
    if (!in) {
        return true;            // eos
    }

    // Slices will be sparse in general.  Index by a "slice bin" number
    slice_map_t svcmap;
    slice(in, svcmap);

    // intern
    ISlice::vector islices;
    for (auto sit : svcmap) {
        auto s = sit.second;
        islices.push_back(ISlice::pointer(s));
    }
    out = make_shared<Img::SumSliceFrame>(islices, in->ident(), in->time());

    return true;
}



bool Img::SumSlices::operator()(const input_pointer& in, output_queue& slices)
{
    if (!in) {
        return true;            // eos
    }

    // Slices will be sparse in general.  Index by a "slice bin" number
    slice_map_t svcmap;
    slice(in, svcmap);

    // intern
    for (auto sit : svcmap) {
        auto s = sit.second;
        slices.push_back(ISlice::pointer(s));
    }

    return true;
}

