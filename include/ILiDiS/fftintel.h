#ifndef FFTINTEL_H
#define FFTINTEL_H

struct FFTintel 
{
    public:
        int classification_category;
        double mean = 0.0;       // mean of log-magnitude
        double variance = 0.0;   // variance
        double skewness = 0.0;   // skewness
        double kurtosis = 0.0;   // excess kurtosis
        double lowFreq = 0.0;    // fraction of energy in low radial band
        double midFreq = 0.0;    // mid radial band
        double highFreq = 0.0;   // high radial band
};

#endif