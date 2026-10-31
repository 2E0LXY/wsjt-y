// JTTY receive-only integration (WSJT-X 3.2.0-rc1 port, branch feature/jtty).
//
// UNRELEASED. See lib/jtty/NOTICE-UPSTREAM.md. Upstream's equivalent
// (mainwindow_jtty.cpp) depends on main-window refactors WSJT-Y does not
// have, so this is a deliberately small re-implementation of the receive
// path only: mode entry, the call into the imported Fortran decoder, and
// plain-text display. No transmit, MMTTY/N1MM link or QSO helpers yet.

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "widegraph.h"
#include "fastgraph.h"
#include "Modulator/Modulator.hpp"
#include "Detector/Detector.hpp"
#include "commons.h"

#include <algorithm>
#include <array>
#include <QTextEdit>
#include <QDateTime>
#include <QTextCursor>
#include <QStringList>

extern dec_data_t dec_data;

namespace
{
  constexpr int jttyMaxUpdates = 30;
  constexpr int jttyMessageSize = 80;      // must match jtty_get_updates (rjtty_sub.f90)
  constexpr int jttyUpdateBufferSize = jttyMaxUpdates * jttyMessageSize;
  constexpr int jttyNsps = 384;            // 31.25 baud at 12 kHz
}

extern "C"
{
  void rjtty_sub_ (short int d2[], int * k, int * nsps, int * nfa, int * nfb,
                   float * f0, float * ftol);
  void jtty_get_updates_ (char text_blocks[], qint64 message_ids[],
                          float frequencies[], float start_tsync[], bool eom[],
                          int * count, fortran_charlen_t);
  void jtty_release_fft_resources ();
}

void MainWindow::on_actionJTTY_triggered ()
{
  m_mode = "JTTY";
  m_bFast9 = false;
  m_bFastMode = false;
  WSPR_config (false);
  m_nsps = 6912;
  m_FFTSize = m_nsps / 2;
  if (m_tci_audio) Q_EMIT m_config.transceiver_blocksize (m_FFTSize);
  else Q_EMIT FFTSize (m_FFTSize);
  m_hsymStop = 620;
  setup_status_bar (false);
  m_toneSpacing = 0.0;
  ui->actionJTTY->setChecked (true);
  m_wideGraph->setMode (m_mode);
  VHF_features_enabled (false);
  ui->cbAutoSeq->setChecked (false);
  m_TRperiod = 180.0;                  // continuous mode; nonzero period needed by the plotter
  m_fastGraph->hide ();
  m_wideGraph->show ();
  m_wideGraph->setPeriod (m_TRperiod, m_nsps);
  if (m_tci_audio && ui->bandComboBox->currentText () != "OOB")
    Q_EMIT m_config.transceiver_period (m_TRperiod);
  if (!m_tci_audio) {
    m_modulator->setTRPeriod (m_TRperiod);
    m_detector->setTRPeriod (m_TRperiod);
  }
  ui->lh_decodes_title_label->setText (tr ("All Decodes"));
  ui->rh_decodes_title_label->setText (tr ("QSO Frequency"));
  ui->lh_decodes_headings_label->setText ("  UTC  Freq  " + tr ("Message"));
  ui->rh_decodes_headings_label->setText ("  UTC  Freq  " + tr ("Message"));
//                         01234567890123456789012345678901234567
  displayWidgets (nWidgets ("11101000010011100001000010011000100000"));

  // Receive only for now: keep every transmit path closed.
  ui->autoButton->setEnabled (false);
  ui->tuneButton->setEnabled (false);
  m_jttyTxLocked = true;

  m_jttyLines.clear ();
  m_jttyLastK = -1;
  switch_mode (Modes::JTTY);
}

bool MainWindow::jtty_decode (int k)
{
  // A non-advancing sample position starts a new display session.
  bool const newSession = (k <= m_jttyLastK);
  m_jttyLastK = k;
  if (newSession) {
    m_jttyLines.clear ();
    ui->decodedTextBrowser->clear ();
    ui->decodedTextBrowser2->clear ();
  }

  int nsps = jttyNsps;
  float f0 = ui->RxFreqSpinBox->value ();
  float ftol = ui->sbFtol->value ();
  int nfa = m_wideGraph->nStartFreq ();
  int nfb = m_wideGraph->Fmax ();
  rjtty_sub_ (dec_data.d2, &k, &nsps, &nfa, &nfb, &f0, &ftol);

  bool changed = false;
  bool anyEom = false;
  int updateCount = 0;
  do {
    std::array<char, jttyUpdateBufferSize> textBlocks {};
    std::array<qint64, jttyMaxUpdates> messageIds {};
    std::array<float, jttyMaxUpdates> frequencies {};
    std::array<float, jttyMaxUpdates> sequenceStarts {};
    std::array<bool, jttyMaxUpdates> complete {};
    jtty_get_updates_ (textBlocks.data (), messageIds.data (), frequencies.data (),
                       sequenceStarts.data (), complete.data (), &updateCount,
                       static_cast<fortran_charlen_t> (jttyUpdateBufferSize));
    for (int i = 0; i < updateCount; ++i) {
      if (messageIds[i] <= 0) continue;
      QString const text = QString::fromLatin1 (
        textBlocks.data () + i * jttyMessageSize, jttyMessageSize).trimmed ();
      auto & line = m_jttyLines[messageIds[i]];
      if (!line.startUtc.isValid ()) {
        double const elapsed = qMax (0.0, double (k) / 12000.0 - double (sequenceStarts[i]));
        line.startUtc = QDateTime::currentDateTimeUtc ().addMSecs (-qRound64 (1000.0 * elapsed));
        changed = true;
      }
      if (line.text != text || line.frequency != frequencies[i]
          || line.complete != complete[i]) {
        line.text = text;
        line.frequency = frequencies[i];
        line.complete = complete[i];
        changed = true;
      }
      if (complete[i]) anyEom = true;
    }
  } while (updateCount == jttyMaxUpdates);

  if (changed) renderJttyLines (f0, ftol);
  return anyEom;
}

void MainWindow::renderJttyLines (float f0, float ftol)
{
  QStringList all, qso;
  for (auto it = m_jttyLines.cbegin (); it != m_jttyLines.cend (); ++it) {
    auto const & l = it.value ();
    if (l.text.isEmpty ()) continue;
    QString const row = QStringLiteral ("%1  %2  %3")
      .arg (l.startUtc.toString (QStringLiteral ("hhmmss")))
      .arg (qRound (l.frequency), 4)
      .arg (l.text);
    all << row;
    if (qAbs (l.frequency - f0) <= ftol) qso << row;
  }
  auto show = [] (QTextEdit * view, QStringList const & rows) {
    view->setPlainText (rows.join (QLatin1Char ('\n')));
    view->moveCursor (QTextCursor::End);
  };
  show (ui->decodedTextBrowser, all);
  show (ui->decodedTextBrowser2, qso);
}

void MainWindow::jttyShutdown ()
{
  jtty_release_fft_resources ();
}
