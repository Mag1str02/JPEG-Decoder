#pragma once

#include "Exceptions.h"

class StreamNavigator {
private:
    const std::vector<uint8_t>& data_;
    size_t beg_ = 0;
    size_t end_ = 0;

public:
    StreamNavigator(const std::vector<uint8_t>& data) : data_(data) {
        end_ = data_.size();
    }
    int operator[](size_t id) {
        return data_.at(beg_ + id);
    }
    int Bit(size_t id) {
        uint8_t tmp = data_.at(beg_ + id / 8);
        return ((tmp >> (7 - id % 8)) & 1);
    }
    size_t Size() {
        return end_ - beg_;
    }
    size_t BitSize() {
        return (end_ - beg_) * 8;
    }
    void SetBoundaries(size_t beg, size_t end) {
        THROW_IF(end < beg, "End less than begin.");
        THROW_IF(end > data_.size(), "End bigger than file size.");
        beg_ = beg;
        end_ = end;
    }
    void MoveBegin(size_t delta) {
        THROW_IF(delta + beg_ > end_, "Invalid move delta.");
        beg_ += delta;
    }
};
