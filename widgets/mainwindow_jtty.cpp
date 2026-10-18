// JTTY keyboard-to-keyboard mode: MainWindow glue
//
// The JTTY codec, encoder and decoder are WSJT-X 3.3's lib/jtty,
// unchanged, so WSJT-Y interoperates with WSJT-X. This file only
// adapts them to WSJT-Y's audio path:
//
//   Rx  Detector::continuousSamples -> JttyReceiver -> JttyPanel
//   Tx  JttyPanel -> genjtty_profile_/gen_jttywave_ -> foxcom_.wave
//       -> Modulator, keyed by its own small PTT sequencer because
//       JTTY transmissions start whenever the operator sends rather
//       than in T/R slots. guiUpdate()'s slot logic stays idle in
//       JTTY because Enable Tx is off and g_iptt is never raised.

#include "widgets/mainwindow.h"
#include "ui_mainwindow.h"

#include <QDateTime>
#include <QDockWidget>
#include <QTimer>
#include <QVector>

#include "commons.h"
#include "Detector/Detector.hpp"
#include "Modulator/Modulator.hpp"
#include "JttyEncoder.hpp"
#include "JttyReceiver.hpp"
#include "widgets/JttyPanel.hpp"
#include "widgets/fastgraph.h"
#include "widgets/widegraph.h"

extern int volatile itone[MAX_NUM_SYMBOLS];  // defined in mainwindow.cpp
extern int volatile icw[NUM_CW_SYMBOLS];

namespace
{
  int constexpr tx_samples_per_symbol = 4 * Jtty::samplesPerSymbol12k; // Modulator runs at 48 kHz
  // margin after the expected end before the sequencer gives up waiting
  qint64 constexpr tx_overrun_ms = 5000;
}

void MainWindow::jttySetup ()
{
  m_jttyReceiver = new JttyReceiver {this};
  m_jttyPanel = new JttyPanel {m_settings};
  m_jttyDock = new QDockWidget {tr ("JTTY"), this};
  m_jttyDock->setObjectName ("JttyDock");
  m_jttyDock->setWidget (m_jttyPanel);
  m_jttyDock->setFeatures (QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
  addDockWidget (Qt::BottomDockWidgetArea, m_jttyDock);
  m_jttyDock->hide ();

  m_jttyTxTimer = new QTimer {this};
  m_jttyTxTimer->setInterval (50);
  connect (m_jttyTxTimer, &QTimer::timeout, this, &MainWindow::jttyTxTick);

  // the Detector runs on the audio thread, so this connection queues
  connect (m_detector, &Detector::continuousSamples, m_jttyReceiver, &JttyReceiver::addSamples);
  connect (m_jttyReceiver, &JttyReceiver::updated, m_jttyPanel, &JttyPanel::applyUpdates);
  connect (m_jttyPanel, &JttyPanel::sendRequested, this, &MainWindow::jttyTransmit);
  connect (m_jttyPanel, &JttyPanel::stopRequested, this, &MainWindow::jttyStopTransmit);
}

void MainWindow::on_actionJTTY_triggered ()
{
  monitor (false);
  m_mode = "JTTY";
  if (m_specOp == SpecOp::HOUND)
    {
      m_config.setSpecial_None ();
      m_specOp = m_config.special_op_id ();
    }
  if (ui->autoButton->isChecked ()) ui->autoButton->click ();
  // JTTY isn't slotted; the period only paces guiUpdate's clock and
  // the Detector's dec_data buffer, which JTTY doesn't use
  m_TRperiod = 60.0;
  m_bFast9 = false;
  m_bFastMode = false;
  WSPR_config (false);
  switch_mode (Modes::JTTY);
  m_nsps = 6912;                  // waterfall resolution as FT4 (JTTY is 127 Hz wide)
  m_FFTSize = m_nsps / 2;
  if (m_tci_audio) Q_EMIT m_config.transceiver_blocksize (m_FFTSize);
  else Q_EMIT FFTSize (m_FFTSize);
  setup_status_bar (false);
  m_toneSpacing = 12000.0 / Jtty::samplesPerSymbol12k;
  ui->actionJTTY->setChecked (true);
  m_wideGraph->setMode (m_mode);
  m_wideGraph->setPeriod (m_TRperiod, m_nsps);
  if (!m_tci_audio)
    {
      m_modulator->setTRPeriod (m_TRperiod);
      m_detector->setTRPeriod (m_TRperiod);
    }
  m_fastGraph->hide ();
  m_wideGraph->show ();
  displayWidgets (nWidgets ("11101000010011100001000000011000100000"));
  // slot-based transmit controls do nothing in JTTY; the panel sends
  ui->autoButton->setEnabled (false);

  m_jttyPanel->setMyCall (m_config.my_callsign ());
  m_jttyPanel->setMyGrid (m_config.my_grid ());
  m_jttyDock->show ();
  m_jttyDock->raise ();
  m_detector->setContinuousTap (true);
  m_jttyReceiver->start ();
  jttyUpdateRxWindow ();
  statusChanged ();
  monitor (true);
}

void MainWindow::jttyLeave ()
{
  if (!m_jttyReceiver) return;
  // restoreState () may have shown the dock before any mode was set
  m_jttyDock->hide ();
  if (!m_jttyReceiver->active ()) return;
  jttyStopTransmit ();
  m_detector->setContinuousTap (false);
  m_jttyReceiver->stop ();
  ui->autoButton->setEnabled (true);
}

void MainWindow::jttyUpdateRxWindow ()
{
  if (!m_jttyReceiver || !m_jttyReceiver->active ()) return;
  auto const center = static_cast<float> (ui->RxFreqSpinBox->value ());
  auto const tolerance = static_cast<float> (ui->sbFtol->value ());
  m_jttyReceiver->setFrequencies (m_wideGraph->nStartFreq (), m_wideGraph->Fmax (), center, tolerance);
  m_jttyPanel->setRxWindow (center, tolerance);
}

void MainWindow::jttyTransmit (QString const& text)
{
  if (m_mode != "JTTY" || m_jttyTransmitting) return;
  if (m_tci_audio)
    {
      showStatusMessage (tr ("JTTY transmit over TCI audio is not supported yet"));
      return;
    }

  QVector<int> tones;
  QString sent;
  if (!Jtty::encodeMessage (text, &tones, &sent))
    {
      showStatusMessage (tr ("JTTY text not sent: a line does not fit in one message once encoded; shorten it"));
      return;
    }
  auto const nsym = tones.size ();
  auto const nwave = tx_samples_per_symbol * nsym;
  if (nwave > static_cast<int> (sizeof (foxcom_.wave) / sizeof (foxcom_.wave[0])))
    {
      showStatusMessage (tr ("JTTY text not sent: message too long"));
      return;
    }
  auto const tx_frequency = ui->TxFreqSpinBox->value ();
  setXIT (tx_frequency);          // before the waveform, which is offset by m_XIT
  Jtty::waveform (tones, tx_samples_per_symbol, 48000.0f, tx_frequency - m_XIT, foxcom_.wave);

  itone[0] = 0;                   // < 100 selects the precomputed wave
  icw[0] = 0;                     // no CW ID after JTTY
  m_jttyTxSymbols = nsym;
  m_jttyTransmitting = true;
  m_jttyAudioStarted = false;
  m_jttyReceiver->setMuted (true);
  m_jttyPanel->setTransmitting (true);
  m_jttyPanel->showTransmitted (tx_frequency, sent);
  write_all ("Tx", sent);
  tx_status_label.setStyleSheet ("QLabel{color: #000000; background-color: #ff0000}");
  tx_status_label.setText (tr ("Tx: JTTY"));

  m_config.transceiver_ptt (true);
  m_jttyTxStartMs = QDateTime::currentMSecsSinceEpoch ();
  m_jttyTxTimer->start ();
}

void MainWindow::jttyTxTick ()
{
  if (!m_jttyTransmitting)
    {
      m_jttyTxTimer->stop ();
      return;
    }
  auto const elapsed = QDateTime::currentMSecsSinceEpoch () - m_jttyTxStartMs;
  qint64 const ptt_delay_ms = qRound64 (1000.0 * m_config.txDelay ());
  if (!m_jttyAudioStarted)
    {
      if (elapsed < ptt_delay_ms) return;
      Q_EMIT sendMessage ("JTTY", static_cast<unsigned> (m_jttyTxSymbols), Jtty::samplesPerSymbol12k,
                          ui->TxFreqSpinBox->value () - m_XIT, -1.0, m_soundOutput,
                          m_config.audio_output_channel (), false, false, 99.0, 60);
      m_jttyAudioStarted = true;
      return;
    }
  // Modulator::start is queued to the audio thread, so ignore its
  // idle state until it has certainly started
  qint64 const expected_ms = qint64 {m_jttyTxSymbols} * 32;  // 31.25 baud
  if (elapsed < ptt_delay_ms + 500) return;
  if (!m_modulator->isActive () || elapsed > ptt_delay_ms + expected_ms + tx_overrun_ms)
    {
      jttyStopTransmit ();
    }
}

void MainWindow::jttyStopTransmit ()
{
  if (!m_jttyTransmitting) return;
  m_jttyTxTimer->stop ();
  bool const aborted {m_modulator->isActive ()};
  Q_EMIT endTransmitMessage (aborted);
  m_jttyTransmitting = false;
  m_jttyPanel->setTransmitting (false);
  tx_status_label.setStyleSheet ("");
  tx_status_label.setText ("");
  // same end-of-transmission delay as stopTx () before dropping PTT
  QTimer::singleShot (200, this, [this] {
      if (m_jttyTransmitting) return;
      m_config.transceiver_ptt (false);
      if (m_jttyReceiver) m_jttyReceiver->setMuted (false);
    });
}
