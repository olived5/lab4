#ifndef REQUESTSCHEDULER_FRFCFS_THRESHOLD_H
#define REQUESTSCHEDULER_FRFCFS_THRESHOLD_H
#include "../../src/RequestScheduler.h"
#include <map>
#include <vector>
#include <cstddef>

namespace MCsim {
class RequestScheduler_FRFCFS_THRESHOLD : public RequestScheduler {
    // The budget belongs to the oldest waiting request in each physical bank.
    // A hit in front of it does not consume its budget; a younger hit does.
    struct AgeState {
        Request *oldest;
        unsigned int overtakes;
        AgeState() : oldest(NULL), overtakes(0) {}
    };
    std::map<std::pair<unsigned int, unsigned int>, AgeState> ages;
    static const unsigned int HIT_THRESHOLD = 4;

    typedef std::pair<unsigned int, unsigned int> BankKey;
    static BankKey key(Request *r) {
        return BankKey(r->addressMap[Rank], r->addressMap[Bank]);
    }
    void synchronize(unsigned int q) {
        // The reference FRFCFS.ini uses one global request queue. Each bank
        // is independently tracked, including requests from other requestors.
        std::map<BankKey, Request *> oldest;
        const unsigned int n = requestQueue[q]->getSize(false, 0);
        for (unsigned int i = 0; i < n; ++i) {
            Request *r = requestQueue[q]->getRequestCheck(i);
            if (r && !oldest.count(key(r))) oldest[key(r)] = r;
        }
        for (std::map<BankKey, AgeState>::iterator it = ages.begin(); it != ages.end();) {
            if (!oldest.count(it->first)) ages.erase(it++);
            else ++it;
        }
        for (std::map<BankKey, Request *>::const_iterator it = oldest.begin(); it != oldest.end(); ++it) {
            AgeState &s = ages[it->first];
            if (s.oldest != it->second) { s.oldest = it->second; s.overtakes = 0; }
        }
    }
    unsigned int select(unsigned int q) {
        const unsigned int n = requestQueue[q]->getSize(false, 0);
        // Preserve FRFCFS's first-hit preference unless it would overtake an
        // older request in the SAME bank whose budget is exhausted.
        for (unsigned int i = 0; i < n; ++i) {
            Request *candidate = requestQueue[q]->getRequestCheck(i);
            if (!candidate || !isRowHit(candidate)) continue;
            const BankKey bank = key(candidate);
            bool blocked = false;
            for (unsigned int j = 0; j < i; ++j) {
                Request *older = requestQueue[q]->getRequestCheck(j);
                if (older && key(older) == bank &&
                    ages[bank].overtakes >= HIT_THRESHOLD) {
                    blocked = true;
                    break;
                }
            }
            if (!blocked) return i;
        }
        return 0; // FCFS fallback, as in the original scheduler.
    }
public:
    RequestScheduler_FRFCFS_THRESHOLD(std::vector<RequestQueue *> &rq,
        std::vector<CommandQueue *> &cq, const std::map<unsigned int, bool> &rt)
        : RequestScheduler(rq, cq, rt) {}
    void requestSchedule() {
        for (size_t q = 0; q < requestQueue.size(); ++q) {
            if (requestQueue[q]->getSize(false, 0) == 0) {
                ages.clear();
                continue;
            }
            synchronize(static_cast<unsigned int>(q));
            const unsigned int index = select(static_cast<unsigned int>(q));
            Request *candidate = requestQueue[q]->getRequest(index);
            if (candidate && commandQueue[candidate->bank]->getSize(true) == 0 &&
                isSchedulable(candidate, isRowHit(candidate))) {
                const BankKey bank = key(candidate);
                // Only a successful younger hit consumes the oldest request's budget.
                bool overtook = false;
                for (unsigned int j = 0; j < index; ++j) {
                    Request *older = requestQueue[q]->getRequestCheck(j);
                    if (older && key(older) == bank) { overtook = true; break; }
                }
                if (overtook && isRowHit(candidate)) ++ages[bank].overtakes;
                updateRowTable(candidate->addressMap[Rank], candidate->addressMap[Bank], candidate->row);
                requestQueue[q]->removeRequest();
                synchronize(static_cast<unsigned int>(q));
            }
            scheduledRequest = NULL;
        }
    }
};
} // namespace MCsim
#endif
