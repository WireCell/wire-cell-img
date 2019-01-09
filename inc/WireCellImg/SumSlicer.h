/* Uniformly slice a frame by charge summation */

#ifndef WIRECELLIMG_SUMSLICER
#define WIRECELLIMG_SUMSLICER

#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IFrameSlicer.h"

#include <string>

namespace WireCell {
    namespace Img {

        class SumSlicer : public IFrameSlicer, public IConfigurable {
        public:
            SumSlicer();
            virtual ~SumSlicer();

            // IConfigurable
            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

            // IFrameSlicer
            bool operator()(const input_pointer& in, output_pointer& out);

        private:

            int m_tick_span;
            std::string m_tag;
        };
    }
}

#endif
