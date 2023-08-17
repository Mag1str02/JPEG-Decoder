#pragma once

#include "STDInclude.h"
#include <fftw3.h>

class DctCalculator {
public:
    // input and output are width by width matrices, first row, then
    // the second row.
    DctCalculator(size_t width, std::vector<double> *input, std::vector<double> *output);

    void Inverse();

    ~DctCalculator();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

class NewFFT {
private:
    size_t width_;
    std::vector<double>* in_;
    std::vector<double>* out_;
    fftw_plan p_;

public:
    void Inverse(const std::vector<double>& input, std::vector<double>& output);
    NewFFT(size_t width, std::vector<double>* in, std::vector<double>* out);
    ~NewFFT();
};

