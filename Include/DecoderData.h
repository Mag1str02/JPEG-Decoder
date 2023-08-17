#pragma once

#include "Section.h"
#include "Image.h"
#include "Huffman.h"

#include <vector>
#include <memory>
#include <chrono>
#include <iostream>

struct Channel
{
    size_t                            dqt_id      = -1;
    size_t                            dc_id       = -1;
    size_t                            ac_id       = -1;
    size_t                            h           = -1;
    size_t                            w           = -1;
    bool                              valid       = false;
    bool                              valid_ac_dc = false;
    size_t                            du_per_mcu  = -1;
    std::vector<std::vector<int64_t>> du;
};

struct YCC
{
    int64_t y  = 0;
    int64_t cb = 128;
    int64_t cr = 128;
};

struct Tree
{
    HuffmanTree tree;
    bool        valid = false;

    void Print() const;
};

struct DQT
{
    bool                 valid = false;
    std::vector<int64_t> table;

    void Print() const;
};
struct DecoderData
{
    size_t begin_cnt;
    size_t end_cnt;
    size_t comment_cnt;
    size_t application_cnt;
    size_t dqt_cnt;
    size_t huffman_cnt;
    size_t image_info_cnt;
    size_t image_data_cnt;
    size_t unknown_section_cnt;

    std::vector<std::shared_ptr<Section>> sections;
    Image                                 image;
    size_t                                width, height;
    size_t                                presicion;
    size_t                                mcu_h     = 0;
    size_t                                mcu_w     = 0;
    size_t                                mcu_x_cnt = 0;
    size_t                                mcu_y_cnt = 0;
    size_t                                mcu_cnt   = 0;
    std::vector<Channel>                  channels;

    std::vector<DQT>  dqts;
    std::vector<Tree> dc, ac;

    void Info();

    void ValidateSectionSet();
    void ValidateImageInfo();
    void PreValidateImageData();

    void ProcessInverseDU();
    void FillImage();
    void PreProcessDC();
    // void Write();

    YCC GetYCCFromXY(size_t x, size_t y);
    RGB YCCToRGB(YCC ycc);
};
