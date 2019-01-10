#include "WireCellImg/NaiveStripper.h"

#include "WireCellUtil/NamedFactory.h"


WIRECELL_FACTORY(NaiveStripper, WireCell::Img::NaiveStripper,
                 WireCell::ISliceStripper, WireCell::IConfigurable)


using namespace WireCell;

Img::NaiveStripper::~NaiveStripper()
{
}

WireCell::Configuration Img::NaiveStripper::default_configuration() const
{
    Configuration cfg;

    // The number of channels to be lacking any activity in a slice to
    // be considere a gap and leading to a new strip.
    cfg["gap"] = 1;

    return cfg;
}


void Img::NaiveStripper::configure(const WireCell::Configuration& cfg)
{
    m_gap = get(cfg, "gap", 1);
}

class NaiveStrip : public IStrip {
    int m_ident;
    vector_t m_values;

public:
    NaiveStrip(int ident) : m_ident(ident) {}

    int ident() const { return m_ident; }
    vector_t values() const { return m_values; }

    // these methods may be used prior to internment into IStrip::pointer

    void append(IChannel::pointer ich, value_t value) {
        m_values.push_back(make_pair(ich, value));
    }

};
class NaiveStripSet : public IStripSet {
    int m_ident;
    IStrip::vector m_strips;

public:

    NaiveStripSet(int ident) : m_ident(ident) {}
    int ident() const { return m_ident; }
    IStrip::vector strips() const { return m_strips; }
    
    // use before interning

    void push_back(const IStrip::pointer& s) { m_strips.push_back(s); }
    size_t size() const { return m_strips.size(); }

};


bool Img::NaiveStripper::operator()(const input_pointer& slice, output_pointer& out)
{
    out = nullptr;
    if (!slice) {
        return true;            // eos
    }

    NaiveStripSet* nss = new NaiveStripSet(slice->ident());

    // Ordered indexing of slice's channel values by channel 
    typedef std::map<size_t, ISlice::pair_t> chval_index_t;
    typedef std::unordered_map<int, chval_index_t> plane_chval_index_t;
    plane_chval_index_t crazy;
    for (const auto& cv : slice->activity()) {
        auto ichan = cv.first;
        const auto value = cv.second;
        const auto pid = ichan->planeid();
        crazy[pid.ident()][ichan->index()] = make_pair(ichan, value);
    }

    // Now unroll the per-plane, ordered index
    for (const auto& pchv : crazy) {
        const auto& indch = pchv.second;
        NaiveStrip* current_strip = nullptr;
        size_t last_ind = 0;
        for (const auto& ind_cv : indch) {
            const size_t this_ind = ind_cv.first;
            if (!current_strip or this_ind - last_ind > m_gap) {
                current_strip = new NaiveStrip(nss->size());
                nss->push_back(IStrip::pointer(current_strip));
            }
            const auto& cv = ind_cv.second;
            current_strip->append(cv.first, cv.second);
        }
    }

    out = IStripSet::pointer(nss);
    return true;
}
