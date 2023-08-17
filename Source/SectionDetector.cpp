#include "SectionDetector.h"
#include <iostream>
#include <algorithm>

std::shared_ptr<Section> SectionDetecter::CreateCurrentSection() {
    int ff = stream_[pos_];
    int marker = stream_[pos_ + 1];
    SECTION_ERROR_IF(ff != 255, "Unknow current section.");
    if (0xe0 <= marker && marker <= 0xef) {
        return std::make_shared<ApplicationSection>(stream_, pos_);
    }
    switch (marker) {
        case 0xd8:
            return std::make_shared<BeginSection>(stream_, pos_);
            break;
        case 0xd9:
            return std::make_shared<EndSection>(stream_, pos_);
            break;
        case 0xfe:
            return std::make_shared<CommentSection>(stream_, pos_);
            break;
        case 0xdb:
            return std::make_shared<DQTSection>(stream_, pos_);
            break;
        case 0xda:
            return std::make_shared<ImageDataSection>(stream_, pos_);
            break;
        case 0xc0:
            return std::make_shared<ImageInfoSection>(stream_, pos_);
            break;
        case 0xc4:
            return std::make_shared<HuffmanSection>(stream_, pos_);
            break;
        default:
            SECTION_ERROR_IF(true, "Unknow current section.");
            break;
    }
}

SectionDetecter::SectionDetecter(const StreamNavigator& stream) : stream_(stream) {
}
void SectionDetecter::GetSections(DecoderData& dec) {
    while (pos_ != stream_.Size()) {
        auto section = CreateCurrentSection();
        pos_ = section->FindEnd();
        dec.sections.push_back(section);
        if (section->Type() == SectionType::End) {
            break;
        }
    }
    std::sort(dec.sections.begin(), dec.sections.end(),
              [](const auto& a, const auto& b) { return a->Type() < b->Type(); });
}
