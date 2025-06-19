#ifndef IWORKER_H
#define IWORKER_H

#include "HttpData.h"
#include <memory>

namespace KWShared{
    class IWorker
    {
        public:
            virtual void start(void* kwwebserver) = 0;
            virtual void load(shared_ptr<HttpData> httpData) = 0;
            virtual void unload(shared_ptr<HttpData> httpData) = 0;
    };
}

#endif // IWORKER_H
