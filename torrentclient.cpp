#include "torrentclient.h"
#include <QtNetwork>
#include <QByteArray>
#include <QDebug>

TorrentClient::TorrentClient() {
}

TorrentClient::~TorrentClient() {
}

void TorrentClient::readyRead() {

}

void TorrentClient::handshake(TorrentInfo &torrentInfo) {
	//qDebug() << "Connecting";
	m_socket.connectToHost(QHostAddress("91.139.201.213"), 36777);
	if(!m_socket.waitForConnected()) {
		qDebug() << "Failed to connect:" << m_socket.errorString();
		return;
	}
	//qDebug() << "Connected";
	QByteArray dataToWrite;

	dataToWrite.push_back(char(19));

	dataToWrite.push_back("BitTorrent protocol");

	for(int i = 0; i < 8; i++) {
		dataToWrite.push_back(char(0));
	}

	dataToWrite.push_back(torrentInfo.infoHash());

	dataToWrite.push_back("ThisIsNotAFakePeerId");

	//qDebug() << "Writing data" << dataToWrite.size();
	m_socket.write(dataToWrite);
	if(!m_socket.waitForBytesWritten()) {
		qDebug() << "Failed to send handshake:" << m_socket.errorString();
		return;
	}
	//qDebug() << "Waiting for a response";
	if(!m_socket.waitForReadyRead()) {
		qDebug() << "Failed to read responce:" << m_socket.errorString();
		return;
	}
	qDebug() << endl;
	qDebug() << "My torrent info hash:" << torrentInfo.infoHash().toHex();
	qDebug() << endl;
	QTextStream out(stdout);
	QByteArray response = m_socket.readAll();
	qDebug() << response.size();
	if(response.isEmpty()) {
		qDebug() << "Response empty";
		return;
	}
	int protocolSize = response[0];
	if(response.size() < protocolSize + 48) {
		qDebug() << "Response too small";
		return;
	}
	QByteArray peerProtocol;
	QByteArray peerReserved;
	QByteArray peerInfoHash;
	QByteArray peerId;
	int i = 1;
	for(int j = 0; j < protocolSize; j++) {
		peerProtocol.push_back(response[i++]);
	}
	for(int j = 0; j < 8; j++) {
		peerReserved.push_back(response[i++]);
	}
	for(int j = 0; j < 20; j++) {
		peerInfoHash.push_back(response[i++]);
	}
	for(int j = 0; j < 20; j++) {
		peerId.push_back(response[i++]);
	}
	qDebug() << "Peer protocol:" << peerProtocol;
	qDebug() << "Peer reserved bytes:" << peerReserved.toHex();
	qDebug() << "Peer info hash:" << peerInfoHash.toHex();
	qDebug() << "Peer Id:" << peerId.toHex();
	qDebug() << endl;

	if(response.size() == i) {
		qDebug() << "End";
		return;
	}
	QByteArray length;
	QByteArray something;
	for(int j = 0; j < 4; j++) {
		length.push_back(response[i]);
	}
	qDebug() << "Response length:" << length;
	for(;i < response.size(); i++) {
		something.push_back(response[i]);
	}
	qDebug() << something.size();
	qDebug() << something.toHex();
}
