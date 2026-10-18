#ifndef JTTY_ENCODER_HPP
#define JTTY_ENCODER_HPP

#include <QString>
#include <QVector>

namespace Jtty
{
  // Encode operator text for transmission: lines are split into 80
  // character messages at spaces as WSJT-X 3.3 does, and each message
  // ends with the end-of-message bit so none overflows the receiver.
  // Returns false, with tones untouched, if a line can't be encoded.
  // sent receives the normalized text actually encoded.
  bool encodeMessage (QString const& text, QVector<int> * tones, QString * sent);

  // 4-GFSK waveform for tones, nsps samples per symbol at fsample Hz
  // with f0 as the lowest tone; out must hold tones.size () * nsps
  void waveform (QVector<int> const& tones, int nsps, float fsample, float f0, float * out);

  int constexpr symbolsPerFrame = 59;
  int constexpr samplesPerSymbol12k = 384;   // 31.25 baud at 12 kHz
}

#endif
