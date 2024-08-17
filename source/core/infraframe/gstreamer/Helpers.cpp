#include "Helpers.h"

class GstInit {
public:
    GstInit()
    {
        if (!gst_is_initialized()) {
            gst_init(nullptr, nullptr);
        }
    }
};

GstInit gstInitInstance;
