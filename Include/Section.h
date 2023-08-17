#pragma once
#include "StreamNavigator.h"
#include "Exceptions.h"
#include <memory>

struct DecoderData;

enum class SectionType {
    None = 0,
    Begin,
    Comment,
    Application,
    DQT,
    ImageInfo,
    Huffman,
    ImageData,
    End
};
std::string SectionTypeToString(SectionType type);

class SearchEndStrategy {
private:
public:
    SearchEndStrategy() = default;
    virtual ~SearchEndStrategy() = default;
    virtual size_t FindEnd(StreamNavigator& stream, size_t begin, size_t& end, size_t& length) {
        (void)stream;
        (void)begin;
        (void)end;
        (void)length;
        SECTION_ERROR_IF(true, "Wrong strategy.");
    }
};

class SimpleSearch : public SearchEndStrategy {
public:
    virtual size_t FindEnd(StreamNavigator& stream, size_t begin, size_t& end,
                           size_t& length) override {
        SECTION_ERROR_IF(begin + 2 > stream.Size(), "Section to small for SS strategy.");
        length = 0;
        end = begin + 2;
        stream.SetBoundaries(begin + 2, end);
        return end;
    }
};
class FixedLengthSearch : public SearchEndStrategy {
public:
    virtual size_t FindEnd(StreamNavigator& stream, size_t begin, size_t& end,
                           size_t& length) override {
        SECTION_ERROR_IF(begin + 4 > stream.Size(), "Section to small for FLS strategy.");
        size_t len = (stream[begin + 2] << 8) + stream[begin + 3];
        end = begin + len + 2;
        SECTION_ERROR_IF(end > stream.Size(), "Section to small for FLS specified length.");
        length = len - 2;
        stream.SetBoundaries(begin + 4, end);
        return end;
    }
};
class FixedLengthTillSearch : public SearchEndStrategy {
public:
    virtual size_t FindEnd(StreamNavigator& stream, size_t begin, size_t& end,
                           size_t& length) override {
        SECTION_ERROR_IF(begin + 4 > stream.Size(), "Section to small for FLTS strategy.");
        size_t len = (stream[begin + 2] << 8) + stream[begin + 3];
        end = begin;
        for (size_t i = begin + len + 2; i < stream.Size(); ++i) {
            if (stream[i] != 0xff) {
                continue;
            }
            if (i + 1 < stream.Size() && stream[i + 1] == 0) {
                continue;
            }
            end = i;
            break;
        }
        SECTION_ERROR_IF(end == begin, "Failed to find end of section with FLTS strategy.");
        length = len - 2;
        stream.SetBoundaries(begin + 4, end);
        return end;
    }
};

class Section {
private:
    const SectionType type_;

protected:
    StreamNavigator stream_;
    size_t begin_, end_;
    size_t length_;
    std::shared_ptr<SearchEndStrategy> search_end_strategy_;

public:
    std::string Info() const {
        return SectionTypeToString(type_) + " (" + std::to_string(begin_) + ", " +
               std::to_string(end_) + ")";
    }
    Section(SectionType type, StreamNavigator stream, size_t begin,
            std::shared_ptr<SearchEndStrategy> strategy)
        : type_(type), stream_(stream), begin_(begin), search_end_strategy_(strategy) {
    }
    SectionType Type() const {
        return type_;
    }
    size_t FindEnd() {
        return search_end_strategy_->FindEnd(stream_, begin_, end_, length_);
    }

    virtual ~Section() {
    }
    virtual void Process(DecoderData& data) {
        (void)data;
    }
};

class BeginSection : public Section {
public:
    BeginSection(StreamNavigator stream, size_t begin)
        : Section(SectionType::Begin, stream, begin, std::make_shared<SimpleSearch>()) {
    }
};
class EndSection : public Section {
public:
    EndSection(StreamNavigator stream, size_t begin)
        : Section(SectionType::End, stream, begin, std::make_shared<SimpleSearch>()) {
    }
};

class CommentSection : public Section {
public:
    CommentSection(StreamNavigator stream, size_t begin)
        : Section(SectionType::Comment, stream, begin, std::make_shared<FixedLengthSearch>()) {
    }
    virtual void Process(DecoderData& data) override;
};
class ApplicationSection : public Section {
public:
    ApplicationSection(StreamNavigator stream, size_t begin)
        : Section(SectionType::Application, stream, begin, std::make_shared<FixedLengthSearch>()) {
    }
};
class DQTSection : public Section {
public:
    DQTSection(StreamNavigator stream, size_t begin)
        : Section(SectionType::DQT, stream, begin, std::make_shared<FixedLengthSearch>()) {
    }
    virtual void Process(DecoderData& data) override;
};
class ImageInfoSection : public Section {
public:
    ImageInfoSection(StreamNavigator stream, size_t begin)
        : Section(SectionType::ImageInfo, stream, begin, std::make_shared<FixedLengthSearch>()) {
    }
    virtual void Process(DecoderData& data) override;
};
class HuffmanSection : public Section {
public:
    HuffmanSection(StreamNavigator stream, size_t begin)
        : Section(SectionType::Huffman, stream, begin, std::make_shared<FixedLengthSearch>()) {
    }
    virtual void Process(DecoderData& data) override;
};
class ImageDataSection : public Section {
public:
    ImageDataSection(StreamNavigator stream, size_t begin)
        : Section(SectionType::ImageData, stream, begin,
                  std::make_shared<FixedLengthTillSearch>()) {
    }
    virtual void Process(DecoderData& data) override;
};
