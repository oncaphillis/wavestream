// @Autor: Sebastian Kloska (sebastian.kloska@snafu.de)

#include <iostream>
#include <iomanip>

#include <memory>
#include <vector>
#include <algorithm>
#include <set>
#include <random>
#include <sstream>

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
// 2 Additionaly we return the length of the chain until the end or the loop
//
// Compelexity should be O(N)* O(log(N/2))

bool detect_feedback(AudioEffect * pStart,size_t &n)
{
    // std::set of pointers. Traverse through the chain
    // and check wether we have seen the current pointer already

    std::set<AudioEffect *> ptrSet;

    n = 0;
    // Loop until end-of-chain or a loop back is detected
    while(pStart != nullptr && pStart->next.get()!=nullptr) {
        // add the current position to the set
        ptrSet.insert(pStart);

        // have we seen the position already ?
        if(ptrSet.find(pStart->next.get())!=ptrSet.end()) {
            n = ptrSet.size();
            return true;
        }
        pStart = pStart->next.get();
    }
    n = ptrSet.size();
    return false;
}

// Test function for detect_feedback loop
//
// 1 Builds a chain of AudioEffect objects of different length between 0...L
// 2 Either terminates the chain with a nullptr or a loopback by chance 1/R
// 3 Calls detect_feedback num times
// 4 Dumps out statitics about the average chain length and number of detected loops

void test_detect_feedback(size_t num)
{
    static const size_t L = 1000; // Random length of chain
    static const size_t R = 2; // Probability of inserting nullptr or loopback at the end 1/R

    std::random_device randdev;
    std::mt19937 randeng(randdev());

    // Random index 0<=idx<L
    std::uniform_int_distribution<> distr_length(0, L-1);

    // Random number 0<=r<R How often do we terminate o a loop (1/R)
    std::uniform_int_distribution<> distr_loop(0, R-1);

    size_t hitnum = 0;
    size_t lengthsum =0;
    int minLength = -1;
    int maxLength = -1;

    for(int i=0;i<num;i++) {
        std::shared_ptr<AudioEffect> loop;
        std::shared_ptr<AudioEffect> root;
        std::shared_ptr<AudioEffect> * current = &root;

        int randlength = distr_length(randeng);

        std::uniform_int_distribution<> distr_idx(0, randlength-1);

        // Select a position within the chain as the potential loop back position
        int loopback_position = distr_idx(randeng);

        // Generate a chain of random length. Select one random node the the loopback target
        for(int j=0;j<randlength;j++)
        {
            current->reset(new DummyEffect());
            if(i == loopback_position)
            {
                loop = *current;
            }
            current = &current->get()->next;
        }
        // Either terminate with loopback or simply leave
        if(distr_loop(randeng) == 0)
        {
            *current = loop;
        }

        size_t n = 0;
        bool hit = 0;
        std::cout << std::setw(6) << i << ":" << (hit=detect_feedback(root.get(),n)) << ":" << n << "/" << randlength
                  << "\r"
                  << std::flush;

        if(n!=randlength)
        {
            std::stringstream ss;
            ss << " Expected chain length " << randlength << " found " << n;
            throw std::runtime_error(ss.str());
        }

        minLength = minLength == -1 || n < minLength ? n : minLength;
        maxLength = minLength == -1 || n > maxLength ? n : maxLength;

        if(hit)
        {
            hitnum++;
        }

        lengthsum += n;
    }
    std::cout << std::endl << "Statistics:" << num << " tests "
              << " chain length " << minLength << "/" << float(lengthsum)/num << "/" << maxLength << " min/avg/max"
              << " hits " << hitnum << "/" << std::fixed << float(hitnum)/num << " abs/%" << std::endl;
}

int main(int argc, char **argv)
{
    int num = 10000;

    if(argc!=1 && argc!=2)
    {
        std::cerr << "Usage: " << argv[1] << " [num] number of random tests" << std::endl;
        ::exit(1);
    }

    if(argc==2)
    {
        std::stringstream ss(argv[1]);

        if( !(ss >> num >> std::ws) || !ss.eof() || num <=0 )
        {
            std::cerr << argv[1] << " does't seem to be a positive number" << std::endl;
            ::exit(2);
        }
    }

    test_detect_feedback(num);
}
