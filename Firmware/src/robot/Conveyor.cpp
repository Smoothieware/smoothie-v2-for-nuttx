#include "Conveyor.h"

#include "AxisDefns.h"
#include "GCode.h"
//#include "Kernel.h"
#include "Block.h"
#include "Planner.h"
#include "ConfigReader.h"
#include "StepTicker.h"
#include "Robot.h"
#include "StepperMotor.h"

#include <functional>
#include <vector>

#define queue_delay_time_ms_key "queue_delay_time_ms"

/*
 * The conveyor manages the planner queue, and starting the executing chain of blocks
 * TODO is this even required anymore?
 */
Conveyor *Conveyor::instance;

Conveyor::Conveyor(PlannerQeue& q) : Module("conveyor"), queue(q)
{
    instance= this;
    running = false;
    allow_fetch = false;
    flush= false;
    halted= false;
}

void Conveyor::configure(ConfigReader& cr)
{
    // Attach to the end_of_move stepper event
    ConfigReader::section_map_t m;
    if(cr.get_section("conveyor", m)) {
        queue_delay_time_ms = cr.get_int(m, queue_delay_time_ms_key, 100);
    }
}

// we allocate the queue here after config is completed so we do not run out of memory during config
void Conveyor::start(uint8_t n)
{
    //THEKERNEL->step_ticker->finished_fnc = std::bind( &Conveyor::all_moves_finished, this);
    Block::init(n); // set the number of motors which determines how big the tick info vector is
    running = true;
}

void Conveyor::on_halt(bool flg)
{
    halted= flg;

    if(flg) {
        flush_queue();
    }
}

void Conveyor::on_idle(void*)
{
    if (running) {
        check_queue();
    }

    // we can garbage collect the block queue here
    if (queue.tail_i != queue.isr_tail_i) {
        if (queue.is_empty()) {
            __debugbreak();
        } else {
            // Cleanly delete block
            Block* block = queue.tail_ref();
            //block->debug();
            block->clear();
            queue.consume_tail();
        }
    }
}

// see if we are idle
// this checks the block queue is empty, and that the step queue is empty and
// checks that all motors are no longer moving
bool Conveyor::is_idle() const
{
    if(queue.is_empty()) {
        for(auto &a : THEROBOT->actuators) {
            if(a->is_moving()) return false;
        }
        return true;
    }

    return false;
}

// Wait for the queue to be empty and for all the jobs to finish in step ticker
void Conveyor::wait_for_idle(bool wait_for_motors)
{
    // wait for the job queue to empty, this means cycling everything on the block queue into the job queue
    // forcing them to be jobs
    running = false; // stops on_idle calling check_queue
    while (!queue.empty()) {
        check_queue(true); // forces queue to be made available to stepticker
        // THEKERNEL->call_event(ON_IDLE, this);
    }

    if(wait_for_motors) {
        // now we wait for all motors to stop moving
        while(!is_idle()) {
            // THEKERNEL->call_event(ON_IDLE, this);
        }
    }

    running = true;
    // returning now means that everything has totally finished
}

/*
 * push the pre-prepared head block onto the queue
 */
// void Conveyor::queue_head_block()
// {
//     // upstream caller will block on this until there is room in the queue
//     while (queue.is_full() && !THEKERNEL->is_halted()) {
//         //check_queue();
//         THEKERNEL->call_event(ON_IDLE, this); // will call check_queue();
//     }

//     if(THEKERNEL->is_halted()) {
//         // we do not want to stick more stuff on the queue if we are in halt state
//         // clear and release the block on the head
//         queue.head_ref()->clear();
//         return; // if we got a halt then we are done here
//     }

//     queue.produce_head();

//     // not sure if this is the correct place but we need to turn on the motors if they were not already on
//     THEKERNEL->call_event(ON_ENABLE, (void*)1); // turn all enable pins on
// }

void Conveyor::check_queue(bool force)
{
    static uint32_t last_time_check = us_ticker_read();

    if(queue.is_empty()) {
        allow_fetch = false;
        last_time_check = us_ticker_read(); // reset timeout
        return;
    }

    // if we have been waiting for more than the required waiting time and the queue is not empty, or the queue is full, then allow stepticker to get the tail
    // we do this to allow an idle system to pre load the queue a bit so the first few blocks run smoothly.
    if(force || queue.is_full() || (us_ticker_read() - last_time_check) >= (queue_delay_time_ms * 1000)) {
        last_time_check = us_ticker_read(); // reset timeout
        if(!flush) allow_fetch = true;
        return;
    }
}

// called from step ticker ISR
bool Conveyor::get_next_block(Block **block)
{
    // mark entire queue for GC if flush flag is asserted
    if (flush){
        while (queue.isr_tail_i != queue.head_i) {
            queue.isr_tail_i = queue.next(queue.isr_tail_i);
        }
    }

    // default the feerate to zero if there is no block available
    this->current_feedrate= 0;

    if(THEKERNEL->is_halted() || queue.isr_tail_i == queue.head_i) return false; // we do not have anything to give

    // wait for queue to fill up, optimizes planning
    if(!allow_fetch) return false;

    Block *b= queue.item_ref(queue.isr_tail_i);
    // we cannot use this now if it is being updated
    if(!b->locked) {
        if(!b->is_ready) __debugbreak(); // should never happen

        b->is_ticking= true;
        b->recalculate_flag= false;
        this->current_feedrate= b->nominal_speed;
        *block= b;
        return true;
    }

    return false;
}

// called from step ticker ISR when block is finished, do not do anything slow here
void Conveyor::block_finished()
{
    // we increment the isr_tail_i so we can get the next block
    queue.isr_tail_i= queue.next(queue.isr_tail_i);
}

/*
    In most cases this will not totally flush the queue, as when streaming
    gcode there is one stalled waiting for space in the queue, in
    queue_head_block() so after this flush, once main_loop runs again one more
    gcode gets stuck in the queue, this is bad. Current work around is to call
    this when the queue in not full and streaming has stopped
*/
void Conveyor::flush_queue()
{
    allow_fetch = false;
    flush= true;

    // TODO force deceleration of last block

    // now wait until the block queue has been flushed
    wait_for_idle(false);

    flush= false;
}

// Debug function
void Conveyor::dump_queue()
{
    for (unsigned int index = queue.tail_i, i = 0; true; index = queue.next(index), i++ ) {
        THEKERNEL->streams->printf("block %03d > ", i);
        queue.item_ref(index)->debug();

        if (index == queue.head_i)
            break;
    }
}
