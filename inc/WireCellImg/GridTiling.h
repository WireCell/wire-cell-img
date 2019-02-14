/** This tiling algorithm makes use of RayGrid for the heavy lifting.
 *
 * It handles one face of a given anode plane.
 *
 * It does not "know" about dead channels.  If your detector has them
 * you may place a component upstream which artifically inserts
 * non-zero signal on dead channels in slice input here.
 */
#ifndef WIRECELLIM_GRIDTILING
#define WIRECELLIM_GRIDTILING

#include "WireCellIface/ITiling.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IAnodePlane.h"
#include "WireCellIface/IAnodeFace.h"

namespace WireCell {
    namespace Img {

        class GridTiling : public ITiling, public IConfigurable {
        public:
            GridTiling();
            virtual ~GridTiling();
            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;


            virtual bool operator()(const input_pointer& slice, output_pointer& blobset);

        private:
            
            IAnodePlane::pointer m_anode;
            IAnodeFace::pointer m_face;
            
        };
    }
}

#endif
