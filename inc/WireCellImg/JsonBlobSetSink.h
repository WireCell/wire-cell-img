#ifndef WIRECELLIMG_JSONBLOBSETSINK
#define WIRECELLIMG_JSONBLOBSETSINK

#include "WireCellIface/IBlobSetSink.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IAnodePlane.h"
#include "WireCellIface/IAnodeFace.h"
#include <vector>

namespace WireCell {
    namespace Img {             // fixme move to sio?

        class JsonBlobSetSink : public IBlobSetSink, public IConfigurable
        {
        public:
            JsonBlobSetSink() ;
            virtual ~JsonBlobSetSink() ;

            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

            virtual bool operator()(const IBlobSet::pointer& bs);

        private:
            
            IAnodePlane::pointer m_anode;
            IAnodeFace::pointer m_face;
            double m_drift_speed;
            std::string m_filename;
        };
    }
}
#endif
