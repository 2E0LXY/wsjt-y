#include "JttyTxStream.hpp"

#include "Audio/soundout.h"
#include "Logger.hpp"

#include "moc_JttyTxStream.cpp"

namespace
{
  // Trailing silence (samples) served past the last real sample before we
  // trust the queue has physically played out, when the device buffer size
  // is unknown. Only lengthens the PTT tail after the final message, never
  // the gap between chained messages.
  constexpr qint64 DEFAULT_DRAIN_GUARD = 9600;   // 200 ms at 48 kHz
  constexpr qint64 DRAIN_GUARD_MARGIN  = 4800;   // 100 ms safety margin
}

JttyTxStream::JttyTxStream (JttyPcmFifo& fifo, QObject * parent)
  : AudioDevice {parent}
  , m_fifo {fifo}
  , m_drainGuard {DEFAULT_DRAIN_GUARD}
  , m_drainTimer {new QTimer {this}}
  , m_active {false}
{
  m_drainTimer->setInterval (25);
  connect (m_drainTimer, &QTimer::timeout, this, &JttyTxStream::pollDrain);
}

void JttyTxStream::start (Channel channel, SoundOutput * stream)
{
  if (m_active) return;
  m_fifo.applyPendingReset ();
  initialize (QIODevice::ReadOnly, channel);
  m_active = true;
  m_stream = stream;
  if (m_stream)
    {
      m_stream->restart (this);
      // Size the drain guard to the actual device buffer (valid after
      // restart) plus a margin.
      qint64 const bufFrames = bytesPerFrame ()
        ? qint64 (m_stream->bufferSize ()) / qint64 (bytesPerFrame ())
        : 0;
      m_drainGuard.store (qMax (bufFrames, DEFAULT_DRAIN_GUARD) + DRAIN_GUARD_MARGIN,
                          std::memory_order_release);
    }
  if (!m_drainTimer->isActive ()) m_drainTimer->start ();
}

void JttyTxStream::stop ()
{
  if (m_stream)
    {
      m_stream->stop ();
    }
  m_active = false;
  m_drainTimer->stop ();
  AudioDevice::close ();
  // Queued PCM is deliberately not dropped here: a GUI stop for the previous
  // session can cross with the next enqueue; JttyPcmFifo::clear() is the
  // explicit abort path.
}

qint64 JttyTxStream::readData (char * data, qint64 maxSize)
{
  if (maxSize == 0) return 0;
  Q_ASSERT (!(maxSize % qint64 (bytesPerFrame ()))); // no torn frames

  qint64 const numFrames {maxSize / qint64 (bytesPerFrame ())};
  qint16 * samples {reinterpret_cast<qint16 *> (data)};

  qint64 const drainGuard = m_drainGuard.load (std::memory_order_acquire);
  for (qint64 frame = 0; frame < numFrames; ++frame)
    {
      samples = load (m_fifo.pullSample (drainGuard), samples);
    }

  return numFrames * qint64 (bytesPerFrame ());
}

void JttyTxStream::pollDrain ()
{
  auto const drain = m_fifo.takeDrainReady ();
  if (drain.ready)
    {
      Q_EMIT drained (drain.epoch, drain.totalAtDrain);
    }
}
