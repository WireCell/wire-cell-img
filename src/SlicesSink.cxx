#include "WireCellImg/SlicesSink.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Units.h"

WIRECELL_FACTORY(SlicesSink, WireCell::Img::SlicesSink,
                 WireCell::ISliceFrameSink, WireCell::IConfigurable)


using namespace WireCell;
using namespace std;

Img::SlicesSink::SlicesSink()
{
}

Img::SlicesSink::~SlicesSink()
{
}

WireCell::Configuration Img::SlicesSink::default_configuration() const
{
    Configuration cfg;

    cfg["verbose"] = false;

    return cfg;
}

void Img::SlicesSink::configure(const WireCell::Configuration& cfg)
{
    m_cfg = cfg;
}

bool Img::SlicesSink::operator()(const ISliceFrame::pointer& sf)
{
    if (!sf) {
        cerr << "sink slices EOS\n";
        return true;
    }

    bool verbose = get(m_cfg, "verbose", false);

    auto slices = sf->slices();

    if (verbose) {
        cerr << "sink slices #" << sf->ident()
             << " @" << sf->time()/units::s << "s with n=" << slices.size() << endl;
    }
    for (auto slice : slices) {
        auto cvmap = slice->activity();
        double qtot = 0;
        for (const auto &cv : cvmap) {
            qtot += cv.second;
        }

        if (verbose) {
            cerr << "\t#" << slice->ident()
                 << " t=" << slice->start()/units::ms << "ms + " << slice->span()/units::us
                 << "us with #ch=" << cvmap.size() << " qtot=" << qtot
                 << endl;
        }
    }
    return true;
}


