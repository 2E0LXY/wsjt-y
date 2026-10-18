#include "JttyPanel.hpp"

#include <cmath>

#include <QDateTime>
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSettings>
#include <QSpinBox>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>
#include <QVBoxLayout>

#include "widgets/JttyMessages.hpp"

#include "moc_JttyPanel.cpp"

namespace
{
  int constexpr max_lines = 500;
  QString const settings_group {"JTTY"};
}

JttyPanel::JttyPanel (QSettings * settings, QWidget * parent)
  : QWidget {parent}
  , settings_ {settings}
  , received_ {new QTextEdit}
  , entry_ {new QLineEdit}
  , his_call_ {new QLineEdit}
  , exchange_ {new QLineEdit}
  , serial_ {new QSpinBox}
  , send_button_ {new QPushButton {tr ("&Send")}}
  , stop_button_ {new QPushButton {tr ("S&top Tx")}}
  , status_ {new QLabel}
  , rx_center_ {1500.f}
  , rx_tolerance_ {50.f}
  , last_snr_ {-10}
  , transmitting_ {false}
{
  received_->setReadOnly (true);
  received_->setLineWrapMode (QTextEdit::NoWrap);
  QFont mono {"Courier New"};
  mono.setStyleHint (QFont::Monospace);
  received_->setFont (mono);
  received_->viewport ()->installEventFilter (this);
  received_->setToolTip (tr ("Messages grow as each 1.9 s frame decodes. Bold lines are within the Rx tolerance.\n"
                             "Double-click a line to take the callsign into \"His call\"."));

  entry_->setFont (mono);
  entry_->setMaxLength (4 * Jtty::maxTransmitLength);
  entry_->setPlaceholderText (tr ("Type a message and press Enter to send"));
  entry_->installEventFilter (this);

  his_call_->setMaximumWidth (110);
  his_call_->setPlaceholderText (tr ("His call"));
  exchange_->setMaximumWidth (80);
  exchange_->setPlaceholderText (tr ("Exch"));
  exchange_->setToolTip (tr ("Contest exchange for %E; empty sends the serial number"));
  serial_->setRange (1, 9999);
  serial_->setPrefix (tr ("Nr "));
  serial_->setToolTip (tr ("Serial number for %N"));

  settings_->beginGroup (settings_group);
  exchange_->setText (settings_->value ("Exchange").toString ());
  serial_->setValue (settings_->value ("Serial", 1).toInt ());
  settings_->endGroup ();
  connect (exchange_, &QLineEdit::textChanged, [this] (QString const& text) {
      settings_->beginGroup (settings_group);
      settings_->setValue ("Exchange", text);
      settings_->endGroup ();
      refreshMacroTips ();
    });
  connect (serial_, QOverload<int>::of (&QSpinBox::valueChanged), [this] (int value) {
      settings_->beginGroup (settings_group);
      settings_->setValue ("Serial", value);
      settings_->endGroup ();
      refreshMacroTips ();
    });
  connect (his_call_, &QLineEdit::textChanged, this, &JttyPanel::refreshMacroTips);

  auto * fields = new QHBoxLayout;
  fields->addWidget (new QLabel {tr ("His call:")});
  fields->addWidget (his_call_);
  fields->addWidget (exchange_);
  fields->addWidget (serial_);
  fields->addStretch ();
  fields->addWidget (status_);

  auto * macros = new QGridLayout;
  for (int key = 1; key <= 8; ++key)
    {
      auto * button = new QPushButton {QString {"F%1"}.arg (key)};
      button->setContextMenuPolicy (Qt::CustomContextMenu);
      connect (button, &QPushButton::clicked, [this, key] {sendFunctionKey (key);});
      connect (button, &QPushButton::customContextMenuRequested, [this, key] {editMacro (key);});
      macros->addWidget (button, (key - 1) / 4, (key - 1) % 4);
      macro_buttons_ << button;
    }

  auto * transmit = new QHBoxLayout;
  transmit->addWidget (entry_, 1);
  transmit->addWidget (send_button_);
  transmit->addWidget (stop_button_);
  connect (send_button_, &QPushButton::clicked, this, &JttyPanel::send);
  connect (stop_button_, &QPushButton::clicked, this, &JttyPanel::stopRequested);
  stop_button_->setEnabled (false);

  auto * layout = new QVBoxLayout {this};
  layout->addWidget (received_, 1);
  layout->addLayout (fields);
  layout->addLayout (macros);
  layout->addLayout (transmit);

  refreshMacroTips ();
}

void JttyPanel::setRxWindow (float center, float tolerance)
{
  if (center == rx_center_ && tolerance == rx_tolerance_) return;
  rx_center_ = center;
  rx_tolerance_ = tolerance;
  render ();
}

void JttyPanel::setTransmitting (bool transmitting)
{
  transmitting_ = transmitting;
  send_button_->setEnabled (!transmitting);
  for (auto * button : macro_buttons_) button->setEnabled (!transmitting);
  stop_button_->setEnabled (transmitting);
  status_->setText (transmitting ? tr ("<b><font color=red>Transmitting</font></b>") : QString {});
}

void JttyPanel::clearReceived ()
{
  lines_.clear ();
  line_index_.clear ();
  received_->clear ();
}

QString JttyPanel::macroTemplate (int functionKey) const
{
  auto const key = Jtty::messageStyleSettingsKey (Jtty::MessageStyle::Contest, functionKey);
  settings_->beginGroup (settings_group);
  auto const saved = settings_->value (key, Jtty::nativeMacroTemplate (functionKey)).toString ();
  settings_->endGroup ();
  return Jtty::migratedNativeMacroTemplate (functionKey, saved);
}

QString JttyPanel::expand (QString const& macroTemplate) const
{
  Jtty::NativeMacroContext context;
  context.myCall = my_call_;
  context.hisCall = his_call_->text ().trimmed ().toUpper ();
  context.serialNumber = serial_->value ();
  context.configuredExchange = exchange_->text ();
  context.grid = my_grid_;
  context.snr = last_snr_;
  return Jtty::expandLiteralMacro (macroTemplate, context).simplified ();
}

void JttyPanel::refreshMacroTips ()
{
  for (int key = 1; key <= macro_buttons_.size (); ++key)
    {
      auto const& text = macroTemplate (key);
      macro_buttons_[key - 1]->setText (QString {"F%1 %2"}.arg (key).arg (text.isEmpty () ? QString {} : expand (text).left (18)));
      macro_buttons_[key - 1]->setToolTip (tr ("%1\nRight-click to edit the template "
                                               "(%M my call, %H his call, %E exchange, %N serial, %G grid, %R report)")
                                           .arg (text));
    }
}

void JttyPanel::editMacro (int functionKey)
{
  bool ok {false};
  auto const text = QInputDialog::getText (this, tr ("JTTY F%1").arg (functionKey),
                                           tr ("Template (%M my call, %H his call, %E exchange, %N serial, %G grid, %R report):"),
                                           QLineEdit::Normal, macroTemplate (functionKey), &ok);
  if (!ok) return;
  settings_->beginGroup (settings_group);
  settings_->setValue (Jtty::messageStyleSettingsKey (Jtty::MessageStyle::Contest, functionKey), text);
  settings_->endGroup ();
  refreshMacroTips ();
}

void JttyPanel::sendFunctionKey (int functionKey)
{
  if (transmitting_) return;
  auto const& text = expand (macroTemplate (functionKey));
  if (text.isEmpty ()) return;
  Q_EMIT sendRequested (text);
  // a serial number is spent once it has been sent in an exchange
  if (macroTemplate (functionKey).contains ("%N")
      || (macroTemplate (functionKey).contains ("%E") && exchange_->text ().trimmed ().isEmpty ()))
    {
      serial_->setValue (serial_->value () + 1);
    }
}

void JttyPanel::send ()
{
  if (transmitting_) return;
  auto const text = entry_->text ().simplified ();
  if (text.isEmpty ()) return;
  Q_EMIT sendRequested (text);
  entry_->clear ();
}

bool JttyPanel::eventFilter (QObject * object, QEvent * event)
{
  if (object == entry_ && event->type () == QEvent::KeyPress)
    {
      auto * key_event = static_cast<QKeyEvent *> (event);
      if (key_event->key () == Qt::Key_Escape)
        {
          Q_EMIT stopRequested ();
          return true;
        }
    }
  if (object == received_->viewport () && event->type () == QEvent::MouseButtonDblClick)
    {
      auto * mouse_event = static_cast<QMouseEvent *> (event);
      auto cursor = received_->cursorForPosition (mouse_event->pos ());
      pickCall (cursor.block ().text ());
      return true;
    }
  return QWidget::eventFilter (object, event);
}

void JttyPanel::pickCall (QString const& lineText)
{
  // first token after the time/frequency/SNR columns that looks like a
  // callsign and isn't ours
  static QRegularExpression const call_re {R"(^(?:[A-Z0-9]{1,3}/)?[A-Z0-9]{1,3}[0-9][A-Z0-9]{0,4}[A-Z](?:/[A-Z0-9]{1,4})?$)"};
  auto const tokens = lineText.split (' ', Qt::SkipEmptyParts);
  for (int i = 3; i < tokens.size (); ++i)
    {
      auto const token = tokens[i].toUpper ();
      if (token == my_call_.toUpper () || token == "CQ" || token == "DE") continue;
      if (call_re.match (token).hasMatch ())
        {
          his_call_->setText (token);
          for (auto const& line : lines_)
            {
              if (line.text.contains (token)) last_snr_ = line.snr;
            }
          entry_->setFocus ();
          return;
        }
    }
}

void JttyPanel::applyUpdates (QVector<Jtty::ReceiveUpdate> const& updates)
{
  auto const now = QDateTime::currentDateTimeUtc ().toString ("hhmmss");
  for (auto const& update : updates)
    {
      if (update.text.isEmpty ()) continue;
      auto const complete = update.terminal == Jtty::ReceiveTerminal::Complete;
      auto known = line_index_.constFind (update.messageId);
      if (known != line_index_.constEnd ())
        {
          auto& line = lines_[*known];
          line.text = update.text;
          line.frequency = update.frequency;
          line.snr = update.snr;
          line.complete = complete;
        }
      else
        {
          line_index_.insert (update.messageId, lines_.size ());
          lines_ << Line {update.messageId, update.frequency, update.snr, update.text, complete, now};
        }
    }
  if (lines_.size () > max_lines)
    {
      lines_.erase (lines_.begin (), lines_.begin () + (lines_.size () - max_lines));
      line_index_.clear ();
      for (int i = 0; i < lines_.size (); ++i)
        {
          if (lines_[i].id) line_index_.insert (lines_[i].id, i);
        }
    }
  render ();
}

void JttyPanel::showTransmitted (float frequency, QString const& text)
{
  lines_ << Line {0, frequency, 0, text, true, QDateTime::currentDateTimeUtc ().toString ("hhmmss")};
  render ();
}

void JttyPanel::render ()
{
  auto * scroll = received_->verticalScrollBar ();
  bool const at_bottom = scroll->value () >= scroll->maximum () - 4;
  QString html;
  for (auto const& line : lines_)
    {
      QString body = line.id
        ? Jtty::formatJttyDecodeLine (line.frequency, line.snr, line.text)
        : Jtty::formatJttyTxLine (line.frequency, line.text);
      body = (line.time + ' ' + body).toHtmlEscaped ().replace (' ', "&nbsp;");
      if (!line.id)
        {
          body = "<span style=\"color:#b00000\">" + body + "</span>";
        }
      else
        {
          if (std::abs (line.frequency - rx_center_) < rx_tolerance_) body = "<b>" + body + "</b>";
          if (!line.complete) body = "<span style=\"color:#606060\">" + body + "&nbsp;...</span>";
        }
      html += body + "<br>";
    }
  received_->setHtml (html);
  if (at_bottom) scroll->setValue (scroll->maximum ());
}
