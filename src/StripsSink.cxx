#include "WireCellImg/StripsSink.h"

#include "WireCellUtil/NamedFactory.h"

#include <iostream>

WIRECELL_FACTORY(StripsSink, WireCell::Img::StripsSink,
                 WireCell::IStripSetSink)

using namespace WireCell;

Img::StripsSink::~StripsSink()
{
}

bool Img::StripsSink::operator()(const IStripSet::pointer& ss)
{
    if (!ss) {
        std::cerr << "Img::StripsSink: EOS\n";
        return true;
    }

    const auto strips = ss->strips();

    std::cerr << "StripSet #" << ss->ident() << " with " << strips.size() << std::endl;
    return true;
}
