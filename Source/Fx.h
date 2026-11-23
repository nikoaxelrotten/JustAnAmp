#pragma once
#include <vector>
#include "Dsp.h"

// Dinamica y efectos de bajo: compresores (Opto / FET), modulacion (chorus, flanger, phaser, octaver, filtro de envolvente), delay estereo y reverb.
namespace jah
{
struct FxParams
{
    int mod = 0;                                   // 0 Off, 1 Chorus, 2 Flanger, 3 Phaser, 4 Tremolo, 5 Slicer, 6 Ring Mod
    float mrate = 3, mdepth = 6, mmix = 0.5f;      // rate y depth 0..10, mix 0..1
    float mshape = 3;
    int mpat = 0;                                  // patron del slicer (0..5)
    int order[3] = { 0, 1, 2 };                    // orden de los efectos: 0 Mod, 1 Delay, 2 Reverb                              // 0..10: forma del tremolo (seno->cuadrada) y filo del slicer
    float dtimeMs = 380, dfb = 0.35f, dmix = 0.0f; // delay
    float rsize = 4, rdamp = 5, rmix = 0.0f;       // reverb (size y damp 0..10)
    double bpm = 120.0, ppq = 0.0;
    bool playing = false, hasPpq = false;
};

// Puerta de ruido: detector de picos con histeresis, retencion y cierre total.
// thrDb <= -79.5 la apaga.
class NoiseGate
{
public:
    void prepare (double sr)
    {
        fs = sr; env = 0.0; g = 1.0; hold = 0; open = true;
        decayK = std::exp (-1.0 / (fs * 0.004));
        atkK = 1.0 - std::exp (-1.0 / (fs * 0.0004));
    }
    void process (float* x, int n, float thrDb, float relMs)
    {
        const bool off = thrDb <= -79.5f;
        const double thr = dbToLin (thrDb), thrClose = thr * 0.6;
        const double relK = 1.0 - std::exp (-1.0 / (fs * 0.001 * clampd (relMs, 5.0, 1000.0)));
        const int holdN = (int) (fs * 0.02);
        for (int i = 0; i < n; ++i)
        {
            const double a = std::abs ((double) x[i]);
            env = std::max (a, env * decayK);
            if (off) open = true;
            else if (env > thr) { open = true; hold = holdN; }
            else if (hold > 0) --hold;
            else if (env < thrClose) open = false;
            g += ((open ? 1.0 : 0.0) - g) * (open ? atkK : relK);
            x[i] = (float) (x[i] * g);
        }
    }
private:
    double fs = 48000.0, env = 0.0, g = 1.0, decayK = 0.99, atkK = 0.05;
    int hold = 0;
    bool open = true;
};

// Patrones del slicer (8 pasos): Gallop, Straight, Offbeat, Chug, 3-3-2, Stutter
constexpr int kNumPatterns = 6;
inline bool slicerStep (int pat, int step)
{
    static const bool t[kNumPatterns][8] = {
        { 1, 1, 0, 1, 0, 1, 1, 0 }, { 1, 0, 1, 0, 1, 0, 1, 0 }, { 0, 1, 0, 1, 0, 1, 0, 1 },
        { 1, 0, 1, 1, 1, 0, 1, 1 }, { 1, 0, 0, 1, 0, 0, 1, 0 }, { 1, 1, 1, 0, 1, 1, 1, 0 } };
    return t[pat < 0 ? 0 : (pat >= kNumPatterns ? kNumPatterns - 1 : pat)][step & 7];
}

struct Smooth
{
    double v = 0, k = 0.001;
    void init (double fs, double sec, double start) { k = 1.0 - std::exp (-1.0 / (fs * sec)); v = start; }
    double next (double t) { v += (t - v) * k; return v; }
};

// Linea de retardo con lectura fraccionaria (Hermite). Se lee con d >= 2 muestras.
struct FracDelay
{
    std::vector<float> buf;
    int mask = 0, w = 0;
    void init (int maxSamples)
    {
        int s = 1;
        while (s < maxSamples + 8) s <<= 1;
        buf.assign ((size_t) s, 0.0f);
        mask = s - 1; w = 0;
    }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); }
    void write (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }
    float read (double d) const
    {
        d = std::max (d, 2.0);
        const double pos = (double) (w - 1) - d;
        const int i0 = (int) std::floor (pos);
        const float f = (float) (pos - i0);
        const float ym = buf[(size_t) ((i0 - 1) & mask)], y0 = buf[(size_t) (i0 & mask)];
        const float y1 = buf[(size_t) ((i0 + 1) & mask)], y2 = buf[(size_t) ((i0 + 2) & mask)];
        const float c1 = 0.5f * (y1 - ym), c2 = ym - 2.5f * y0 + 2.0f * y1 - 0.5f * y2, c3 = 0.5f * (y2 - ym) + 1.5f * (y0 - y1);
        return ((c3 * f + c2) * f + c1) * f + y0;
    }
};

//==============================================================================
class ModFx
{
public:
    void prepare (double sr)
    {
        fs = sr;
        const int maxD = (int) (fs * 0.06);
        cL.init (maxD); cR.init (maxD); fL.init (maxD); fR.init (maxD);
        sRate.init (fs, 0.05, 3); sDepth.init (fs, 0.05, 6); sMix.init (fs, 0.03, 0.5);
        gateK = 1.0 - std::exp (-1.0 / (fs * 0.004));
        trK = 1.0 - std::exp (-1.0 / (fs * 0.0003));
        fadeInc = 1.0 / (fs * 0.03);
        cur = -1;
    }

    // Entrada estereo (inL/inR pueden ser los mismos buffers que l/r)
    void process (const float* inL, const float* inR, float* l, float* r, int n, const FxParams& p)
    {
        if (p.mod != cur)
        {
            cur = p.mod;
            resetState();
        }
        if (cur == 0)
        {
            for (int i = 0; i < n; ++i) { const float a = inL[i], b = inR[i]; l[i] = a; r[i] = b; }
            return;
        }

        const double slK = 1.0 - std::exp (-1.0 / (fs * 0.001 * (6.0 * std::pow (0.25 / 6.0, p.mshape / 10.0))));
        const double bpm = clampd (p.bpm, 20.0, 400.0);
        static const double stepBeats[5] = { 1.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0 };
        const double stepSamples = stepBeats[std::min (4, (int) (p.mrate / 2.0f))] * 60.0 / bpm * fs;
        if (cur == 5 && p.playing && p.hasPpq)
        {
            const double sb = stepBeats[std::min (4, (int) (p.mrate / 2.0f))];
            const double q = p.ppq / sb;
            step = ((int) std::floor (q)) & 7;
            stepPos = (q - std::floor (q)) * stepSamples;
        }

        for (int i = 0; i < n; ++i)
        {
            const double rate = sRate.next (p.mrate), depth = sDepth.next (p.mdepth), mix = sMix.next (p.mmix);
            fade = std::min (1.0, fade + fadeInc);
            const double m = (cur == 4 || cur == 5) ? fade : mix * fade;   // tremolo y slicer: 100% procesado, no dejan pasar senal
            const float xl = inL[i], xr = inR[i];
            double wl = xl, wr = xr;

            switch (cur)
            {
                case 1: // Chorus: dos voces por canal, LFOs en cuadratura
                {
                    ph1 = wrap (ph1 + 0.1 * std::pow (40.0, rate / 10.0) / fs);
                    ph2 = wrap (ph2 + 0.127 * std::pow (40.0, rate / 10.0) / fs);
                    const double dm = 0.5 + depth / 10.0 * 5.5;
                    cL.write (xl); cR.write (xr);
                    const double k = fs / 1000.0;
                    wl = 0.5 * (cL.read ((17.0 + dm * sn (ph1)) * k) + cL.read ((25.0 + dm * sn (ph2 + 0.25)) * k));
                    wr = 0.5 * (cR.read ((17.0 + dm * sn (ph1 + 0.5)) * k) + cR.read ((25.0 + dm * sn (ph2 + 0.75)) * k));
                    break;
                }
                case 2: // Flanger: retardo corto barrido con realimentacion
                {
                    ph1 = wrap (ph1 + 0.05 * std::pow (40.0, rate / 10.0) / fs);
                    const double sweep = 0.4 + depth / 10.0 * 4.0, k = fs / 1000.0;
                    wl = fL.read ((0.5 + sweep * (0.5 + 0.5 * sn (ph1))) * k);
                    wr = fR.read ((0.5 + sweep * (0.5 + 0.5 * sn (ph1 + 0.25))) * k);
                    fL.write ((float) (xl + 0.55 * wl));
                    fR.write ((float) (xr + 0.55 * wr));
                    break;
                }
                case 3: // Phaser: 6 allpass de primer orden con barrido exponencial
                {
                    ph1 = wrap (ph1 + 0.05 * std::pow (60.0, rate / 10.0) / fs);
                    const double fmax = 700.0 + depth / 10.0 * 3800.0;
                    for (int c = 0; c < 2; ++c)
                    {
                        const double lfo = 0.5 + 0.5 * sn (ph1 + (c ? 0.25 : 0.0));
                        const double f = 180.0 * std::pow (fmax / 180.0, lfo);
                        const double t = std::tan (kPi * f / fs), a = (t - 1.0) / (t + 1.0);
                        double v = (c ? xr : xl) + 0.4 * apFb[c];
                        for (int s = 0; s < 6; ++s)
                        {
                            const double y = a * v + apZ[c][s];
                            apZ[c][s] = v - a * y;
                            v = y;
                        }
                        apFb[c] = v;
                        (c ? wr : wl) = v;
                    }
                    break;
                }
                case 4: // Tremolo: con Shape alto la onda se vuelve cuadrada; con Depth 10 corta del todo
                {
                    ph1 = wrap (ph1 + std::pow (12.0, rate / 10.0) / fs);
                    const double sh = p.mshape / 10.0, kk = 0.15 + sh * sh * 14.0;
                    const double lfo = std::tanh (kk * sn (ph1)) / std::tanh (kk);
                    const double tgtG = 1.0 - depth / 10.0 * (0.5 + 0.5 * lfo);
                    tg += (tgtG - tg) * trK;
                    wl = xl * tg; wr = xr * tg;
                    break;
                }
                case 5: // Slicer: patron de 8 pasos sincronizado al tempo
                {
                    stepPos += 1.0;
                    if (stepPos >= stepSamples) { stepPos -= stepSamples; step = (step + 1) & 7; }
                    const double target = slicerStep (p.mpat, step) ? 1.0 : 1.0 - depth / 10.0;
                    gate += (target - gate) * slK;
                    wl = xl * gate; wr = xr * gate;
                    break;
                }
                case 6: // Ring modulator: Depth pasa de AM a ring mod completo
                {
                    ph1 = wrap (ph1 + 25.0 * std::pow (80.0, rate / 10.0) / fs);
                    const double d = depth / 10.0;
                    { const double rm = (1.0 - d) + d * sn (ph1); wl = xl * rm; wr = xr * rm; }
                    break;
                }
                default: break;
            }
            l[i] = (float) (xl * (1.0 - m) + wl * m);
            r[i] = (float) (xr * (1.0 - m) + wr * m);
        }
    }

private:
    static double wrap (double p) { return p - std::floor (p); }
    static double sn (double ph) { return std::sin (2.0 * kPi * ph); }

    void resetState()
    {
        cL.clear(); cR.clear(); fL.clear(); fR.clear();
        for (auto& a : apZ) for (auto& z : a) z = 0.0;
        apFb[0] = apFb[1] = 0.0;
        ph1 = 0.0; ph2 = 0.0; step = 0; stepPos = 0.0; gate = 1.0; tg = 1.0; fade = 0.0;
    }

    double fs = 48000.0, ph1 = 0, ph2 = 0, apZ[2][6] = {}, apFb[2] = {}, stepPos = 0, gate = 1, tg = 1, trK = 0.05, gateK = 0.01, fade = 0, fadeInc = 0.001;
    int cur = -1, step = 0;
    FracDelay cL, cR, fL, fR;
    Smooth sRate, sDepth, sMix;
};

//==============================================================================
class StereoDelay
{
public:
    void prepare (double sr)
    {
        fs = sr;
        dl.init ((int) (fs * 1.6)); dr.init ((int) (fs * 1.6));
        sT.init (fs, 0.06, 380.0); sMix.init (fs, 0.03, 0.0);
        lpL.reset(); lpR.reset(); hpL.reset(); hpR.reset();
        lpL.set (5000.0, fs); lpR.set (5000.0, fs); hpL.set (120.0, fs); hpR.set (120.0, fs);
    }

    void process (float* l, float* r, int n, const FxParams& p)
    {
        const double fb = clampd (p.dfb, 0.0f, 0.92f);
        for (int i = 0; i < n; ++i)
        {
            const double t = clampd (sT.next (p.dtimeMs), 20.0, 1500.0) * fs / 1000.0;
            const double mix = sMix.next (p.dmix);
            const float in = 0.5f * (l[i] + r[i]);
            const float yl = dl.read (t), yr = dr.read (t);
            // ping-pong: la entrada va a L, L alimenta a R, R realimenta a L (filtrado)
            const double back = hpR.hp (lpR.lp (yr));
            dl.write ((float) (in + fb * back));
            dr.write (yl);
            l[i] += (float) (yl * mix);
            r[i] += (float) (yr * mix);
        }
    }

private:
    double fs = 48000.0;
    FracDelay dl, dr;
    Smooth sT, sMix;
    OnePole lpL, lpR, hpL, hpR;
};

//==============================================================================
// Reverb FDN de 8 lineas con matriz de Hadamard, difusion de entrada y damping.
class Reverb
{
public:
    void prepare (double sr)
    {
        fs = sr;
        static const int base[N] = { 1423, 1789, 1993, 2161, 2579, 2897, 3257, 3571 };
        static const int apBase[4] = { 142, 107, 379, 277 };
        const double sc = fs / 48000.0;
        for (int i = 0; i < N; ++i)
        {
            len[i] = (int) (base[i] * sc) | 1;
            line[i].assign ((size_t) len[i], 0.0f);
            pos[i] = 0; lp[i] = 0.0f;
        }
        for (int i = 0; i < 4; ++i)
        {
            apLen[i] = (int) (apBase[i] * sc) | 1;
            apBuf[i].assign ((size_t) apLen[i], 0.0f);
            apPos[i] = 0;
        }
        sMix.init (fs, 0.03, 0.0);
        sSize.init (fs, 0.1, 4.0); sDamp.init (fs, 0.1, 5.0);
        updateCoeffs (4.0, 5.0);
    }

    void process (float* l, float* r, int n, const FxParams& p)
    {
        for (int i = 0; i < n; ++i)
        {
            const double size = sSize.next (p.rsize), damp = sDamp.next (p.rdamp), mix = sMix.next (p.rmix);
            if ((i & 31) == 0) updateCoeffs (size, damp);

            float x = 0.5f * (l[i] + r[i]) * 0.35f;
            for (int a = 0; a < 4; ++a)                 // difusion (allpass de Schroeder)
            {
                float& b = apBuf[a][(size_t) apPos[a]];
                const float v = b;
                const float y = -0.6f * x + v;
                b = x + 0.6f * y;
                if (++apPos[a] >= apLen[a]) apPos[a] = 0;
                x = y;
            }

            float y[N];
            for (int k = 0; k < N; ++k)
            {
                const float v = line[k][(size_t) pos[k]];
                lp[k] += dampA * (v - lp[k]);
                y[k] = lp[k] * g[k];
            }
            // Hadamard 8x8 normalizada
            float a0 = y[0] + y[1], a1 = y[0] - y[1], a2 = y[2] + y[3], a3 = y[2] - y[3];
            float a4 = y[4] + y[5], a5 = y[4] - y[5], a6 = y[6] + y[7], a7 = y[6] - y[7];
            float b0 = a0 + a2, b1 = a1 + a3, b2 = a0 - a2, b3 = a1 - a3;
            float b4 = a4 + a6, b5 = a5 + a7, b6 = a4 - a6, b7 = a5 - a7;
            const float h[N] = { b0 + b4, b1 + b5, b2 + b6, b3 + b7, b0 - b4, b1 - b5, b2 - b6, b3 - b7 };
            const float s = 0.35355339f;
            for (int k = 0; k < N; ++k)
            {
                line[k][(size_t) pos[k]] = x + h[k] * s;
                if (++pos[k] >= len[k]) pos[k] = 0;
            }

            const float wl = (y[0] - y[1] + y[2] - y[3] + y[4] - y[5] + y[6] - y[7]);
            const float wr = (y[0] + y[1] - y[2] - y[3] + y[4] + y[5] - y[6] - y[7]);
            l[i] += (float) (wl * mix);
            r[i] += (float) (wr * mix);
        }
    }

private:
    static constexpr int N = 8;

    void updateCoeffs (double size, double damp)
    {
        const double s01 = clampd (size / 10.0, 0.0, 1.0);
        const double rt60 = 0.3 + std::pow (s01, 1.6) * 6.0;
        for (int i = 0; i < N; ++i)
            g[i] = (float) std::pow (10.0, -3.0 * len[i] / (fs * rt60));
        const double fc = 14000.0 * std::pow (0.12, clampd (damp / 10.0, 0.0, 1.0));
        dampA = (float) (1.0 - std::exp (-2.0 * kPi * fc / fs));
    }

    double fs = 48000.0;
    int len[N] = {}, pos[N] = {}, apLen[4] = {}, apPos[4] = {};
    float lp[N] = {}, g[N] = {}, dampA = 0.5f;
    std::vector<float> line[N], apBuf[4];
    Smooth sMix, sSize, sDamp;
};

//==============================================================================
//==============================================================================
// Compresor con dos caracteres: 1 = Opto (suave, RMS, release que depende de la reduccion),
// 2 = FET (rapido, de picos, ratio ajustable). Con Blend se mezcla en paralelo con la senal seca.
class Compressor
{
public:
    void prepare (double sr) { fs = sr; hp.set (60.0, fs); reset(); }
    void reset() { ms = 0.0; peak = 0.0; gr = 0.0; hp.reset(); }

    void process (float* x, int n, int type, double thrDb, double ratio, double atkMs, double relMs, double blend, double makeupDb)
    {
        if (type != lastType) { gr = 0.0; ms = 0.0; peak = 0.0; lastType = type; }
        if (type == 0) return;
        const bool opto = type == 1;
        const double mk = dbToLin (makeupDb);
        const double rmsK = 1.0 - std::exp (-1.0 / (fs * 0.008));
        const double pkAtk = 1.0 - std::exp (-1.0 / (fs * 0.0001));
        const double pkRel = 1.0 - std::exp (-1.0 / (fs * 0.03));
        const double atk = 1.0 - std::exp (-1.0 / (fs * 0.001 * (opto ? clampd (atkMs, 4.0, 80.0) : clampd (atkMs, 0.05, 60.0))));
        const double ratioUse = opto ? 3.0 : clampd (ratio, 1.2, 20.0);
        const double knee = opto ? 9.0 : 2.0;
        const double slope = 1.0 - 1.0 / ratioUse;
        for (int i = 0; i < n; ++i)
        {
            const double in = x[i];
            const double sc = hp.hp (in);
            double lvDb;
            if (opto) { ms += rmsK * (sc * sc - ms); lvDb = 10.0 * std::log10 (ms + 1e-12); }
            else { const double a = std::abs (sc); peak += (a > peak ? pkAtk : pkRel) * (a - peak); lvDb = 20.0 * std::log10 (peak + 1e-9); }
            const double over = lvDb - thrDb;
            double target;
            if (over <= -knee * 0.5) target = 0.0;
            else if (over >= knee * 0.5) target = over * slope;
            else { const double t = over + knee * 0.5; target = slope * t * t / (2.0 * knee); }
            const double relEff = opto ? relMs * (0.35 + 0.65 * clampd (gr / 12.0, 0.0, 1.0)) : relMs;
            const double rel = 1.0 - std::exp (-1.0 / (fs * 0.001 * clampd (relEff, 5.0, 3000.0)));
            gr += (target > gr ? atk : rel) * (target - gr);
            const double y = in * std::exp (-gr * 0.1151293) * mk;
            x[i] = (float) (in * (1.0 - blend) + y * blend);
        }
    }

    double lastGainReduction() const { return gr; }

private:
    double fs = 48000.0, ms = 0.0, peak = 0.0, gr = 0.0;
    int lastType = 0;
    OnePole hp;
};

//==============================================================================
// Octaver analogico: flip-flops sobre los cruces por cero de la senal filtrada (-1 y -2 octavas).
class Octaver
{
public:
    void prepare (double sr)
    {
        fs = sr; lp.set (200.0, fs); lp2.set (200.0, fs);
        sdA = 1.0 - std::exp (-1.0 / (fs * 0.001));
        sdR = 1.0 - std::exp (-1.0 / (fs * 0.03));
        envA = 1.0 - std::exp (-1.0 / (fs * 0.003));
        envR = 1.0 - std::exp (-1.0 / (fs * 0.06));
        reset();
    }
    void reset() { env = 0.0; envSd = 0.0; hi = false; f1 = false; f2 = false; lp.reset(); lp2.reset(); a1.reset(); a2.reset(); b1.reset(); b2.reset(); }
    void setTone (double hz) { a1.set (hz, fs); a2.set (hz, fs); b1.set (hz, fs); b2.set (hz, fs); }

    double process (double x, double o1, double o2, double direct)
    {
        const double sd = lp2.lp (lp.lp (x));
        const double a = std::abs (x);
        env += (a > env ? envA : envR) * (a - env);
        const double as = std::abs (sd);
        envSd += (as > envSd ? sdA : sdR) * (as - envSd);
        const double thr = std::max (0.003, envSd * 0.35);     // histeresis relativa: ignora los armonicos
        if (! hi && sd > thr) { hi = true; f1 = ! f1; if (f1) f2 = ! f2; }
        else if (hi && sd < -thr) hi = false;
        const double amp = env * 1.3;
        const double s1 = a2.lp (a1.lp ((f1 ? 1.0 : -1.0) * amp));
        const double s2 = b2.lp (b1.lp ((f2 ? 1.0 : -1.0) * amp));
        return direct * x + o1 * s1 + o2 * s2;
    }

private:
    double fs = 48000.0, env = 0.0, envSd = 0.0, envA = 0.01, envR = 0.001, sdA = 0.02, sdR = 0.001;
    bool hi = false, f1 = false, f2 = false;
    OnePole lp, lp2, a1, a2, b1, b2;
};

//==============================================================================
// Filtro de envolvente (pasabajos resonante, barrido por el nivel de la entrada).
class EnvFilter
{
public:
    void prepare (double sr)
    {
        fs = sr;
        envA = 1.0 - std::exp (-1.0 / (fs * 0.004));
        envR = 1.0 - std::exp (-1.0 / (fs * 0.12));
        reset();
    }
    void reset() { env = 0.0; for (auto& c : ch) c = {}; }

    void process (float* l, float* r, int n, double sens, double range, double res, double mix)
    {
        const double sg = 1.5 * std::pow (1.55, sens);
        const double octs = 2.0 + range * 0.5;
        const double q = 0.7 + res * res * 0.12;
        const double k = 1.0 / q;
        const double comp = 1.0 / (1.0 + (q - 0.7) * 0.08);
        for (int i = 0; i < n; ++i)
        {
            const double a = std::max (std::abs ((double) l[i]), std::abs ((double) r[i]));
            env += (a > env ? envA : envR) * (a - env);
            const double e = std::tanh (env * sg * 0.5);
            const double fc = clampd (60.0 * std::pow (2.0, e * octs), 20.0, fs * 0.4);
            const double g = std::tan (kPi * fc / fs);
            const double a1 = 1.0 / (1.0 + g * (g + k)), a2 = g * a1, a3 = g * a2;
            float* io[2] = { l, r };
            for (int c = 0; c < 2; ++c)
            {
                const double v0 = io[c][i];
                const double v3 = v0 - ch[c].ic2;
                const double v1 = a1 * ch[c].ic1 + a2 * v3;
                const double v2 = ch[c].ic2 + a2 * ch[c].ic1 + a3 * v3;
                ch[c].ic1 = 2.0 * v1 - ch[c].ic1;
                ch[c].ic2 = 2.0 * v2 - ch[c].ic2;
                io[c][i] = (float) (v0 * (1.0 - mix) + v2 * comp * mix);
            }
        }
    }

private:
    struct St { double ic1 = 0.0, ic2 = 0.0; };
    double fs = 48000.0, env = 0.0, envA = 0.01, envR = 0.001;
    St ch[2];
};

//==============================================================================
// Modulacion para bajo: tipo 0 Off, 1 Chorus, 2 Flanger (solo sobre los agudos, los graves quedan intactos),
// 3 Phaser, 4 Octaver, 5 Filtro de envolvente. mrate/mdepth/mshape/mmix cambian de sentido segun el tipo.
class BassMod
{
public:
    static constexpr int kChunk = 512;

    void prepare (double sr)
    {
        fs = sr;
        mod.prepare (sr); oct.prepare (sr); envf.prepare (sr);
        lpL.set (160.0, fs); lpR.set (160.0, fs);
    }

    void process (float* l, float* r, int n, const FxParams& p)
    {
        FxParams pm = p;
        const int t = p.mod;
        pm.mod = (t >= 1 && t <= 3) ? t : 0;
        const double toneHz = 80.0 * std::pow (8.0, clampd (p.mshape, 0.0f, 10.0f) / 10.0);
        oct.setTone (toneHz);
        for (int s = 0; s < n; s += kChunk)
        {
            const int m = std::min (kChunk, n - s);
            float* L = l + s; float* R = r + s;
            if (t == 1 || t == 2)
            {
                for (int i = 0; i < m; ++i)
                {
                    lowL[i] = (float) lpL.lp (L[i]); lowR[i] = (float) lpR.lp (R[i]);
                    hL[i] = L[i] - lowL[i]; hR[i] = R[i] - lowR[i];
                }
                mod.process (hL, hR, hL, hR, m, pm);
                for (int i = 0; i < m; ++i) { L[i] = lowL[i] + hL[i]; R[i] = lowR[i] + hR[i]; }
            }
            else
            {
                mod.process (L, R, L, R, m, pm);          // fase / pass-through
                if (t == 4)
                {
                    const double o1 = p.mrate / 10.0, o2 = p.mdepth / 10.0, direct = p.mmix;
                    for (int i = 0; i < m; ++i)
                    {
                        const double y = oct.process (0.5 * ((double) L[i] + (double) R[i]), o1, o2, direct);
                        L[i] = R[i] = (float) y;
                    }
                }
                else if (t == 5)
                    envf.process (L, R, m, p.mrate, p.mdepth, p.mshape, p.mmix);
            }
        }
    }

private:
    double fs = 48000.0;
    ModFx mod;
    Octaver oct;
    EnvFilter envf;
    OnePole lpL, lpR;
    float lowL[kChunk], lowR[kChunk], hL[kChunk], hR[kChunk];
};

class FxChain
{
public:
    void prepare (double sr)
    {
        mod.prepare (sr); delay.prepare (sr); reverb.prepare (sr);
    }
    // in: mono desde el amp. l/r: salida estereo. El orden de Mod, Delay y Reverb lo da p.order.
    void process (const float* in, float* l, float* r, int n, const FxParams& p)
    {
        for (int i = 0; i < n; ++i) { l[i] = in[i]; r[i] = in[i]; }
        for (int k = 0; k < 3; ++k)
        {
            switch (p.order[k])
            {
                case 0:  mod.process (l, r, n, p); break;
                case 1:  delay.process (l, r, n, p); break;
                default: reverb.process (l, r, n, p); break;
            }
        }
    }

private:
    BassMod mod;
    StereoDelay delay;
    Reverb reverb;
};
} // namespace jah
