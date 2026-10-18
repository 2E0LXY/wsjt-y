// JTTY loopback check: encode text with the transmit path's encoder,
// synthesize it at 12 kHz with noise, stream it into JttyReceiver in
// Detector-sized blocks and require the decoded text to match.
//
// usage: jtty_loopback [snr_dB_in_2500Hz] [message] [noise_seed]

#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

#include <QByteArray>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QMap>
#include <QStringList>
#include <QString>
#include <QTimer>

#include "JttyEncoder.hpp"
#include "JttyReceiver.hpp"

int main (int argc, char * argv[])
{
  QCoreApplication app {argc, argv};
  double const snr_db = argc > 1 ? std::atof (argv[1]) : -10.0;
  QString const message = argc > 2 ? QString::fromLocal8Bit (argv[2]) : QStringLiteral ("CQ 2E0LXY IO93");

  QVector<int> tones;
  QString sent;
  if (!Jtty::encodeMessage (message, &tones, &sent))
    {
      std::fprintf (stderr, "encode failed\n");
      return 2;
    }
  int const nsps = Jtty::samplesPerSymbol12k;
  float const f0 = 1500.f;
  std::vector<float> wave (tones.size () * nsps);
  Jtty::waveform (tones, nsps, 12000.f, f0, wave.data ());

  // 2 s of noise either side, signal at the requested SNR in 2500 Hz
  int const lead = 2 * 12000;
  std::vector<qint16> pcm (lead + wave.size () + 6 * 12000);
  std::mt19937 rng {argc > 3 ? static_cast<unsigned> (std::atoi (argv[3])) : 1234u};
  std::normal_distribution<float> gauss {0.f, 1.f};
  float const noise_rms = 1000.f;
  // noise power in 2500 Hz of 6000 Hz bandwidth; sine power is amp^2/2
  float const amp = std::sqrt (2.f * noise_rms * noise_rms * 2500.f / 6000.f * std::pow (10.f, snr_db / 10.f));
  for (std::size_t i = 0; i < pcm.size (); ++i)
    {
      float x = noise_rms * gauss (rng);
      if (i >= static_cast<std::size_t> (lead) && i - lead < wave.size ()) x += amp * wave[i - lead];
      pcm[i] = static_cast<qint16> (std::max (-32767.f, std::min (32767.f, x)));
    }

  JttyReceiver receiver;
  receiver.setFrequencies (200, 3000, f0, 50.f);
  QMap<qint64, QString> messages;   // by decoder message id, in arrival order
  QMap<qint64, bool> finished;
  QObject::connect (&receiver, &JttyReceiver::updated, [&] (QVector<Jtty::ReceiveUpdate> const& updates) {
      for (auto const& update : updates)
        {
          std::printf ("  update id=%lld f=%.1f snr=%d terminal=%d \"%s\"\n",
                       static_cast<long long> (update.messageId), update.frequency, update.snr,
                       static_cast<int> (update.terminal), update.text.toLatin1 ().constData ());
          messages[update.messageId] = update.text;
          finished[update.messageId] = update.terminal == Jtty::ReceiveTerminal::Complete;
        }
    });
  receiver.start ();

  // feed at roughly real time x8 in the Detector's 3456-sample blocks
  std::size_t position {0};
  QTimer feeder;
  feeder.setInterval (36);
  QObject::connect (&feeder, &QTimer::timeout, [&] {
      if (position >= pcm.size ()) return;
      auto const count = std::min<std::size_t> (3456, pcm.size () - position);
      receiver.addSamples (QByteArray {reinterpret_cast<char const *> (pcm.data () + position),
                                       static_cast<int> (count * sizeof (qint16))});
      position += count;
    });
  feeder.start ();

  QElapsedTimer clock;
  clock.start ();
  int drained {0};
  QTimer watchdog;
  watchdog.setInterval (100);
  QObject::connect (&watchdog, &QTimer::timeout, [&] {
      // all audio fed and the decoder has had time to drain it
      if (position >= pcm.size () && (clock.elapsed () > 120000 || drained++ > 30)) app.quit ();
    });
  watchdog.start ();
  app.exec ();
  receiver.stop ();

  QStringList parts;
  bool complete {!messages.isEmpty ()};
  for (auto id : messages.keys ())
    {
      parts << messages[id];
      complete = complete && finished[id];
    }
  auto const decoded = parts.join (' ').simplified ();
  std::printf ("sent:    \"%s\" (%d symbols, %d frames)\ndecoded: \"%s\" complete=%d\n",
               sent.toLatin1 ().constData (), tones.size (), tones.size () / Jtty::symbolsPerFrame,
               decoded.toLatin1 ().constData (), complete);
  bool const ok {complete && decoded == sent.simplified ()};
  std::printf ("%s\n", ok ? "PASS" : "FAIL");
  return ok ? 0 : 1;
}
