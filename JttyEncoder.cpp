#include "JttyEncoder.hpp"

#include <QByteArray>
#include <QStringList>

#include "widgets/JttyMessages.hpp"

extern "C" {
  void genjtty_profile_ (char * msg, int const * exchange_profile,
                         int itone[], int * nsym, int frame_starts[],
                         int const * is_final, fortran_charlen_t);
  void gen_jttywave_ (int itone[], int * nsym, int * nsps, float * bt, float * fsample, float * f0,
                      float xjunk[], float wave[], int * icmplx, int * nwave);
}

namespace
{
  int constexpr max_frames = 16;  // MAX_FRAMES in lib/jtty/jtty_mod.f90

  // encode one <= 80 character segment; false if it won't fit one message
  bool encode_segment (QString const& text, bool final, QVector<int> * tones)
  {
    auto frame = Jtty::transmitFrame (text).toLatin1 ();
    if (frame.isEmpty ()) return false;
    tones->resize (Jtty::symbolsPerFrame * max_frames);
    int nsym {0};
    int frame_starts[max_frames] {};
    int const profile {static_cast<int> (Jtty::NativeExchangeProfile::None)};
    int const is_final {final ? 1 : 0};
    genjtty_profile_ (frame.data (), &profile, tones->data (), &nsym, frame_starts, &is_final,
                      static_cast<fortran_charlen_t> (Jtty::maxTransmitLength));
    if (nsym <= 0) return false;
    tones->resize (nsym);
    return true;
  }
}

namespace Jtty
{
  bool encodeMessage (QString const& text, QVector<int> * tones, QString * sent)
  {
    auto const prepared = prepareTransmitText (text.toUpper ());
    QVector<int> all;
    QStringList sent_lines;
    for (auto const& line : prepared.text.split (QLatin1Char {'\n'}))
      {
        QVector<QVector<int>> segments;
        QStringList texts;
        auto const encoded = encodeTransmitLine (line, [&] (QString const& segment) {
            QVector<int> segment_tones;
            if (!encode_segment (segment, false, &segment_tones)) return false;
            segments << segment_tones;
            texts << segment;
            return true;
          });
        if (!encoded) return false;
        if (segments.isEmpty ()) continue;
        // re-encode the line's last segment with the end-of-message bit
        if (!encode_segment (texts.last (), true, &segments.last ())) return false;
        for (auto const& segment : segments) all += segment;
        sent_lines << line.simplified ();
      }
    if (all.isEmpty ()) return false;
    *tones = all;
    if (sent) *sent = sent_lines.join (' ');
    return true;
  }

  void waveform (QVector<int> const& tones, int nsps, float fsample, float f0, float * out)
  {
    auto itone = tones;           // gen_jttywave_ takes non-const
    int nsym {itone.size ()};
    float bt {2.0f};
    int icmplx {0};
    int nwave {nsps * nsym};
    gen_jttywave_ (itone.data (), &nsym, &nsps, &bt, &fsample, &f0, out, out, &icmplx, &nwave);
  }
}
