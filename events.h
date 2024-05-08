#include <library/cpp/actors/core/event_local.h>
#include <library/cpp/actors/core/events.h>

struct TEvents {
    // Вам нужно самостоятельно сюда добавить все необходимые events в NActors::TEvents::ES_PRIVATE
    struct LabEvents{
        enum {
            Done = EventSpaceBegin(NActors::TEvents::ES_PRIVATE),
            WriteValueRequest
        };
    };

    struct TEvDone: public NActors::TEventLocal<TEvDone, LabEvents::Done> {
        DEFINE_SIMPLE_LOCAL_EVENT(TEvDone, "LabEvents: Done");
    };

    struct TEvWriteValueRequest: public NActors::TEventLocal<TEvWriteValueRequest, LabEvents::WriteValueRequest> {
        long value;

        TEvWriteValueRequest(long& v) : value(v) {}
    };   
};
