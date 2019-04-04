/** BlobSolving takes in a cluster and produces another. 
 *
 * It uses information about the measured charge in channels and the
 * inter-slice adjacency of input blobs to solve for the blob charge.
 * Any blob solution below some configurable charge value will be
 * omitted from the output cluster.
 */  
#ifndef WIRECELL_BLOBSOLVING_H
#define WIRECELL_BLOBSOLVING_H

#include "WireCellIface/ICluseterFilter.h"
#include "WireCellIface/IConfigurable.h"

namespace WireCell {

    namespace Img {


        class BlobSolving : public IClusterFilter, public IConfigurable {
        };

    }  // Img
    

}  // WireCell


#endif /* WIRECELL_BLOBSOLVING_H */
