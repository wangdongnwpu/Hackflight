/*
   gyrometer.hpp : Support for gyrometer (a.k.a. gyroscope) 

   Copyright (c) 2018 Simon D. Levy

   This file is part of Hackflight.

   Hackflight is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Hackflight is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with Hackflight.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <math.h>

#include "debug.hpp"
#include "sensors/surfacemount.hpp"
#include "board.hpp"

namespace hf {

    class Gyrometer : public SurfaceMountSensor {

        friend class Hackflight;

        public:

            Gyrometer(void)
            {
                _gx = 0;
                _gy = 0;
                _gz = 0;
            }

        protected:

            virtual void modifyState(state_t & state, float time) override
            {
                (void)time;

                // NB: We negate gyro X, Y to simplify PID controller
                state.angularVelocities[0] =  _gx;
                state.angularVelocities[1] = -_gy;
                state.angularVelocities[2] = -_gz;
            }

            virtual bool ready(float time) override
            {
                (void)time;

                bool result = board->getGyrometer(_gx, _gy, _gz);

                return result;
            }

        private:

            float _gx;
            float _gy;
            float _gz;

    };  // class Gyrometer

} // namespace
