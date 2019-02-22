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
{
}
Img::JsonBlobSetSink::~JsonBlobSetSink()
{
}


void Img::JsonBlobSetSink::configure(const WireCell::Configuration& cfg)
{
    m_anode = Factory::find_tn<IAnodePlane>(cfg["anode"].asString());
    m_face = m_anode->face(cfg["face"].asInt());
    m_drift_speed = get(cfg, "drift_speed", m_drift_speed);
    m_filename = get(cfg, "filename", m_filename);
}

WireCell::Configuration Img::JsonBlobSetSink::default_configuration() const
{
    Configuration cfg;

    // File name for output files.  A "%d" will be filled in with blob
    // set ident number.
    cfg["filename"] = m_filename;
    cfg["anode"] = "";
    cfg["face"] = 0;
    cfg["drift_speed"] = m_drift_speed;
    return cfg;
}

bool Img::JsonBlobSetSink::operator()(const IBlobSet::pointer& bs)
{
    if (!bs) {
        std::cerr << "JsonBlobSetSink: eos\n";
        return true;
    }
    const auto& coords = m_face->raygrid();
    auto slice = bs->slice();
    auto frame = slice->frame();
    const double start = slice->start();
    const double time = frame->time();
    const double x = (start-time)*m_drift_speed;

    std::cerr << "BlobSet: frame:"<<frame->ident() <<", slide:"<<slice->ident()
              << " set:" << bs->ident()
              << " time=" << time/units::ms << "ms, start="<<start/units::ms << "ms"
              << " x=" << x << std::endl;

    //const double span = slice->span();
    //const double dx = span/m_drift_speed;

    Json::Value jblobs = Json::arrayValue;

    const auto& blobs = bs->blobs();
    if (blobs.empty()) {
        std::cerr << "JsonBlobSetSink: no input blobs\n";
        return true;
    }

    for (const auto& iblob: blobs) {
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
        jblob["values"]["number"] = jblobs.size();
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

