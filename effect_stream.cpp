#include <memory>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>

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
            si->receive(rOut);
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
public:
    WavPcmSource(std::istream & is)
        : _inputStream(is)
    {
        const std::string riffTag="RIFF";
        static const int tagSize = 4;

        uint8_t buff[tagSize];

        if(!is.read((char *)buff,tagSize) || std::string((char*)buff,tagSize)!=riffTag)
        {
            throw std::runtime_error("Failed to detect '" + riffTag +"'");
        }

        if(!is.read((char*)buff,tagSize))
        {
            throw std::runtime_error("Failed to detect '" + riffTag + "' size");
        }

        size_t size = getLong(buff);

        const std::string wavTag ="WAVE";

        if(!is.read((char*)buff,tagSize) || std::string((char*)buff,tagSize)!=wavTag)
        {
            throw std::runtime_error("Failed to detect '" + wavTag + "'");
        }

        const std::string fmtTag ="fmt ";

        if(!is.read((char *)buff,tagSize) || std::string((char*)buff,tagSize)!=fmtTag)
        {
            throw std::runtime_error("Failed to detect '" + fmtTag + "'");
        }

        if(!is.read((char*)buff,tagSize))
        {
            throw std::runtime_error("Failed to detect '" + fmtTag + "' size");
        }
        size = getLong(buff);
        static const int fmtSize = 16;
        if(size!=fmtSize)
        {
            std::stringstream ss;
            ss << "expected fmt chunk of size " << fmtSize;
            throw std::runtime_error(ss.str());
        }

        uint8_t fmtBuff[fmtSize];

        if(!is.read((char*)fmtBuff,fmtSize))
        {
            throw std::runtime_error("Failed to read '" + fmtTag + "' buffer");
        }

        const std::string dataTag ="data";

        if(!is.read((char *)buff,tagSize) || std::string((char*)buff,tagSize)!=dataTag)
        {
            throw std::runtime_error("Failed to detect '" + dataTag + "'");
        }

        if(!is.read((char *)buff,tagSize) )
        {
            throw std::runtime_error("Failed to read '" + dataTag + "' size");
        }

        const uint8_t * fmtPtr = fmtBuff;

        if(getWord(fmtPtr,fmtPtr)!=1)
        {
            throw std::runtime_error("Only handle PCM data");
        }

        _channels = getWord(fmtPtr,fmtPtr);
        if(_channels!=1 && _channels!=2)
        {
            throw std::runtime_error("Only handle Mono/Stereo");
        }

        _sampleRate = getLong(fmtPtr,fmtPtr);
        _dataRate = getLong(fmtPtr,fmtPtr);
        _frameSize = getWord(fmtPtr,fmtPtr);
        _bitSize  = getWord(fmtPtr,fmtPtr);

        if(_bitSize != 8 && _bitSize != 16 &&  _bitSize != 24 &&  _bitSize != 32)
        {
            throw std::runtime_error("Sorr only 8/16/24/32 bts/sample for now");
        }

        std::cerr << "Channels:" << _channels << " "
                  << "SampleRate:" << _sampleRate << " "
                  << "DataRate:" << _dataRate << " "
                  << "FrameSize:" << _frameSize << " "
                  << "BitSize:" << _bitSize << std::endl;


        if(_frameSize != _channels * _bitSize / 8)
        {
            std::stringstream ss;
            ss<< "Expected FrameSize to be " << _channels * _bitSize / 8;
            throw std::runtime_error(ss.str());
        }

        if(_dataRate != _frameSize * _sampleRate)
        {
            std::stringstream ss;
            ss<< "Expected DataRate to be " <<_frameSize * _sampleRate;
            throw std::runtime_error(ss.str());
        }
    }

private:
    uint16_t getWord(const uint8_t * buff,const uint8_t * & next=_dummyRef)
    {
        uint16_t w = uint16_t(buff[1]) << 8 | uint16_t(buff[0]);
        next = buff+2;
        return w;
    }

    uint32_t getLong(const uint8_t * buff,const uint8_t * & next=_dummyRef)
    {
        uint32_t l = uint32_t(buff[3]) << 24 | uint32_t(buff[2]) << 16 | uint32_t(buff[1]) << 8 | uint32_t(buff[0]);
        next = buff+4;
        return l;
    }

    int _channels;
    uint32_t _sampleRate;
    uint32_t _dataRate;
    uint16_t _frameSize;
    uint16_t _bitSize;
    int _sampleSize;

    static const uint8_t * _dummyRef;
    std::istream & _inputStream;
};

const uint8_t * WavPcmSource::_dummyRef;


const std::string fn = "/usr/share/sounds/speech-dispatcher/pipe.wav";

int main(int argc, char **argv) {
    std::istream * is=&std::cin;

    std::ifstream fi;

    if(argc==2)
    {
        fi.open(argv[1]);
        is = &fi;
    }

    WavPcmSource src(*is);

    return 0;
}
