#ifndef WIRECELLIMG_STRIPSSINK
#define WIRECELLIMG_STRIPSSINK

#include "WireCellIface/ISinkNode.h"
#include "WireCellIface/IStripSet.h"



namespace WireCell {

    // don't bother with an intermediate strip set sink node interface...
    class IStripSetSink : public ISinkNode<IStripSet> {
    public:
        typedef std::shared_ptr<IStripSetSink> pointer;

        virtual ~IStripSetSink() {}

        virtual std::string signature() {
            return typeid(IStripSetSink).name();
        }
    };
        
    namespace Img {


        class StripsSink : public IStripSetSink {
        public:
            
            virtual ~StripsSink();

            virtual bool operator()(const IStripSet::pointer& ss);
        };

    }
}

#endif
