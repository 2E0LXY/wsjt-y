#ifndef REMOTEBRIDGE_HPP
#define REMOTEBRIDGE_HPP

#include <QObject>
#include <QUrl>
#include <QAbstractSocket>
#include <QList>

#include "Radio.hpp"

class QWebSocket;
class QWebSocketServer;
class QTimer;
#include <QJsonObject>

// WSJT-Y remote-control bridge.
//
// Two independent, optional modes — both can be active at once:
//
//  - Relay mode: connects OUTBOUND over WSS to a relay (e.g.
//    wss://aprsnet.uk/wsjty-relay). Works over WAN with no port
//    forwarding; TLS + auth are handled by the relay.
//
//  - Direct mode: WSJT-Y itself runs a WebSocket SERVER on a configured
//    port, for a companion app on the same LAN, or over WAN if the user
//    forwards that port on their router. No relay involved. This is
//    plain ws:// (unencrypted transport) — same trust model as the
//    existing local UDP MessageClient protocol already used for JTAlert/
//    GridTracker/etc — protected by the same auth token as relay mode,
//    but the token itself travels in the clear, so treat a WAN-exposed
//    direct connection as lower-assurance than relay mode. LAN-only use
//    is the safe case; WAN direct is a deliberate simpler-but-weaker
//    alternative to running a relay.
//
// This class deliberately knows nothing about MainWindow/UI — it only
// emits Qt signals for incoming commands and exposes plain send*() calls
// for outgoing events, mirroring the shape of Network/MessageClient so the
// two can be wired into MainWindow the same way. Outgoing events go to
// every connected sink (relay + all direct clients) at once.
class RemoteBridge : public QObject
{
  Q_OBJECT

public:
  using Frequency = Radio::Frequency;

  explicit RemoteBridge (QObject * parent = nullptr);
  ~RemoteBridge () override;

  // ---- Relay mode ----
  // (Re)configure and connect. An empty relay_url disables relay mode
  // (socket is closed, no reconnect attempts).
  void configure (QUrl const& relay_url, QString const& token);
  bool is_connected () const;

  // ---- Direct mode ----
  // Starts (or reconfigures) the local WebSocket server. port 0 disables
  // it. Returns false if the port couldn't be bound (e.g. already in
  // use) — check error_string() for why.
  bool enable_direct_server (quint16 port, QString const& token);
  void disable_direct_server ();
  bool direct_server_listening () const;
  quint16 direct_server_port () const;
  QString error_string () const;
  int direct_client_count () const;
  // Best-effort local IPv4 addresses this machine can be reached on —
  // shown to the user so they know what to type into the phone.
  static QStringList local_ipv4_addresses ();

  // ---- Outgoing: WSJT-Y -> (relay and/or direct clients) -> companion app ----
  void send_decode (QString const& utc_hms, int snr, double dt, quint32 audio_freq_hz,
                     QString const& mode, QString const& message, bool is_cq, bool for_me);
  void send_status (Frequency dial_frequency, QString const& mode, QString const& dx_call,
                     QString const& dx_grid, bool tx_enabled, bool transmitting);
  void send_qso_logged (QString const& call, QString const& grid, Frequency dial_frequency,
                         QString const& mode, QString const& report_sent, QString const& report_rcvd);
  void send_decodes_cleared ();

Q_SIGNALS:
  // ---- Incoming: companion app -> (relay or direct) -> WSJT-Y ----
  void reply_requested (QString call, QString grid, quint32 audio_freq_hz);
  void halt_tx_requested ();
  void set_band_requested (Frequency freq_hz, QString band_name);
  void connection_state_changed (bool connected);          // relay mode
  void direct_client_count_changed (int count);             // direct mode

private Q_SLOTS:
  void on_connected ();
  void on_disconnected ();
  void on_text_message_received (QString message);
  void on_error (QAbstractSocket::SocketError);
  void attempt_reconnect ();
  void on_direct_new_connection ();
  void on_direct_client_disconnected ();
  void on_direct_client_text_message_received (QString message);

private:
  void open_socket ();
  void send_json (QJsonObject const& obj);
  void handle_incoming_json (QString const& message);
  void replay_last_status (QWebSocket * client);

  QWebSocket * socket_ = nullptr;
  QTimer     * reconnect_timer_ = nullptr;
  QUrl         relay_url_;
  QString      token_;
  bool         enabled_ = false;

  QWebSocketServer  * direct_server_ = nullptr;
  QList<QWebSocket *> direct_clients_;
  QString              direct_token_;
  QString              last_error_;
  QJsonObject          last_status_;
};

#endif
