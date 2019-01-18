/** Make strips based on channel index.

    This component is "naive" because it does not consider dead channels or
    other real world detector pathology.

 */
#ifndef WIRECELLIMG_NAIEVESTRIPPER
#define WIRECELLIMG_NAIEVESTRIPPER

#include "WireCellIface/ISliceStripper.h"
#include "WireCellIface/IConfigurable.h"

namespace WireCell {
    namespace Img {

        class NaiveStripper : public ISliceStripper, public IConfigurable {
        public:
            virtual ~NaiveStripper();

            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

            virtual bool operator()(const input_pointer& in, output_pointer& out);
        private:

            size_t m_gap;

        };
    }
}


#endif

