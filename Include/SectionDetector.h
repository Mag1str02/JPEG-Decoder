#include "Section.h"
#include "DecoderData.h"

class SectionDetecter {
private:
    StreamNavigator stream_;
    size_t pos_ = 0;

    std::shared_ptr<Section> CreateCurrentSection();

public:
    SectionDetecter(const StreamNavigator& stream);
    void GetSections(DecoderData& dec);
};
