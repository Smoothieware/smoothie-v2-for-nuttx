#pragma once

#include <vector>
#include <tuple>
#include <functional>

class SlowTicker
{
    public:
        // setup the Singleton instance
        static SlowTicker &getInstance()
        {
            static SlowTicker instance;
            return instance;
        }


        void set_frequency( int frequency );
        bool start();
        bool stop();

        int attach(uint32_t frequency, std::function<void(void)> cb);
        void detach(int n);
        void tick();

    private:
        SlowTicker() {};


        using callback_t = std::tuple<int, uint32_t, std::function<void(void)>>;
        std::vector<callback_t> callbacks;
        uint32_t max_frequency{0};
        uint32_t interval{0};

        int fd{-1}; // timer device
        bool started{false};
};
