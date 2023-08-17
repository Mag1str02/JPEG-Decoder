#pragma once

#include "StreamNavigator.h"
#include "DecoderData.h"

const std::vector<size_t> kTransformX = {
    0,                       //
    1, 0,                    //
    0, 1, 2,                 //
    3, 2, 1, 0,              //
    0, 1, 2, 3, 4,           //
    5, 4, 3, 2, 1, 0,        //
    0, 1, 2, 3, 4, 5, 6,     //
    7, 6, 5, 4, 3, 2, 1, 0,  //
    1, 2, 3, 4, 5, 6, 7,     //
    7, 6, 5, 4, 3, 2,        //
    3, 4, 5, 6, 7,           //
    7, 6, 5, 4,              //
    5, 6, 7,                 //
    7, 6,                    //
    7                        //
};
const std::vector<size_t> kTransformY = {
    0,                       //
    0, 1,                    //
    2, 1, 0,                 //
    0, 1, 2, 3,              //
    4, 3, 2, 1, 0,           //
    0, 1, 2, 3, 4, 5,        //
    6, 5, 4, 3, 2, 1, 0,     //
    0, 1, 2, 3, 4, 5, 6, 7,  //
    7, 6, 5, 4, 3, 2, 1,     //
    2, 3, 4, 5, 6, 7,        //
    7, 6, 5, 4, 3,           //
    4, 5, 6, 7,              //
    7, 6, 5,                 //
    6, 7,                    //
    7,                       //
};

size_t Last(size_t n) {
    return (n & 0xf0) >> 4;
}
size_t First(size_t n) {
    return (n & 0x0f);
}

class BitReader {
private:
    StreamNavigator stream_;
    size_t pos_ = 0;

public:
    BitReader(StreamNavigator stream) : stream_(stream) {
    }
    size_t Pos() const {
        return pos_;
    }

    int Read() {
        if (pos_ % 8 == 0 && pos_ != 0) {
            size_t i = pos_ / 8;
            if (stream_[i - 1] == 0xff) {
                DATA_ERROR_IF(stream_[i] != 0, "FF without 00.");
                pos_ += 8;
            }
        }
        // std::cout << stream_.bit(pos_) << std::flush;
        return stream_.Bit(pos_++);
    }
};

class MCUReader {
private:
    BitReader reader_;
    DecoderData& data_;
    int64_t HuffmanValue(HuffmanTree& tree) {
        int value;
        while (!tree.Move(reader_.Read(), value)) {
        }
        return value;
    }
    std::pair<int64_t, int64_t> ReadPair(HuffmanTree& tree, bool dc = false) {
        std::pair<int64_t, int64_t> ans = {0, 0};
        int value = HuffmanValue(tree);
        int len = 0;
        if (dc) {
            ans.first = 0;
            len = value;
        } else {
            ans.first = Last(value);
            len = First(value);
        }
        DATA_ERROR_IF(len >= 16, "Oops, now you have to fix it.");
        if (!len) {
            return ans;
        }
        int cnt = len;

        ans.second = reader_.Read();
        int mult = ans.second;
        --cnt;
        while (cnt) {
            --cnt;
            ans.second <<= 1;
            ans.second += reader_.Read();
        }
        if (!mult) {
            ans.second = ans.second - (1 << len) + 1;
        }
        return ans;
    }
    std::vector<std::pair<int64_t, int64_t>> ReadPairs(HuffmanTree& dc, HuffmanTree& ac) {
        std::vector<std::pair<int64_t, int64_t>> ans;
        ans.reserve(64);
        ans.push_back(ReadPair(dc, true));
        size_t cnt_elems = 1;
        while (cnt_elems < 64) {
            auto pair = ReadPair(ac);
            ans.push_back(pair);
            cnt_elems += 1 + pair.first;
            if (ans.back().first == 0 && ans.back().second == 0) {
                cnt_elems = 64;
                break;
            }
        }
        DATA_ERROR_IF(cnt_elems != 64, "Wrong elemnts amount for du.");
        // std::cout << "\nDU readed" << std::endl;
        return ans;
    }
    std::vector<int64_t> ReadDU(HuffmanTree& dc, HuffmanTree& ac) {
        auto pairs = ReadPairs(dc, ac);
        // std::cout << "|" << std::flush;
        // std::cout << std::endl;
        // for (auto [zero, value] : pairs) {
        //     std::cout << zero << " " << value << std::endl;
        // }
        std::vector<int64_t> res(64);
        size_t current = 0;
        for (auto [zeros, value] : pairs) {
            while (zeros) {
                --zeros;
                DATA_ERROR_IF(current >= 64, "To many pairs for DU.");
                res[kTransformX[current] + 8 * kTransformY[current]] = 0;
                ++current;
            }
            DATA_ERROR_IF(current >= 64, "To many pairs for DU.");
            res[kTransformX[current] + 8 * kTransformY[current]] = value;
            ++current;
        }
        for (; current < 64; ++current) {
            res[kTransformX[current] + 8 * kTransformY[current]] = 0;
        }
        // std::cout << std::endl;
        // for (size_t y = 0; y < 8; ++y) {
        //     for (size_t x = 0; x < 8; ++x) {
        //         std::cout << res[x + y * 8] << " ";
        //     }
        //     std::cout << std::endl;
        // }
        return res;
    }

    void ReadMCU() {
        for (size_t i = 0; i < data_.channels.size(); ++i) {
            auto& dc = data_.dc[data_.channels[i].dc_id].tree;
            auto& ac = data_.ac[data_.channels[i].ac_id].tree;
            for (size_t k = 0; k < data_.channels[i].du_per_mcu; ++k) {
                // std::cout << "Reading block with: " << data_.channels[i].dc_id << " "
                //           << data_.channels[i].ac_id << std::endl;
                data_.channels[i].du.push_back(ReadDU(dc, ac));
            }
        }
    }

public:
    MCUReader(StreamNavigator stream, DecoderData& data) : reader_(stream), data_(data) {
    }
    void ReadData() {
        // std::cout << data_.mcu_cnt << std::endl;
        for (size_t i = 0; i < data_.mcu_cnt; ++i) {
            ReadMCU();
            // std::cout << std::endl << i + 1 << " MCU readed " << std::endl;
        }
    }
};