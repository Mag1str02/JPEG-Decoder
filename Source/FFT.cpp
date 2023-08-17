#include "FFT.h"
#include "Exceptions.h"


class DctCalculator::Impl
{
private:
    size_t               width_;
    std::vector<double>* input_;
    std::vector<double>* output_;

public:
    void Inverse()
    {
        double*   in  = fftw_alloc_real(width_ * width_);
        double*   out = fftw_alloc_real(width_ * width_);
        fftw_plan p   = fftw_plan_r2r_2d(width_, width_, in, out, FFTW_REDFT01, FFTW_REDFT01, FFTW_ESTIMATE);

        for (size_t x = 0; x < width_; ++x)
        {
            for (size_t y = 0; y < width_; ++y)
            {
                size_t id       = y * width_ + x;
                double fraction = 1.0;
                if (x == 0)
                {
                    fraction *= sqrt(2);
                }
                if (y == 0)
                {
                    fraction *= sqrt(2);
                }

                in[id] = (*input_)[id] * fraction;
            }
        }

        fftw_execute(p);

        for (size_t i = 0; i < width_ * width_; ++i)
        {
            (*output_)[i] = out[i] / 16;
        }

        fftw_destroy_plan(p);
        fftw_free(in);
        fftw_free(out);
    }
    Impl(size_t width, std::vector<double>* input, std::vector<double>* output) : width_(width), input_(input), output_(output)
    {
        INVALID_ARGUMENT_IF(width <= 1, "Small width.");
        INVALID_ARGUMENT_IF(input_->size() != width_ * width_, "Input size mismatch.");
        INVALID_ARGUMENT_IF(output_->size() != width_ * width_, "Input size mismatch.");
    }
};

DctCalculator::DctCalculator(size_t width, std::vector<double>* input, std::vector<double>* output) :
    impl_(std::make_unique<Impl>(width, input, output))
{}

void DctCalculator::Inverse() { impl_->Inverse(); }

DctCalculator::~DctCalculator() = default;

void NewFFT::Inverse(const std::vector<double>& input, std::vector<double>& output)
{
    INVALID_ARGUMENT_IF(input.size() != width_ * width_, "Input size mismatch.");
    INVALID_ARGUMENT_IF(output.size() != width_ * width_, "Output size mismatch.");

    for (size_t x = 0; x < width_; ++x)
    {
        for (size_t y = 0; y < width_; ++y)
        {
            size_t id       = y * width_ + x;
            double fraction = 1.0;
            if (x == 0)
            {
                fraction *= sqrt(2);
            }
            if (y == 0)
            {
                fraction *= sqrt(2);
            }

            in_->at(id) = input[id] * fraction;
        }
    }

    fftw_execute(p_);
    fftw_cleanup();

    for (size_t i = 0; i < width_ * width_; ++i)
    {
        output[i] = out_->at(i) / 16;
    }
}
NewFFT::NewFFT(size_t width, std::vector<double>* in, std::vector<double>* out) : width_(width)
{
    in_  = in;
    out_ = out;
    p_   = fftw_plan_r2r_2d(width_, width_, &in_->at(0), &out_->at(0), FFTW_REDFT01, FFTW_REDFT01, FFTW_ESTIMATE);
}
NewFFT::~NewFFT()
{
    fftw_destroy_plan(p_);
    // fftw_free(in_);
    // fftw_free(out_);
}
