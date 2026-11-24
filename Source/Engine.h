#pragma once
#include <juce_dsp/juce_dsp.h>
#include "Dsp.h"
#include <algorithm>
#include <cmath>

// Motor de sonido de JustAnAmp (bajo): pedal + head, todo en mono, sin cabina.
// Las etapas no lineales corren sobremuestreadas 4x; los filtros trabajan en double.
namespace jah
{
// Interpola una tabla de n puntos repartidos en t = 0..1
inline double interpTable (const double* tbl, int n, double t)
{
    t = clampd (t, 0.0, 1.0) * (n - 1);
    const int i = (int) t;
    const int j = std::min (i + 1, n - 1);
    return tbl[i] + (tbl[j] - tbl[i]) * (t - i);
}

// Curva suave, impar y acotada en +-1, pendiente 1 en el origen. k alto = recorte mas duro.
inline double shape (double x, double k)
{
    const double ax = std::abs (x);
    if (k == 2.0) return x / std::sqrt (1.0 + ax * ax);
    return x / std::pow (1.0 + std::pow (ax, k), 1.0 / k);
}

//==============================================================================
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    double process (double x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1 = z2 = 0.0; }

    void setPeak (double f, double q, double gDb, double fs)
    {
        const double A = std::pow (10.0, gDb / 40.0), w = 2 * kPi * clampd (f, 10.0, fs * 0.45) / fs;
        const double al = std::sin (w) / (2 * q), c = std::cos (w);
        norm (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void setLowShelf (double f, double gDb, double fs)
    {
        const double A = std::pow (10.0, gDb / 40.0), w = 2 * kPi * clampd (f, 10.0, fs * 0.45) / fs;
        const double c = std::cos (w), beta = 2 * std::sqrt (A) * (std::sin (w) / (2 * 0.7071));
        norm (A * ((A + 1) - (A - 1) * c + beta), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - beta),
              (A + 1) + (A - 1) * c + beta, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - beta);
    }
    void setHighShelf (double f, double gDb, double fs)
    {
        const double A = std::pow (10.0, gDb / 40.0), w = 2 * kPi * clampd (f, 10.0, fs * 0.45) / fs;
        const double c = std::cos (w), beta = 2 * std::sqrt (A) * (std::sin (w) / (2 * 0.7071));
        norm (A * ((A + 1) + (A - 1) * c + beta), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - beta),
              (A + 1) - (A - 1) * c + beta, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - beta);
    }

private:
    void norm (double B0, double B1, double B2, double A0, double A1, double A2)
    {
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
};

//==============================================================================
// Etapa de valvula: acople (HP) -> ganancia -> curva asimetrica -> filtro pasabajos (Miller).
struct TubeStage
{
    double gain = 1, k = 2, bias = 0, norm = 1, off = 0;
    OnePole hp, lp;

    void setup (double g, double k_, double b, double hpHz, double lpHz, double fs)
    {
        gain = g; k = k_; bias = b;
        off = shape (bias, k);
        const double e = 1e-4;
        norm = 1.0 / std::max (0.2, (shape (bias + e, k) - shape (bias - e, k)) / (2 * e));
        hp.set (hpHz, fs);
        lp.set (lpHz, fs);
    }
    double process (double x)
    {
        x = hp.hp (x);
        return lp.lp ((shape (x * gain + bias, k) - off) * norm);
    }
    void reset() { hp.reset(); lp.reset(); }
};

//==============================================================================
struct Params
{
    int head = 0, pedal = 0;                   // head: 0 Pulga, 1 NotFieldy, 2 RoAguMiSSTruVe, 3 Ryandemar, 4 Canciller (doble GK), 5 TimmyC (doble SVT) | pedal: 0 Off, 1 SansAmp, 2 Darkglass
    float gain = 5, bass = 5, mid = 5, himid = 5, treble = 5, contour = 0, ulo = 0, uhi = 0, freq = 2, masterDb = 0;
    float eq[7] = { 0, 0, 0, 0, 0, 0, 0 };    // ecualizador grafico (dB)
    bool bright = false, boost = false;
    float dirty = 4;                           // mezcla limpio/sucio de los heads dobles (4 y 5)
    float pdrive = 4, pblend = 5, plevelDb = 0, pbass = 5, ptreble = 5, pextra = 5;
};

// Voicing de cada head: etapas de preamp, potencia y filtro de salida
struct HeadVoice
{
    const char* name;
    int nStages;
    double gainMinDb, gainMaxDb;
    double base[3], k[3], bias[3], coupHP[3], millerLP[3], w[3];
    double inHP;
    double pwrDrive, pwrK, sag;     // etapa de potencia
    double outLP;                   // pasabajos de salida (sin cabina)
    double trimDb;
    double levelComp[5];            // compensacion de nivel segun Gain (dB)
};

inline const HeadVoice& voiceFor (int h, bool dirty = false)
{
    static const HeadVoice v[4] = {
        // Pulga: estado solido, rapido y brillante, sin sag
        { "Pulga", 2, -4.0, 24.0, { 1.0, 1.0, 1.0 }, { 3.2, 3.0, 2.0 }, { 0.05, 0.04, 0.0 }, { 30, 40, 40 }, { 16000, 14000, 12000 }, { 0.6, 0.4, 0.0 },
          30.0, 1.0, 4.0, 0.0, 11000.0, 0.0, { 7.6, 4.8, 0.5, -3.4, -5.5 } },
        // NotFieldy: preamp de valvulas muy agresivo, "clack" de agudos
        { "NotFieldy", 3, 0.0, 38.0, { 1.4, 1.2, 1.0 }, { 2.0, 2.2, 2.4 }, { 0.18, 0.14, 0.10 }, { 35, 50, 60 }, { 11000, 9000, 8000 }, { 0.4, 0.35, 0.25 },
          30.0, 1.0, 3.0, 0.2, 12000.0, 0.0, { 4.8, 1.8, -1.6, -3.7, -5.0 } },
        // RoAguMiSSTruVe: todo valvulas, potencia con mucho sag, redondo y pesado
        { "RoAguMiSSTruVe", 2, 0.0, 26.0, { 1.5, 1.3, 1.0 }, { 2.0, 2.0, 2.0 }, { 0.22, 0.18, 0.0 }, { 25, 30, 40 }, { 9000, 7500, 7500 }, { 0.55, 0.45, 0.0 },
          25.0, 1.7, 2.2, 0.7, 7500.0, 0.0, { -0.1, -1.6, -3.5, -4.7, -5.3 } },
        // Ryandemar: preamp valvular, potencia de estado solido: firme y moderno
        { "Ryandemar", 2, -2.0, 28.0, { 1.5, 1.3, 1.0 }, { 2.0, 2.4, 2.0 }, { 0.20, 0.12, 0.0 }, { 30, 40, 40 }, { 10000, 9000, 8000 }, { 0.55, 0.45, 0.0 },
          30.0, 1.2, 3.2, 0.18, 9000.0, 0.0, { 2.2, -0.1, -3.2, -5.2, -6.0 } }
    };
    // Heads dobles. Canciller: dos GK en paralelo, uno limpio y otro "sucio" (los pedales entran solo al sucio).
    // TimmyC: SVT limpio + SVT empujado hasta la distorsion (el pedal tambien entra solo al sucio).
    static const HeadVoice dual[4] = {
        { "Canciller limpio", 2, -4.0, 8.0, { 1.0, 1.0, 1.0 }, { 3.2, 3.0, 2.0 }, { 0.05, 0.04, 0.0 }, { 30, 40, 40 }, { 16000, 14000, 12000 }, { 0.6, 0.4, 0.0 },
          30.0, 1.0, 4.0, 0.0, 11000.0, 6.4, { 0, 0, 0, 0, 0 } },
        { "Canciller sucio", 2, 4.0, 30.0, { 1.2, 1.2, 1.0 }, { 3.4, 3.4, 2.0 }, { 0.08, 0.06, 0.0 }, { 30, 40, 40 }, { 14000, 12000, 10000 }, { 0.6, 0.4, 0.0 },
          30.0, 1.2, 4.0, 0.05, 11000.0, 0.0, { -0.6, -2.5, -4.8, -6.1, -6.7 } },
        { "TimmyC limpio", 2, 0.0, 10.0, { 1.5, 1.3, 1.0 }, { 2.0, 2.0, 2.0 }, { 0.22, 0.18, 0.0 }, { 25, 30, 40 }, { 9000, 7500, 7500 }, { 0.55, 0.45, 0.0 },
          25.0, 1.7, 2.2, 0.7, 7500.0, -2.3, { 0, 0, 0, 0, 0 } },
        { "TimmyC sucio", 3, 6.0, 34.0, { 1.5, 1.4, 1.2 }, { 2.2, 2.6, 3.2 }, { 0.22, 0.18, 0.12 }, { 30, 40, 50 }, { 9000, 7500, 6500 }, { 0.45, 0.35, 0.20 },
          25.0, 2.2, 3.0, 0.5, 7500.0, 0.0, { -3.9, -4.8, -5.8, -6.6, -6.8 } }
    };
    if (h >= 4) return dual[((h > 5 ? 5 : h) - 4) * 2 + (dirty ? 1 : 0)];
    return v[h < 0 ? 0 : h];
}

//==============================================================================
// Pedales de bajo: 1 = estilo SansAmp (preamp valvular con Blend), 2 = estilo Darkglass (distorsion en paralelo
// que conserva los graves). Corren sobremuestreados.
struct BassPedal
{
    int type = 0;
    double G = 1, k = 2, bias = 0.1, off = 0, norm = 1, pb = 0.5, lvl = 1, comp = 1;
    OnePole inHP, dHP1, dHP2, lp1, lp2;
    Biquad pre, grunt, bassS, trebS, presS;

    static double compDb (int t, double drive)
    {
        static const double tbl[2][5] = { { 9.9, 2.5, -1.7, -3.2, -3.6 }, { 13.6, 4.1, -2.7, -5.8, -6.8 } };
        return interpTable (tbl[t == 2 ? 1 : 0], 5, drive / 10.0);
    }

    void setup (int t, const Params& p, double fs)
    {
        type = t;
        pb = clampd (p.pblend / 10.0, 0.0, 1.0);
        lvl = dbToLin (p.plevelDb);
        comp = dbToLin (compDb (t, p.pdrive));
        const double drive = clampd (p.pdrive, 0.0, 10.0);
        if (t == 1)
        {
            G = dbToLin (drive * 3.6);
            k = 2.0; bias = 0.14;
            inHP.set (35.0, fs);
            pre.setPeak (900.0, 0.8, 3.0, fs);
            lp1.set (7500.0, fs); lp2.set (9000.0, fs);
            bassS.setLowShelf (90.0, (p.pbass - 5.0) * 1.8, fs);
            trebS.setHighShelf (2800.0, (p.ptreble - 5.0) * 1.8, fs);
            presS.setHighShelf (5500.0, (p.pextra - 5.0) * 1.6, fs);
            grunt.setPeak (200.0, 1.0, 0.0, fs);
        }
        else
        {
            G = dbToLin (drive * 4.0);
            k = 3.0; bias = 0.25;
            dHP1.set (110.0, fs); dHP2.set (110.0, fs);
            pre.setHighShelf (1800.0, 5.0, fs);
            grunt.setPeak (190.0, 1.0, clampd (p.pextra, 0.0, 10.0) * 1.0, fs);
            lp1.set (6500.0, fs); lp2.set (6500.0, fs);
            bassS.setLowShelf (60.0, (p.pbass - 5.0) * 2.0, fs);
            trebS.setHighShelf (4000.0, (p.ptreble - 5.0) * 2.0, fs);
            presS.setHighShelf (5500.0, 0.0, fs);
        }
        off = shape (bias, k);
        const double e = 1e-4;
        norm = 1.0 / std::max (0.2, (shape (bias + e, k) - shape (bias - e, k)) / (2 * e));
    }

    void reset()
    {
        inHP.reset(); dHP1.reset(); dHP2.reset(); lp1.reset(); lp2.reset();
        pre.reset(); grunt.reset(); bassS.reset(); trebS.reset(); presS.reset();
    }

    double process (double x)
    {
        double d;
        if (type == 1)
        {
            d = pre.process (inHP.hp (x));
            d = (shape (d * G + bias, k) - off) * norm;
            d = lp2.lp (lp1.lp (d)) * 0.35 * comp;
        }
        else
        {
            d = dHP2.hp (dHP1.hp (x));
            d = pre.process (grunt.process (d));
            d = (shape (d * G + bias, k) - off) * norm;
            d = lp2.lp (lp1.lp (d)) * 0.35 * comp;
        }
        double y = x * (1.0 - pb) + d * pb;
        y = presS.process (trebS.process (bassS.process (y)));
        return y * lvl;
    }
};

//==============================================================================
// Una cadena de amp: preamp (1 a 3 etapas) -> tono -> potencia con sag. Corre sobremuestreada.
struct AmpChain
{
    HeadVoice vo = voiceFor (0);
    TubeStage stage[3];
    OnePole inHP, dcHP;
    Biquad bassS, loMid, hiMid, trebS, cLow, cMid, cHigh, bLow, bHigh, brightS, uLoS, uLoD, uHiS, eqB[7];
    double env = 0, sagAtk = 0.01, sagRel = 0.0001, trimLin = 1.0;

    void reset()
    {
        for (auto& st : stage) st.reset();
        inHP.reset(); dcHP.reset();
        for (auto* b : { &bassS, &loMid, &hiMid, &trebS, &cLow, &cMid, &cHigh, &bLow, &bHigh, &brightS, &uLoS, &uLoD, &uHiS }) b->reset();
        for (auto& b : eqB) b.reset();
        env = 0.0;
    }

    // layout de tono: 0 estilo GK, 1 estilo Mesa + grafico, 2 estilo SVT, 3 SVT moderno. gnarly: voz "sucia" del head doble.
    void update (const Params& sm, const HeadVoice& v, int layout, bool gnarly, double g, double fsOs)
    {
        vo = v;
        const double t = std::pow (g, 1.4);
        const double gainDb = vo.gainMinDb + (vo.gainMaxDb - vo.gainMinDb) * t;
        for (int s = 0; s < vo.nStages; ++s)
            stage[s].setup (vo.base[s] * dbToLin (vo.w[s] * gainDb), vo.k[s], vo.bias[s], vo.coupHP[s], vo.millerLP[s], fsOs);
        inHP.set (vo.inHP, fsOs);

        auto peak0 = [fsOs] (Biquad& b, double f) { b.setPeak (f, 1.0, 0.0, fsOs); };
        auto lsh0 = [fsOs] (Biquad& b) { b.setLowShelf (100.0, 0.0, fsOs); };
        auto hsh0 = [fsOs] (Biquad& b) { b.setHighShelf (4000.0, 0.0, fsOs); };
        lsh0 (bassS); peak0 (loMid, 500); peak0 (hiMid, 1300); hsh0 (trebS); lsh0 (cLow); peak0 (cMid, 480); hsh0 (cHigh);
        lsh0 (bLow); hsh0 (bHigh); hsh0 (brightS); lsh0 (uLoS); peak0 (uLoD, 550); hsh0 (uHiS);
        static const double eqF[7] = { 45, 110, 280, 700, 1600, 3600, 8000 };
        for (int b = 0; b < 7; ++b) peak0 (eqB[b], eqF[b]);

        static const double midF[5] = { 220, 450, 800, 1600, 3000 };
        const int fi = (int) clampd (std::round (sm.freq), 0.0, 4.0);
        const double ul = clampd (sm.ulo / 10.0, 0.0, 1.0), uh = clampd (sm.uhi / 10.0, 0.0, 1.0);
        switch (layout)
        {
            case 0:   // bass / lo mid / hi mid / treble + contour + boost
            {
                const double c = clampd (sm.contour / 10.0, 0.0, 1.0);
                bassS.setLowShelf (55.0, (sm.bass - 5.0) * 3.0, fsOs);
                loMid.setPeak (250.0, 0.9, (sm.mid - 5.0) * 2.4, fsOs);
                hiMid.setPeak (1300.0, 0.9, (sm.himid - 5.0) * 2.4, fsOs);
                trebS.setHighShelf (3800.0, (sm.treble - 5.0) * 2.8, fsOs);
                cMid.setPeak (480.0, 0.7, -11.0 * c - (gnarly ? 5.0 : 0.0), fsOs);
                cLow.setLowShelf (70.0, 5.0 * c, fsOs);
                cHigh.setHighShelf (gnarly ? 2500.0 : 4500.0, (gnarly ? 3.0 : 0.0) + 6.0 * c, fsOs);
                if (sm.boost) { bLow.setLowShelf (90.0, 4.5, fsOs); bHigh.setHighShelf (5000.0, 5.0, fsOs); }
                break;
            }
            case 1:   // bass / mid / treble + bright + grafico de 7 bandas
            {
                bassS.setLowShelf (70.0, (sm.bass - 5.0) * 2.8, fsOs);
                loMid.setPeak (700.0, 0.8, (sm.mid - 5.0) * 2.0, fsOs);
                trebS.setHighShelf (3500.0, (sm.treble - 5.0) * 2.8, fsOs);
                if (sm.bright) brightS.setHighShelf (3000.0, 6.0, fsOs);
                for (int b = 0; b < 7; ++b) eqB[b].setPeak (eqF[b], 1.4, clampd (sm.eq[b], -12.0, 12.0), fsOs);
                break;
            }
            case 2:   // bass / mid + frecuencia / treble + ultra lo / ultra hi
            {
                bassS.setLowShelf (50.0, (sm.bass - 5.0) * 3.0, fsOs);
                loMid.setPeak (midF[fi], 0.75, (sm.mid - 5.0) * 2.4, fsOs);
                trebS.setHighShelf (3200.0, (sm.treble - 5.0) * 2.0, fsOs);
                uLoS.setLowShelf (45.0, 7.0 * ul, fsOs);
                uLoD.setPeak (550.0, 0.8, -5.0 * ul, fsOs);
                uHiS.setHighShelf (7500.0, 8.0 * uh, fsOs);
                break;
            }
            default:  // igual que el anterior, con Ultra Lo / Ultra Hi continuos y mas rango
            {
                bassS.setLowShelf (55.0, (sm.bass - 5.0) * 3.0, fsOs);
                loMid.setPeak (midF[fi], 0.8, (sm.mid - 5.0) * 2.4, fsOs);
                trebS.setHighShelf (3000.0, (sm.treble - 5.0) * 2.4, fsOs);
                uLoS.setLowShelf (40.0, 8.0 * ul, fsOs);
                uLoD.setPeak (550.0, 0.8, -4.0 * ul, fsOs);
                uHiS.setHighShelf (8000.0, 10.0 * uh, fsOs);
                break;
            }
        }
        sagAtk = 1.0 - std::exp (-1.0 / (fsOs * 0.004));
        sagRel = 1.0 - std::exp (-1.0 / (fsOs * 0.15));
        dcHP.set (20.0, fsOs);
        trimLin = dbToLin (vo.trimDb + interpTable (vo.levelComp, 5, g));
    }

    double tone (double v)
    {
        v = brightS.process (v);
        v = bassS.process (v);
        v = loMid.process (v);
        v = hiMid.process (v);
        v = trebS.process (v);
        v = cMid.process (v); v = cLow.process (v); v = cHigh.process (v);
        v = bLow.process (v); v = bHigh.process (v);
        v = uLoS.process (v); v = uLoD.process (v); v = uHiS.process (v);
        for (auto& b : eqB) v = b.process (v);
        return v;
    }

    double process (double x)
    {
        double v = inHP.hp (x);
        for (int s = 0; s < vo.nStages; ++s)
        {
            v = stage[s].process (v);
            if (s == 0) v = tone (v);
        }
        const double a = std::abs (v);
        env += (a > env ? sagAtk : sagRel) * (a - env);
        v /= 1.0 + vo.sag * env;
        v = shape (v * vo.pwrDrive, vo.pwrK);
        return dcHP.hp (v) * trimLin;
    }
};

//==============================================================================
class Engine
{
public:
    static constexpr int kSub = 64;

    void prepare (double sampleRate, int /*maxBlock*/)
    {
        fs = sampleRate;
        fsOs = sampleRate * 4.0;
        os.initProcessing ((size_t) kSub);
        os.reset();
        resetStates();
        sm = tgt;
        started = false;
    }

    int getLatency() const { return (int) std::lround (os.getLatencyInSamples()); }

    void setParams (const Params& p)
    {
        if (started && (p.head != tgt.head || p.pedal != tgt.pedal))
            fadeLeft = kFade;
        tgt = p;
    }

    void process (float* x, int n)
    {
        if (! started)
        {
            sm = tgt; started = true;
            resetStates();
        }
        int pos = 0;
        while (pos < n)
        {
            const int m = std::min (kSub, n - pos);
            const double a = 1.0 - std::exp (-(double) m / (fs * 0.02));
            auto sg = [a] (float& cur, float t) { cur += (t - cur) * (float) a; };
            sg (sm.gain, tgt.gain); sg (sm.bass, tgt.bass); sg (sm.mid, tgt.mid); sg (sm.himid, tgt.himid); sg (sm.treble, tgt.treble);
            sg (sm.contour, tgt.contour); sg (sm.ulo, tgt.ulo); sg (sm.uhi, tgt.uhi); sg (sm.freq, tgt.freq); sg (sm.dirty, tgt.dirty);
            for (int b = 0; b < 7; ++b) sg (sm.eq[b], tgt.eq[b]);
            sg (sm.pdrive, tgt.pdrive); sg (sm.pblend, tgt.pblend); sg (sm.plevelDb, tgt.plevelDb);
            sg (sm.pbass, tgt.pbass); sg (sm.ptreble, tgt.ptreble); sg (sm.pextra, tgt.pextra);
            sm.head = tgt.head; sm.pedal = tgt.pedal; sm.bright = tgt.bright; sm.boost = tgt.boost;

            if (sm.head != curHead || sm.pedal != curPedal)
            {
                curHead = sm.head; curPedal = sm.pedal;
                resetStates();
            }
            updateCoeffs();

            float* ptrs[1] = { x + pos };
            juce::dsp::AudioBlock<float> blk (ptrs, 1, (size_t) m);
            auto up = os.processSamplesUp (blk);
            float* d = up.getChannelPointer (0);
            for (size_t i = 0; i < up.getNumSamples(); ++i)
                d[i] = (float) processSample (d[i]);
            os.processSamplesDown (blk);

            for (int i = 0; i < m; ++i)
            {
                double v = outLP2.lp (outLP1.lp ((double) x[pos + i]));
                if (fadeLeft > 0)
                {
                    v *= 1.0 - (double) fadeLeft / kFade;
                    --fadeLeft;
                }
                x[pos + i] = (float) v;
            }
            pos += m;
        }
    }

private:
    static constexpr int kFade = 480;

    void resetStates()
    {
        chainA.reset(); chainB.reset();
        outLP1.reset(); outLP2.reset();
        pedal.reset();
        fadeLeft = started ? kFade : 0;
    }

    void updateCoeffs()
    {
        const int head = sm.head;
        dual = head >= 4;
        const int layout = head == 4 ? 0 : (head == 5 ? 2 : head);
        const double g = clampd (sm.gain / 10.0, 0.0, 1.0);
        if (! dual)
            chainA.update (sm, voiceFor (head, false), layout, false, g, fsOs);
        else
        {
            chainA.update (sm, voiceFor (head, false), layout, false, 0.5, fsOs);    // lado limpio: ganancia fija
            chainB.update (sm, voiceFor (head, true), layout, true, g, fsOs);         // lado sucio: responde al Gain
        }
        dirtyAmt = dual ? clampd (sm.dirty / 10.0, 0.0, 1.0) : 0.0;
        pedal.setup (sm.pedal, sm, fsOs);
        outLP1.set (chainA.vo.outLP, fs);
        outLP2.set (chainA.vo.outLP, fs);
    }

    double processSample (double x)
    {
        const double xp = sm.pedal != 0 ? pedal.process (x) : x;
        if (! dual) return chainA.process (xp);
        const double a = chainA.process (x);            // limpio: sin pedal
        const double b = chainB.process (xp);           // sucio: recibe los pedales
        return a * (1.0 - dirtyAmt) + b * dirtyAmt;
    }

    double fs = 48000.0, fsOs = 192000.0;
    juce::dsp::Oversampling<float> os { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false };
    Params tgt, sm;
    bool started = false, dual = false;
    int curHead = -1, curPedal = -1, fadeLeft = 0;
    double dirtyAmt = 0.0;

    AmpChain chainA, chainB;
    BassPedal pedal;
    OnePole outLP1, outLP2;
};
} // namespace jah
