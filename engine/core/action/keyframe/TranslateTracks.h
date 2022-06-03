//
// (c) 2022 Eduardo Doria.
//

#ifndef TRANSLATETRACKS_H
#define TRANSLATETRACKS_H

#include "action/TimedAction.h"

namespace Supernova{
    class TranslateTracks: public TimedAction{

    public:
        TranslateTracks(Scene* scene);
        TranslateTracks(Scene* scene, std::vector<float> times, std::vector<Vector3> values);

        void setTimes(std::vector<float> times);
        void setValues(std::vector<Vector3> values);
    };
}

#endif //TRANSLATETRACKS_H