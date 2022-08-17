// Copyright Microsoft and Project Verona Contributors.
// SPDX-License-Identifier: MIT
#pragma once

#include <snmalloc/snmalloc.h>

#include <atomic>

namespace verona::rt
{
  class ThreadState
  {
  public:
    // The global state proceeds:
    // NotInLD -> PreScan -> Scan -> BelieveDone -> ReallyDone -> Sweep
    // -> Finished
    // In the ReallyDone state, the retracted flag may be true or false. If
    // it is true, the global state proceeds to Scan. If it is false, the
    // global state proceeds to Sweep, and then Finished.

    // Scheduler thread state proceeds:
    // NotInLD -> WantLD (may be skipped) -> PreScan (may be skipped) ->
    // Scan

    // The scheduler thread is responsible for moving from NotInLD to WantLD.

    // WantLD triggers moving into PreScan. Once all threads have observed
    // PreScan, then the global state moves to Scan, and thus the last thread
    // skips PreScan and immediately enters Scan.

    // At this point, the scheduler thread is responsible for changing its own
    // state to BelieveDone_Vote when it believes it has finished scanning.
    // BelieveDone_Vote -> BelieveDone_Voted (may be skipped) -> BelieveDone

    // At this point, the scheduler thread is responsible for changing its own
    // state to either BelieveDone_Confirm or BelieveDone_Retract.
    // BelieveDone_Confirm -> BelieveDone_Ack (may be skipped) -> ReallyDone
    // BelieveDone_Retract -> BelieveDone_Ack (may be skipped) -> ReallyDone

    // If all scheduler threads issued BelieveDone_Confirm, then the retracted
    // flag will be false.
    // ReallyDone -> ReallyDone_Confirm -> Sweep -> Finished (may be skipped) ->
    // -> NotInLD (may be skipped)
    // It's possible to go from Finished directly to PreScan if
    // another LD has been started and the thread is the last to find out.

    // If any scheduler thread issued BelieveDone_Retract, then the retracted
    // flag will be true.
    // ReallyDone -> ReallyDone_Retract -> Scan

    enum State
    {
      NotInLD,

      // A scheduler thread can change its own state from NotInLD to WantLD if
      // it would like to begin a Leak Detector Cycle.
      WantLD,

      // At this point, we are about to start scanning cowns to determine
      // reachability.  As the system does not hit a hard barrier, we begin
      // counting work that will need to be addressed later.  In particular, any
      // messages that are sent when in PreScan, may land on a cown that is in
      // the scan state.  We count these messages, so we know once, we have
      // completed them all.
      PreScan,

      // All threads are at least in PreScan, we can proceed with scanning
      // cowns and messages they are processing. Any messages sent from PreScan
      // or earlier must be scanned.
      Scan,

      // All threads are at least in Scan, we can consider terminating the
      // protocol as everyone is now performing this protocol.
      AllInScan,

      // When a scheduler thread has both reached its marker and has not
      // scheduled an unmarked cown, it reports that it believes it is done.
      BelieveDone_Vote,

      // A scheduler thread may be in the BelieveDone_Voted state on the way to
      // being in the BelieveDone state.
      BelieveDone_Voted,

      // Once a scheduler thread is in BelieveDone, it must issue exactly one
      // response of either BelieveDone_Confirm or BelieveDone_Retract.
      // If the global message count is non-zero, then the last thread to enter
      // BelieveDone must retract, so the process can continue.
      BelieveDone,
      BelieveDone_Confirm,
      BelieveDone_Retract,

      // A scheduler thread may be in the BelieveDone_Ack state on the way to
      // being in the ReallyDone state.
      BelieveDone_Ack,

      // Once in the ReallyDone state, a scheduler thread will transition
      // either to ReallyDone_Confirm, in which case it should collect any
      // unmarked cowns and will next transition into NotInLD, or
      // ReallyDone_Retract, in which case it will next transition to Scan,
      // where it should continue the protocol.
      ReallyDone,
      ReallyDone_Confirm,
      ReallyDone_Retract,

      // This LD epoch is now in the sweep phase
      Sweep,

      // This LD epoch has completed.
      Finished,
    };

    // ThreadState counters.
    struct StateCounters {
      std::atomic<bool> retracted = false;
      std::atomic<State> state = NotInLD;
      std::atomic<size_t> vote_yes = 0;
      std::atomic<size_t> voters = 0;
      std::atomic<size_t> active_threads = 0;
      std::atomic<size_t> barrier_count = 0;
    };

  private:
    StateCounters internal_state;

  public:
    constexpr ThreadState() = default;

    State get_state()
    {
      return internal_state.state;
    }

    State next(State s)
    {
      switch (internal_state.state)
      {
        case NotInLD:
        {
          switch (s)
          {
            case NotInLD:
            case Finished:
              return NotInLD;

            case WantLD:
            {
              internal_state.state = PreScan;
              return vote_one<PreScan, Scan>();
            }

            default:
              abort();
          }
        }

        case PreScan:
        {
          switch (s)
          {
            case NotInLD:
            case WantLD:
            case Finished:
              return vote_one<PreScan, Scan>();

            case PreScan:
              return PreScan;

            default:
              abort();
          }
        }

        case Scan:
        {
          switch (s)
          {
            case PreScan:
              return vote<Scan, AllInScan>();

            case Scan:
              return Scan;

            default:
              abort();
          }
        }

        case AllInScan:
        {
          switch (s)
          {
            case Scan:
            case AllInScan:
            case ReallyDone_Retract:
              return AllInScan;

            case BelieveDone_Vote:
              return vote<BelieveDone_Voted, BelieveDone>();

            case BelieveDone_Voted:
              return BelieveDone_Voted;

            default:
              abort();
          }
        }

        case BelieveDone:
        {
          switch (s)
          {
            case BelieveDone_Voted:
              return BelieveDone;

            case BelieveDone_Confirm:
              return vote<BelieveDone_Ack, ReallyDone>();

            case BelieveDone_Retract:
            {
              internal_state.retracted = true;
              return vote<BelieveDone_Ack, ReallyDone>();
            }

            case BelieveDone_Ack:
              return BelieveDone_Ack;

            default:
              abort();
          }
        }

        case ReallyDone:
        {
          switch (s)
          {
            case BelieveDone_Ack:
            case ReallyDone:
            {
              if (internal_state.retracted)
              {
                vote<ReallyDone_Retract, AllInScan>();
                return ReallyDone_Retract;
              }

              vote<ReallyDone_Confirm, Sweep>();
              return ReallyDone_Confirm;
            }

            case ReallyDone_Confirm:
            case ReallyDone_Retract:
              return s;

            default:
              abort();
          }
        }

        case Sweep:
        {
          switch (s)
          {
            case ReallyDone_Confirm:
              return Sweep;

            case Sweep:
              return vote<Finished, NotInLD>();

            case Finished:
              return Finished;

            default:
              abort();
          }
        }

        default:
          abort();
      }
    }

    template<State next>
    void reset()
    {
      if (next == AllInScan)
        internal_state.retracted = false;

      internal_state.vote_yes.store(0);
      internal_state.state = next;
    }

    template<State next>
    void reset_one()
    {
      if (next == AllInScan)
        internal_state.retracted = false;

      internal_state.vote_yes.store(1);
      internal_state.state = next;
    }

    void set_barrier(size_t thread_count)
    {
      internal_state.retracted = false;
      internal_state.state = NotInLD;
      internal_state.vote_yes = 0;
      internal_state.voters = thread_count;
      internal_state.barrier_count = thread_count;
      internal_state.active_threads = thread_count;
    }

  private:
    template<State intermediate, State next>
    State vote()
    {
      size_t vote = internal_state.vote_yes.fetch_add(1) + 1;

      if (vote == internal_state.voters)
      {
        reset<next>();
        return next;
      }

      return intermediate;
    }

    template<State intermediate, State next>
    State vote_one()
    {
      size_t vote = internal_state.vote_yes.fetch_add(1) + 1;

      if (vote == internal_state.voters)
      {
        reset_one<next>();
        return next;
      }

      return intermediate;
    }
  public:

    /// @warn Requires holding the threadpool lock.
    bool add_thread()
    {
      if (internal_state.state >= Scan)
        return false;
      internal_state.active_threads++;
      internal_state.voters++;
      internal_state.barrier_count++;
      return true;
    }
    
    /// @warn Should be holding the threadpool lock.
    size_t exit_thread()
    {
      return internal_state.barrier_count.fetch_sub(1) -1;
    }

    size_t get_active_threads()
    {
      return internal_state.active_threads;
    }

    void dec_active_threads()
    {
      internal_state.active_threads--;
    }

    void inc_active_threads()
    {
      internal_state.active_threads++;
    }

    /// @warn Should be holding the threadpool lock.
    ///
    /// Allows a thread to park, i.e., stop servicing cowns.
    bool park_thread()
    {
      // The thread cannot park if the LD protocol is in progress
      // or if this is the last active thread.
      if (internal_state.state != NotInLD || internal_state.active_threads < 2)
        return false;

      // Make sure nobody voted.
      assert(internal_state.vote_yes == 0);
      internal_state.voters--;
      internal_state.active_threads--;
      return true;
    }

    /// @warn Should be holding the threadpool lock.
    ///
    /// Increase the number of threads pariticipating in the ld protocol, i.e.,
    //  servicing cowns.
    bool unpark_thread()
    {
      if (internal_state.state != NotInLD)
        return false;
      assert(internal_state.vote_yes == 0);
      internal_state.voters++;
      internal_state.active_threads++;
      return true;
    }
  };

  inline std::ostream&
  operator<<(std::ostream& os, const ThreadState::State& obj)
  {
    os.width(20);
    os << std::left;
    switch (obj)
    {
      case ThreadState::NotInLD:
        os << "NotInLD";
        break;
      case ThreadState::WantLD:
        os << "WantLD";
        break;
      case ThreadState::PreScan:
        os << "PreScan";
        break;
      case ThreadState::Scan:
        os << "Scan";
        break;
      case ThreadState::AllInScan:
        os << "AllInScan";
        break;
      case ThreadState::BelieveDone_Vote:
        os << "BelieveDone_Vote";
        break;
      case ThreadState::BelieveDone_Voted:
        os << "BelieveDone_Voted";
        break;
      case ThreadState::BelieveDone:
        os << "BelieveDone";
        break;
      case ThreadState::BelieveDone_Confirm:
        os << "BelieveDone_Confirm";
        break;
      case ThreadState::BelieveDone_Retract:
        os << "BelieveDone_Retract";
        break;
      case ThreadState::BelieveDone_Ack:
        os << "BelieveDone_Ack";
        break;
      case ThreadState::Finished:
        os << "Finished";
        break;
      case ThreadState::ReallyDone:
        os << "ReallyDone";
        break;
      case ThreadState::ReallyDone_Confirm:
        os << "ReallyDone_Confirm";
        break;
      case ThreadState::ReallyDone_Retract:
        os << "ReallyDone_Retract";
        break;
      case ThreadState::Sweep:
        os << "Sweep";
        break;
    }
    return os;
  }
} // namespace verona::rt
