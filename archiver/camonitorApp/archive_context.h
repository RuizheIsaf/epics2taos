#pragma once

#include <aws/core/utils/memory/stl/AWSString.h>

namespace Aws
{
    namespace Client
    {
        /**
        * Call-back context for all async client methods. This allows you to pass a context to your callbacks so that you can identify your requests.
        * It is entirely intended that you override this class in-lieu of using a void* for the user context. The base class just gives you the ability to
        * pass a uuid for your context.
        */
        class AWS_CORE_API ArchiveContext:public AsyncCallerContext
        {
        public:
            /**
             * Initializes object with generated UUID
             */
            ArchiveContext();

            /**
             * Sets underlying UUID
             */
            inline void SetBuffPointer(void *p) { pbuff = p; }

            /**
             * Sets underlying UUID
             */
            inline void FreeBuff() { free(pbuff); }

        private:
            
            void *pbuff;
        };
    }
}
