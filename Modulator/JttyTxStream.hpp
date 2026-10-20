#ifndef JTTY_TX_STREAM_HPP__
#define JTTY_TX_STREAM_HPP__

// JTTY continuous transmit source (adapted from WSJT-X 3.2.0-rc1,
// Modulator/JttyTxStream). Upstream couples this to its rewritten TX
// request/evidence audio layer (TxAudioQueue/TxRequest/TxPlaybackEvidence),
// which WSJT-Y does not have and which would change the transmit path for
// every existing mode. This version drains a JttyPcmFifo directly into the
// existing SoundOutput, leaving the FT8/FT4/etc. path untouched.

#include <atomic>
#include <QPointer>
#include <QTimer>

#include "Audio/AudioDevice.hpp"
#include "JttyPcmFifo.hpp"

class SoundOutput;

class JttyTxStream
  : public AudioDevice
{
  Q_OBJECT;

public:
  explicit JttyTxStream (JttyPcmFifo& fifo, QObject * parent = nullptr);

  bool isActive () const {return m_active;}

  Q_SLOT void start (Channel channel, SoundOutput * stream);
  Q_SLOT void stop ();

  // emitted once all queued real samples have physically played out
  Q_SIGNAL void drained (qint64 epoch, qint64 totalAtDrain);

protected:
  qint64 readData (char * data, qint64 maxSize) override;
  qint64 writeData (char const * /* data */, qint64 /* maxSize */) override
  {
    return -1;                  // we don't consume data
  }

private:
  Q_SLOT void pollDrain ();

  JttyPcmFifo& m_fifo;
  std::atomic<qint64> m_drainGuard;
  QPointer<SoundOutput> m_stream;
  QTimer * m_drainTimer;
  std::atomic<bool> m_active;
};

#endif
