#include "RemoteBridge.hpp"

#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>
#include <QTimer>
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkInterface>
#include <QDebug>

namespace
{
  constexpr int reconnect_interval_ms {5000};
}

RemoteBridge::RemoteBridge (QObject * parent)
  : QObject {parent}
{
}

RemoteBridge::~RemoteBridge ()
{
  disable_direct_server ();
}

bool RemoteBridge::enable_direct_server (quint16 port, QString const& token)
{
  disable_direct_server ();

  direct_token_ = token;
  direct_server_ = new QWebSocketServer {QStringLiteral ("WSJT-Y Remote"),
                                          QWebSocketServer::NonSecureMode, this};
  // Plain ws:// — see the class comment in RemoteBridge.hpp for the
  // trust-model rationale (same as the existing local UDP protocol;
  // auth token required either way, but not itself encrypted here).
  if (!direct_server_->listen (QHostAddress::Any, port))
    {
      last_error_ = direct_server_->errorString ();
      qDebug () << "RemoteBridge: direct server failed to listen on port" << port << ":" << last_error_;
      direct_server_->deleteLater ();
      direct_server_ = nullptr;
      return false;
    }

  connect (direct_server_, &QWebSocketServer::newConnection, this, &RemoteBridge::on_direct_new_connection);
  qDebug () << "RemoteBridge: direct server listening on port" << direct_server_->serverPort ();
  return true;
}

void RemoteBridge::disable_direct_server ()
{
  for (auto * client : direct_clients_)
    {
      client->close ();
      client->deleteLater ();
    }
  direct_clients_.clear ();

  if (direct_server_)
    {
      direct_server_->close ();
      direct_server_->deleteLater ();
      direct_server_ = nullptr;
    }
}

bool RemoteBridge::direct_server_listening () const
{
  return direct_server_ && direct_server_->isListening ();
}

quint16 RemoteBridge::direct_server_port () const
{
  return direct_server_ ? direct_server_->serverPort () : 0;
}

QString RemoteBridge::error_string () const
{
  return last_error_;
}

int RemoteBridge::direct_client_count () const
{
  return direct_clients_.size ();
}

QStringList RemoteBridge::local_ipv4_addresses ()
{
  QStringList out;
  for (auto const& iface : QNetworkInterface::allInterfaces ())
    {
      if (!(iface.flags () & QNetworkInterface::IsUp)) continue;
      if (iface.flags () & QNetworkInterface::IsLoopBack) continue;
      for (auto const& entry : iface.addressEntries ())
        {
          auto const ip = entry.ip ();
          if (ip.protocol () == QAbstractSocket::IPv4Protocol)
            {
              out << ip.toString ();
            }
        }
    }
  return out;
}

void RemoteBridge::on_direct_new_connection ()
{
  while (direct_server_ && direct_server_->hasPendingConnections ())
    {
      auto * client = direct_server_->nextPendingConnection ();
      QUrlQuery const q {client->requestUrl ()};
      auto const supplied_token = q.queryItemValue ("token");
      if (direct_token_.isEmpty () || supplied_token != direct_token_)
        {
          qDebug () << "RemoteBridge: direct client rejected (bad/missing token) from"
                     << client->peerAddress ().toString ();
          client->close (QWebSocketProtocol::CloseCodePolicyViolated, "invalid token");
          client->deleteLater ();
          continue;
        }

      connect (client, &QWebSocket::disconnected, this, &RemoteBridge::on_direct_client_disconnected);
      connect (client, &QWebSocket::textMessageReceived,
               this, &RemoteBridge::on_direct_client_text_message_received);
      direct_clients_.append (client);
      qDebug () << "RemoteBridge: direct client connected from" << client->peerAddress ().toString ()
                 << "(" << direct_clients_.size () << "total)";
      Q_EMIT direct_client_count_changed (direct_clients_.size ());
    }
}

void RemoteBridge::on_direct_client_disconnected ()
{
  auto * client = qobject_cast<QWebSocket *> (sender ());
  if (!client) return;
  direct_clients_.removeAll (client);
  client->deleteLater ();
  Q_EMIT direct_client_count_changed (direct_clients_.size ());
}

void RemoteBridge::on_direct_client_text_message_received (QString message)
{
  handle_incoming_json (message);
}

void RemoteBridge::configure (QUrl const& relay_url, QString const& token)
{
  relay_url_ = relay_url;
  token_ = token;
  enabled_ = relay_url_.isValid () && !relay_url_.host ().isEmpty ();

  if (reconnect_timer_)
    {
      reconnect_timer_->stop ();
    }

  if (socket_)
    {
      socket_->close ();
      socket_->deleteLater ();
      socket_ = nullptr;
    }

  if (enabled_)
    {
      open_socket ();
    }
}

bool RemoteBridge::is_connected () const
{
  return socket_ && socket_->state () == QAbstractSocket::ConnectedState;
}

void RemoteBridge::open_socket ()
{
  if (!enabled_) return;

  if (!socket_)
    {
      socket_ = new QWebSocket {};
      connect (socket_, &QWebSocket::connected, this, &RemoteBridge::on_connected);
      connect (socket_, &QWebSocket::disconnected, this, &RemoteBridge::on_disconnected);
      connect (socket_, &QWebSocket::textMessageReceived, this, &RemoteBridge::on_text_message_received);
      connect (socket_, QOverload<QAbstractSocket::SocketError>::of (&QWebSocket::error),
               this, &RemoteBridge::on_error);
    }

  if (!reconnect_timer_)
    {
      reconnect_timer_ = new QTimer {this};
      reconnect_timer_->setSingleShot (true);
      connect (reconnect_timer_, &QTimer::timeout, this, &RemoteBridge::attempt_reconnect);
    }

  // Auth token travels as a query parameter on the WSS handshake URL —
  // the relay validates it before upgrading the connection. TLS (wss://)
  // is provided by the relay; WSJT-Y makes an outbound-only connection.
  QUrl url {relay_url_};
  QUrlQuery q {url};
  if (!token_.isEmpty ()) q.addQueryItem ("token", token_);
  url.setQuery (q);

  socket_->open (url);
}

void RemoteBridge::attempt_reconnect ()
{
  if (!enabled_) return;
  if (socket_ && socket_->state () != QAbstractSocket::UnconnectedState) return;
  open_socket ();
}

void RemoteBridge::on_connected ()
{
  qDebug () << "RemoteBridge: connected to" << relay_url_.toString ();
  Q_EMIT connection_state_changed (true);
}

void RemoteBridge::on_disconnected ()
{
  Q_EMIT connection_state_changed (false);
  if (enabled_ && reconnect_timer_)
    {
      reconnect_timer_->start (reconnect_interval_ms);
    }
}

void RemoteBridge::on_error (QAbstractSocket::SocketError)
{
  // textMessageReceived won't fire again until reconnected; disconnected()
  // will also fire and schedules the retry, so nothing further to do here
  // beyond logging.
  if (socket_)
    {
      qDebug () << "RemoteBridge: socket error:" << socket_->errorString ();
    }
}

void RemoteBridge::on_text_message_received (QString message)
{
  handle_incoming_json (message);
}

void RemoteBridge::handle_incoming_json (QString const& message)
{
  auto const doc = QJsonDocument::fromJson (message.toUtf8 ());
  if (!doc.isObject ()) return;
  auto const obj = doc.object ();
  auto const type = obj.value ("type").toString ();

  if ("reply" == type)
    {
      Q_EMIT reply_requested (obj.value ("call").toString ().toUpper (),
                               obj.value ("grid").toString ().toUpper (),
                               static_cast<quint32> (obj.value ("audio_freq_hz").toInt (0)));
    }
  else if ("halt_tx" == type)
    {
      Q_EMIT halt_tx_requested ();
    }
  else if ("set_band" == type)
    {
      Q_EMIT set_band_requested (static_cast<Frequency> (obj.value ("freq_hz").toDouble (0)),
                                  obj.value ("band").toString ());
    }
  // Unknown message types are ignored — keeps the wire protocol
  // forward-compatible with a newer app talking to an older WSJT-Y.
}

void RemoteBridge::send_json (QJsonObject const& obj)
{
  auto const text = QString::fromUtf8 (QJsonDocument {obj}.toJson (QJsonDocument::Compact));
  if (is_connected ())
    {
      socket_->sendTextMessage (text);
    }
  for (auto * client : direct_clients_)
    {
      client->sendTextMessage (text);
    }
}

void RemoteBridge::send_decode (QString const& utc_hms, int snr, double dt, quint32 audio_freq_hz,
                                 QString const& mode, QString const& message, bool is_cq, bool for_me)
{
  send_json (QJsonObject {
      {"type", "decode"}, {"time", utc_hms}, {"snr", snr}, {"dt", dt},
      {"audio_freq_hz", static_cast<int> (audio_freq_hz)}, {"mode", mode},
      {"message", message}, {"cq", is_cq}, {"for_me", for_me}
    });
}

void RemoteBridge::send_status (Frequency dial_frequency, QString const& mode, QString const& dx_call,
                                 QString const& dx_grid, bool tx_enabled, bool transmitting)
{
  send_json (QJsonObject {
      {"type", "status"}, {"dial_freq_hz", static_cast<double> (dial_frequency)},
      {"mode", mode}, {"dx_call", dx_call}, {"dx_grid", dx_grid},
      {"tx_enabled", tx_enabled}, {"transmitting", transmitting}
    });
}

void RemoteBridge::send_qso_logged (QString const& call, QString const& grid, Frequency dial_frequency,
                                     QString const& mode, QString const& report_sent, QString const& report_rcvd)
{
  send_json (QJsonObject {
      {"type", "qso_logged"}, {"call", call}, {"grid", grid},
      {"dial_freq_hz", static_cast<double> (dial_frequency)}, {"mode", mode},
      {"report_sent", report_sent}, {"report_rcvd", report_rcvd}
    });
}

void RemoteBridge::send_decodes_cleared ()
{
  send_json (QJsonObject {{"type", "decodes_cleared"}});
}
