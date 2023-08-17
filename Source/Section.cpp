#include "Section.h"
#include "DecoderData.h"
#include <iostream>
#include <string>
#include "MCUReader.h"

void CommentSection::Process(DecoderData& data) {
    std::string comment;
    comment.resize(stream_.Size());
    for (size_t i = 0; i < stream_.Size(); ++i) {
        comment[i] = stream_[i];
    }
    data.image.SetComment(comment);
}
void DQTSection::Process(DecoderData& data) {
    while (stream_.Size() > 0) {
        size_t id = First(stream_[0]);
        size_t bytes = Last(stream_[0]);

        DATA_ERROR_IF(bytes > 1, "Wrong bytes per value.");
        DATA_ERROR_IF(bytes == 1 && stream_.Size() < 129, "Wrong DQT section size.");
        DATA_ERROR_IF(bytes == 0 && stream_.Size() < 65, "Wrong DQT section size.");

        while (data.dqts.size() <= id) {
            data.dqts.push_back({});
        }
        data.dqts[id].table = std::vector<int64_t>(64, 0);
        for (size_t i = 0; i < 64; ++i) {
            if (bytes == 0) {
                data.dqts[id].table[kTransformX[i] + kTransformY[i] * 8] = stream_[i + 1];
            } else {
                data.dqts[id].table[kTransformX[i] + kTransformY[i] * 8] =
                    (stream_[2 * i + 1] << 8) + stream_[2 * i + 2];
            }
        }
        data.dqts[id].valid = true;
        if (bytes == 1) {
            stream_.MoveBegin(129);
        } else {
            stream_.MoveBegin(65);
        }
    }
}
void ImageInfoSection::Process(DecoderData& data) {
    // SECTION_ERROR_IF(stream_.Size() < 6, "ImageInfo to short.");
    data.presicion = stream_[0];
    data.height = (stream_[1] << 8) + stream_[2];
    data.width = (stream_[3] << 8) + stream_[4];
    data.image.SetSize(data.width, data.height);
    data.channels.resize(stream_[5]);
    // SECTION_ERROR_IF(stream_.Size() != 6ull + 3ull * stream_[5], "Wrong ImageInfo size.");

    for (size_t i = 6; i < 6 + 3 * data.channels.size(); i += 3) {
        size_t id = stream_[i] - 1;
        size_t sampling_mode = stream_[i + 1];
        size_t dqt_id = stream_[i + 2];
        DATA_ERROR_IF(id >= data.channels.size(), "Wrong channel id.");
        size_t horizontal = Last(sampling_mode);
        size_t vertical = First(sampling_mode);

        data.channels[id].w = horizontal;
        data.channels[id].h = vertical;
        data.channels[id].dqt_id = dqt_id;
        data.channels[id].valid = true;
    }
    data.ValidateImageInfo();
}
void HuffmanSection::Process(DecoderData& data) {
    while (stream_.Size() > 0) {
        // SECTION_ERROR_IF(stream_.Size() < 17, "To small Huffman.");
        size_t id = First(stream_[0]);
        size_t ac_dc = Last(stream_[0]);

        DATA_ERROR_IF(ac_dc > 1, "Wrong AC/DC bit.");

        std::vector<Tree>* ac_dc_vec;
        if (ac_dc == 1) {
            ac_dc_vec = &(data.ac);
        } else {
            ac_dc_vec = &(data.dc);
        }

        while (ac_dc_vec->size() <= id) {
            ac_dc_vec->push_back({});
        }
        std::vector<uint8_t> code_lengths(16);
        size_t values_amount = 0;
        for (size_t i = 0; i < 16; ++i) {
            code_lengths[i] = stream_[i + 1];
            values_amount += stream_[i + 1];
        }
        std::vector<uint8_t> values(values_amount);
        // SECTION_ERROR_IF(stream_.Size() < 17 + values_amount, "To small Huffman.");

        for (size_t i = 0; i < values_amount; ++i) {
            values[i] = static_cast<size_t>(stream_[i + 17]);
        }
        stream_.MoveBegin(17 + values_amount);
        ac_dc_vec->back().valid = true;
        ac_dc_vec->back().tree.Build(code_lengths, values);
    }
}
void ImageDataSection::Process(DecoderData& data) {
    // SECTION_ERROR_IF(stream_.Size() < 1, "To small ImageData.");
    size_t c_amount = stream_[0];
    DATA_ERROR_IF(c_amount != data.channels.size(), "Channel amount mismatch.");
    // SECTION_ERROR_IF(stream_.Size() < 4 + 2 * c_amount, "To small to contain channel info.");
    for (size_t i = 1; i < 1 + 2 * c_amount; i += 2) {
        size_t c_id = stream_[i] - 1;
        size_t dc_id = Last(stream_[i + 1]);
        size_t ac_id = First(stream_[i + 1]);
        DATA_ERROR_IF(c_id >= c_amount || !data.channels[c_id].valid, "Wrong channel id.");
        data.channels[c_id].dc_id = dc_id;
        data.channels[c_id].ac_id = ac_id;
        data.channels[c_id].valid_ac_dc = true;
    }
    DATA_ERROR_IF(stream_[1 + 2 * c_amount] != 0, "Wrong prog.");
    DATA_ERROR_IF(stream_[2 + 2 * c_amount] != 0x3f, "Wrong prog.");
    DATA_ERROR_IF(stream_[3 + 2 * c_amount] != 0, "Wrong prog.");
    data.PreValidateImageData();
    stream_.MoveBegin(4 + 2 * c_amount);

    // for (size_t i = 0; i < stream_.BitSize(); ++i) {
    //     std::cout << stream_.bit(i);
    // }
    // std::cout << std::endl;

    MCUReader reader(stream_, data);
    reader.ReadData();
}

std::string SectionTypeToString(SectionType type) {
    switch (type) {
        case SectionType::None:
            return "Unknown";
        case SectionType::Begin:
            return "Begin";
        case SectionType::End:
            return "End";
        case SectionType::Comment:
            return "Comment";
        case SectionType::DQT:
            return "DQT";
        case SectionType::ImageInfo:
            return "ImageInfo";
        case SectionType::Huffman:
            return "Huffman";
        case SectionType::ImageData:
            return "ImageData";
        case SectionType::Application:
            return "Application";
    }
}