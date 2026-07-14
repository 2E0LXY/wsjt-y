// -*- Mode: C++ -*-
#ifndef WIDEGRAPH_H_
#define WIDEGRAPH_H_

#include <QWidget>
#include <QScopedPointer>
#include <QDir>
#include <QHash>
#include <QVariant>
#include <QPair>
#include <QList>
#include "WFPalette.hpp"

#define MAX_SCREENSIZE 2048

namespace Ui {
  class WideGraph;
}

class QSettings;
class Configuration;
class QDockWidget;

class WideGraph : public QWidget
{
  Q_OBJECT

public:
  explicit WideGraph(QSettings *, QWidget *parent = 0);
  ~WideGraph ();

  // Hosting dock (set once, right after construction, by MainWindow). All
  // the existing show()/hide()/showNormal() call sites throughout
  // mainwindow.cpp keep working unchanged -- WideGraph mirrors its own
  // visibility onto the dock via showEvent/hideEvent below, so callers
  // don't need to know or care that this is now docked rather than a
  // separate top-level window.
  void setHostDock (QDockWidget * dock);

  // The lower control strip (bins/pixel, palette, gain/zero sliders etc,
  // shown via image 2's "Controls" checkbox) is relocated out of this
  // widget's own layout into a new spot in the main window -- above the
  // band-select/Auto Hop toolbar. MainWindow grabs this pointer once at
  // startup and re-parents it there; on_cbControls_toggled() still just
  // calls setVisible() on it, which works identically regardless of which
  // layout it physically lives in.
  QWidget * controlsWidget () const;

  // Appends an additional widget to the bottom of this widget's own
  // vertical layout (below the waterfall plot and Controls strip).
  // Used to host Band Select as a third row in the same dock as the
  // waterfall, since it's the only way to guarantee "waterfall, then
  // controls, then band select" stacking -- a separate toolbar or dock
  // for band select would render in a different area of the main
  // window regardless of add order.
  void appendBottomWidget (QWidget * w);

  void   dataSink2(float s[], float df3, int ihsym, int ndiskdata, float pdB);
  void   setRxFreq(int n);
  int    rxFreq();
  int    nStartFreq();
  int    Fmin();
  int    Fmax();
  int    fSpan();
  void   saveSettings();
  void   setFsample(int n);
  void   setPeriod(double trperiod, int nsps);
  void   setTxFreq(int n);
  void   setMode(QString mode);
  void   setSubMode(int n);
  bool   flatten();
  bool   useRef();
  void   setTol(int n);
  void   setSuperFox(bool b);
  void   setSuperHound(bool b);
  int    smoothYellow();
  void   setRxBand (QString const& band);
  void   setWSPRtransmitted();
  void   drawRed(int ia, int ib);
  void   setVHF(bool bVHF);
  void   setRedFile(QString fRed);
  void   setFST4_FreqRange(int fLow,int fHigh);
  void   setSingleDecode(bool b);
  void   setDiskUTC(int nutc);
  void   restartTotalPower();
  void   setDecodeLabels(QList<QPair<int,QString>> const& labels, bool show);

signals:
  void freezeDecode2(int n);
  void f11f12(int n);
  void setXIT2(int n);
  void setFreq3(int rxFreq, int txFreq);

public slots:
  void wideFreezeDecode(int n);
  void setFreq2(int rxFreq, int txFreq);
  void setDialFreq(double d);

protected:
  void keyPressEvent (QKeyEvent *e) override;
  void closeEvent (QCloseEvent *) override;
  void showEvent (QShowEvent *) override;
  void hideEvent (QHideEvent *) override;

private slots:
  void on_waterfallAvgSpinBox_valueChanged(int arg1);
  void on_bppSpinBox_valueChanged(int arg1);
  void on_spec2dComboBox_currentIndexChanged(int);
  void on_fSplitSpinBox_valueChanged(int n);
  void on_fStartSpinBox_valueChanged(int n);
  void on_paletteComboBox_activated(const QString &palette);
  void on_cbFlatten_toggled(bool b);
  void on_cbRef_toggled(bool b);
  void on_cbControls_toggled(bool b);
  // Z
  void on_cbBars_toggled(bool b);
  void on_cbClear_toggled(bool b);
  void on_pbClear_clicked();

  void on_adjust_palette_push_button_clicked (bool);
  void on_gainSlider_valueChanged(int value);
  void on_zeroSlider_valueChanged(int value);
  void on_gain2dSlider_valueChanged(int value);
  void on_zero2dSlider_valueChanged(int value);
  void on_smoSpinBox_valueChanged(int n);  
  void on_sbPercent2dPlot_valueChanged(int n);

private:
  void readPalette ();
  void setRxRange ();
  void replot();

  QScopedPointer<Ui::WideGraph> ui;
  QDockWidget * m_hostDock = nullptr;

  QSettings * m_settings;
  QDir m_palettes_path;
  WFPalette m_userPalette;
  QHash<QString, QVariant> m_fMinPerBand;

  double m_tr0;
  double m_TRperiod;

  qint32 m_waterfallAvg;
  qint32 m_nsps;
  qint32 m_fMax;
  qint32 m_nSubMode;
  qint32 m_nsmo;
  qint32 m_Percent2DScreen;
  qint32 m_jz=MAX_SCREENSIZE;
  qint32 m_n;
  // Z
  bool	 m_bars;
  bool	 m_clear;
  bool   m_bFlatten;
  bool   m_bRef;
  bool   m_bHaveTransmitted;    //Set true at end of a WSPR or FT4 transmission

  QString m_rxBand;
  QString m_mode;
  QString m_waterfallPalette;  
  float   m_swide[MAX_SCREENSIZE];
  QString m_user_defined;
};

#endif // WIDEGRAPH_H
