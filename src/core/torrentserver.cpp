#include "torrentserver.h"
#include "peer.h"
#include <QDebug>

TorrentServer::TorrentServer(QTorrent *qTorrent) : m_qTorrent(qTorrent) {
	connect(&m_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

TorrentServer::~TorrentServer() {
	disconnect(&m_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

bool TorrentServer::startServer(int port) {
	if(port) {
		if(!m_server.listen(QHostAddress::Any, port)) {
			qDebug() << "Failed to start server:" << m_server.errorString();
			return false;
		}
		qDebug() << "Server started on port" << QString::number(m_server.serverPort());
		return true;
	}

	// Attempt to get a port in the range [6881,6889]
	for(int port = 6881; port <= 6889; port++) {
		if(m_server.listen(QHostAddress::Any, port)) {
			qDebug() << "Server started on port" << QString::number(m_server.serverPort());
			return true;
		}
	}

	// Just try to get a free port
	if(!m_server.listen()) {
		qDebug() << "Failed to start server:" << m_server.errorString();
		return false;
	}
	qDebug() << "Server started on port" << QString::number(m_server.serverPort());
	return true;
}

void TorrentServer::newConnection() {
	QTcpSocket* socket = m_server.nextPendingConnection();
	Peer* peer = Peer::createClient(m_qTorrent, socket);
	m_peers.push_back(peer);
}

QTorrent* TorrentServer::qTorrent() {
	return m_qTorrent;
}

QTcpServer& TorrentServer::server() {
	return m_server;
}

int TorrentServer::port() {
	return m_server.serverPort();
}

QHostAddress TorrentServer::address() {
	return m_server.serverAddress();
}

QList<Peer*>& TorrentServer::peers() {
	return m_peers;
}