// A BlobSet filter which solves the matrix equation M=G*B for B, a
// vector of blob charge, M a vector of measured charge on strips
// ("merged wires" in WCP language) and G a connectivity matrix.
//
// An element of M is the sum of samples from all channels with
// contiguous coverage of some blobs in a wire plane.  Anode planes
// with wrapped wires must combine seed blobs which may have been
// found on an independent per-plane basis.
//
// This performs a straight-forward, unbiased solution.  In
// particular, it does not bias the result with additional information
// such as if any blob overlaps with another in a neighboring time
// slice.

#ifndef WIRECELLIMG_BLOBSOLVER
#define WIRECELLIMG_BLOBSOLVER

#include "WireCellIface/IBlobSetPipeline.h"
#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IAnodePlane.h"

namespace WireCell {
    namespace Img {

        class BlobSolver : public IBlobSetPipeline, public IConfigurable {
        public:
            
            BlobSolver();
            virtual ~BlobSolver();
            virtual bool operator()(const input_pointer& in, output_pointer& out);


            // IConfigurable
            virtual void configure(const WireCell::Configuration& cfg);
            virtual WireCell::Configuration default_configuration() const;

        private:
            IAnodePlane::pointer m_anode;

        };
    }
}

#endif
