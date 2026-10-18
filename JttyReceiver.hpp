#ifndef JTTY_RECEIVER_HPP
#define JTTY_RECEIVER_HPP

#include <memory>
#include <vector>

#include <QObject>
#include <QByteArray>
#include <QTimer>
#include <QVector>

#include "JttyDecoder.hpp"

//
// Continuous JTTY reception
//
// Buffers the 12 kHz audio published by the Detector while JTTY is
// the current mode and feeds it to the JTTY decoder from lib/jtty.
// Lives on the GUI thread and decodes in short slices so the Fortran
// decoder, which shares DSP scratch and FFTW plans with the rest of
// lib, is only ever entered from one thread.
//
class JttyReceiver final
  : public QObject
{
  Q_OBJECT

public:
  explicit JttyReceiver (QObject * parent = nullptr);
  ~JttyReceiver ();

  bool active () const {return active_;}

  // search band and the operator's Rx frequency and tolerance, in Hz
  void setFrequencies (int low, int high, float center, float tolerance);

  // replace incoming audio with silence, e.g. while we transmit, so
  // sample positions stay continuous but our own signal isn't decoded
  void setMuted (bool muted) {muted_ = muted;}

  Q_SLOT void start ();
  Q_SLOT void stop ();
  Q_SLOT void addSamples (QByteArray pcm);

  Q_SIGNAL void updated (QVector<Jtty::ReceiveUpdate> const&);

private:
  void pump ();
  void trim ();

  std::unique_ptr<Jtty::Decoder> decoder_;
  std::vector<qint16> buffer_;  // samples [base_, base_ + buffer_.size ())
  qint64 base_;
  qint64 session_;
  bool active_;
  bool muted_;
  int low_;
  int high_;
  float center_;
  float tolerance_;
  QTimer pump_timer_;
};

#endif
