#include "WireCellImg/BlobSetSync.h"


#include "WireCellUtil/NamedFactory.h"

#include <iostream>

WIRECELL_FACTORY(BlobSetSync, WireCell::Img::BlobSetSync,
                 WireCell::IBlobSetFanin, WireCell::IConfigurable)
using namespace WireCell;

Img::BlobSetSync::BlobSetSync()
    : m_multiplicity(0)
{
}

Img::BlobSetSync::~BlobSetSync()
{
}


WireCell::Configuration Img::BlobSetSync::default_configuration() const
{
    Configuration cfg;
    cfg["multiplicity"] = (int)m_multiplicity;
    return cfg;
}

void Img::BlobSetSync::configure(const WireCell::Configuration& cfg)
{
    int m = get<int>(cfg, "multiplicity", (int)m_multiplicity);
    if (m<=0) {
        THROW(ValueError() << errmsg{"FrameFanin multiplicity must be positive"});
    }
    m_multiplicity = m;
}

std::vector<std::string>  Img::BlobSetSync::input_types()
{
    const std::string tname = std::string(typeid(input_type).name());
    std::vector<std::string> ret(m_multiplicity, tname);
    return ret;

}

bool Img::BlobSetSync::operator()(const input_vector& invec, output_pointer& out)
{
    // This doesn't really do much.
    size_t n = invec.size();
    auto vp = new IBlobSet::vector(n);
    for (size_t ind=0; ind<n; ++ind) {
        (*vp)[ind] = invec[ind];
    }
    out = IBlobSet::shared_vector(vp);
    return true;
}

            
