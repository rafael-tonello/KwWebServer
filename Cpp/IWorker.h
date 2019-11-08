#ifndef IWORKER_H
#define IWORKER_H

#include "HttpData.h"

namespace KWShared{
    class IWorker
    {
        public:
            virtual void start(void* kwwebserver) = 0;
            virtual void load(HttpData* httpData) = 0;
            virtual void unload(HttpData* httpData) = 0;

        protected:

        private:
    };
}

#endif // IWORKER_H
