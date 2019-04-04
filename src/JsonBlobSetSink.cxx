#include "WireCellImg/JsonBlobSetSink.h"

#include "WireCellUtil/Units.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/NamedFactory.h"

#include <fstream>
#include <iostream>             // debug


WIRECELL_FACTORY(JsonBlobSetSink, WireCell::Img::JsonBlobSetSink,
                 WireCell::IBlobSetSink, WireCell::IConfigurable)


using namespace WireCell;

Img::JsonBlobSetSink::JsonBlobSetSink()
    : m_drift_speed(1.6*units::mm/units::us)
    , m_filename("blobs-%02d.json")
    , m_face(0)
{
}
Img::JsonBlobSetSink::~JsonBlobSetSink()
{
}


void Img::JsonBlobSetSink::configure(const WireCell::Configuration& cfg)
{
    m_face = get(cfg, "face", m_face);
    m_drift_speed = get(cfg, "drift_speed", m_drift_speed);
    m_filename = get(cfg, "filename", m_filename);
}

WireCell::Configuration Img::JsonBlobSetSink::default_configuration() const
{
    Configuration cfg;

    // File name for output files.  A "%d" will be filled in with blob
    // set ident number.
    cfg["filename"] = m_filename;
    // Set to -1 to not apply any face filter, otherwise only consider matching face number
    cfg["face"] = m_face;
    cfg["drift_speed"] = m_drift_speed;
    return cfg;
}

bool Img::JsonBlobSetSink::operator()(const IBlobSet::pointer& bs)
{
    if (!bs) {
        std::cerr << "JsonBlobSetSink: eos\n";
        return true;
    }

    const auto& blobs = bs->blobs();
    if (blobs.empty()) {
        std::cerr << "JsonBlobSetSink: no blobs\n";
        return true;
    }

    auto slice = blobs[0]->slice();
    auto frame = slice->frame();
    const double start = slice->start();
    const double time = frame->time();
    const double x = (start-time)*m_drift_speed;



    if (blobs.empty()) {
        //std::cerr << "JsonBlobSetSink: no input blobs\n";
        return true;
    }

    std::cerr << "JsonBlobSetSink: frame:"<<frame->ident() <<", slice:"<<slice->ident()
              << " set:" << bs->ident()
              << " time=" << time/units::ms << "ms, start="<<start/units::ms << "ms"
              << " x=" << x
              << " nblobs=" << blobs.size()
              << std::endl;

    //const double span = slice->span();
    //const double dx = span/m_drift_speed;

    Json::Value jblobs = Json::arrayValue;


    for (const auto& iblob: blobs) {
        if (m_face >= 0 and m_face != iblob->face()->ident()) {
            continue;           // filter
        }
        const auto& coords = iblob->face()->raygrid();
        const auto& blob = iblob->shape();
        Json::Value jcorners = Json::arrayValue;        
        for (const auto& corner : blob.corners()) {
            Json::Value jcorner = Json::arrayValue;
            auto pt = coords.ray_crossing(corner.first, corner.second);
            jcorner.append(x);
            jcorner.append(pt.y());
            jcorner.append(pt.z());
            jcorners.append(jcorner);
        }
        Json::Value jblob;
        jblob["points"] = jcorners;
        jblob["values"]["charge"] = iblob->value();
        jblob["values"]["uncert"] = iblob->uncertainty();
        jblob["values"]["ident"] = iblob->ident();
        jblob["values"]["inset"] = jblobs.size();
        //std::cerr << jblob << std::endl;
        jblobs.append(jblob);
    }


    Json::Value top;
    //top["points"] = points;
    top["blobs"] = jblobs;

    std::string fname = m_filename;
    if (m_filename.find("%") != std::string::npos) {
        fname = String::format(m_filename, bs->ident());
    }
    std::ofstream fstr(fname);
    fstr << top;


    return true;    
}

