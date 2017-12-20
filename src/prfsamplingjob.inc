//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#define _USE_MATH_DEFINES

#include "prfsamplingjob_canonical.h"

#if defined(BALLOON_3PARAM)

#define PRF_SAMPLING_JOB_CLS_NAME PrfSamplingJob_3Param

#elif defined(CANONICAL_HRF)

#define PRF_SAMPLING_JOB_CLS_NAME PrfSamplingJobCanonical

#else // CSS-pRF 5-param

#define PRF_SAMPLING_JOB_CLS_NAME PrfSamplingJob

#endif // CSS-pRF 5-param

#include <QDebug>
#include <QSettings>

#include "qmatfilemodel/qmatfilemodel.h"
#include "matio.h"

#include <OpenCL/opencl.h>
#include "prfcommon.h"
#include "prftypecastarray.h"

#include <Eigen/SVD>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>

#include <random>

#define CSS_PRF 0
#define DUM_WAN 1


static const float m_ExptMean = 0.5;
static const float m_GainMean = 10;


#ifdef CANONICAL_HRF

#else

static const float m_KappaMean = 0.65;
static const float m_GammaMean = 0.41;
static const float m_TauMean = 0.98;
static const float m_GrubbMean = 0.32;
static const float m_RhoMean = 0.34;

static const float m_V0 = 0.02;

static const float m_EpsilonMean = 1.43;

#endif


static const char *OPENCL_HDR = "#define SUBDIV %1 \n\
    #define TOT_DUR %2 \n\
    #define DUR TOT_DUR \n\
    #define NSTEPS (DUR * SUBDIV) \n\
    #define TR %3 \n\
    #define TSTEP (TR / SUBDIV) \n\
    #define AR 0 \n\
    \n\
    #define GAUSS_X_MIN %4 \n\
    #define GAUSS_X_STEP %5 \n\
    #define GAUSS_X_COUNT %6 \n\
    \n\
    #define GAUSS_Y_MIN %7 \n\
    #define GAUSS_Y_STEP %8 \n\
    #define GAUSS_Y_COUNT %9 \n\
    \n\
    #define GAUSS_RFSIZE_MIN %10 \n\
    #define GAUSS_RFSIZE_STEP %11 \n\
    #define GAUSS_RFSIZE_COUNT %12 \n\
    \n\
    #define W %13 \n\
    #define H %14 \n\
    \n\
    #define GAUSS_OFFSET(x, y, rfsize) \\\n\
        (((x * GAUSS_Y_COUNT + y) * GAUSS_RFSIZE_COUNT + rfsize) * TOT_DUR) \n\n";

static const char *OPENCL_BODY = "__kernel void reset(__global float *signals,       \n\
    __global float *flows,                                                           \n\
    __global float *volumes,                                                         \n\
    __global float *deoxys) {                                                        \n\
                                                                                     \n\
    int id = get_global_id(0);                                                       \n\
                                                                                     \n\
    signals[id] = 0.0f;                                                              \n\
    flows[id] = 1.0f;                                                                \n\
    volumes[id] = 1.0f;                                                              \n\
    deoxys[id] = 1.0f;                                                               \n\
}                                                                                    \n\
                                                                                     \n\
void sim_gauss_prod(__global float *stimulus,                                        \n\
    float u_x, float u_y, float rfsize,                                              \n\
    float *ret) {                                                                    \n\
                                                                                     \n\
    float sigma_2 = rfsize * rfsize;                                                 \n\
    int ofs = 0;                                                                     \n\
    float pdf;                                                                       \n\
    float tmp;                                                                       \n\
    float accum;                                                                     \n\
                                                                                     \n\
    for (int t = 0; t < TOT_DUR; t++) {                                              \n\
        accum = 0;                                                                   \n\
        for (int y = 0; y < H; y++) {                                                \n\
            for (int x = 0; x < W; x++) {                                            \n\
                tmp = -0.5 * (pow(x - u_x, 2) / sigma_2 + pow(y - u_y, 2) / sigma_2);    \n\
                pdf = exp(tmp) / (2 * M_PI * sigma_2);                               \n\
                accum += pdf * stimulus[ofs];                                        \n\
                ofs++;                                                               \n\
            }                                                                        \n\
         }                                                                           \n\
         ret[t] = accum;                                                             \n\
     }                                                                               \n\
}                                                                                    \n\
                                                                                     \n\
void interp_gaussian(float x, float y, float rfsize,                                 \n\
    __global float *gaussians,                                                       \n\
    float *interp) {                                                                 \n\
                                                                                     \n\
    x = (x - GAUSS_X_MIN) / GAUSS_X_STEP;                                            \n\
    y = (y - GAUSS_Y_MIN) / GAUSS_Y_STEP;                                            \n\
    rfsize = (rfsize - GAUSS_RFSIZE_MIN) / GAUSS_RFSIZE_STEP;                        \n\
                                                                                     \n\
    int x1 = (int) floor(x);                                                         \n\
    int x2 = (int) ceil(x);                                                          \n\
                                                                                     \n\
    int y1 = (int) floor(y);                                                         \n\
    int y2 = (int) ceil(y);                                                          \n\
                                                                                     \n\
    int rfsize1 = (int) floor(rfsize);                                               \n\
    int rfsize2 = (int) ceil(rfsize);                                                \n\
                                                                                     \n\
    float unused;                                                                    \n\
                                                                                     \n\
    float x_frac = fract(x, &unused);                                                \n\
    float y_frac = fract(y, &unused);                                                \n\
    float rfsize_frac = fract(rfsize, &unused);                                      \n\
                                                                                     \n\
    int ofs_A = GAUSS_OFFSET(x1, y1, rfsize1);                                       \n\
    int ofs_B = GAUSS_OFFSET(x1, y1, rfsize2);                                       \n\
    int ofs_C = GAUSS_OFFSET(x1, y2, rfsize1);                                       \n\
    int ofs_D = GAUSS_OFFSET(x1, y2, rfsize2);                                       \n\
    int ofs_E = GAUSS_OFFSET(x2, y1, rfsize1);                                       \n\
    int ofs_F = GAUSS_OFFSET(x2, y1, rfsize2);                                       \n\
    int ofs_G = GAUSS_OFFSET(x2, y2, rfsize1);                                       \n\
    int ofs_H = GAUSS_OFFSET(x2, y2, rfsize2);                                       \n\
                                                                                     \n\
    // tri-linear interpolation                                                      \n\
    for (int t = 0; t < TOT_DUR; t++) {                                              \n\
        float vA = gaussians[ofs_A + t];                                             \n\
        float vB = gaussians[ofs_B + t];                                             \n\
        float vC = gaussians[ofs_C + t];                                             \n\
        float vD = gaussians[ofs_D + t];                                             \n\
        float vE = gaussians[ofs_E + t];                                             \n\
        float vF = gaussians[ofs_F + t];                                             \n\
        float vG = gaussians[ofs_G + t];                                             \n\
        float vH = gaussians[ofs_H + t];                                             \n\
                                                                                     \n\
        float vM = vB * rfsize_frac + vA * (1.0f - rfsize_frac);                     \n\
        float vN = vD * rfsize_frac + vC * (1.0f - rfsize_frac);                     \n\
        float vO = vF * rfsize_frac + vE * (1.0f - rfsize_frac);                     \n\
        float vP = vH * rfsize_frac + vG * (1.0f - rfsize_frac);                     \n\
                                                                                     \n\
        float vR = vN * y_frac + vM * (1.0f - y_frac);                               \n\
        float vS = vO * y_frac + vP * (1.0f - y_frac);                               \n\
                                                                                     \n\
        interp[t] = vS * x_frac + vR * (1.0f - x_frac);                              \n\
    }                                                                                \n\
}                                                                                    \n\
                                                                                     \n\
                                                                                     \n\
void matmul(__global float *A, float *B,                                             \n\
        float *C,                                                                    \n\
        int mA, int nA, int mB, int nB) {                                            \n\
                                                                                     \n\
        for (int i = 0; i < mA; i++) {                                               \n\
            for (int k = 0; k < nB; k++) {                                           \n\
                int ofs = i * nB + k;                                                \n\
                float tmp = 0;                                                       \n\
                for (int m = 0; m < mB; m++) {                                       \n\
                    tmp += A[i * nA + m] * B[m * nB + k];                            \n\
                }                                                                    \n\
                C[ofs] = tmp;                                                        \n\
            }                                                                        \n\
        }                                                                            \n\
    }                                                                                \n\
                                                                                     \n\
                                                                                     \n\
void unit_length(float *A, int nA) {                                                 \n\
    float sum = 0;                                                                   \n\
    for (int i = 0; i < nA; i++) {                                                   \n\
        sum += A[i] * A[i];                                                          \n\
    }                                                                                \n\
    sum = pow(sum, 0.5f);                                                            \n\
    for (int i = 0; i < nA; i++) {                                                   \n\
        A[i] /= sum;                                                                 \n\
    }                                                                                \n\
}                                                                                    \n\
                                                                                     \n\
                                                                                     \n\
__kernel void fwd_model(__global float *stimulus,                                    \n\
    __global float *pmatrix,                                                         \n\
                                                                                     \n\
    __global float *xs,                                                              \n\
    __global float *ys,                                                              \n\
    __global float *rfsizes,                                                         \n\
    __global float *exponents,                                                       \n\
    __global float *gains,                                                           \n\
                                                                                     \n\
    __global float *kappas,                                                          \n\
    __global float *gammas,                                                          \n\
    __global float *taus,                                                            \n\
    __global float *grubbs,                                                          \n\
    __global float *rhos,                                                            \n\
                                                                                     \n\
    __global float *signals,                                                         \n\
    __global float *flows,                                                           \n\
    __global float *volumes,                                                         \n\
    __global float *deoxys,                                                          \n\
                                                                                     \n\
    __global float *Ys) {                                                            \n\
                                                                                     \n\
    int id = get_global_id(0);                                                       \n\
                                                                                     \n\
    float u_x = xs[id];                                                              \n\
    float u_y = ys[id];                                                              \n\
    float rfsize = rfsizes[id];                                                      \n\
                                                                                     \n\
                                                                                     \n\
    float kappa = kappas[id];                                                        \n\
    float gamma = gammas[id];                                                        \n\
    float tau = taus[id];                                                            \n\
    float grubb = grubbs[id];                                                        \n\
    float rho = rhos[id];                                                            \n\
                                                                                     \n\
    float exponent = exponents[id];                                                  \n\
    float gain = gains[id];                                                          \n\
                                                                                     \n\
    float s = signals[id];                                                           \n\
    float f = flows[id];                                                             \n\
    float v = volumes[id];                                                           \n\
    float q = deoxys[id];                                                            \n\
                                                                                     \n\
    float ds, df, dv, dq;                                                            \n\
                                                                                     \n\
    float k1 = 7 * rho;                                                              \n\
    float k2 = 2;                                                                    \n\
    float k3 = 2 * rho - 0.2;                                                        \n\
    const float V0 = 0.02;                                                           \n\
                                                                                     \n\
    float z[DUR];                                                                    \n\
    float Y[DUR];                                                                    \n\
                                                                                     \n\
    sim_gauss_prod(stimulus, u_x, u_y, rfsize, z);                                   \n\
                                                                                     \n\
    for (int n = 0; n < NSTEPS; n++) {                                               \n\
        int t = (int) (n / SUBDIV);                                                  \n\
                                                                                     \n\
        Y[t] = V0 * (k1 * (1 - q) + k2 * (1 - q / v) + k3 * (1 - v));                \n\
                                                                                     \n\
        ds = z[t] - kappa * s - gamma * (f - 1);                                     \n\
        df = s;                                                                      \n\
        dv = (f - pow(v, (1 / grubb))) / tau;                                        \n\
                                                                                     \n\
        dq = (f * (1 - pow((1 - rho), (1 / f))) / rho - pow(v, (1 / grubb)) * q / v) / tau; \n\
                                                                                     \n""\
        s += ds * TSTEP;                                                             \n\
        f += df * TSTEP;                                                             \n\
        v += dv * TSTEP;                                                             \n\
        q += dq * TSTEP;                                                             \n\
    }                                                                                \n""\
#ifdef DUMOULIN_WANDELL                                                              \n\
    for (int t = 0; t < DUR; t++) {                                                  \n\
        Y[t] = Y[t] * gain;                                                          \n\
    }                                                                                \n\
#else // DUMOULIN_WANDELL                                                            \n\
                                                                                     \n\
    for (int t = 0; t < DUR; t++) {                                                  \n\
        Y[t] = copysign((float) pow(fabs((double) Y[t]), (double) exponent) * gain, Y[t]);                     \n\
    }                                                                                \n\
#endif // DUMOULIN_WANDELL                                                           \n\
                                                                                     \n\
    float Y_1[TOT_DUR];                                                              \n\
    matmul(pmatrix, Y, Y_1, TOT_DUR, TOT_DUR, TOT_DUR, 1);                           \n\
    for (int t = 0; t < DUR; t++) {                                                  \n\
        Y[t] -= Y_1[t];                                                              \n\
    }                                                                                \n\
                                                                                     \n\
    for (int t = 0; t < DUR; t++) {                                                  \n\
        Ys[id * TOT_DUR + t] = Y[t];                                                 \n\
    }                                                                                \n"
"                                                                                    \n\
    signals[id] = s;                                                                 \n\
    flows[id] = f;                                                                   \n\
    volumes[id] = v;                                                                 \n\
    deoxys[id] = q;                                                                  \n\
}                                                                                    \n\
                                                                                     \n\
                                                                                     \n\
__kernel void mean_squared_error(__global float *Ys,                                 \n\
    __global float *data,                                                            \n\
    __global float *mse) {                                                           \n\
                                                                                     \n\
    int id = get_global_id(0);                                                       \n\
    int ofs = id * TOT_DUR;                                                          \n\
    float accum = 0;                                                                 \n\
    for (int t = 0; t < TOT_DUR; t++) {                                              \n\
        accum += pow(data[ofs] - Ys[ofs], 2.0f);                                     \n\
        ofs++;                                                                       \n\
    }                                                                                \n\
    mse[id] = accum / TOT_DUR;                                                       \n\
}                                                                                    \n\
                                                                                     \n\
                                                                                     \n\
__kernel void prepare_data(__global float *pmatrix,                                  \n\
    __global float *data) {                                                          \n\
                                                                                     \n\
    int id = get_global_id(0);                                                       \n\
    int ofs = id * TOT_DUR;                                                          \n\
    float data_1[TOT_DUR];                                                           \n\
    float data_2[TOT_DUR];                                                           \n\
                                                                                     \n\
    for (int t = 0; t < TOT_DUR; t++) {                                              \n\
        data_1[t] = data[ofs + t];                                                   \n\
    }                                                                                \n\
                                                                                     \n\
    matmul(pmatrix, data_1, data_2, TOT_DUR, TOT_DUR, TOT_DUR, 1);                   \n\
    for (int t = 0; t < TOT_DUR; t++) {                                              \n\
        data_1[t] -= data_2[t];                                                      \n\
    }                                                                                \n\
                                                                                     \n\
    // unit_length(data_1, TOT_DUR);                                                 \n\
    for (int t = 0; t < TOT_DUR; t++) {                                              \n\
        data[ofs + t] = data_1[t];                                                   \n\
    }                                                                                \n\
}                                                                                    \n\
                                                                                     \n\
                                                                                     \n\
#define normal_logpdf(mean, std, x) \
    (log(1.0f / (std * sqrt(2.0f * M_PI))) - (pow(x - mean, 2.0f) / (2.0f * pow(std, 2.0f)))) \n\
                                                                                     \n\
__kernel void eval_proposal(__global float *Ys,                                      \n\
    __global float *data,                                                            \n\
    __global float *noise_stdevs,                                                    \n\
    __global float *logprobs) {                                                      \n\
                                                                                     \n\
    int id = get_global_id(0);                                                       \n\
    int ofs = id * TOT_DUR;                                                          \n\
    float mean = 0;                                                                  \n\
    float stdev = 0;                                                                 \n\
    for (int t = 0; t < TOT_DUR; t++) {                                              \n\
        float diff = Ys[ofs + t] - data[ofs + t];                                    \n\
        mean += diff;                                                                \n\
        stdev += pow(diff, 2.0f);                                                    \n\
    }                                                                                \n\
    mean /= TOT_DUR;                                                                 \n\
    stdev = sqrt(stdev / TOT_DUR - pow(mean, 2.0f));                                 \n\
    // noise_stdevs[id] = min(noise_stdevs[id], stdev);                              \n\
    // stdev = noise_stdevs[id];                                                     \n\
    noise_stdevs[id] = stdev;                                                        \n\
    float logp = 0;                                                                  \n\
    for (int t = 0; t < TOT_DUR; t++) {                                              \n\
        float diff = Ys[ofs + t] - data[ofs + t];                                    \n\
        logp += normal_logpdf(0, stdev, diff);                                       \n\
    }                                                                                \n\
    logprobs[id] = logp;                                                             \n\
}                                                                                    \n"
"                                                                                    \n"
#ifdef CANONICAL_HRF
"__kernel void reset_canonical() {                                                   \n"
"    // nothing to do                                                                \n"
"}                                                                                   \n"
"                                                                                    \n"
"__kernel void fwd_model_canonical(                                                  \n"
"    __global float *stimulus,                                                       \n"
"    __global float *pmatrix,                                                        \n"
"    __global float *xs,                                                             \n"
"    __global float *ys,                                                             \n"
"    __global float *rfsizes,                                                        \n"
"    __global float *expts,                                                          \n"
"    __global float *gains,                                                          \n"
"    __global float *hrf,                                                            \n"
"             int n_hrf,                                                             \n"
"    __global float *Ys) {                                                           \n"
"                                                                                    \n"
"    int id = get_global_id(0);                                                      \n"
"                                                                                    \n"
"    float u_x = xs[id];                                                             \n"
"    float u_y = ys[id];                                                             \n"
"    float rfsize = rfsizes[id];                                                     \n"
"    float exponent = expts[id];                                                     \n"
"    float gain = gains[id];                                                         \n"
"                                                                                    \n"
"    float z[DUR];                                                                   \n"
"    float Y[TOT_DUR];                                                               \n"
"                                                                                    \n"
"    sim_gauss_prod(stimulus, u_x, u_y, rfsize, z);                                  \n"
"                                                                                    \n"
"    // do convolution with HRF                                                      \n"
"    for (int i = 0; i < TOT_DUR; i++) {                                             \n"
"                                                                                    \n"
"        float accum = 0;                                                            \n"
"                                                                                    \n"
"        for (int k = 0; k < n_hrf; k++) {                                           \n"
"            int t = i - k;                                                          \n"
"            if (t >= 0 && t < TOT_DUR) {                                            \n"
"                accum += z[t] * hrf[k];                                             \n"
"            }                                                                       \n"
"        }                                                                           \n"
"                                                                                    \n"
"        Y[i] = accum;                                                               \n"
"    }                                                                               \n"
"                                                                                    \n"
"                                                                                    \n"
"#ifdef DUMOULIN_WANDELL                                                             \n"
"    for (int t = 0; t < DUR; t++) {                                                 \n"
"        Y[t] = Y[t] * gain;                                                         \n"
"    }                                                                               \n"
"#else // DUMOULIN_WANDELL                                                           \n"
"    // exponent + gain                                                              \n"
"    for (int t = 0; t < DUR; t++) {                                                 \n"
"        Y[t] = copysign((float) pow(fabs((double) Y[t]), (double) exponent) * gain, Y[t]);                    \n"
"    }                                                                               \n"
"#endif // DUMOULIN_WANDELL                                                          \n"
"                                                                                    \n"
"    float Y_1[TOT_DUR];                                                             \n"
"                                                                                    \n"
"    // detrend                                                                      \n"
"    matmul(pmatrix, Y, Y_1, TOT_DUR, TOT_DUR, TOT_DUR, 1);                          \n"
"    for (int t = 0; t < DUR; t++) {                                                 \n"
"        Y[t] -= Y_1[t];                                                             \n"
"    }                                                                               \n"
"                                                                                    \n"
"    // output                                                                       \n"
"    for (int t = 0; t < DUR; t++) {                                                 \n"
"        Ys[id * TOT_DUR + t] = Y[t];                                                \n"
"    }                                                                               \n"
"}                                                                                   \n"
#endif // CANONICAL_HRF
"                                                                                    \n"
#ifdef BALLOON_3PARAM
"__kernel void fwd_model_3param(                                                     \n"
"    __global float *stimulus,                                                       \n"
"    __global float *pmatrix,                                                        \n"
"    __global float *xs,                                                             \n"
"    __global float *ys,                                                             \n"
"    __global float *rfsizes,                                                        \n"
"    __global float *expts,                                                          \n"
"    __global float *gains,                                                          \n"
"    __global float *kappas,                                                         \n"
"    __global float *taus,                                                           \n"
"    __global float *epsilons,                                                       \n"
"             float TE,                                                              \n"
"    __global float *signals,                                                        \n"
"    __global float *flows,                                                          \n"
"    __global float *volumes,                                                        \n"
"    __global float *deoxys,                                                         \n"
"    __global float *Ys) {                                                           \n"
"                                                                                    \n"
"    int id = get_global_id(0);                                                      \n"
"                                                                                    \n"
"    float u_x = xs[id];                                                             \n"
"    float u_y = ys[id];                                                             \n"
"    float rfsize = rfsizes[id];                                                     \n"
"    float expt = expts[id];                                                         \n"
"    float gain = gains[id];                                                         \n"
"                                                                                    \n"
"    float tau = taus[id];                                                           \n"
"    float kappa = kappas[id];                                                       \n"
"    float epsilon = epsilons[id];                                                   \n"
"    float gamma = 0.32;                                                             \n"
"    float grubb = 0.32;                                                             \n"
"    float rho = 0.32;                                                               \n"
"                                                                                    \n"
"    float s = signals[id];                                                          \n"
"    float f = flows[id];                                                            \n"
"    float v = volumes[id];                                                          \n"
"    float q = deoxys[id];                                                           \n"
"                                                                                    \n"
"    float ds, df, dv, dq;                                                           \n"
"                                                                                    \n"
"    float V_0 = 100 * 0.08;                                                         \n"
"    float r_0 = 25;                                                                 \n"
"    float nu_0 = 40.3;                                                              \n"
"    float E_0 = 0.4;                                                                \n"
"                                                                                    \n"
"    float k_1 = 4.3 * nu_0 * E_0 * TE;                                              \n"
"    float k_2 = epsilon * r_0 * E_0 * TE;                                           \n"
"    float k_3 = 1.0 - epsilon;                                                      \n"
"                                                                                    \n"
"    float z[DUR];                                                                   \n"
"    float Y[DUR];                                                                   \n"
"                                                                                    \n"
"    sim_gauss_prod(stimulus, u_x, u_y, rfsize, z);                                  \n"
"    for (int n = 0; n < NSTEPS; n++) {                                              \n"
"        int t = (int) (n / SUBDIV);                                                 \n"
"                                                                                    \n"
"        Y[t] = V_0 * (k_1 * (1 - q) + k_2 * (1 - q / v) + k_3 * (1 - v));           \n"
"                                                                                    \n"
"        ds = z[t] - kappa * s - gamma * (f - 1);                                    \n"
"        df = s;                                                                     \n"
"        dv = (f - pow(v, (1 / grubb))) / tau;                                       \n"
"                                                                                    \n"
"        dq = (f * (1 - pow((1 - (double) rho), (1 / (double) f))) / rho - pow((double) v, (1 / (double) grubb)) * q / v) / tau;   \n"
"                                                                                    \n"
"        s += ds * TSTEP;                                                            \n"
"        f += df * TSTEP;                                                            \n"
"        v += dv * TSTEP;                                                            \n"
"        q += dq * TSTEP;                                                            \n"
"    }                                                                               \n"
"                                                                                    \n"
"#ifdef DUMOULIN_WANDELL                                                             \n"
"    for (int t = 0; t < DUR; t++) {                                                 \n"
"        Y[t] = Y[t] * gain;                                                         \n"
"    }                                                                               \n"
"#else // DUMOULIN_WANDELL                                                           \n"
"    // exponent + gain                                                              \n"
"    for (int t = 0; t < DUR; t++) {                                                 \n"
"        Y[t] = copysign((float) pow(fabs((double) Y[t]), (double) expt) * gain, Y[t]);                        \n"
"    }                                                                               \n"
"#endif                                                                              \n"
"                                                                                    \n"
"    float Y_1[TOT_DUR];                                                             \n"
"                                                                                    \n"
"    // detrend                                                                      \n"
"    matmul(pmatrix, Y, Y_1, TOT_DUR, TOT_DUR, TOT_DUR, 1);                          \n"
"    for (int t = 0; t < DUR; t++) {                                                 \n"
"        Y[t] -= Y_1[t];                                                             \n"
"    }                                                                               \n"
"                                                                                    \n"
"    // output                                                                       \n"
"    for (int t = 0; t < DUR; t++) {                                                 \n"
"        Ys[id * TOT_DUR + t] = Y[t];                                                \n"
"    }                                                                               \n"
"                                                                                    \n"
"    signals[id] = s;                                                                \n"
"    flows[id] = f;                                                                  \n"
"    volumes[id] = v;                                                                \n"
"    deoxys[id] = q;                                                                 \n"
"}                                                                                   \n"
#endif // BALLOON_3PARAM
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n"
"                                                                                    \n";

PRF_SAMPLING_JOB_CLS_NAME::PRF_SAMPLING_JOB_CLS_NAME()
{

}

const QString& PRF_SAMPLING_JOB_CLS_NAME::error() const {
    return m_Error;
}

QString PRF_SAMPLING_JOB_CLS_NAME::status() const {
    return "Not Implemented";
}

QString PRF_SAMPLING_JOB_CLS_NAME::parameters() const {

    return QString("dataFileName: %1, dataVariableSpec: %2, stage2FileName: %3, numberOfSteps: %4, temparature: %5, ")
            .arg(m_DataFileName).arg(m_DataVariableSpec).arg(m_Stage2FileName).arg(m_NumberOfSteps).arg(m_Temperature) +
        QString("xStdev: %1, yStdev: %2, rfSizeStdev: %3, ").arg(m_XStdev).arg(m_YStdev).arg(m_RFSizeStdev) +
        (m_pRF_Model == CSS_PRF ? QString("exptStdev: %1, gainStdev: %2, ")
            .arg(m_ExptStdev).arg(m_GainStdev) : QString("gainStdev: %1").arg(m_GainStdev)) +
#if defined(BALLOON_3PARAM)
        QString("kappaStdev: %1, tauStdev: %2, epsilonStdev: %3, TE: %4 ")
            .arg(m_KappaStdev).arg(m_TauStdev).arg(m_EpsilonStdev).arg(m_TE) +
#elif defined(CANONICAL_HRF)
        QString("Canonical HRF, ") +
#else
        QString("kappaStdev: %1, gammaStdev: %2, tauStdev: %3, grubbStdev: %4, rhoStdev: %5, ")
            .arg(m_KappaStdev).arg(m_GammaStdev).arg(m_TauStdev).arg(m_GrubbStdev).arg(m_RhoStdev) +
        QString("subdiv: %1, TR: %2, ").arg(m_SubDiv).arg(m_TR) +
#endif
        QString("maxNoiseStdev: %1").arg(m_MaxNoiseStdev);
}

void PRF_SAMPLING_JOB_CLS_NAME::set_pRF_Model(int pRF_Model) {
    m_pRF_Model = pRF_Model;
}

void PRF_SAMPLING_JOB_CLS_NAME::setWriteDebug(bool writeDebug) {
    m_WriteDebug = writeDebug;
}

void PRF_SAMPLING_JOB_CLS_NAME::setDataFileName(const QString &fname) {
    m_DataFileName = fname;
}

void PRF_SAMPLING_JOB_CLS_NAME::setDataVariableSpec(const QString &spec) {
    m_DataVariableSpec = spec;
}

void PRF_SAMPLING_JOB_CLS_NAME::setStage2FileName(const QString &fname) {
    m_Stage2FileName = fname;
}

void PRF_SAMPLING_JOB_CLS_NAME::setNumberOfSteps(int n) {
    m_NumberOfSteps = n;
}

void PRF_SAMPLING_JOB_CLS_NAME::setTemperature(float t) {
    m_Temperature = t;
}

void PRF_SAMPLING_JOB_CLS_NAME::setXStdev(float stdev) {
    m_XStdev = stdev;
}

void PRF_SAMPLING_JOB_CLS_NAME::setYStdev(float stdev) {
    m_YStdev = stdev;
}

void PRF_SAMPLING_JOB_CLS_NAME::setRFSizeStdev(float stdev) {
    m_RFSizeStdev = stdev;
}

void PRF_SAMPLING_JOB_CLS_NAME::setExptStdev(float stdev) {
    m_ExptStdev = stdev;
}

void PRF_SAMPLING_JOB_CLS_NAME::setGainStdev(float stdev) {
    m_GainStdev = stdev;
}

#ifdef CANONICAL_HRF



#else

void PRF_SAMPLING_JOB_CLS_NAME::setKappaStdev(float stdev) {
    m_KappaStdev = stdev;
}

void PRF_SAMPLING_JOB_CLS_NAME::setTauStdev(float stdev) {
    m_TauStdev = stdev;
}

#ifdef BALLOON_3PARAM

void PRF_SAMPLING_JOB_CLS_NAME::setEpsilonStdev(float stdev) {
    m_EpsilonStdev = stdev;
}

void PRF_SAMPLING_JOB_CLS_NAME::setTE(float TE) {
    m_TE = TE;
}

#else // BALLOON_3PARAM

void PRF_SAMPLING_JOB_CLS_NAME::setGammaStdev(float stdev) {
    m_GammaStdev = stdev;
}


void PRF_SAMPLING_JOB_CLS_NAME::setGrubbStdev(float stdev) {
    m_GrubbStdev = stdev;
}

void PRF_SAMPLING_JOB_CLS_NAME::setRhoStdev(float stdev) {
    m_RhoStdev = stdev;
}

#endif // BALLOON_3PARAM


void PRF_SAMPLING_JOB_CLS_NAME::setTR(float tr) {
    m_TR = tr;
}

void PRF_SAMPLING_JOB_CLS_NAME::setSubdiv(int n) {
    m_SubDiv = n;
}

#endif

void PRF_SAMPLING_JOB_CLS_NAME::setMaxNoiseStdev(float stdev) {
    m_MaxNoiseStdev = stdev;
}

void PRF_SAMPLING_JOB_CLS_NAME::saveConfig(const QString &fname) {
    QSettings config(fname, QSettings::IniFormat);
    config.setValue("sampling/dataFileName", m_DataFileName);
    config.setValue("sampling/dataVariableSpec", m_DataVariableSpec);
    config.setValue("sampling/stage2FileName", m_Stage2FileName);
    config.setValue("sampling/numberOfSteps", m_NumberOfSteps);
    config.setValue("sampling/temperature", m_Temperature);

    config.setValue("sampling/xStdev", m_XStdev);
    config.setValue("sampling/yStdev", m_YStdev);
    config.setValue("sampling/rfsizeStdev", m_RFSizeStdev);
    if (m_pRF_Model == CSS_PRF) {
        config.setValue("sampling/exptStdev", m_ExptStdev);
    }
    config.setValue("sampling/gainStdev", m_GainStdev);

#ifdef CANONICAL_HRF
#else
    config.setValue("sampling/subdiv", m_SubDiv);
    config.setValue("sampling/kappaStdev", m_KappaStdev);
    config.setValue("sampling/tauStdev", m_TauStdev);
    config.setValue("sampling/TR", m_TR);

#ifdef BALLOON_3PARAM
    config.setValue("sampling/epsilonStdev", m_EpsilonStdev);
    config.setValue("sampling/TE", m_TE);
#else // BALLOON_3PARAM
    config.setValue("sampling/gammaStdev", m_GammaStdev);
    config.setValue("sampling/grubbStdev", m_GrubbStdev);
    config.setValue("sampling/rhoStdev", m_RhoStdev);
#endif // BALLOON_3PARAM


#endif // CANONICAL_HRF
}

void PRF_SAMPLING_JOB_CLS_NAME::loadConfig(const QString &fname) {
    QSettings config(fname, QSettings::IniFormat);
    m_DataFileName = config.value("sampling/dataFileName").toString();
    m_DataVariableSpec = config.value("sampling/dataVariableSpec").toString();
    m_Stage2FileName = config.value("sampling/stage2FileName").toString();
    m_NumberOfSteps = config.value("sampling/numberOfSteps").toInt();
    m_Temperature = config.value("sampling/temperature").toFloat();

    m_XStdev = config.value("sampling/xStdev").toFloat();
    m_YStdev = config.value("sampling/yStdev").toFloat();
    m_RFSizeStdev = config.value("sampling/rfsizeStdev").toFloat();
    if (m_pRF_Model == CSS_PRF) {
        m_ExptStdev = config.value("sampling/exptStdev").toFloat();
    }
    m_GainStdev = config.value("sampling/gainStdev").toFloat();

#ifdef CANONICAL_HRF
#else
    m_SubDiv = config.value("sampling/subdiv").toInt();
    m_KappaStdev = config.value("sampling/kappaStdev").toFloat();
    m_TauStdev = config.value("sampling/tauStdev").toFloat();
    m_TR = config.value("sampling/TR").toFloat();

#ifdef BALLOON_3PARAM
    m_EpsilonStdev = config.value("sampling/epsilonStdev").toInt();
    m_TE = config.value("sampling/TE").toInt();
#else // BALLOON_3PARAM
    m_GammaStdev = config.value("sampling/gammaStdev").toFloat();
    m_GrubbStdev = config.value("sampling/grubbStdev").toFloat();
    m_RhoStdev = config.value("sampling/rhoStdev").toFloat();
#endif // BALLOON_3PARAM

#endif // CANONICAL_HRF
}

static void mat_deleter(mat_t *mat) {
    Mat_Close(mat);
}

static void var_deleter(matvar_t *var) {
    Mat_VarFree(var);
}

static void program_deleter(cl_program prog) {
    clReleaseProgram(prog);
}

static void buffer_deleter(cl_mem buf) {
    clReleaseMemObject(buf);
}

static void queue_deleter(cl_command_queue queue) {
    clReleaseCommandQueue(queue);
}

static void kernel_deleter(cl_kernel kernel) {
    clReleaseKernel(kernel);
}

template<typename _Matrix_Type_>
_Matrix_Type_ pseudoInverse(const _Matrix_Type_ &a, double epsilon = std::numeric_limits<double>::epsilon())
{
    Eigen::JacobiSVD< _Matrix_Type_ > svd(a ,Eigen::ComputeThinU | Eigen::ComputeThinV);
    double tolerance = epsilon * std::max(a.cols(), a.rows()) *svd.singularValues().array().abs()(0);
    return svd.matrixV() *  (svd.singularValues().array().abs() > tolerance).select(svd.singularValues().array().inverse(), 0).matrix().asDiagonal() * svd.matrixU().adjoint();
}

static float inline normal_logpdf(float mean, float std, float x) {
    return (log(1.0f / (std * sqrt(2.0f * M_PI))) - (pow(x - mean, 2.0f) / (2.0f * pow(std, 2.0f))));
}

void PRF_SAMPLING_JOB_CLS_NAME::run() {
    emit jobStarted(this);

    QSharedPointer<mat_t> data_mat(Mat_Open(m_DataFileName.toLatin1(), MAT_ACC_RDONLY), mat_deleter);
    if (data_mat.isNull()) {
        setError("Failed to open specified data file.");
        return;
    }

    QSharedPointer<mat_t> stage2_mat(Mat_Open(m_Stage2FileName.toLatin1(), MAT_ACC_RDONLY), mat_deleter);
    if (stage2_mat.isNull()) {
        setError("Failed to open specified stage2 file.");
        return;
    }

#ifdef CANONICAL_HRF
    matvar_t *mat_top_hrf = NULL;
    matvar_t *mat_hrf = QMatFileModel::resolveVariableSpec(data_mat.data(), "hrf", &mat_top_hrf);
    QSharedPointer<matvar_t> _mat_top_hrf(mat_top_hrf, var_deleter);
    Q_UNUSED(_mat_top_hrf);
    if (mat_hrf == NULL) {
        setError("Failed to resolve canonical HRF variable.");
        return;
    }
    if (Mat_VarReadDataAll(data_mat.data(), mat_hrf) != 0) {
        setError("Failed to read canonical HRF data.");
        return;
    }
    int n_hrf = mat_hrf->dims[0] * mat_hrf->dims[1];
    if (mat_hrf->rank != 2 || n_hrf < 1) {
        setError("Canonical HRF variable has unexpected layout.");
        return;
    }
    QSharedPointer<Prf::TypeCastArrayBase> hrf_cast(Prf::TypeCastArrayFactory::create(mat_hrf->class_type, mat_hrf->data));
    QVector<float> hrf(n_hrf);
    for (int i = 0; i < n_hrf; i++) {
        hrf[i] = hrf_cast->asFloat(i);
    }
#endif

    matvar_t *mat_top_data = NULL;
    matvar_t *mat_data = QMatFileModel::resolveVariableSpec(data_mat.data(), m_DataVariableSpec, &mat_top_data);
    QSharedPointer<matvar_t> _mat_top_data(mat_top_data, var_deleter);
    Q_UNUSED(_mat_top_data);
    if (mat_data == NULL) {
        setError("Failed to resolve data variable.");
        return;
    }

    matvar_t *mat_top_stimulus = NULL;
    matvar_t *mat_stimulus = QMatFileModel::resolveVariableSpec(data_mat.data(), "stimulus{0,0}", &mat_top_data);
    QSharedPointer<matvar_t> _mat_top_stimulus(mat_top_stimulus, var_deleter);
    Q_UNUSED(_mat_top_stimulus);
    if (mat_stimulus == NULL) {
        setError("Failed to resolve stimulus variable.");
        return;
    }
    if (Mat_VarReadDataAll(data_mat.data(), mat_stimulus) != 0) {
        setError("Failed to read stimulus variable.");
        return;
    }

    matvar_t *mat_top_initial = NULL;
    matvar_t *mat_initial = QMatFileModel::resolveVariableSpec(data_mat.data(), "results", &mat_top_initial);
    QSharedPointer<matvar_t> _mat_top_initial(mat_top_initial, var_deleter);
    Q_UNUSED(_mat_top_initial);
    if (mat_initial == NULL) {
        setError("Failed to resolve initial estimates variable.");
        return;
    }
    if (Mat_VarReadDataAll(data_mat.data(), mat_initial) != 0) {
        setError("Failed to read initial estimates data.");
        return;
    }
    matvar_t *mat_starting_ang = Mat_VarGetStructFieldByName(mat_initial, "ang", 0);
    if (mat_starting_ang == NULL) {
        setError("Failed to resolve initial angle.");
        return;
    }
    matvar_t *mat_starting_ecc = Mat_VarGetStructFieldByName(mat_initial, "ecc", 0);
    if (mat_starting_ecc == NULL) {
        setError("Failed to resolve initial eccentricity.");
        return;
    }
    matvar_t *mat_starting_rfsize = Mat_VarGetStructFieldByName(mat_initial, "rfsize", 0);
    if (mat_starting_rfsize == NULL) {
        setError("Failed to resolve starting RF size.");
        return;
    }
    matvar_t *mat_starting_expt = Mat_VarGetStructFieldByName(mat_initial, "expt", 0);
    if (mat_starting_expt == NULL) {
        setError("Failed to resolve initial exponent.");
        return;
    }
    matvar_t *mat_starting_gain = Mat_VarGetStructFieldByName(mat_initial, "gain", 0);
    if (mat_starting_gain == NULL) {
        setError("Failed to resolve initial gain.");
        return;
    }

    if (mat_starting_gain->rank != 2 || mat_starting_gain->dims[0] * mat_starting_gain->dims[1] != mat_starting_expt->dims[0] * mat_starting_expt->dims[1]
        || mat_starting_expt->rank != 2 || mat_starting_expt->dims[0] * mat_starting_expt->dims[1] != mat_starting_rfsize->dims[0] * mat_starting_rfsize->dims[1]
        || mat_starting_rfsize->rank != 2 || mat_starting_rfsize->dims[0] * mat_starting_rfsize->dims[1] != mat_starting_ecc->dims[0] * mat_starting_ecc->dims[1]
        || mat_starting_ecc->rank != 2 || mat_starting_ecc->dims[0] * mat_starting_ecc->dims[1] != mat_starting_ang->dims[0] * mat_starting_ang->dims[1]
        || mat_starting_ang->rank != 2 || mat_starting_ang->dims[0] * mat_starting_ang->dims[1] != mat_data->dims[0]) {

        setError("Initial estimate arrays are not correctly aligned.");
        return;
    }

    QSharedPointer<Prf::TypeCastArrayBase> starting_ang_cast(Prf::TypeCastArrayFactory::create(mat_starting_ang->class_type, mat_starting_ang->data));
    QSharedPointer<Prf::TypeCastArrayBase> starting_ecc_cast(Prf::TypeCastArrayFactory::create(mat_starting_ecc->class_type, mat_starting_ecc->data));
    QSharedPointer<Prf::TypeCastArrayBase> starting_rfsize_cast(Prf::TypeCastArrayFactory::create(mat_starting_rfsize->class_type, mat_starting_rfsize->data));
    QSharedPointer<Prf::TypeCastArrayBase> starting_expt_cast(Prf::TypeCastArrayFactory::create(mat_starting_expt->class_type, mat_starting_expt->data));
    QSharedPointer<Prf::TypeCastArrayBase> starting_gain_cast(Prf::TypeCastArrayFactory::create(mat_starting_gain->class_type, mat_starting_gain->data));

    qDebug() << "Reading reduced variable...";
    matvar_t *mat_reduced = Mat_VarRead(stage2_mat.data(), "reduced");
    QSharedPointer<matvar_t> _mat_reduced(mat_reduced, var_deleter);
    Q_UNUSED(_mat_reduced);
    if (mat_reduced == NULL) {
        setError("Failed to read reduced variable from stage2 file.");
        return;
    }

    if (mat_reduced->rank != 2) {
        setError("Variable reduced from stage2 has wrong rank.");
        return;
    }

    if (mat_reduced->dims[0] != mat_data->dims[1]) {
        setError("Variable reduced from stage2 and data variable have conflicting dimensions");
        return;
    }

    if (mat_reduced->class_type != MAT_C_SINGLE || mat_reduced->isComplex) {
        setError("Variable reduced from stage2 has wrong type");
        return;
    }

    int x_range[3];
    int y_range[3];
    int rfsize_range[3];
    int x_len, y_len, rfsize_len;

    if ((x_len = read_range_var(stage2_mat.data(), "x_range", &x_range[0])) < 0)
        return;

    if ((y_len = read_range_var(stage2_mat.data(), "y_range", &y_range[0])) < 0)
        return;

    if ((rfsize_len = read_range_var(stage2_mat.data(), "rfsize_range", &rfsize_range[0])) < 0)
        return;

    float x_max = x_range[0] + (x_len - 1) * x_range[2];
    float y_max = y_range[0] + (y_len - 1) * y_range[2];
    float rfsize_max = rfsize_range[0] + (rfsize_len - 1) * rfsize_range[2];

    qDebug() << "x_max:" << x_max << "y_max:" << y_max << "rfsize_max:" << rfsize_max;

    float W;
    float H;

    if (read_single(stage2_mat.data(), "W", &W) < 0) return;
    if (read_single(stage2_mat.data(), "H", &H) < 0) return;

    qDebug() << "W:" << W << "H:" << H;

    m_XMean = W / 2;
    m_YMean = H / 2;
    m_RFSizeMean = qMin(W, H) / 3;

    qDebug() << "m_XMean:" << m_XMean << "m_YMean:" << m_YMean << "m_RFSizeMean:" << m_RFSizeMean;


    qDebug() << "x_range:" << x_range[0] << x_range[1] << x_range[2];
    qDebug() << "y_range:" << y_range[0] << y_range[1] << y_range[2];
    qDebug() << "rfsize_range:" << rfsize_range[0] << rfsize_range[1] << rfsize_range[2];
    qDebug() << "x_len:" << x_len << "y_len:" << y_len << "rfsize_len:" << rfsize_len;
    qDebug() << "mat_reduced->dims[1]:" << mat_reduced->dims[1];

    if (x_len * y_len * rfsize_len != mat_reduced->dims[1]) {
        setError("X, Y, RFSize ranges conflict with reduced variable dimensions");
        return;
    }


    qDebug() << "Reading data variable...";
    if (Mat_VarReadDataAll(data_mat.data(), mat_data) != 0) {
        setError("Failed to read data variable.");
        return;
    }

    QVector<float> data_single;
    {
        QSharedPointer<Prf::TypeCastArrayBase> cast(Prf::TypeCastArrayFactory::create(mat_data->class_type, mat_data->data));
        // int nelem = mat_data->dims[0] * mat_data->dims[1];
        for (int i = 0; i < mat_data->dims[0]; i++) {
            for (int k = 0; k < mat_data->dims[1]; k++) {
                data_single.append(cast->asFloat(k * mat_data->dims[0] + i));
            }
        }
    }

    int TOT_DUR = mat_stimulus->dims[2];
    QVector<float> stimulus_single(H * W * TOT_DUR);
    {
        int ofs = 0;
        QSharedPointer<Prf::TypeCastArrayBase> cast(Prf::TypeCastArrayFactory::create(mat_stimulus->class_type, mat_stimulus->data));
        for (int t = 0; t < TOT_DUR; t++) { // TOT_DUR
            for (int x = 0; x < W; x++) { // W
                for (int y = 0; y < H; y++) { // H
                    // we want it to be TOT_DUR, H, W
                    stimulus_single[((t * H) + y) * W + x] = cast->asFloat(ofs);
                    ofs++;
                }
            }
        }
    }

    cl_device_id dev_id;
    QSharedPointer<struct _cl_context> ctx = PrfCommon::create_cl_context(dev_id, m_Error);
    if (ctx.isNull()) {
        setError(m_Error);
        return;
    }

    int ntasks = PrfCommon::get_ntasks();
    // int TOT_DUR = mat_reduced->dims[0];
    qDebug() << "TOT_DUR:" << TOT_DUR;

    QSharedPointer<struct _cl_command_queue> queue(clCreateCommandQueue(ctx.data(), dev_id, NULL, NULL), queue_deleter);
    if (queue.isNull()) {
        setError("Failed to create OpenCL command queue.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_stage2(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * TOT_DUR * x_len * y_len * rfsize_len, NULL, NULL), buffer_deleter);
    if (g_stage2.isNull()) {
        setError("Failed to create g_stage2 buffer.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_pmatrix(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * TOT_DUR * TOT_DUR, NULL, NULL), buffer_deleter);
    if (g_pmatrix.isNull()) {
        setError("Failed to create g_pmatrix buffer.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_xs(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * ntasks, NULL, NULL), buffer_deleter);
    if (g_xs.isNull()) {
        setError("Failed to create g_xs buffer.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_ys(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * ntasks, NULL, NULL), buffer_deleter);
    if (g_ys.isNull()) {
        setError("Failed to create g_xs buffer.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_rfsizes(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * ntasks, NULL, NULL), buffer_deleter);
    if (g_rfsizes.isNull()) {
        setError("Failed to create g_rfsizes buffer.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_expts(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * ntasks, NULL, NULL), buffer_deleter);
    if (g_expts.isNull()) {
        setError("Failed to create g_expts buffer.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_gains(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * ntasks, NULL, NULL), buffer_deleter);
    if (g_gains.isNull()) {
        setError("Failed to create g_gains buffer.");
        return;
    }

#ifdef CANONICAL_HRF
    QSharedPointer<struct _cl_mem> g_hrf(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * n_hrf, NULL, NULL), buffer_deleter);
    if (g_hrf.isNull()) {
        setError("Failed to create g_hrf buffer.");
        return;
    }
#else // CANONICAL_HRF
    QSharedPointer<struct _cl_mem> g_kappas(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * ntasks, NULL, NULL), buffer_deleter);
    if (g_kappas.isNull()) {
        setError("Failed to create g_kappas buffer.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_taus(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * ntasks, NULL, NULL), buffer_deleter);
    if (g_taus.isNull()) {
        setError("Failed to create g_taus buffer.");
        return;
    }

#ifdef BALLOON_3PARAM

    QSharedPointer<struct _cl_mem> g_epsilons(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * ntasks, NULL, NULL), buffer_deleter);
    if (g_epsilons.isNull()) {
        setError("Failed to create g_epsilons buffer.");
        return;
    }

#else // BALLOON_3PARAM

    QSharedPointer<struct _cl_mem> g_gammas(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * ntasks, NULL, NULL), buffer_deleter);
    if (g_gammas.isNull()) {
        setError("Failed to create g_gammas buffer.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_grubbs(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * ntasks, NULL, NULL), buffer_deleter);
    if (g_grubbs.isNull()) {
        setError("Failed to create g_grubbs buffer.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_rhos(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * ntasks, NULL, NULL), buffer_deleter);
    if (g_rhos.isNull()) {
        setError("Failed to create g_rhos buffer.");
        return;
    }

#endif // BALLOON_3PARAM

    QSharedPointer<struct _cl_mem> g_signals(clCreateBuffer(ctx.data(), CL_MEM_READ_WRITE, sizeof(float) * ntasks, NULL, NULL), buffer_deleter);
    if (g_signals.isNull()) {
        setError("Failed to create g_signals buffer.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_flows(clCreateBuffer(ctx.data(), CL_MEM_READ_WRITE, sizeof(float) * ntasks, NULL, NULL), buffer_deleter);
    if (g_flows.isNull()) {
        setError("Failed to create g_flows buffer.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_volumes(clCreateBuffer(ctx.data(), CL_MEM_READ_WRITE, sizeof(float) * ntasks, NULL, NULL), buffer_deleter);
    if (g_volumes.isNull()) {
        setError("Failed to create g_volumes buffer.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_deoxys(clCreateBuffer(ctx.data(), CL_MEM_READ_WRITE, sizeof(float) * ntasks, NULL, NULL), buffer_deleter);
    if (g_deoxys.isNull()) {
        setError("Failed to create g_deoxys buffer.");
        return;
    }
#endif // CANONICAL_HRF

    QSharedPointer<struct _cl_mem> g_Ys(clCreateBuffer(ctx.data(), CL_MEM_READ_WRITE, sizeof(float) * TOT_DUR * ntasks, NULL, NULL), buffer_deleter);
    if (g_Ys.isNull()) {
        setError("Failed to create g_Ys buffer.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_data(clCreateBuffer(ctx.data(), CL_MEM_READ_WRITE, sizeof(float) * TOT_DUR * ntasks, NULL, NULL), buffer_deleter);
    if (g_data.isNull()) {
        setError("Failed to create g_data buffer.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_stimulus(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * TOT_DUR * W * H, NULL, NULL), buffer_deleter);
    if (g_stimulus.isNull()) {
        setError("Failed to create g_stimulus buffer.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_logprobs(clCreateBuffer(ctx.data(), CL_MEM_WRITE_ONLY, sizeof(float) * ntasks, NULL, NULL), buffer_deleter);
    if (g_logprobs.isNull()) {
        setError("Failed to create g_logprobs buffer.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_noise_stdevs(clCreateBuffer(ctx.data(), CL_MEM_READ_WRITE, sizeof(float) * ntasks, NULL, NULL), buffer_deleter);
    if (g_noise_stdevs.isNull()) {
        setError("Failed to create g_noise_stdevs buffer.");
        return;
    }

#if defined(CANONICAL_HRF)
    int m_SubDiv = 20;
    float m_TR = 1.0;
#elif defined(BALLOON_3PARAM)
    // float m_TR = 1.0;
#endif

    QString hdr = QString(OPENCL_HDR)
        .arg(m_SubDiv)
        .arg(TOT_DUR)
        .arg(m_TR, 0, 'f', 10)
        .arg(x_range[0]).arg(x_range[2]).arg(x_len)
        .arg(y_range[0]).arg(y_range[2]).arg(y_len)
        .arg(rfsize_range[0]).arg(rfsize_range[2]).arg(rfsize_len)
        .arg(W).arg(H);

    if (m_pRF_Model == DUM_WAN) {
        hdr += "\n#define DUMOULIN_WANDELL\n";
    }

    qDebug() << "hdr:" << hdr;

    int err;
    QByteArray ba = (hdr + OPENCL_BODY).toLatin1();
    // qDebug() << ba;
    const char *ba2 = ba.data();
    QSharedPointer<struct _cl_program> program(clCreateProgramWithSource(ctx.data(), 1, &ba2, NULL, &err), program_deleter);
    if (err != CL_SUCCESS) {
        setError("Failed to create program from source.");
        return;
    }

    if (clBuildProgram(program.data(), 1, &dev_id, "-Werror -cl-std=CL1.2", NULL, NULL) != CL_SUCCESS) {
        setError("Failed to build program.");
        size_t sz;
        clGetProgramBuildInfo(program.data(), dev_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &sz);
        QByteArray log(sz + 1, 0);
        clGetProgramBuildInfo(program.data(), dev_id, CL_PROGRAM_BUILD_LOG, sz, log.data(), &sz);
        qDebug() << QString::fromLatin1(log);
        return;
    }

    QSharedPointer<struct _cl_kernel> prepare_data(clCreateKernel(program.data(), "prepare_data", &err), kernel_deleter);
    if (err != CL_SUCCESS) {
        setError(QString("Failed to create prepare_data() kernel: %1").arg(err));
        return;
    }

#ifdef CANONICAL_HRF
    qDebug() << "Using reset_canonical() ...";
    QSharedPointer<struct _cl_kernel> reset(clCreateKernel(program.data(), "reset_canonical", &err), kernel_deleter);
#else
    qDebug() << "Using reset() ...";
    QSharedPointer<struct _cl_kernel> reset(clCreateKernel(program.data(), "reset", &err), kernel_deleter);
#endif
    if (err != CL_SUCCESS) {
        setError(QString("Failed to create reset() kernel: %1").arg(err));
        return;
    }

#if defined(CANONICAL_HRF)
    qDebug() << "Using fwd_model_canonical() ...";
    QSharedPointer<struct _cl_kernel> fwd_model(clCreateKernel(program.data(), "fwd_model_canonical", &err), kernel_deleter);
#elif defined(BALLOON_3PARAM)
    qDebug() << "Using fwd_model_3param() ...";
    QSharedPointer<struct _cl_kernel> fwd_model(clCreateKernel(program.data(), "fwd_model_3param", &err), kernel_deleter);
#else
    qDebug() << "Using fwd_model() ...";
    QSharedPointer<struct _cl_kernel> fwd_model(clCreateKernel(program.data(), "fwd_model", &err), kernel_deleter);
#endif
    if (err != CL_SUCCESS) {
        setError(QString("Failed to create fwd_model() kernel: %1").arg(err));
        return;
    }

    QSharedPointer<struct _cl_kernel> eval_proposal(clCreateKernel(program.data(), "eval_proposal", &err), kernel_deleter);
    if (err != CL_SUCCESS) {
        setError(QString("Failed to create eval_proposal() kernel: %1").arg(err));
        return;
    }

    cl_mem tmp;

    clSetKernelArg(prepare_data.data(), 0, sizeof(cl_mem), &(tmp = g_pmatrix.data()));
    clSetKernelArg(prepare_data.data(), 1, sizeof(cl_mem), &(tmp = g_data.data()));

#ifdef CANONICAL_HRF
#else
    clSetKernelArg(reset.data(), 0, sizeof(cl_mem), &(tmp = g_signals.data()));
    clSetKernelArg(reset.data(), 1, sizeof(cl_mem), &(tmp = g_flows.data()));
    clSetKernelArg(reset.data(), 2, sizeof(cl_mem), &(tmp = g_volumes.data()));
    clSetKernelArg(reset.data(), 3, sizeof(cl_mem), &(tmp = g_deoxys.data()));
#endif

    clSetKernelArg(fwd_model.data(), 0, sizeof(cl_mem), &(tmp = g_stimulus.data()));
    clSetKernelArg(fwd_model.data(), 1, sizeof(cl_mem), &(tmp = g_pmatrix.data()));

    clSetKernelArg(fwd_model.data(), 2, sizeof(cl_mem), &(tmp = g_xs.data()));
    clSetKernelArg(fwd_model.data(), 3, sizeof(cl_mem), &(tmp = g_ys.data()));
    clSetKernelArg(fwd_model.data(), 4, sizeof(cl_mem), &(tmp = g_rfsizes.data()));
    clSetKernelArg(fwd_model.data(), 5, sizeof(cl_mem), &(tmp = g_expts.data()));
    clSetKernelArg(fwd_model.data(), 6, sizeof(cl_mem), &(tmp = g_gains.data()));

#if defined(CANONICAL_HRF)
    clSetKernelArg(fwd_model.data(), 7, sizeof(cl_mem), &(tmp = g_hrf.data()));
    clSetKernelArg(fwd_model.data(), 8, sizeof(int), &n_hrf);
    clSetKernelArg(fwd_model.data(), 9, sizeof(cl_mem), &(tmp = g_Ys.data()));
#elif defined(BALLOON_3PARAM)
    clSetKernelArg(fwd_model.data(), 7, sizeof(cl_mem), &(tmp = g_kappas.data()));
    clSetKernelArg(fwd_model.data(), 8, sizeof(cl_mem), &(tmp = g_taus.data()));
    clSetKernelArg(fwd_model.data(), 9, sizeof(cl_mem), &(tmp = g_epsilons.data()));
    clSetKernelArg(fwd_model.data(), 10, sizeof(float), &m_TE);

    clSetKernelArg(fwd_model.data(), 11, sizeof(cl_mem), &(tmp = g_signals.data()));
    clSetKernelArg(fwd_model.data(), 12, sizeof(cl_mem), &(tmp = g_flows.data()));
    clSetKernelArg(fwd_model.data(), 13, sizeof(cl_mem), &(tmp = g_volumes.data()));
    clSetKernelArg(fwd_model.data(), 14, sizeof(cl_mem), &(tmp = g_deoxys.data()));
    clSetKernelArg(fwd_model.data(), 15, sizeof(cl_mem), &(tmp = g_Ys.data()));
#else
    clSetKernelArg(fwd_model.data(), 7, sizeof(cl_mem), &(tmp = g_kappas.data()));
    clSetKernelArg(fwd_model.data(), 8, sizeof(cl_mem), &(tmp = g_gammas.data()));
    clSetKernelArg(fwd_model.data(), 9, sizeof(cl_mem), &(tmp = g_taus.data()));
    clSetKernelArg(fwd_model.data(), 10, sizeof(cl_mem), &(tmp = g_grubbs.data()));
    clSetKernelArg(fwd_model.data(), 11, sizeof(cl_mem), &(tmp = g_rhos.data()));

    clSetKernelArg(fwd_model.data(), 12, sizeof(cl_mem), &(tmp = g_signals.data()));
    clSetKernelArg(fwd_model.data(), 13, sizeof(cl_mem), &(tmp = g_flows.data()));
    clSetKernelArg(fwd_model.data(), 14, sizeof(cl_mem), &(tmp = g_volumes.data()));
    clSetKernelArg(fwd_model.data(), 15, sizeof(cl_mem), &(tmp = g_deoxys.data()));
    clSetKernelArg(fwd_model.data(), 16, sizeof(cl_mem), &(tmp = g_Ys.data()));
#endif

    clSetKernelArg(eval_proposal.data(), 0, sizeof(cl_mem), &(tmp = g_Ys.data()));
    clSetKernelArg(eval_proposal.data(), 1, sizeof(cl_mem), &(tmp = g_data.data()));
    clSetKernelArg(eval_proposal.data(), 2, sizeof(cl_mem), &(tmp = g_noise_stdevs.data()));
    clSetKernelArg(eval_proposal.data(), 3, sizeof(cl_mem), &(tmp = g_logprobs.data()));

#ifdef CANONICAL_HRF
    if (clEnqueueWriteBuffer(queue.data(), g_hrf.data(), false, 0,
        sizeof(float) * n_hrf, &hrf[0], 0, NULL, NULL) != CL_SUCCESS) {

        setError("Failed to enqueue g_hrf buffer write.");
        return;
    }
#endif


    if (clEnqueueWriteBuffer(queue.data(), g_stage2.data(), false, 0,
        sizeof(float) * TOT_DUR * x_len * y_len * rfsize_len, mat_reduced->data, 0, NULL, NULL) != CL_SUCCESS) {

        setError("Failed to enqueue g_stage2 buffer write.");
        return;
    }

    if (clEnqueueWriteBuffer(queue.data(), g_stimulus.data(), false, 0,
        sizeof(float) * TOT_DUR * W * H, &stimulus_single[0], 0, NULL, NULL) != CL_SUCCESS) {

        setError("Failed to enqueue g_stimulus buffer write.");
        return;
    }


    int polydeg = 4;
    qDebug() << "Computing pmatrix...";
    Eigen::MatrixXd polymat(TOT_DUR, polydeg + 1);
    for (int t = 0; t < TOT_DUR; t++) {
        for (int p = 0; p <= polydeg; p++) {
            polymat(t, p) = pow(t, p);
        }
    }

    Eigen::MatrixXd polyinv = pseudoInverse(polymat);
    qDebug() << "polyinv rows:" << polyinv.rows() << "cols:" << polyinv.cols();
    Eigen::MatrixXd pmatrix = polymat * polyinv;
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> pmatrixf(pmatrix.cast<float>());
    if (clEnqueueWriteBuffer(queue.data(), g_pmatrix.data(), false, 0, sizeof(float) * TOT_DUR * TOT_DUR, pmatrixf.data(), 0, NULL, NULL) != CL_SUCCESS) {
        setError("Failed to enqueue g_pmatrix buffer write.");
        return;
    }
    clEnqueueBarrier(queue.data());

    QVector<float> xs;
    QVector<float> ys;
    QVector<float> rfsizes;
    QVector<float> expts;
    QVector<float> gains;
#ifdef CANONICAL_HRF
#else
    QVector<float> kappas;
    QVector<float> taus;

#ifdef BALLOON_3PARAM
    QVector<float> epsilons;
#else // BALLOON_3PARAM
    QVector<float> gammas;
    QVector<float> grubbs;
    QVector<float> rhos;
#endif // BALLOON_3PARAM

#endif
    QVector<float> noise_stdevs;

    int nvoxels = mat_data->dims[0];

    QVector<QVector<float> > x_samples(nvoxels);
    QVector<QVector<float> > y_samples(nvoxels);
    QVector<QVector<float> > rfsize_samples(nvoxels);
    QVector<QVector<float> > expt_samples(nvoxels);
    QVector<QVector<float> > gain_samples(nvoxels);

#ifdef CANONICAL_HRF
#else // CANONICAL_HRF
    QVector<QVector<float> > kappa_samples(nvoxels);
    QVector<QVector<float> > tau_samples(nvoxels);

#ifdef BALLOON_3PARAM
    QVector<QVector<float> > epsilon_samples(nvoxels);
#else
    QVector<QVector<float> > gamma_samples(nvoxels);
    QVector<QVector<float> > grubb_samples(nvoxels);
    QVector<QVector<float> > rho_samples(nvoxels);
#endif // BALLOON_3PARAM

#endif // CANONICAL_HRF

    QVector<QVector<float> > logprob_samples(nvoxels);

    QVector<QVector<float> > noise_stdev_samples(nvoxels);

    QVector<float> logprobs(nvoxels);

    qDebug() << "Preparing data...";
    for (int i = 0; i < nvoxels; i += ntasks) {
        int n = qMin(ntasks, nvoxels - i);
        qDebug() << "i:" << i << "n:" << n;
        if (clEnqueueWriteBuffer(queue.data(), g_data.data(), false, 0, sizeof(float) * TOT_DUR * n, &data_single[i * TOT_DUR], 0, NULL, NULL) != CL_SUCCESS) {
            setError("Failed to write g_data buffer.");
            return;
        }
        clEnqueueBarrier(queue.data());
        size_t global_work_size = n;
        size_t local_work_size = 1;
        cl_event ev;
        qint64 time_1(QDateTime::currentMSecsSinceEpoch());

        if (clEnqueueNDRangeKernel(queue.data(), prepare_data.data(), 1, NULL, &global_work_size, NULL, 0, NULL, &ev) != CL_SUCCESS) {
            setError("Failed to enqueue prepare_data() kernel.");
            return;
        }
        //clEnqueueBarrier(queue.data());
        clWaitForEvents(1, &ev);
        clReleaseEvent(ev);
        qDebug() << "Kernel took:" << (QDateTime::currentMSecsSinceEpoch() - time_1) << "ms";

        time_1 = QDateTime::currentMSecsSinceEpoch();
        err = clEnqueueReadBuffer(queue.data(), g_data.data(), true, 0, sizeof(float) * TOT_DUR * n, &data_single[i * TOT_DUR], 0, NULL, NULL);
        if (err != CL_SUCCESS) {
            setError(QString("Failed to read g_data buffer: %1").arg(err));
            return;
        }
        qDebug() << "Data read took:" << (QDateTime::currentMSecsSinceEpoch() - time_1) << "ms";

    }
    qDebug() << "Data ready...";

    QVector<float> Ys(nvoxels * TOT_DUR);
    // QVector<float> mse(nvoxels);

    for (int i = 0; i < nvoxels; i++) {
        float ang = starting_ang_cast->asFloat(i);
        float ecc = starting_ecc_cast->asFloat(i);
        float x = cos(ang * M_PI / 180) * ecc + W / 2.0f;
        float y = sin(ang * M_PI / 180) * ecc + H / 2.0f;
        xs.append(x);
        ys.append(y);
        rfsizes.append(starting_rfsize_cast->asFloat(i));
        expts.append(starting_expt_cast->asFloat(i));
        gains.append(starting_gain_cast->asFloat(i));

#ifdef CANONICAL_HRF
#else
        kappas.append(m_KappaMean);
        taus.append(m_TauMean);

#ifdef BALLOON_3PARAM
        epsilons.append(m_EpsilonMean);
#else // BALLOON_3PARAM
        gammas.append(m_GammaMean);   
        grubbs.append(m_GrubbMean);
        rhos.append(m_RhoMean);
#endif // BALLOON_3PARAM

#endif

        noise_stdevs.append(0.00001f);
    }

    QVector<float> old_xs(xs);
    QVector<float> old_ys(ys);
    QVector<float> old_rfsizes(rfsizes);
    QVector<float> old_expts(expts);
    QVector<float> old_gains(gains);

#ifdef CANONICAL_HRF
#else
    QVector<float> old_kappas(kappas);
    QVector<float> old_taus(taus);

#ifdef BALLOON_3PARAM
    QVector<float> old_epsilons(epsilons);
#else // BALLOON_3PARAM
    QVector<float> old_gammas(gammas);
    QVector<float> old_grubbs(grubbs);
    QVector<float> old_rhos(rhos);
#endif // BALLOON_3PARAM

#endif

    float zero = 0.0f;
    float minus_inf = -1.0f / zero;

    QVector<float> old_logprobs(nvoxels, minus_inf);

    QVector<float> old_noise_stdevs(nvoxels, 0.00001f);

    std::random_device rd;
    std::mt19937 gen(rd());


    mat_t *starting_debug_mat = Mat_CreateVer("starting_debug.mat", NULL, MAT_FT_MAT5);
    if (write_var(starting_debug_mat, "xs", &xs[0], xs.size(), 1) < 0) return;
    if (write_var(starting_debug_mat, "ys", &ys[0], xs.size(), 1) < 0) return;
    if (write_var(starting_debug_mat, "rfsizes", &rfsizes[0], xs.size(), 1) < 0) return;
    if (write_var(starting_debug_mat, "expts", &expts[0], xs.size(), 1) < 0) return;
    if (write_var(starting_debug_mat, "gains", &gains[0], xs.size(), 1) < 0) return;
#ifdef CANONICAL_HRF
    if (write_var(starting_debug_mat, "hrf", &hrf[0], n_hrf, 1) < 0) return;
#else
    if (write_var(starting_debug_mat, "kappas", &kappas[0], xs.size(), 1) < 0) return;
    if (write_var(starting_debug_mat, "taus", &taus[0], xs.size(), 1) < 0) return;

#ifdef BALLOON_3PARAM
    if (write_var(starting_debug_mat, "epsilons", &epsilons[0], xs.size(), 1) < 0) return;
#else // BALLOON_3PARAM
    if (write_var(starting_debug_mat, "gammas", &gammas[0], xs.size(), 1) < 0) return;
    if (write_var(starting_debug_mat, "grubbs", &grubbs[0], xs.size(), 1) < 0) return;
    if (write_var(starting_debug_mat, "rhos", &rhos[0], xs.size(), 1) < 0) return;
#endif  // BALLOON_3PARAM

#endif
    Mat_Close(starting_debug_mat);

    qDebug() << "Running Monte Carlo...";

    for (int i = 0; i < m_NumberOfSteps; i++) {
        qDebug() << "Step" << i;

        int cnt_acc = 0;
        int cnt_acc_rand = 0;
        int cnt_rej = 0;

        for (int k = 0; k < nvoxels; k += ntasks) {
            // qDebug() << "k:" << k;

            qint64 time_1(QDateTime::currentMSecsSinceEpoch());

            if (isInterruptionRequested()) {
                emit jobInterrupted(this);
                return;
            }

            int n = qMin(ntasks, xs.size() - k);

            clEnqueueWriteBuffer(queue.data(), g_data.data(), false, 0, sizeof(float) * TOT_DUR * n, &data_single[k * TOT_DUR], 0, NULL, NULL);

            clEnqueueWriteBuffer(queue.data(), g_xs.data(), false, 0, sizeof(float) * n, &xs[k], 0, NULL, NULL);
            clEnqueueWriteBuffer(queue.data(), g_ys.data(), false, 0, sizeof(float) * n, &ys[k], 0, NULL, NULL);
            clEnqueueWriteBuffer(queue.data(), g_rfsizes.data(), false, 0, sizeof(float) * n, &rfsizes[k], 0, NULL, NULL);
            clEnqueueWriteBuffer(queue.data(), g_expts.data(), false, 0, sizeof(float) * n, &expts[k], 0, NULL, NULL);
            clEnqueueWriteBuffer(queue.data(), g_gains.data(), false, 0, sizeof(float) * n, &gains[k], 0, NULL, NULL);

#ifdef CANONICAL_HRF
#else
            clEnqueueWriteBuffer(queue.data(), g_kappas.data(), false, 0, sizeof(float) * n, &kappas[k], 0, NULL, NULL);
            clEnqueueWriteBuffer(queue.data(), g_taus.data(), false, 0, sizeof(float) * n, &taus[k], 0, NULL, NULL);

#ifdef BALLOON_3PARAM
            clEnqueueWriteBuffer(queue.data(), g_epsilons.data(), false, 0, sizeof(float) * n, &epsilons[k], 0, NULL, NULL);
#else // BALLOON_3PARAM
            clEnqueueWriteBuffer(queue.data(), g_gammas.data(), false, 0, sizeof(float) * n, &gammas[k], 0, NULL, NULL);       
            clEnqueueWriteBuffer(queue.data(), g_grubbs.data(), false, 0, sizeof(float) * n, &grubbs[k], 0, NULL, NULL);
            clEnqueueWriteBuffer(queue.data(), g_rhos.data(), false, 0, sizeof(float) * n, &rhos[k], 0, NULL, NULL);
#endif // BALLOON_3PARAM

#endif

            clEnqueueWriteBuffer(queue.data(), g_noise_stdevs.data(), false, 0, sizeof(float) * n, &noise_stdevs[k], 0, NULL, NULL);
            clEnqueueBarrier(queue.data());

            size_t global_work_size[] = {n};
            size_t local_work_size[] = {1};

            if (clEnqueueNDRangeKernel(queue.data(), reset.data(), 1, NULL, &global_work_size[0], NULL, 0, NULL, NULL) != CL_SUCCESS) {
                setError("clEnqueueNDRangeKernel() failed for reset() kernel.");
                return;
            }
            clEnqueueBarrier(queue.data());

            cl_event ev;
            qint64 time_2 = QDateTime::currentMSecsSinceEpoch();
            if ((err = clEnqueueNDRangeKernel(queue.data(), fwd_model.data(), 1, NULL, &global_work_size[0], NULL, 0, NULL, &ev)) != CL_SUCCESS) {
                setError(QString("clEnqueueNDRangeKernel() failed for fwd_model() kernel: %1").arg(err));
                return;
            }
            // clEnqueueBarrier(queue.data());
            clWaitForEvents(1, &ev);
            clReleaseEvent(ev);
            // qDebug() << "fwd_model() kernel took:" << (QDateTime::currentMSecsSinceEpoch() - time_2), "ms";

            if ((err = clEnqueueNDRangeKernel(queue.data(), eval_proposal.data(), 1, NULL, &global_work_size[0], NULL, 0, NULL, NULL)) != CL_SUCCESS) {
                setError(QString("clEnqueueNDRangeKernel() failed for eval_proposal() kernel: %1").arg(err));
                return;
            }
            clEnqueueBarrier(queue.data());

            // cl_event ev;
            err = clEnqueueReadBuffer(queue.data(), g_Ys.data(), false, 0, sizeof(float) * n * TOT_DUR, &Ys[k * TOT_DUR], 0, NULL, &ev);
            if (err != CL_SUCCESS) {
                setError(QString("Failed to read g_Ys buffer: %1").arg(err));
                return;
            }
            clWaitForEvents(1, &ev);
            clReleaseEvent(ev);

            err = clEnqueueReadBuffer(queue.data(), g_logprobs.data(), false, 0, sizeof(float) * n, &logprobs[k], 0, NULL, &ev);
            if (err != CL_SUCCESS) {
                setError(QString("Failed to read g_logprobs buffer: %1").arg(err));
                return;
            }
            clWaitForEvents(1, &ev);
            clReleaseEvent(ev);

            err = clEnqueueReadBuffer(queue.data(), g_noise_stdevs.data(), false, 0, sizeof(float) * n, &noise_stdevs[k], 0, NULL, &ev);
            if (err != CL_SUCCESS) {
                setError(QString("Failed to read g_noise_stdevs buffer: %1").arg(err));
                return;
            }
            clWaitForEvents(1, &ev);
            clReleaseEvent(ev);

            // qDebug() << "Iteration took:" << (QDateTime::currentMSecsSinceEpoch() - time_1), "ms";
        }

        qint64 time_1 = QDateTime::currentMSecsSinceEpoch();
        for (int k = 0; k < nvoxels; k++) {
            // logprobs[k] += normal_logpdf(m_XMean, m_XStdev, xs[k]);
            // logprobs[k] += normal_logpdf(m_YMean, m_YStdev, ys[k]);
            // logprobs[k] += normal_logpdf(m_RFSizeMean, m_RFSizeStdev, rfsizes[k]);
            if (m_pRF_Model == CSS_PRF) {
                // logprobs[k] += normal_logpdf(m_ExptMean, m_ExptStdev, expts[k]);
            }
            // logprobs[k] += normal_logpdf(m_GainMean, m_GainStdev, gains[k]);

#ifdef CANONICAL_HRF
#else
            logprobs[k] += normal_logpdf(m_KappaMean, m_KappaStdev, kappas[k]);
            logprobs[k] += normal_logpdf(m_TauMean, m_TauStdev, taus[k]);

#ifdef BALLOON_3PARAM
            logprobs[k] += normal_logpdf(m_EpsilonMean, m_EpsilonStdev, epsilons[k]);
#else // BALLOON_3PARAM
            logprobs[k] += normal_logpdf(m_GammaMean, m_GammaStdev, gammas[k]);
            logprobs[k] += normal_logpdf(m_GrubbMean, m_GrubbStdev, grubbs[k]);
            logprobs[k] += normal_logpdf(m_RhoMean, m_RhoStdev, rhos[k]);
#endif // BALLOON_3PARAM

#endif

            if (noise_stdevs[k] > m_MaxNoiseStdev) {
                // disallow noise higher than defined maximum
                logprobs[k] += (minus_inf);
            } else {
                logprobs[k] += log(1.0f / m_MaxNoiseStdev);
            }

            float ratio = (float) qMin<double>(1.0, exp(logprobs[k] - old_logprobs[k]));
            float rnd = std::uniform_real_distribution<float>(0, 1)(gen);
            if (rnd < ratio) {
                if (logprobs[k] > old_logprobs[k])
                    cnt_acc++;
                else
                    cnt_acc_rand++;

                x_samples[k].append(xs[k]);
                y_samples[k].append(ys[k]);
                rfsize_samples[k].append(rfsizes[k]);
                if (m_pRF_Model == CSS_PRF) {
                    expt_samples[k].append(expts[k]);
                }
                gain_samples[k].append(gains[k]);

#ifdef CANONICAL_HRF
#else
                kappa_samples[k].append(kappas[k]);
                tau_samples[k].append(taus[k]);

#ifdef BALLOON_3PARAM
                epsilon_samples[k].append(epsilons[k]);
#else // BALLOON_3PARAM
                gamma_samples[k].append(gammas[k]);
                grubb_samples[k].append(grubbs[k]);
                rho_samples[k].append(rhos[k]);
#endif // BALLOON_3PARAM

#endif

                noise_stdev_samples[k].append(noise_stdevs[k]);

                logprob_samples[k].append(logprobs[k]);
            } else {
                cnt_rej++;

                xs[k] = old_xs[k];
                ys[k] = old_ys[k];
                rfsizes[k] = old_rfsizes[k];
                expts[k] = old_expts[k];
                gains[k] = old_gains[k];

#ifdef CANONICAL_HRF
#else
                kappas[k] = old_kappas[k];
                taus[k] = old_taus[k];

#ifdef BALLOON_3PARAM
                epsilons[k] = old_epsilons[k];
#else // BALLOON_3PARAM
                gammas[k] = old_gammas[k];
                grubbs[k] = old_grubbs[k];
                rhos[k] = old_rhos[k];
#endif // BALLOON_3PARAM

#endif

                noise_stdevs[k] = old_noise_stdevs[k];
            }
        }
        qDebug() << "Accept/reject took:" << (QDateTime::currentMSecsSinceEpoch() - time_1) << "ms";

        qDebug() << "cnt_acc:" << cnt_acc << "cnt_acc_rand:" << cnt_acc_rand << "cnt_rej:" << cnt_rej << "acr:" << (float) (cnt_acc + cnt_acc_rand) / nvoxels;
        emit progressChanged(this, QString("step: %1, cnt_acc: %2, cnt_acc_rand: %3, cnt_rej: %4, acr: %5")
            .arg(i).arg(cnt_acc).arg(cnt_acc_rand).arg(cnt_rej).arg((float) (cnt_acc + cnt_acc_rand) / nvoxels));

        // prepare for next iteration
        old_xs = xs;
        old_ys = ys;
        old_rfsizes = rfsizes;
        old_expts = expts;
        old_gains = gains;

#ifdef CANONICAL_HRF
#else
        old_kappas = kappas;
        old_taus = taus;

#ifdef BALLOON_3PARAM
        old_epsilons = epsilons;
#else // BALLOON_3PARAM
        old_gammas = gammas;
        old_grubbs = grubbs;
        old_rhos = rhos;
#endif // BALLOON_3PARAM

#endif

        old_logprobs = logprobs;

        time_1 = QDateTime::currentMSecsSinceEpoch();
        // make a step
        for (int k = 0; k < nvoxels; k++) {
            xs[k] += std::normal_distribution<float>(0, m_XStdev * m_Temperature)(gen);
            ys[k] += std::normal_distribution<float>(0, m_YStdev * m_Temperature)(gen);
            rfsizes[k] += std::normal_distribution<float>(0, m_RFSizeStdev * m_Temperature)(gen);
            if (m_pRF_Model == CSS_PRF) {
                expts[k] += std::normal_distribution<float>(0, m_ExptStdev * m_Temperature)(gen);
            }
            gains[k] += std::normal_distribution<float>(0, m_GainStdev * m_Temperature)(gen);

#ifdef CANONICAL_HRF
#else
            kappas[k] += std::normal_distribution<float>(0, m_KappaStdev * m_Temperature)(gen);
            taus[k] += std::normal_distribution<float>(0, m_TauStdev * m_Temperature)(gen);

#ifdef BALLOON_3PARAM
            epsilons[k] += std::normal_distribution<float>(0, m_EpsilonStdev * m_Temperature)(gen);
#else // BALLOON_3PARAM
            gammas[k] += std::normal_distribution<float>(0, m_GammaStdev * m_Temperature)(gen);
            grubbs[k] += std::normal_distribution<float>(0, m_GrubbStdev * m_Temperature)(gen);
            rhos[k] += std::normal_distribution<float>(0, m_RhoStdev * m_Temperature)(gen);
#endif // BALLOON_3PARAM

#endif

            noise_stdevs[k] += std::uniform_real_distribution<float>(0, m_MaxNoiseStdev * m_Temperature)(gen);

            if (xs[k] < x_range[0]) xs[k] = x_range[0];
            if (xs[k] > x_max) xs[k] = x_max;

            if (ys[k] < y_range[0]) ys[k] = y_range[0];
            if (ys[k] > y_max) ys[k] = y_max;

            if (rfsizes[k] < rfsize_range[0]) rfsizes[k] = rfsize_range[0];
            if (rfsizes[k] > rfsize_max) rfsizes[k] = rfsize_max;

            if (gains[k] < 0) gains[k] = 0.00001;
            if (expts[k] < 0) expts[k] = 0.00001;

#ifdef BALLOON_3PARAM
            if (epsilons[k] < 0) epsilons[k] = 0;
#endif

            if (noise_stdevs[k] < 0.00001f) noise_stdevs[k] = 0.00001f;
        }
        qDebug() << "Making a step took:" << (QDateTime::currentMSecsSinceEpoch() - time_1) << "ms";

        if (m_WriteDebug) {
            time_1 = QDateTime::currentMSecsSinceEpoch();
            mat_t *mcmc_debug_mat = Mat_CreateVer("mcmc_debug.mat", NULL, MAT_FT_MAT5);
            if (write_var(mcmc_debug_mat, "Ys", &Ys[0], TOT_DUR, nvoxels) < 0) return;
            // if (write_var(mcmc_debug_mat, "mse", &mse[0], 1, nvoxels) < 0) return;
            if (write_var(mcmc_debug_mat, "data_single", &data_single[0], TOT_DUR, nvoxels)) return;
            if (write_var(mcmc_debug_mat, "logprobs", &logprobs[0], 1, nvoxels)) return;
            if (write_var(mcmc_debug_mat, "noise_stdevs", &noise_stdevs[0], 1, nvoxels)) return;
            if (write_var(mcmc_debug_mat, "stimulus_single", &stimulus_single[0], W, H * TOT_DUR));
            Mat_Close(mcmc_debug_mat);
            qDebug() << "Writing mcmc_debug.mat took:" << (QDateTime::currentMSecsSinceEpoch() - time_1) << "ms";
        }

        // break;

    }

    qDebug() << "Monte Carlo finished, saving results....";
    mat_t *mcmc_results_mat = Mat_CreateVer("mcmc_results.mat", NULL, MAT_FT_MAT5);
    QSharedPointer<struct _mat_t> _mcmc_results_mat(mcmc_results_mat, mat_deleter);
    Q_UNUSED(_mcmc_results_mat);
    if (save_samples("x_samples", x_samples) < 0) return;
    if (save_samples("y_samples", y_samples) < 0) return;
    if (save_samples("rfsize_samples", rfsize_samples) < 0) return;
    if (m_pRF_Model == CSS_PRF) {
        if (save_samples("expt_samples", expt_samples) < 0) return;
    }
    if (save_samples("gain_samples", gain_samples) < 0) return;

#ifdef CANONICAL_HRF
#else
    if (save_samples("kappa_samples", kappa_samples) < 0) return;
    if (save_samples("tau_samples", tau_samples) < 0) return;

#ifdef BALLOON_3PARAM
    if (save_samples("epsilon_samples", epsilon_samples) < 0) return;
#else // BALLOON_3PARAM
    if (save_samples("gamma_samples", gamma_samples) < 0) return;
    if (save_samples("grubb_samples", grubb_samples) < 0) return;
    if (save_samples("rho_samples", rho_samples) < 0) return;
#endif

#endif

    if (save_samples("noise_stdev_samples", noise_stdev_samples) < 0) return;

    if (save_samples("logprob_samples", logprob_samples) < 0) return;


    emit jobFinished(this);
}


void PRF_SAMPLING_JOB_CLS_NAME::setError(const QString &err) {
    qDebug() << err;
    m_Error = err;
    emit statusChanged(this, err);
}


int PRF_SAMPLING_JOB_CLS_NAME::read_range_var(mat_t *mat, const char *varname, int *data) {
    matvar_t *var = Mat_VarRead(mat, varname);
    if (var == NULL) {
        setError(QString("Failed to read %1 variable from stage2 file.").arg(varname));
        return -1;
    }
    QSharedPointer<matvar_t> _var(var, var_deleter);
    Q_UNUSED(_var);
    if (!QMatFileModel::isFullReal(var)) {
        setError(QString("Variable %1 from stage2 file is not of expected numeric type.").arg(varname));
        return -2;
    }
    if (var->rank != 2 || var->dims[0] != 1 || var->dims[1] != 3) {
        setError(QString("Variable %1 from stage2 file has wrong dimensions.").arg(varname));
        return -3;
    }
    QSharedPointer<Prf::TypeCastArrayBase> cast(Prf::TypeCastArrayFactory::create(var->class_type, var->data));
    data[0] = cast->asInt32(0);
    data[1] = cast->asInt32(1);
    data[2] = cast->asInt32(2);
    int cnt = 0;
    for (int i = data[0]; i < data[1]; i += data[2]) cnt++;
    qDebug() << varname << "len:" << cnt;
    return cnt;
}


int PRF_SAMPLING_JOB_CLS_NAME::read_single(mat_t *mat, const char *varname, float *data) {
    matvar_t *var = Mat_VarRead(mat, varname);
    if (var == NULL) {
        setError(QString("Failed to read %1 variable from stage2 file.").arg(varname));
        return -1;
    }
    QSharedPointer<matvar_t> _var(var, var_deleter);
    Q_UNUSED(_var);
    if (!QMatFileModel::isFullReal(var)) {
        setError(QString("Variable %1 from stage2 file is not of expected numeric type.").arg(varname));
        return -2;
    }
    if (var->rank != 2 || var->dims[0] != 1 || var->dims[1] != 1) {
        setError(QString("Variable %1 from stage2 file has wrong dimensions.").arg(varname));
        return -3;
    }
    QSharedPointer<Prf::TypeCastArrayBase> cast(Prf::TypeCastArrayFactory::create(var->class_type, var->data));
    data[0] = cast->asFloat(0);
    return 0;
}

int PRF_SAMPLING_JOB_CLS_NAME::write_var(mat_t *mat, const char *varname, float *data, int dim0, int dim1) {
    if (mat == NULL) {
        setError("Trying to write variable to invalid file descriptor.");
        return -1;
    }
    size_t dims[] = {dim0, dim1};
    matvar_t *var = Mat_VarCreate(varname, MAT_C_SINGLE, MAT_T_SINGLE, 2, &dims[0], data, MAT_F_GLOBAL);
    if (var == NULL) {
        setError(QString("Failed to create variable %1").arg(varname));
        return -2;
    }
    if (Mat_VarWrite(mat, var, MAT_COMPRESSION_ZLIB) != 0) {
        setError(QString("Failed to write variable %1").arg(varname));
        return -3;
    }
    Mat_VarFree(var);
    return 0;
}

int PRF_SAMPLING_JOB_CLS_NAME::write_cell_vector(mat_t *mat, const char *varname, const QVector<QVector<float> > &data) {
    if (mat == NULL) {
        setError("Trying to write variable to invalid file descriptor.");
        return -1;
    }
    size_t dims[] = {data.size(), 1};
    matvar_t *var = Mat_VarCreate(varname, MAT_C_CELL, MAT_T_CELL, 2, &dims[0], NULL, MAT_F_GLOBAL);
    if (var == NULL) {
        setError(QString("Failed to create variable %1").arg(varname));
        return -2;
    }
    for (int i = 0; i < data.size(); i++) {
        size_t dims[] = {data[i].size(), 1};
        matvar_t *cell = Mat_VarCreate(NULL, MAT_C_SINGLE, MAT_T_SINGLE, 2, &dims[0], (void*) data[i].data(), 0);
        if (cell == NULL) {
            setError(QString("Failed to create cell in variable %1").arg(varname));
            Mat_VarFree(var);
            return -3;
        }
        Mat_VarSetCell(var, i, cell);
    }
    if (Mat_VarWrite(mat, var, MAT_COMPRESSION_ZLIB) != 0) {
        setError(QString("Failed to write variable %1").arg(varname));
        return -4;
    }
}

int PRF_SAMPLING_JOB_CLS_NAME::save_samples(const char *varname, const QVector<QVector<float> > &data) {
    QFileInfo fi(m_DataFileName);
    QString fname = fi.absoluteDir().absoluteFilePath(QString("mcmc_%1.mat").arg(varname));
    mat_t *mat = Mat_CreateVer(fname.toLatin1(), NULL, MAT_FT_MAT5);
    if (mat == NULL) {
        setError("Error creating samples output file.");
        return -5;
    }
    int ret = write_cell_vector(mat, varname, data);
    Mat_Close(mat);
    return ret;
}

/* void PrfSamplingJob::load_data() {

} */
