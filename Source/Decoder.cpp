#include "Decoder.h"
#include "StreamNavigator.h"
#include "SectionDetector.h"

//___Decoder___________________________________________________________________________________________________________________

class Decoder
{
public:
    Decoder(std::istream& stream) : stream_(stream) {}
    Image Decode()
    {
        ReadStream();
        FindSections();
        ProcessSections();
        data_.ProcessInverseDU();
        data_.FillImage();
        // data_.Write();
        // data_.Info();
        return data_.image;
    }

private:
    std::vector<uint8_t> stream_data_;
    void                 FindSections()
    {
        SectionDetecter sec_dec(stream_data_);
        sec_dec.GetSections(data_);
        data_.ValidateSectionSet();
    }
    void ProcessSections()
    {
        for (auto& section : data_.sections)
        {
            section->Process(data_);
        }
    }
    void ReadStream()
    {
        if (!stream_.good())
        {
            throw DataError("Specified file is not valid");
        }
        stream_.seekg(0, std::ios::end);
        size_t sz = stream_.tellg();
        stream_.seekg(0, std::ios::beg);
        stream_data_.resize(sz);
        stream_.read(reinterpret_cast<char*>(stream_data_.data()), sz);
    }
    DecoderData   data_;
    std::istream& stream_;
};

//___Final_____________________________________________________________________________________________________________________

Image Decode(std::istream& input)
{
    Decoder decoder(input);
    return decoder.Decode();
}
