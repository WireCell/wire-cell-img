#include "WireCellImg/StripesSink.h"

#include "WireCellUtil/NamedFactory.h"

#include <iostream>

WIRECELL_FACTORY(StripesSink, WireCell::Img::StripesSink,
                 WireCell::IStripeSetSink)

using namespace WireCell;

Img::StripesSink::~StripesSink()
{
}

bool Img::StripesSink::operator()(const IStripeSet::pointer& ss)
{
    if (!ss) {
        std::cerr << "Img::StripesSink: EOS\n";
        return true;
    }

    const auto stripes = ss->stripes();

    std::cerr << "StripeSet #" << ss->ident() << " with " << stripes.size() << std::endl;
    return true;
}
