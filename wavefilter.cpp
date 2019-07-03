#include <memory>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>

#include "jobpool.h"

class SplitJob : public Job {

public:
    typedef JobQueue queue_t;
    SplitJob(queue_t & from,queue_t & left,queue_t & right)
        : _from(from)
        , _left(left)
        , _right(right)
    {
    }

    bool run () override
    {
        queue_t::dataptr_t data;

        if(!_from.pop(data))
        {
            _left.finish();
            _right.finish();
            return false;
        }

        queue_t::dataptr_t ldata;
        queue_t::dataptr_t rdata;

        bool odd = (data->_vector.size() & 1) != 0;

        int ool = odd ? _oddToggle ? 1 : 0 : 0;
        int oor = odd ? _oddToggle ? 0 : 1 : 0;


        ldata.reset( new queue_t::Data(data->_vector.size() / 2 + oor ) );
        rdata.reset( new queue_t::Data(data->_vector.size() / 2 + ool ) );
#if 0
        std::cerr << ool << "::" << oor << " @ "
                  << ldata->_vector.size() << "::"
                  << rdata->_vector.size() << "::"
                  << std::endl;
#endif
        for(size_t i=0; i<data->_vector.size() / 2;i++)
        {
            ldata->_vector[ i ] = data->_vector[ i * 2 + ool ];
            rdata->_vector[ i ] = data->_vector[ i * 2 + oor ];
        }

        if(odd)
        {
            if(_oddToggle)
            {
                rdata->_vector[data->_vector.size() / 2 ] = data->_vector.back();
            }
            else
            {
                ldata->_vector[data->_vector.size() / 2 ] = data->_vector.back();
            }
        }

        _oddToggle = odd ? _oddToggle ? false : true : true ;

        _left.push(ldata);
        _right.push(rdata);

        return true;
    }

private:
    queue_t & _from;
    queue_t & _left;
    queue_t & _right;
    size_t _oddToggle = false; // last data was odd sized
};


class WavPcmReadJob : public Job
{
public:
    typedef JobQueue queue_t;

    WavPcmReadJob(std::istream & is,queue_t & to)
        : _is(is)
        , _to(to)
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

        _dataSize = getLong(buff);

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
                  << "BitSize:" << _bitSize << " "
                  << "Datasize:" << _dataSize << std::endl;


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
    virtual
    ~WavPcmReadJob()
    {
        std::cerr << "Samples read:" << _samplesRead << std::endl;
    }

public:
    bool run() override
    {
        static const size_t chunkSize = 1000;
        std::vector<uint8_t> buff(_frameSize*chunkSize);

        if(!_is)
        {
			_to.finish();
            return false;
        }

        _is.read((char *)buff.data(),buff.size());

        if(_is.gcount()==0)
        {
            _to.finish();
            return false;
        }
		

        if(_is.gcount() % (_frameSize) != 0 )
        {
            std::stringstream ss;
            ss << " expected read bytes to be multiples of " << _frameSize << " found " << _is.gcount();
            throw std::runtime_error(ss.str());
        }

        buff.resize(_is.gcount());

        queue_t::dataptr_t data(new queue_t::Data( buff.size() / _frameSize * _channels ));

        size_t iidx = 0;
        size_t oidx = 0;

        uint8_t *ptr = buff.data();

        for(int i=0;i<_is.gcount() / _frameSize ;i++)
        {
            for(int j=0;j<_channels;j++)
            {
                long lo=0;
                unsigned long mx=0;

                for(int k=1;k<=_bitSize/8;k++)
                {
                    lo <<= 8;
                    mx <<= 8;
                    lo  |= ptr[ _bitSize / 8 - k ];
                    mx  |= 0xff;
                }

                ptr += _bitSize/8;
                data->_vector[oidx] = 1.0f - float(lo) / float( mx / 2 );
                oidx++;
            }
        }

        _samplesRead += data->_vector.size();

        _to.push(data);
        return true;
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
    size_t _dataSize;
    size_t _samplesRead=0;
    static const uint8_t * _dummyRef;
    std::istream & _is;
    queue_t & _to;
};

const uint8_t * WavPcmReadJob::_dummyRef;


class WavPcmWriteJob : public Job
{
    typedef Job super;
public:
    typedef JobQueue queue_t;

    WavPcmWriteJob(std::ostream & os,queue_t & from)
        : _os(os)
        , _from(from)
    {
        writeHeader();
    }

    WavPcmWriteJob(const std::string & fname,queue_t & from)
        : _os(_of)
        , _from(from)
    {
        _of.open(fname);
        writeHeader();
    }

    virtual
    ~WavPcmWriteJob() override
    {
        _from.finish();
        if(_os)
        {
            _os.seekp(4,std::ios::beg);
            std::vector<uint8_t> size(4);
            putLong(size.data(),_dataSize+34);
            _os.write((char*)size.data(),size.size());

            _os.seekp(40,std::ios::beg);
            putLong(size.data(),_dataSize);
            _os.write((char*)size.data(),size.size());
        }
    }

    bool run() override
    {
        queue_t::dataptr_t data;

        if(!_from.pop(data))
        {
            return false;
        }

        std::vector<int8_t> buff(data->_vector.size());
        int8_t *ptr = buff.data();

        for(size_t i=0;i<data->_vector.size();i++)
        {
            // std::cerr << data->_vector[i]  << "::"
            //          << (int)int8_t(data->_vector[i] * float(0x7f)) << std::endl;

            int8_t v = int8_t(data->_vector[i] * float(0x7f));
            *(ptr++) = v;
        }

        if( ! _os.write((char *)buff.data(),buff.size()) )
        {
            return false;
        }

        _dataSize+=buff.size();

        return true;
    }

private:

    void writeHeader()
    {
        std::string tag="RIFF";
        _os.write(tag.c_str(),tag.size());
        std::vector<uint8_t> size(4);

        putLong(size.data(),0x01020304);
        _os.write((char*)size.data(),size.size());

        tag="WAVE";
        _os.write(tag.c_str(),tag.size());

        tag="fmt ";
        _os.write(tag.c_str(),tag.size());

        putLong(size.data(),16);
        _os.write((char*)size.data(),size.size());

        std::vector<uint8_t> fmt(16);
        uint8_t * fmpt = fmt.data();

        putWord(fmpt,1,fmpt); //PCM
        putWord(fmpt,1,fmpt); // CHANELS
        putLong(fmpt,44100,fmpt);
        putLong(fmpt,176400/2,fmpt);
        putWord(fmpt,1,fmpt); // frame
        putWord(fmpt,8,fmpt); // bits

        _os.write((char*)fmt.data(),fmt.size());

        tag="data";
        _os.write(tag.c_str(),tag.size());

        putLong(size.data(),0xf1f2f3f4);
        _os.write((char*)size.data(),size.size());
    }


    void putLong(uint8_t *data,unsigned long l, uint8_t * & next=_dummy)
    {
        for(int i=0;i<4;i++)
        {
            data[i] = uint8_t(l & 0xff);
            l >>= 8;
        }
        next = data+4;
    }
    void putWord(uint8_t *data,unsigned int l,uint8_t * & next = _dummy)
    {
        for(int i=0;i<2;i++)
        {
            data[i] = uint8_t(l & 0xff);
            l >>= 8;
        }
        next = data+2;
    }
    size_t _dataSize=0;
    std::ofstream _of;
    std::ostream & _os;
    queue_t & _from;
    static uint8_t * _dummy;
};

uint8_t * WavPcmWriteJob::_dummy;

int main(int argc, char **argv)
{
    if(argc!=2)
    {
        std::cerr << "Usage: wavefilter audio.wav" << std::endl;
        exit(1);
    }

	std::ifstream fi;
    fi.open(argv[1],std::ios::binary);

    // Data queue frok file to split-job
    JobQueue read_q;

    // left output queue for split-job
    JobQueue left_q;

    // right output queue for split-job
    JobQueue right_q;

    JobPool jp;

    // Pool of jobs read->split->write
    JobPool::jobptr_t read_j  = JobPool::jobptr_t(new WavPcmReadJob(fi,read_q));
    JobPool::jobptr_t split_j = JobPool::jobptr_t(new SplitJob(read_q,left_q,right_q));
    JobPool::jobptr_t left_j = JobPool::jobptr_t(new WavPcmWriteJob("left.wav",left_q));
    JobPool::jobptr_t right_j = JobPool::jobptr_t(new WavPcmWriteJob("right.wav",right_q));

    // Add jobs ti pool
    jp.addJobs({read_j,split_j,left_j,right_j});

    // start the bool
    jp.start();

    // pool waits for all threads in destructor
    return 0;
}
