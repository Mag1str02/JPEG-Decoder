#include "DecoderData.h"
#include "FFT.h"

int Clamp(int a) {
    if (a < 0) {
        a = 0;
    }
    if (a > 255) {
        a = 255;
    }
    return a;
}

void DQT::Print() const {
    if (!valid) {
        std::cout << "  EMPTY" << std::endl;
        return;
    }
    std::cout << std::endl << std::hex;
    for (size_t y = 0; y < 8; ++y) {
        for (size_t x = 0; x < 8; ++x) {
            std::cout << std::setfill('0') << std::setw(2) << table[x + y * 8] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::dec;
}

void Tree::Print() const {
    if (!valid) {
        std::cout << " EMPTY" << std::endl;
        return;
    }
    // std::cout << " " << tree.Size() << std::endl;
    // tree.PrintCodes();
}

void DecoderData::ValidateSectionSet() {
    begin_cnt = 0;
    end_cnt = 0;
    comment_cnt = 0;
    application_cnt = 0;
    dqt_cnt = 0;
    huffman_cnt = 0;
    image_info_cnt = 0;
    image_data_cnt = 0;
    unknown_section_cnt = 0;

    for (const auto& section : sections) {
        switch (section->Type()) {
            case SectionType::Begin:
                ++begin_cnt;
                break;
            case SectionType::End:
                ++end_cnt;
                break;
            case SectionType::Comment:
                ++comment_cnt;
                break;
            case SectionType::Application:
                ++application_cnt;
                break;
            case SectionType::DQT:
                ++dqt_cnt;
                break;
            case SectionType::Huffman:
                ++huffman_cnt;
                break;
            case SectionType::ImageInfo:
                ++image_info_cnt;
                break;
            case SectionType::ImageData:
                ++image_data_cnt;
                break;
            case SectionType::None:
                ++unknown_section_cnt;
                break;
        }
    }

    SECTION_ERROR_IF(unknown_section_cnt != 0, "Unknown sections.");
    SECTION_ERROR_IF(begin_cnt != 1, "Wrong amount of Begin sections.");
    SECTION_ERROR_IF(end_cnt != 1, "Wrong amount of End sections.");
    SECTION_ERROR_IF(comment_cnt > 1, "Wrong amount of Comment sections.");
    SECTION_ERROR_IF(image_data_cnt != 1, "Wrong amount of ImageData sections.");
    SECTION_ERROR_IF(image_info_cnt != 1, "Wrong amount of ImageInfo sections.");
    SECTION_ERROR_IF(huffman_cnt == 0, "Wrong amount of Huffman sections.");
    SECTION_ERROR_IF(dqt_cnt == 0, "Wrong amount of Huffman sections.");
}
void DecoderData::ValidateImageInfo() {
    DATA_ERROR_IF(height == 0, "Invalid height.");
    DATA_ERROR_IF(width == 0, "Invalid width.");
    DATA_ERROR_IF(channels.empty() || channels.size() > 3, "Invalid channels amount.");
    for (size_t i = 0; i < channels.size(); ++i) {
        auto dqt_id = channels[i].dqt_id;
        size_t h = channels[i].h;
        size_t w = channels[i].w;
        mcu_w = std::max(mcu_w, w);
        mcu_h = std::max(mcu_h, h);
        DATA_ERROR_IF(!channels[i].valid, "Invalid channel.");
        DATA_ERROR_IF(dqts.size() <= dqt_id || !dqts[dqt_id].valid, "Invalid DQT id.");
        DATA_ERROR_IF(h == 0 || h > 2, "Wrong subsampling height.");
        DATA_ERROR_IF(w == 0 || w > 2, "Wrong subsampling width.");
    }
    for (size_t i = 0; i < channels.size(); ++i) {
        channels[i].du_per_mcu = channels[i].h * channels[i].w;
        channels[i].h = mcu_h / channels[i].h;
        channels[i].w = mcu_w / channels[i].w;
    }
    mcu_x_cnt = (((width - 1) / (mcu_w * 8)) + 1);
    mcu_y_cnt = (((height - 1) / (mcu_h * 8)) + 1);
    mcu_cnt = mcu_x_cnt * mcu_y_cnt;
}
void DecoderData::PreValidateImageData() {
    for (size_t i = 0; i < channels.size(); ++i) {
        auto dc_id = channels[i].dc_id;
        auto ac_id = channels[i].ac_id;
        DATA_ERROR_IF(!channels[i].valid_ac_dc, "Invalid ACDC.");
        DATA_ERROR_IF(dc_id >= dc.size() || !dc[dc_id].valid, "Wrong DC id.");
        DATA_ERROR_IF(ac_id >= ac.size() || !ac[ac_id].valid, "Wrong AC id.");
    }
}

void ProcessDU(std::vector<int64_t>& du, const std::vector<int64_t>& dqt, NewFFT& calculator) {
    std::vector<double> source(64);
    std::vector<double> res(64);
    for (size_t i = 0; i < dqt.size(); ++i) {
        source[i] = du[i] * dqt[i];
    }
    calculator.Inverse(source, res);
    for (size_t i = 0; i < dqt.size(); ++i) {
        du[i] = Clamp(res[i] + 128);
    }
}

void DecoderData::PreProcessDC() {
    for (auto& channel : channels) {
        auto& du = channel.du;
        for (size_t i = 1; i < du.size(); ++i) {
            du[i][0] += du[i - 1][0];
        }
    }
}
void DecoderData::ProcessInverseDU() {
    PreProcessDC();
    std::vector<double>* in = new std::vector<double>(64);
    std::vector<double>* out = new std::vector<double>(64);
    NewFFT calculator(8, in, out);
    for (auto& channel : channels) {
        for (auto& unit : channel.du) {
            ProcessDU(unit, dqts[channel.dqt_id].table, calculator);
        }
    }
    delete in;
    delete out;
}

YCC DecoderData::GetYCCFromXY(size_t x, size_t y) {
    YCC res;
    size_t mcu_id = (y / (mcu_h * 8)) * mcu_x_cnt + x / (mcu_w * 8);
    // std::cout << "X: " << x << " Y: " << y << " MCU_ID: " << mcu_id << std::endl;
    x %= (mcu_w * 8);
    y %= (mcu_h * 8);
    // std::cout << x << " " << y << std::endl;
    if (!channels.empty()) {
        int32_t c_id = 0;
        int32_t du_cnt = channels[c_id].du_per_mcu;
        int32_t c_mcu_w = channels[c_id].w;
        int32_t c_mcu_h = channels[c_id].h;
        int32_t block_x = x / (c_mcu_w * 8);
        int32_t block_y = y / (c_mcu_h * 8);
        int32_t block_r_id = block_y * (mcu_w / c_mcu_w) + block_x;
        int32_t r_x = (x % (c_mcu_w * 8)) / c_mcu_w;
        int32_t r_y = (y % (c_mcu_h * 8)) / c_mcu_h;
        int32_t r_id = r_y * 8 + r_x;
        int32_t block_id = mcu_id * du_cnt + block_r_id;
        // std::cout << "BlockId: " << block_id << " RID: " << r_id << std::endl;
        res.y = channels[c_id].du[block_id][r_id];
    }
    if (channels.size() > 1) {
        int32_t c_id = 1;
        int32_t du_cnt = channels[c_id].du_per_mcu;
        int32_t c_mcu_w = channels[c_id].w;
        int32_t c_mcu_h = channels[c_id].h;
        int32_t block_x = x / (c_mcu_w * 8);
        int32_t block_y = y / (c_mcu_h * 8);
        int32_t block_r_id = block_y * (mcu_w / c_mcu_w) + block_x;
        int32_t r_x = (x % (c_mcu_w * 8)) / c_mcu_w;
        int32_t r_y = (y % (c_mcu_h * 8)) / c_mcu_h;
        int32_t r_id = r_y * 8 + r_x;
        int32_t block_id = mcu_id * du_cnt + block_r_id;
        // std::cout << "BlockId: " << block_id << " RID: " << r_id << std::endl;
        res.cb = channels[c_id].du[block_id][r_id];
    }
    if (channels.size() > 2) {
        int32_t c_id = 2;
        int32_t du_cnt = channels[c_id].du_per_mcu;
        int32_t c_mcu_w = channels[c_id].w;
        int32_t c_mcu_h = channels[c_id].h;
        int32_t block_x = x / (c_mcu_w * 8);
        int32_t block_y = y / (c_mcu_h * 8);
        int32_t block_r_id = block_y * (mcu_w / c_mcu_w) + block_x;
        int32_t r_x = (x % (c_mcu_w * 8)) / c_mcu_w;
        int32_t r_y = (y % (c_mcu_h * 8)) / c_mcu_h;
        int32_t block_id = mcu_id * du_cnt + block_r_id;
        // std::cout << "BlockId: " << block_id << " RID: " << r_id << std::endl;
        res.cr = channels[c_id].du[block_id][r_y * 8 + r_x];
    }
    // std::cout << std::endl;
    // std::cout << res.y << " " << res.cb << " " << res.cr << std::endl;
    return res;
}
RGB DecoderData::YCCToRGB(YCC ycc) {
    RGB res;
    double y = static_cast<double>(ycc.y);
    double cb = static_cast<double>(ycc.cb - 128);
    double cr = static_cast<double>(ycc.cr - 128);
    res.r = Clamp(std::round(y + 1.402000 * cr));
    res.g = Clamp(std::round(y - 0.344136 * cb - 0.714136 * cr));
    res.b = Clamp(std::round(y + 1.772000 * cb));

    return res;
}

void UpdatePixel(DecoderData& data, size_t y_begin, size_t y_end) {
    for (size_t y = y_begin; y < y_end; ++y) {
        for (size_t x = 0; x < data.width; ++x) {
            auto p = data.YCCToRGB(data.GetYCCFromXY(x, y));
            data.image.SetPixel(y, x, p);
        }
    }
}

void DecoderData::FillImage() {
    // std::cout << channels[0].du.size() << " " << channels[1].du.size() << " "
    //           << channels[2].du.size() << std::endl;
    std::vector<std::thread> threads;
    size_t num_threads = std::thread::hardware_concurrency();
    std::vector<size_t> boundaries;
    size_t step = height / num_threads;
    for (size_t i = 0; i < num_threads; ++i) {
        boundaries.push_back(step * i);
    }
    boundaries.push_back(height);
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back(UpdatePixel, std::ref(*this), boundaries[i], boundaries[i + 1]);
    }
    for (auto& t : threads) {
        t.join();
    }
}

void DecoderData::Info() {
    std::cout << std::endl;
    std::cout << "STRUCTS_AMOUNT:" << std::endl;
    std::cout << "  DQT: " << dqts.size() << std::endl;
    for (size_t i = 0; i < dqts.size(); ++i) {
        dqts[i].Print();
        std::cout << std::endl;
    }
    std::cout << "  Huffman DC: " << dc.size() << std::endl;
    for (size_t i = 0; i < dc.size(); ++i) {
        dc[i].Print();
        std::cout << std::endl;
    }
    std::cout << "  Huffman AC: " << ac.size() << std::endl;
    for (size_t i = 0; i < ac.size(); ++i) {
        ac[i].Print();
        std::cout << std::endl;
    }
    std::cout << "SECTIONS:" << std::endl;
    std::cout << "Begin: " << begin_cnt << std::endl;
    std::cout << "End: " << end_cnt << std::endl;
    std::cout << "Comment: " << comment_cnt << std::endl;
    std::cout << "Unknow_section_cnt: " << unknown_section_cnt << std::endl;
    std::cout << "  Application: " << application_cnt << std::endl;
    std::cout << "  DQT: " << dqt_cnt << std::endl;
    std::cout << "  Huffman: " << huffman_cnt << std::endl;
    std::cout << "  ImageData: " << image_data_cnt << std::endl;
    std::cout << "  ImageInfo: " << image_info_cnt << std::endl;
    std::cout << "PICTURE: " << std::endl;
    std::cout << "  Size: " << width << "x" << height << std::endl;
    std::cout << "  MCU: " << mcu_w << "x" << mcu_h << " " << mcu_cnt << std::endl;
    std::cout << "  Channels: " << channels.size() << std::endl;
    for (size_t i = 0; i < channels.size(); ++i) {
        std::cout << "      " << i + 1 << ": DU per MCU: " << channels[i].du_per_mcu
                  << " Subsampling mode: " << channels[i].w << "x" << channels[i].h
                  << " DQT: " << channels[i].dqt_id << " DC: " << channels[i].dc_id
                  << " AC: " << channels[i].ac_id << std::endl;
    }
    std::cout << std::endl;
}

// void DecoderData::Write() {
// }