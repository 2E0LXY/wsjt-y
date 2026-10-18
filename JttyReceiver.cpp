#include "JttyReceiver.hpp"

#include <algorithm>

#include <QElapsedTimer>

#include "JttyReceiveTiming.hpp"

#include "moc_JttyReceiver.cpp"

namespace
{
  // upstream's live decode cadence: one search step is a quarter frame
  qint64 constexpr step_samples = Jtty::receiveFrameSamples / 4;
  qint64 constexpr window_samples = Jtty::receiveFrameSamples + step_samples;
  int constexpr pump_budget_ms = 12;
  int constexpr pump_interval_ms = 50;
  // never hold more than a minute of audio, even if decoding stalls
  qint64 constexpr max_buffered_samples = 60 * 12000;
}

JttyReceiver::JttyReceiver (QObject * parent)
  : QObject {parent}
  , base_ {0}
  , session_ {0}
  , active_ {false}
  , muted_ {false}
  , low_ {200}
  , high_ {3000}
  , center_ {1500.f}
  , tolerance_ {50.f}
{
  pump_timer_.setInterval (pump_interval_ms);
  connect (&pump_timer_, &QTimer::timeout, this, &JttyReceiver::pump);
}

JttyReceiver::~JttyReceiver ()
{
  stop ();
}

void JttyReceiver::setFrequencies (int low, int high, float center, float tolerance)
{
  low_ = low;
  high_ = std::max (low + 100, high);
  center_ = center;
  tolerance_ = tolerance;
}

void JttyReceiver::start ()
{
  stop ();
  decoder_.reset (new Jtty::Decoder);
  if (!decoder_->valid ())
    {
      decoder_.reset ();
      return;
    }
  buffer_.clear ();
  base_ = 0;
  decoder_->begin (++session_, 0, 0);
  active_ = true;
  pump_timer_.start ();
}

void JttyReceiver::stop ()
{
  pump_timer_.stop ();
  if (decoder_)
    {
      decoder_->end ();
      auto const& updates = decoder_->takeUpdates ();
      if (!updates.isEmpty ()) Q_EMIT updated (updates);
      decoder_.reset ();
    }
  buffer_.clear ();
  active_ = false;
}

void JttyReceiver::addSamples (QByteArray pcm)
{
  if (!active_) return;
  auto const count = pcm.size () / static_cast<int> (sizeof (qint16));
  auto const * samples = reinterpret_cast<qint16 const *> (pcm.constData ());
  if (muted_)
    {
      buffer_.insert (buffer_.end (), count, 0);
    }
  else
    {
      buffer_.insert (buffer_.end (), samples, samples + count);
    }
  if (static_cast<qint64> (buffer_.size ()) > max_buffered_samples)
    {
      // decoding fell too far behind; start a fresh session rather
      // than hand the decoder a gap it cannot account for
      start ();
    }
}

void JttyReceiver::pump ()
{
  if (!active_ || !decoder_) return;
  QElapsedTimer budget;
  budget.start ();
  auto const end = base_ + static_cast<qint64> (buffer_.size ());
  while (budget.elapsed () < pump_budget_ms)
    {
      auto const search = decoder_->nextSearchSample ();
      auto const stop_sample = search + window_samples;
      if (stop_sample > end) break;
      auto const first = std::max (decoder_->nextRequiredSample (), base_);
      auto const offset = first - base_;
      auto const count = static_cast<int> (stop_sample - first);
      auto const processed = decoder_->process (buffer_.data () + offset, count, first, stop_sample, 1,
                                                low_, high_, center_, tolerance_);
      auto const& updates = decoder_->takeUpdates ();
      if (!updates.isEmpty ()) Q_EMIT updated (updates);
      if (processed < 0)
        {
          // the decoder wanted audio we no longer hold
          start ();
          return;
        }
      if (processed == 0) break;
    }
  trim ();
}

void JttyReceiver::trim ()
{
  auto const keep_from = decoder_->nextRequiredSample ();
  auto const drop = std::min<qint64> (keep_from - base_, static_cast<qint64> (buffer_.size ()));
  if (drop > 0)
    {
      buffer_.erase (buffer_.begin (), buffer_.begin () + drop);
      base_ += drop;
    }
}
