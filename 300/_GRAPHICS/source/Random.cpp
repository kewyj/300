/**-------------------------------------------------------------------------------------/
 * @file
 *  Random.cpp
 * @author
 *  Lee Fu Sheng Roy, 670000622, f.lee@digipen.edu
 * @date
 *  2023/05/21
 * @brief
 *  Class definition of Camera.
 * @copyright
 *  Copyright (C) 2023 DigiPen Institute of Technology.
 *-------------------------------------------------------------------------------------*/

/************************************************************************/
/* concatenation of following two 16-bit multiply with carry generators */
/* x(n)=a*x(n-1)+carry mod 2^16 and y(n)=b*y(n-1)+carry mod 2^16,       */
/* number and carry packed within the same 32 bit integer.              */
/************************************************************************/
#include "Random.hpp"

namespace GFX {
    namespace Utils {

        /* return a random float >= 0 and < 1 */
        static unsigned int SEED_X = 521288629;
        static unsigned int SEED_Y = 362436069;

        unsigned rand()
        {
            /* Use any pair of non-equal numbers from this list for "a" and "b"
                18000 18030 18273 18513 18879 19074 19098 19164 19215 19584
                19599 19950 20088 20508 20544 20664 20814 20970 21153 21243
                21423 21723 21954 22125 22188 22293 22860 22938 22965 22974
                23109 23124 23163 23208 23508 23520 23553 23658 23865 24114
                24219 24660 24699 24864 24948 25023 25308 25443 26004 26088
                26154 26550 26679 26838 27183 27258 27753 27795 27810 27834
                27960 28320 28380 28689 28710 28794 28854 28959 28980 29013
                29379 29889 30135 30345 30459 30714 30903 30963 31059 31083
            */
            static unsigned int a = 18030, b = 31059;

            SEED_X = a * (SEED_X & 65535) + (SEED_X >> 16);
            SEED_Y = b * (SEED_Y & 65535) + (SEED_Y >> 16);

            return ((SEED_X << 16) + (SEED_Y & 65535));
        }

        float frand()
        {
            return (rand() + 1) / static_cast<float>(static_cast<unsigned>(-1));
        }

        void srand(unsigned seed1, unsigned seed2)
        {
            if (seed1)
                SEED_X = seed1; /* Use default seeds if parameter is 0 */
            if (seed2)
                SEED_Y = seed2;
        }

        int Random(int low, int high)
        {
            int r1 = static_cast<int>(GFX::Utils::rand() / 2 - 1);
            return r1 % (high - low + 1) + low;
        }
        float Random(float low, float high)
        {
            float r = frand();
            float v = r * (high - low) + low;
            return v;
        }
    } // namespace Utils
} // namespace CS170
