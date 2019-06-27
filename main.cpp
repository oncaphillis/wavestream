// @Autor: Sebastian Kloska (sebastian.kloska@snafu.de)

#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <set>
#include <random>

// AudioEffect is the base class for effects that can process
// audio and have a subsequent effect (next).

struct AudioEffect {
    virtual ~AudioEffect() = default;
    virtual void process(float* buf, size_t num) = 0;
    std::shared_ptr<AudioEffect> next;
};

// A dummy effect -- doing nothing
struct DummyEffect : public AudioEffect {
public:
    void process(float *buf, size_t num) {
    }
};

void read_wav(std::istream & istream) {
    const std::string riffTag="RIFF";
    const std::string wavTag ="WAVE";

    char buff0[riffTag.length()];
    char buff1[wavTag.length()];

}

// Implement a function that checks if there is a feedback loop
// in the effects chain.
//
// 1 If a loop can be found we return true.
// 2 Additionaly we return the pointer to the Effect where the feedback loop starts
//   e.g. the one where the next AudioEffect points back into the chain of effects
// 3 We also return the length of the chain. Either until the loop back or the end of the chain

bool detect_feedback(AudioEffect * pStart,AudioEffect * & rLoopPtr,size_t &n)
{
    // Set of pointers. Traverse through the chain
    // and check wether we have senn the current pointer already

    std::set<AudioEffect *> ptrSet;
    rLoopPtr = nullptr;
    n = 0;
    while(pStart != nullptr && pStart->next.get()!=nullptr) {
        // add the current position to the set
        ptrSet.insert(pStart);
        n++;
        // have we seen the position already ?
        if(ptrSet.find(pStart->next.get())!=ptrSet.end()) {
            rLoopPtr = pStart;
            return true;
        }
        pStart = pStart->next.get();
    }

    return false;
}

// Test function for detect_feedback loop
//
// 1 Builds a vector of N shared pointers to audio effects
// 2 Interconnects these AudioEffects by random
// 3 Adds nullptr every Rth time on everage
// 4 Checks if it is able to detect a feedback loop starting at each AudioEffect in the array

void test_detect_feedback()
{
    static const size_t N = 1000; // Size of pool of audio effects
    static const size_t R = 40; // Every R time on everage we set next == nullptr

    // Pool of AudioEffects
    std::vector<std::shared_ptr<AudioEffect>> test_vector(N);

    // Fill the pool...
    for(auto & aptr : test_vector) {
        aptr.reset(new DummyEffect());
    }

    // random number generator with range 0 <= R < N
    std::random_device randdev;
    std::mt19937 randeng(randdev());
    // Random index 0<=idx<N
    std::uniform_int_distribution<> distr_idx(0, N-1);

    // Random number 0<=r<=R
    std::uniform_int_distribution<> distr_null(0, R);

    // Link each Node with a random node from the vector
    // Every Rth insert a nullptr as next effect
    int i=0;
    for(auto & aptr : test_vector) {
        bool isnull = distr_null(randeng) == R;

        if(isnull) {
            aptr->next.reset();
        } else {
            size_t idx = distr_idx(randeng);
            aptr->next=test_vector[idx];
        }
        i++;
    }

    // check each node if its the start of a chain with a feedback
    int n = 0;
    AudioEffect * loopev;
    for(auto & aptr : test_vector) {
        size_t n=99;
        std::cout << "#" << n++ << " " << detect_feedback(aptr.get(),loopev,n) << " " << n << std::endl;
    }
}

int main()
{
    test_detect_feedback();
}
