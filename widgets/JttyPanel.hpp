#ifndef JTTY_PANEL_HPP
#define JTTY_PANEL_HPP

#include <QHash>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include "JttyDecoder.hpp"

class QLabel;
class QLineEdit;
class QPushButton;
class QSettings;
class QSpinBox;
class QTextEdit;

//
// JTTY operating panel
//
// Keyboard-to-keyboard window for JTTY: a receive pane that grows
// messages in place as their frames decode, a transmit line, and
// F1-F8 macro keys using the same templates and settings keys as
// WSJT-X 3.3 so templates carry over.
//
class JttyPanel final
  : public QWidget
{
  Q_OBJECT

public:
  explicit JttyPanel (QSettings * settings, QWidget * parent = nullptr);

  void setMyCall (QString const& call) {my_call_ = call;}
  void setMyGrid (QString const& grid) {my_grid_ = grid;}
  void setRxWindow (float center, float tolerance);
  void setTransmitting (bool transmitting);
  void clearReceived ();

  Q_SLOT void applyUpdates (QVector<Jtty::ReceiveUpdate> const&);
  Q_SLOT void showTransmitted (float frequency, QString const& text);

  // operator asked to send this (already macro expanded) text
  Q_SIGNAL void sendRequested (QString const& text) const;
  Q_SIGNAL void stopRequested () const;

protected:
  bool eventFilter (QObject *, QEvent *) override;

private:
  struct Line
  {
    qint64 id;
    float frequency;
    int snr;
    QString text;
    bool complete;
    QString time;
  };

  QString expand (QString const& macroTemplate) const;
  QString macroTemplate (int functionKey) const;
  void editMacro (int functionKey);
  void refreshMacroTips ();
  void sendFunctionKey (int functionKey);
  void send ();
  void render ();
  void pickCall (QString const& lineText);

  QSettings * settings_;
  QTextEdit * received_;
  QLineEdit * entry_;
  QLineEdit * his_call_;
  QLineEdit * exchange_;
  QSpinBox * serial_;
  QPushButton * send_button_;
  QPushButton * stop_button_;
  QLabel * status_;
  QVector<QPushButton *> macro_buttons_;
  QVector<Line> lines_;
  QHash<qint64, int> line_index_;
  QString my_call_;
  QString my_grid_;
  float rx_center_;
  float rx_tolerance_;
  int last_snr_;
  bool transmitting_;
};

#endif
