#include "Huffman.h"
#include "Exceptions.h"

class HuffmanTree::Impl {
private:
    struct Node {
        std::shared_ptr<Node> left = nullptr;
        std::shared_ptr<Node> right = nullptr;
        int64_t value = -1;

        bool IsTerminal() const {
            return value != -1;
        }
    };

    size_t size_ = 0;
    std::shared_ptr<Node> root_;
    std::shared_ptr<Node> current_;
    std::vector<size_t> traverse_;
    std::vector<std::pair<size_t, std::vector<bool>>> codes_;

    bool AddCode(std::shared_ptr<Node> current, size_t len, size_t value) {
        if (len == 0) {
            if (!current->IsTerminal()) {
                current->value = value;
                return true;
            }
            return false;
        }
        if (!current->left) {
            current->left = std::make_shared<Node>();
        }
        codes_.back().second.push_back(false);
        if (!current->left->IsTerminal() && AddCode(current->left, len - 1, value)) {
            return true;
        }
        codes_.back().second.pop_back();
        if (!current->right) {
            current->right = std::make_shared<Node>();
        }
        codes_.back().second.push_back(true);
        if (!current->right->IsTerminal() && AddCode(current->right, len - 1, value)) {
            return true;
        }
        codes_.back().second.pop_back();
        return false;
    }

public:
    Impl(const std::vector<uint8_t> &code_lengths, const std::vector<uint8_t> &values) {
        root_ = std::make_shared<Node>();
        current_ = root_;

        size_t current_code = 0;
        INVALID_ARGUMENT_IF(code_lengths.size() > 16, "Max len of huffman is 16.");
        for (size_t i = 0; i < code_lengths.size(); ++i) {
            for (size_t k = 0; k < code_lengths[i]; ++k) {
                INVALID_ARGUMENT_IF(current_code >= values.size(), "To many codes in code_lengts.");
                codes_.push_back({values[current_code], {}});
                INVALID_ARGUMENT_IF(!AddCode(root_, i + 1, values[current_code++]),
                                    "Impossible to add key.");
            }
        }
        INVALID_ARGUMENT_IF(current_code != values.size(), "To many values.");

        size_ = current_code;
    }
    bool Move(bool bit, int &value) {
        INVALID_ARGUMENT_IF(!current_, "Tree is not builded yet.");
        if (!bit) {
            INVALID_ARGUMENT_IF(!current_->left, "Cannot go left in tree.");
            current_ = current_->left;
        } else {
            INVALID_ARGUMENT_IF(!current_->right, "Cannot go right in tree.");
            current_ = current_->right;
        }
        if (current_->IsTerminal()) {
            value = current_->value;
            current_ = root_;
            return true;
        }
        return false;
    }
    size_t Size() const {
        return size_;
    }

    void PrintCodes() const {
        for (const auto &[value, code] : codes_) {
            std::cout << std::setfill('0') << std::setw(2) << std::hex << value << ": ";
            for (auto v : code) {
                std::cout << (v ? '1' : '0');
            }
            std::cout << std::endl;
        }
        std::cout << std::dec;
    }
};

void HuffmanTree::Build(const std::vector<uint8_t> &code_lengths,
                        const std::vector<uint8_t> &values) {
    impl_ = std::make_unique<Impl>(code_lengths, values);
}

// Moves the state of the huffman tree by |bit|. If the node is terminated,
// returns true and overwrites |value|. If it is intermediate, returns false
// and value is unmodified.
bool HuffmanTree::Move(bool bit, int &value) {
    INVALID_ARGUMENT_IF(!impl_, "Tree is not builded yet.");
    return impl_->Move(bit, value);
}
// size_t HuffmanTree::Size() const {
//     INVALID_ARGUMENT_IF(!impl_, "Tree is not builded yet.");
//     return impl_->Size();
// }
// void HuffmanTree::PrintCodes() const {
//     INVALID_ARGUMENT_IF(!impl_, "Tree is not builded yet.");
//     return impl_->PrintCodes();
// }
HuffmanTree::HuffmanTree() {
}
HuffmanTree::HuffmanTree(HuffmanTree &&other) {
    impl_ = std::move(other.impl_);
}
HuffmanTree &HuffmanTree::operator=(HuffmanTree &&other) {
    impl_ = std::move(other.impl_);
    return *this;
}
HuffmanTree::~HuffmanTree() {
}
