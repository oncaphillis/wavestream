#include <memory>
#include <vector>
#include <fstream>

#if 0

class AudioSink
{
public:
    virtual
    void receive(const std::vector<float> & rIn) = 0;
};

class AudioSource
{
public:
    typedef std::shared_ptr<AudioSink> SinkPtr_t;
    void addSink(SinkPtr_t sink)
    {
        _sinks.push_back(sink);
    }
protected:
    void doEmit(const std::vector<float> & rOut)
    {
        for(auto & si : _sinks)
        {
            si->process(rOut);
        }
    }
private:
    std::vector<SinkPtr_t> _sinks;
};

class AudioFilter : public AudioSource,public AudioSink
{

protected:
};

class WavPcmSource : public AudioSource
{
    WavPcmSource(std::istream & is)
        : _inputStream(is)
    {
        const std::string riffTag="RIFF";
        const std::string wavTag ="WAVE";

        char buff0[riffTag.length()];
        char buff1[wavTag.length()];

    }

    WavPcmSource(const std::string & fileName)
        : WavPcmSource(std::ifstream(fileName))
    {
    }

private:
    std::istream & _is;
};
#endif

int main(int argc, char **argv) {
    return 0;
}
